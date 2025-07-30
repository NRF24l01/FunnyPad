#pragma once
#include <string>
#include <vector>
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

namespace soundpad {

class SoundpadAudio : public QObject {
    Q_OBJECT
public:
    explicit SoundpadAudio(const std::string& sinkName = "SoundpadSink");
    ~SoundpadAudio();

    // Воспроизвести WAV-файл (16-bit PCM, 44.1kHz, stereo)
    bool playWav(const std::string& wavFilePath); // start playback (async)
    void stop();
    void seek(qint64 ms);
    qint64 currentTime() const;
    qint64 totalTime() const;

    // Получить список всех источников звука: (имя, описание)
    std::vector<std::pair<std::string, std::string>> getSourceList();
    
    // Получить список всех устройств вывода: (имя, описание)
    std::vector<std::pair<std::string, std::string>> getSinkList();

    // Подключить выбранный source к нашей sink через loopback
    bool mergeWithMic(const std::string& sourceName);
    
    // Установить устройство вывода для воспроизведения
    void setOutputSink(const std::string& sinkName);
    
    // Получить текущее устройство вывода
    std::string getOutputSink() const;

signals:
    void playbackStarted(qint64 totalMs);
    void playbackProgress(qint64 currentMs);
    void playbackStopped();

private:
    void playbackThreadFunc(const std::string& wavFilePath);

    // PulseAudio helpers
    static bool sinkExists(const std::string& sinkName);
    static bool sourceExists(const std::string& sourceName);
    static void createNullSink(const std::string& sinkName);
    static void createRemapSource(const std::string& masterMonitor, const std::string& sourceName);
    static void ensureAudioObjectsExist(const std::string& sinkName);

    std::string sinkName_;        // Virtual sink for mic merging
    std::string outputSinkName_;  // Selected output device for playback
    QThread workerThread_;
    mutable QMutex mutex_;
    QWaitCondition seekCond_;
    bool stopRequested_ = false;
    qint64 seekToMs_ = -1;
    qint64 currentMs_ = 0;
    qint64 totalMs_ = 0;
    std::string currentFile_;
};

} // namespace soundpad
