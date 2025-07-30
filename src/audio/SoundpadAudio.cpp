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

// Helper: Check if a sink exists
bool SoundpadAudio::sinkExists(const std::string& sinkName) {
    qDebug() << "[SoundpadAudio] Checking if sink exists:" << QString::fromStdString(sinkName);
    pa_mainloop *ml = pa_mainloop_new();
    pa_context *ctx = pa_context_new(pa_mainloop_get_api(ml), "SoundpadCheckSink");

    struct SinkCheckContext {
        std::string name;
        bool found = false;
        bool done = false;
    } context{sinkName};

    pa_context_set_state_callback(ctx, [](pa_context *c, void *userdata) {
        auto *ctxData = static_cast<SinkCheckContext*>(userdata);
        if (pa_context_get_state(c) == PA_CONTEXT_READY) {
            pa_operation *op = pa_context_get_sink_info_list(
                c,
                [](pa_context *, const pa_sink_info *info, int eol, void *userdata) {
                    auto *ctxData = static_cast<SinkCheckContext*>(userdata);
                    if (eol > 0) {
                        ctxData->done = true;
                        return;
                    }
                    if (info && info->name) {
                        if (ctxData->name == info->name) {
                            ctxData->found = true;
                        }
                    }
                },
                userdata
            );
            if (op) pa_operation_unref(op);
        }
    }, &context);

    pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    while (!context.done) pa_mainloop_iterate(ml, 1, nullptr);

    pa_context_disconnect(ctx);
    pa_context_unref(ctx);
    pa_mainloop_free(ml);

    return context.found;
}

// Helper: Check if a source exists
bool SoundpadAudio::sourceExists(const std::string& sourceName) {
    qDebug() << "[SoundpadAudio] Checking if source exists:" << QString::fromStdString(sourceName);
    pa_mainloop *ml = pa_mainloop_new();
    pa_context *ctx = pa_context_new(pa_mainloop_get_api(ml), "SoundpadCheckSource");

    struct SourceCheckContext {
        std::string name;
        bool found = false;
        bool done = false;
    } context{sourceName};

    pa_context_set_state_callback(ctx, [](pa_context *c, void *userdata) {
        auto *ctxData = static_cast<SourceCheckContext*>(userdata);
        if (pa_context_get_state(c) == PA_CONTEXT_READY) {
            pa_operation *op = pa_context_get_source_info_list(
                c,
                [](pa_context *, const pa_source_info *info, int eol, void *userdata) {
                    auto *ctxData = static_cast<SourceCheckContext*>(userdata);
                    if (eol > 0) {
                        ctxData->done = true;
                        return;
                    }
                    if (info && info->name) {
                        if (ctxData->name == info->name) {
                            ctxData->found = true;
                        }
                    }
                },
                userdata
            );
            if (op) pa_operation_unref(op);
        }
    }, &context);

    pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    while (!context.done) pa_mainloop_iterate(ml, 1, nullptr);

    pa_context_disconnect(ctx);
    pa_context_unref(ctx);
    pa_mainloop_free(ml);

    return context.found;
}

// Helper: Create null sink
void SoundpadAudio::createNullSink(const std::string& sinkName) {
    qDebug() << "[SoundpadAudio] Creating null sink:" << QString::fromStdString(sinkName);
    pa_mainloop *ml = pa_mainloop_new();
    pa_context *ctx = pa_context_new(pa_mainloop_get_api(ml), "SoundpadCreateSink");

    struct SinkCreateContext {
        std::string name;
        bool done = false;
    } context{sinkName};

    pa_context_set_state_callback(ctx, [](pa_context *c, void *userdata) {
        auto *ctxData = static_cast<SinkCreateContext*>(userdata);
        if (pa_context_get_state(c) == PA_CONTEXT_READY) {
            std::string args = "sink_name=" + ctxData->name +
                               " sink_properties=device.description=" + ctxData->name;
            pa_operation *op = pa_context_load_module(
                c, "module-null-sink", args.c_str(),
                [](pa_context *, uint32_t, void *userdata) {
                    auto *ctxData = static_cast<SinkCreateContext*>(userdata);
                    ctxData->done = true;
                }, userdata
            );
            if (op) pa_operation_unref(op);
        }
    }, &context);

    pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    while (!context.done) pa_mainloop_iterate(ml, 1, nullptr);

    pa_context_disconnect(ctx);
    pa_context_unref(ctx);
    pa_mainloop_free(ml);
}

