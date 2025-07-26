#pragma once
#include <string>
#include <vector>

namespace soundpad {

class SoundpadAudio {
public:
    explicit SoundpadAudio(const std::string& sinkName = "SoundpadSink");
    ~SoundpadAudio();

    // Воспроизвести WAV-файл (16-bit PCM, 44.1kHz, stereo)
    bool playWav(const std::string& wavFilePath);

    // Получить список всех источников звука: (имя, описание)
    std::vector<std::pair<std::string, std::string>> getSourceList();

    // Подключить выбранный source к нашей sink через loopback
    bool mergeWithMic(const std::string& sourceName);

private:
    std::string sinkName_;
};

} // namespace soundpad
