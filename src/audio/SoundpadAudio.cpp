#include "SoundpadAudio.hpp"
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <cstdlib>
#include <QDebug>
#include <fstream>
#include <iostream>
#include <vector>

namespace soundpad {

SoundpadAudio::SoundpadAudio(const std::string& sinkName)
    : sinkName_(sinkName)
{
    qDebug() << "SoundpadAudio created with sink:" << QString::fromStdString(sinkName_);
}

SoundpadAudio::~SoundpadAudio()
{
    qDebug() << "SoundpadAudio destroyed";
}

bool SoundpadAudio::playWav(const std::string& wavFilePath) {
    std::ifstream file(wavFilePath, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open WAV file: " << wavFilePath << std::endl;
        return false;
    }

    // Пропускаем 44 байта заголовка WAV
    file.seekg(44);

    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };

    int error;
    pa_simple *s = pa_simple_new(
        nullptr,
        "SoundpadApp",
        PA_STREAM_PLAYBACK,
        sinkName_.c_str(),
        "playback",
        &ss,
        nullptr,
        nullptr,
        &error
    );

    if (!s) {
        std::cerr << "pa_simple_new() failed: " << pa_strerror(error) << std::endl;
        return false;
    }

    constexpr size_t bufferSize = 4096;
    std::vector<char> buffer(bufferSize);

    while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
        if (pa_simple_write(s, buffer.data(), static_cast<size_t>(file.gcount()), &error) < 0) {
            std::cerr << "pa_simple_write() failed: " << pa_strerror(error) << std::endl;
            pa_simple_free(s);
            return false;
        }
    }

    if (pa_simple_drain(s, &error) < 0) {
        std::cerr << "pa_simple_drain() failed: " << pa_strerror(error) << std::endl;
        pa_simple_free(s);
        return false;
    }

    pa_simple_free(s);
    return true;
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