// Helper: Create remap source
void SoundpadAudio::createRemapSource(const std::string& masterMonitor, const std::string& sourceName) {
    qDebug() << "[SoundpadAudio] Creating remap source:" << QString::fromStdString(sourceName)
             << "from master:" << QString::fromStdString(masterMonitor);
    pa_mainloop *ml = pa_mainloop_new();
    pa_context *ctx = pa_context_new(pa_mainloop_get_api(ml), "SoundpadCreateSource");

    struct SourceCreateContext {
        std::string master;
        std::string name;
        bool done = false;
    } context{masterMonitor, sourceName};

    pa_context_set_state_callback(ctx, [](pa_context *c, void *userdata) {
        auto *ctxData = static_cast<SourceCreateContext*>(userdata);
        if (pa_context_get_state(c) == PA_CONTEXT_READY) {
            std::string args = "master=" + ctxData->master +
                               " source_name=" + ctxData->name +
                               " source_properties=device.description=" + ctxData->name;
            pa_operation *op = pa_context_load_module(
                c, "module-remap-source", args.c_str(),
                [](pa_context *, uint32_t, void *userdata) {
                    auto *ctxData = static_cast<SourceCreateContext*>(userdata);
                    ctxData->done = true;
                }, userdata
            );
            if (op) pa_operation_unref(op);
        }
    }, &context);

    pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    while (!context.done) pa_mainloop_iterate(ml, 1, nullptr);

    pa_context_disconnect(ctx);
    pa_context_unref(ctx);
    pa_mainloop_free(ml);
}

// Call this before playback or merging
void SoundpadAudio::ensureAudioObjectsExist(const std::string& sinkName) {
    qDebug() << "[SoundpadAudio] Ensuring audio objects exist for sink:" << QString::fromStdString(sinkName);
    if (!sinkExists(sinkName)) {
        createNullSink(sinkName);
    }
    std::string monitorName = sinkName + ".monitor";
    if (!sourceExists("VirtualMic")) {
        createRemapSource(monitorName, "VirtualMic");
    }
}

SoundpadAudio::SoundpadAudio(const std::string& sinkName)
    : QObject(nullptr), sinkName_(sinkName), outputSinkName_("")
{
    qDebug() << "[SoundpadAudio] Constructor called";
    ensureAudioObjectsExist(sinkName_);
    qDebug() << "SoundpadAudio created with sink:" << QString::fromStdString(sinkName_);
}

SoundpadAudio::~SoundpadAudio()
{
    qDebug() << "[SoundpadAudio] Destructor called";
    stop();
    if (workerThread_.isRunning()) {
        workerThread_.quit();
        workerThread_.wait();
    }
    qDebug() << "SoundpadAudio destroyed";
}

bool SoundpadAudio::playWav(const std::string& wavFilePath) {
    qDebug() << "[SoundpadAudio] playWav called for file:" << QString::fromStdString(wavFilePath);
    ensureAudioObjectsExist(sinkName_);
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
    qDebug() << "[SoundpadAudio] stop called";
    QMutexLocker locker(&mutex_);
    stopRequested_ = true;
    seekCond_.wakeAll();
}

void SoundpadAudio::seek(qint64 ms) {
    qDebug() << "[SoundpadAudio] seek called to ms:" << ms;
    QMutexLocker locker(&mutex_);
    seekToMs_ = ms;
    seekCond_.wakeAll();
}

qint64 SoundpadAudio::currentTime() const {
    qDebug() << "[SoundpadAudio] currentTime called";
    QMutexLocker locker(&mutex_);
    return currentMs_;
}

qint64 SoundpadAudio::totalTime() const {
    qDebug() << "[SoundpadAudio] totalTime called";
    QMutexLocker locker(&mutex_);
    return totalMs_;
}

