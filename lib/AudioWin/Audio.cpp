#include "Audio.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <atomic>
#include <iostream>

namespace {
    enum class InputMode { Microphone, File, Generated };
    constexpr float PI2 = 6.28318530718f;
}

struct AudioImpl {
    ma_device        device{};
    ma_decoder       decoder{};
    bool             decoderOpen = false;

    InputMode        mode = InputMode::Microphone;
    float (*process)(float) = nullptr;

    // oscillatore per la modalita' "generato"
    float phase = 0.0f;
    float freq  = 440.0f; // La4, tono di test

    std::atomic<bool> running{false};
};

// ---------------------------------------------------------------------------
// Callback realtime: chiamata dal device audio per ogni blocco di campioni.
// Qui avviene, campione per campione, la chiamata a process() richiesta.
// ---------------------------------------------------------------------------
static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    AudioImpl* impl = static_cast<AudioImpl*>(pDevice->pUserData);
    float* out = static_cast<float*>(pOutput);
    const float* in = static_cast<const float*>(pInput);

    for (ma_uint32 i = 0; i < frameCount; i++) {
        float sample = 0.0f;

        switch (impl->mode) {
            case InputMode::Microphone: {
                sample = in ? in[i] : 0.0f;
                break;
            }
            case InputMode::File: {
                float frame = 0.0f;
                ma_uint64 framesRead = 0;
                if (impl->decoderOpen) {
                    ma_decoder_read_pcm_frames(&impl->decoder, &frame, 1, &framesRead);
                }
                if (framesRead == 0) {
                    // fine file: riparte in loop
                    ma_decoder_seek_to_pcm_frame(&impl->decoder, 0);
                    frame = 0.0f;
                }
                sample = frame;
                break;
            }
            case InputMode::Generated: {
                sample = sinf(impl->phase);
                impl->phase += PI2 * impl->freq / static_cast<float>(SAMPLE_RATE_HZ);
                if (impl->phase > PI2) impl->phase -= PI2;
                break;
            }
        }

        float processed = impl->process ? impl->process(sample) : 0.0f;

        // clamp di sicurezza per non danneggiare l'uscita/altoparlanti
        if (processed > 1.0f)  processed = 1.0f;
        if (processed < -1.0f) processed = -1.0f;

        out[i] = processed;
    }
}

Audio::Audio() : _impl(new AudioImpl()) {}

Audio::~Audio() {
    end();
    delete static_cast<AudioImpl*>(_impl);
}

bool Audio::begin(float (*process)(float)) {
    AudioImpl* _impl = static_cast<AudioImpl*>(this->_impl);
    _impl->process = process;

    // ---- scelta interattiva della sorgente audio -------------------------
    printf("Seleziona la sorgente audio in ingresso:\n");
    printf("  1) Microfono (live)\n");
    printf("  2) File audio (wav/mp3/flac...)\n");
    printf("  3) Segnale generato (tono di test)\n");
    printf("Scelta [1]: ");
    fflush(stdout);

    std::string line;
    std::getline(std::cin, line);
    int choice = line.empty() ? 1 : atoi(line.c_str());

    ma_device_config config;

    if (choice == 2) {
        _impl->mode = InputMode::File;

        printf("Percorso del file audio: ");
        fflush(stdout);
        std::string path;
        std::getline(std::cin, path);

        ma_decoder_config decConfig = ma_decoder_config_init(ma_format_f32, 1, SAMPLE_RATE_HZ);
        if (ma_decoder_init_file(path.c_str(), &decConfig, &_impl->decoder) != MA_SUCCESS) {
            printf("Impossibile aprire il file: %s\n", path.c_str());
            return false;
        }
        _impl->decoderOpen = true;

        config = ma_device_config_init(ma_device_type_playback);
        config.playback.format   = ma_format_f32;
        config.playback.channels = 1;
        config.sampleRate        = SAMPLE_RATE_HZ;
    }
    else if (choice == 3) {
        _impl->mode = InputMode::Generated;

        config = ma_device_config_init(ma_device_type_playback);
        config.playback.format   = ma_format_f32;
        config.playback.channels = 1;
        config.sampleRate        = SAMPLE_RATE_HZ;
    }
    else {
        _impl->mode = InputMode::Microphone;

        config = ma_device_config_init(ma_device_type_duplex);
        config.capture.format    = ma_format_f32;
        config.capture.channels  = 1;
        config.playback.format   = ma_format_f32;
        config.playback.channels = 1;
        config.sampleRate        = SAMPLE_RATE_HZ;
    }

    config.dataCallback = data_callback;
    config.pUserData    = _impl;

    if (ma_device_init(nullptr, &config, &_impl->device) != MA_SUCCESS) {
        printf("Errore nell'inizializzazione del device audio.\n");
        return false;
    }

    if (ma_device_start(&_impl->device) != MA_SUCCESS) {
        printf("Errore nell'avvio del device audio.\n");
        ma_device_uninit(&_impl->device);
        return false;
    }

    _impl->running = true;
    printf("Audio avviato (%d Hz).\n", SAMPLE_RATE_HZ);
    return true;
}

void Audio::end() {
    AudioImpl* _impl = static_cast<AudioImpl*>(this->_impl);
    if (_impl->running) {
        ma_device_uninit(&_impl->device);
        _impl->running = false;
    }
    if (_impl->decoderOpen) {
        ma_decoder_uninit(&_impl->decoder);
        _impl->decoderOpen = false;
    }
}
