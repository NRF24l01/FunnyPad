#include "SoundpadAudio.hpp"
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <cstdlib>
#include <QDebug>
#include <fstream>
#include <iostream>
#include <vector>
#include <QTimer>
#include <QFileInfo>
#include <QMetaObject>
#include <thread>
#include <chrono>

namespace soundpad {

SoundpadAudio::SoundpadAudio(const std::string& sinkName)
    : QObject(nullptr), sinkName_(sinkName)
{
    qDebug() << "SoundpadAudio created with sink:" << QString::fromStdString(sinkName_);
}

SoundpadAudio::~SoundpadAudio()
{
    stop();
    if (workerThread_.isRunning()) {
        workerThread_.quit();
        workerThread_.wait();
    }
    qDebug() << "SoundpadAudio destroyed";
}

bool SoundpadAudio::playWav(const std::string& wavFilePath) {
    stop();
    currentFile_ = wavFilePath;
    stopRequested_ = false;
    seekToMs_ = -1;
    std::thread([this, wavFilePath]() {
        playbackThreadFunc(wavFilePath);
    }).detach();
    return true;
}

void SoundpadAudio::stop() {
    QMutexLocker locker(&mutex_);
    stopRequested_ = true;
    seekCond_.wakeAll();
}

void SoundpadAudio::seek(qint64 ms) {
    QMutexLocker locker(&mutex_);
    seekToMs_ = ms;
    seekCond_.wakeAll();
}

qint64 SoundpadAudio::currentTime() const {
    QMutexLocker locker(&mutex_);
    return currentMs_;
}

qint64 SoundpadAudio::totalTime() const {
    QMutexLocker locker(&mutex_);
    return totalMs_;
}

void SoundpadAudio::playbackThreadFunc(const std::string& wavFilePath) {
    qDebug() << "[SoundpadAudio] playbackThreadFunc started for file:" << QString::fromStdString(wavFilePath);
    std::ifstream file(wavFilePath, std::ios::binary);
    if (!file) {
        qDebug() << "[SoundpadAudio] Не удалось открыть WAV файл:" << QString::fromStdString(wavFilePath);
        emit playbackStopped();
        return;
    }
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(44, std::ios::beg); // skip header
    qint64 dataSize = static_cast<qint64>(fileSize) - 44;
    qint64 bytesPerSec = 44100 * 2 * 2;
    const qint64 blockAlign = 4; // 16-bit stereo PCM: 4 bytes per frame
    qDebug() << "[SoundpadAudio] fileSize:" << static_cast<qint64>(fileSize)
             << "dataSize:" << dataSize << "bytesPerSec:" << bytesPerSec;
    {
        QMutexLocker locker(&mutex_);
        totalMs_ = (dataSize * 1000) / bytesPerSec;
        currentMs_ = 0;
    }
    emit playbackStarted(totalMs_);

    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };
    int error;
    pa_simple *s = pa_simple_new(
        nullptr, "SoundpadApp", PA_STREAM_PLAYBACK, sinkName_.c_str(),
        "playback", &ss, nullptr, nullptr, &error
    );
    if (!s) {
        qDebug() << "[SoundpadAudio] pa_simple_new failed:" << pa_strerror(error);
        emit playbackStopped();
        return;
    }

    constexpr size_t bufferSize = 4096;
    std::vector<char> buffer(bufferSize);
    qint64 playedBytes = 0;