std::vector<std::pair<std::string, std::string>> SoundpadAudio::getSourceList()
{
    qDebug() << "[SoundpadAudio] getSourceList called";

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

std::vector<std::pair<std::string, std::string>> SoundpadAudio::getSinkList()
{
    qDebug() << "[SoundpadAudio] getSinkList called";

    pa_mainloop *ml = pa_mainloop_new();
    pa_context *ctx = pa_context_new(pa_mainloop_get_api(ml), "SoundpadContext");

    struct SinkListContext {
        std::vector<std::pair<std::string, std::string>> sinks;
        bool done = false;
    } context;

    pa_context_set_state_callback(ctx, [](pa_context *c, void *userdata) {
        if (pa_context_get_state(c) == PA_CONTEXT_READY) {
            qDebug() << "[SoundpadAudio] PA context ready, querying sink list";
            pa_operation *op = pa_context_get_sink_info_list(
                c,
                [](pa_context *, const pa_sink_info *info, int eol, void *userdata) {
                    auto *data = static_cast<SinkListContext*>(userdata);
                    if (eol > 0) {
                        data->done = true;
                        qDebug() << "[SoundpadAudio] End of sink list";
                        return;
                    }
                    if (info) {
                        std::string name = info->name ? info->name : "";
                        std::string desc = info->description ? info->description : "";

                        // Include all sinks except our virtual one
                        if (name != "SoundpadSink") {
                            qDebug() << "[SoundpadAudio] Found sink:" << QString::fromStdString(name)
                                     << "(" << QString::fromStdString(desc) << ")";
                            data->sinks.emplace_back(name, desc);
                        }
                    }
                },
                userdata
            );
            if (op)
                pa_operation_unref(op);
            else
                qDebug() << "[SoundpadAudio] Failed to create operation for getting sink list";
        } else if (pa_context_get_state(c) == PA_CONTEXT_FAILED ||
                   pa_context_get_state(c) == PA_CONTEXT_TERMINATED) {
            qDebug() << "[SoundpadAudio] PA context failed or terminated";
            static_cast<SinkListContext*>(userdata)->done = true;
        }
    }, &context);

    if (pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
        qDebug() << "[SoundpadAudio] Failed to connect context:" << pa_strerror(pa_context_errno(ctx));
        pa_context_unref(ctx);
        pa_mainloop_free(ml);
        return {};
    }

    // Wait for the operation to complete
    int ret = 0;
    while (!context.done && pa_mainloop_iterate(ml, 1, &ret) >= 0) {
        // Just iterate until done
    }

    pa_context_disconnect(ctx);
    pa_context_unref(ctx);
    pa_mainloop_free(ml);

    qDebug() << "[SoundpadAudio] getSinkList() finished. Total:" << context.sinks.size();
    return context.sinks;
}

void SoundpadAudio::setOutputSink(const std::string& sinkName)
{
    qDebug() << "[SoundpadAudio] setOutputSink called with:" << QString::fromStdString(sinkName);
    QMutexLocker locker(&mutex_);
    outputSinkName_ = sinkName;
}

std::string SoundpadAudio::getOutputSink() const
{
    QMutexLocker locker(&mutex_);
    return outputSinkName_;
}

// Modify playbackThreadFunc to use both the selected output sink and the virtual sink
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
    
    // Determine which sink to use for user's headphones
    std::string headphonesSink;
    {
        QMutexLocker locker(&mutex_);
        headphonesSink = outputSinkName_;
    }
    
    // Create a client for the virtual sink (for mic) and a client for the headphones
    pa_simple *virtualSink = nullptr;
    pa_simple *headphonesOutput = nullptr;
    
    // Always connect to the virtual sink for mic merging
    virtualSink = pa_simple_new(
        nullptr, "SoundpadAppVirtual", PA_STREAM_PLAYBACK, sinkName_.c_str(),
        "virtual-playback", &ss, nullptr, nullptr, &error
    );
    
    if (!virtualSink) {
        qDebug() << "[SoundpadAudio] Failed to connect to virtual sink:" << pa_strerror(error);
        emit playbackStopped();
        return;
    }
    
    // Connect to the headphones output if specified
    if (!headphonesSink.empty()) {
        qDebug() << "[SoundpadAudio] Also connecting to headphones sink:" << QString::fromStdString(headphonesSink);
        headphonesOutput = pa_simple_new(
            nullptr, "SoundpadAppHeadphones", PA_STREAM_PLAYBACK, headphonesSink.c_str(),
            "headphones-playback", &ss, nullptr, nullptr, &error
        );
        
        if (!headphonesOutput) {
            qDebug() << "[SoundpadAudio] Failed to connect to headphones sink:" << pa_strerror(error);
            // Continue anyway - we'll still output to the virtual sink
        }
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
        
        // Write to virtual sink (for mic)
        if (pa_simple_write(virtualSink, buffer.data(), toWrite, &error) < 0) {
            qDebug() << "[SoundpadAudio] pa_simple_write to virtual sink failed:" << pa_strerror(error);
            // Continue anyway, don't break the loop
        }
        
        // Write to headphones if connected
        if (headphonesOutput) {
            if (pa_simple_write(headphonesOutput, buffer.data(), toWrite, &error) < 0) {
                qDebug() << "[SoundpadAudio] pa_simple_write to headphones failed:" << pa_strerror(error);
                // Continue anyway, don't break the loop
            }
        }
        
        playedBytes += toWrite;
        {
            QMutexLocker locker(&mutex_);
            currentMs_ = (playedBytes * 1000) / bytesPerSec;
        }
        emit playbackProgress(currentMs_);
        double msPerBlock = (double)toWrite / (double)bytesPerSec * 1000.0;
        if (msPerBlock > 0.0)
            std::this_thread::sleep_for(std::chrono::milliseconds((int)msPerBlock));
    }
    
    // Drain and free both outputs
    if (virtualSink) {
        pa_simple_drain(virtualSink, &error);
        pa_simple_free(virtualSink);
    }
    
    if (headphonesOutput) {
        pa_simple_drain(headphonesOutput, &error);
        pa_simple_free(headphonesOutput);
    }
    
    qDebug() << "[SoundpadAudio] playbackThreadFunc finished";
    emit playbackStopped();
}


bool SoundpadAudio::mergeWithMic(const std::string& sourceName)
{
    qDebug() << "[SoundpadAudio] mergeWithMic called for source:" << QString::fromStdString(sourceName);
    ensureAudioObjectsExist(sinkName_);
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