    while (true) {
        {
            QMutexLocker locker(&mutex_);
            if (stopRequested_) {
                qDebug() << "[SoundpadAudio] stopRequested_ set, breaking loop";
                break;
            }
            if (seekToMs_ >= 0) {
                qint64 seekByte = (seekToMs_ * bytesPerSec) / 1000;
                seekByte = std::min(seekByte, dataSize);
                // Align seekByte to blockAlign (round down)
                seekByte = (seekByte / blockAlign) * blockAlign;
                file.clear(); // сбросить флаги ошибок
                file.seekg(44 + seekByte, std::ios::beg);
                playedBytes = seekByte;
                currentMs_ = seekToMs_;
                qDebug() << "[SoundpadAudio] Seek requested to ms:" << seekToMs_
                         << "seekByte:" << seekByte << "playedBytes:" << playedBytes;
                seekToMs_ = -1;
                // Если после seek мы в конце файла — завершить воспроизведение
                if (playedBytes >= dataSize) {
                    qDebug() << "[SoundpadAudio] Seeked to end of file, breaking loop";
                    break;
                }
            }
        }
        // Проверка конца файла перед чтением
        if (playedBytes >= dataSize) {
            qDebug() << "[SoundpadAudio] playedBytes >= dataSize, breaking loop";
            break;
        }
        // Читаем только оставшееся количество байт
        size_t bytesLeft = static_cast<size_t>(dataSize - playedBytes);
        if (bytesLeft == 0) {
            qDebug() << "[SoundpadAudio] bytesLeft == 0, breaking loop";
            break;
        }
        size_t readSize = std::min(buffer.size(), bytesLeft);
        file.read(buffer.data(), readSize);
        std::streamsize bytesRead = file.gcount();
        if (bytesRead <= 0) {
            qDebug() << "[SoundpadAudio] bytesRead <= 0, breaking loop";
            break; // конец файла или ошибка
        }
        // Если bytesRead < buffer.size(), не отправлять остаток буфера (только bytesRead)
        size_t toWrite = static_cast<size_t>(bytesRead);
        if (toWrite > bytesLeft) {
            toWrite = bytesLeft;
        }
        if (pa_simple_write(s, buffer.data(), toWrite, &error) < 0) {
            qDebug() << "[SoundpadAudio] pa_simple_write failed:" << pa_strerror(error);
            break;
        }
        playedBytes += toWrite;
        {
            QMutexLocker locker(&mutex_);
            currentMs_ = (playedBytes * 1000) / bytesPerSec;
        }
        emit playbackProgress(currentMs_);
        double msPerBlock = (double)toWrite / (double)bytesPerSec * 1000.0;
        qDebug() << "[SoundpadAudio] playedBytes:" << playedBytes
                 << "currentMs_:" << currentMs_
                 << "toWrite:" << toWrite
                 << "msPerBlock:" << msPerBlock;
        if (msPerBlock > 0.0)
            std::this_thread::sleep_for(std::chrono::milliseconds((int)msPerBlock));
    }
    pa_simple_drain(s, &error);
    pa_simple_free(s);
    qDebug() << "[SoundpadAudio] playbackThreadFunc finished";
    emit playbackStopped();
}


std::vector<std::pair<std::string, std::string>> SoundpadAudio::getSourceList()
{
    qDebug() << "getSourceList() called";

    pa_mainloop *ml = pa_mainloop_new();
    pa_context *ctx = pa_context_new(pa_mainloop_get_api(ml), "SoundpadContext");

    struct SourceListContext {
        std::vector<std::pair<std::string, std::string>> sources;
        bool done = false;
    } context;

    pa_context_set_state_callback(ctx, [](pa_context *c, void *userdata) {
        if (pa_context_get_state(c) == PA_CONTEXT_READY) {
            pa_operation *op = pa_context_get_source_info_list(
                c,
                [](pa_context *, const pa_source_info *info, int eol, void *userdata) {
                    auto *data = static_cast<SourceListContext*>(userdata);
                    if (eol > 0) {
                        data->done = true;
                        return;
                    }
                    if (info) {
                        std::string name = info->name ? info->name : "";
                        std::string desc = info->description ? info->description : "";

                        qDebug() << "Found source:" << QString::fromStdString(name)
                                 << "(" << QString::fromStdString(desc) << ")";

                        data->sources.emplace_back(name, desc);
                    }
                },
                userdata
            );
            if (op)
                pa_operation_unref(op);
        }
    }, &context);

    if (pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
        qDebug() << "Failed to connect context";
        pa_context_unref(ctx);
        pa_mainloop_free(ml);
        return {};
    }

    while (!context.done) {
        pa_mainloop_iterate(ml, 1, nullptr);
    }

    pa_context_disconnect(ctx);
    pa_context_unref(ctx);
    pa_mainloop_free(ml);

    qDebug() << "getSourceList() finished. Total:" << context.sources.size();
    return context.sources;
}

bool SoundpadAudio::mergeWithMic(const std::string& sourceName)
{
    qDebug() << "Merging source with mic:" << QString::fromStdString(sourceName)
             << "into sink:" << QString::fromStdString(sinkName_);

    std::string cmd = "pactl load-module module-loopback source=" + sourceName + " sink=" + sinkName_;
    int result = std::system(cmd.c_str());

    if (result != 0) {
        qDebug() << "pactl load-module failed with code:" << result;
        return false;
    }

    qDebug() << "Loopback module loaded successfully";
    return true;
}

} // namespace soundpad
