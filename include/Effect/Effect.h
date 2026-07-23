#pragma once
#include <stdint.h>
#include <math.h>

#include "Utils/AllPassFilter.h"
#include "Utils/BiquadFilter.h"
#include "Utils/CombFilter.h"
#include "Utils/DelayLine.h"
#include "Utils/LFO.h"
#include "Utils/Param.h"

// ============================================================================
// AudioEffect - classe base per ogni effetto della catena.
// process() riceve/restituisce un campione float in [-1.0, +1.0].
// Tutti gli effetti concreti vivono come istanze statiche (nessun new/malloc
// nel percorso critico dell'ISR audio).
// ============================================================================
class AudioEffect
{
public:
    virtual ~AudioEffect() {}
    virtual void begin() {}
    virtual float process(float in) = 0;
    virtual void updateParams() = 0;

    AudioEffect(const char* name, uint8_t id, ParameterManager parameterManager) : name(name), id(id), parameterManager(parameterManager) {}

    void setEnabled(bool en) { enabled = en; }
    bool isEnabled() const   { return enabled; }

    uint8_t getId() { return id; }
    const char* getName() { return name; }

    ParameterManager parameterManager;

protected:
    bool enabled = true;
    uint8_t id;
    const char* name;
};

namespace Params{
    namespace Chorus{
        const int n_params = 5;

        float _mix;
        float _depthMs;
        float _baseDelayMs;
        float _voice1RateHz;
        float _voice2RateHz;
    
        const ParamStruct<float> voice1=    {"voice1",    .settable=true, .default_value=0.20f, .min=0.05f, .max=2.0f};
        const ParamStruct<float> voice2=    {"voice2",    .settable=true, .default_value=0.27f, .min=0.05f, .max=2.0f};
        const ParamStruct<float> depth=     {"depth",     .settable=true, .default_value=6.0f,  .min=0.0f,  .max=20.0f};
        const ParamStruct<float> basedelay= {"basedelay", .settable=true, .default_value=20.0f, .min=5.0f,  .max=40.0f};
        const ParamStruct<float> mix=       {"mix",       .settable=true, .default_value=0.5f,  .min=0.0f,  .max=1.0f};

        Param params[n_params] = { Param(&_mix, mix), Param(&_depthMs, depth), Param(&_baseDelayMs, basedelay), Param(&_voice1RateHz, voice1), Param(&_voice2RateHz, voice2) };
        ParameterManager pm(params, n_params);
    }

    namespace Flanger{
        const int n_params = 5;

        float _rateHz;
        float _depthMs;
        float _baseDelayMs;
        float _feedback;
        float _mix;
    
        const ParamStruct<float> rate=      {"rate",      .settable=true, .default_value=0.25f, .min=0.05f, .max=5.0f};
        const ParamStruct<float> depth=     {"depth",     .settable=true, .default_value=3.0f,  .min=0.0f,  .max=15.0f};
        const ParamStruct<float> basedelay= {"basedelay", .settable=true, .default_value=4.0f,  .min=0.5f,  .max=15.0f};
        const ParamStruct<float> feedback=  {"feedback",  .settable=true, .default_value=0.3f,  .min=-0.95f,.max=0.95f};
        const ParamStruct<float> mix=       {"mix",       .settable=true, .default_value=0.5f,  .min=0.0f,  .max=1.0f};

        Param params[n_params] = { Param(&_rateHz, rate), Param(&_depthMs, depth), Param(&_baseDelayMs, basedelay), Param(&_feedback, feedback), Param(&_mix, mix) };
        ParameterManager pm(params, n_params);
    }

    namespace Limiter{
        const int n_params = 3;

        float _threshold;
        float _attackMs;
        float _releaseMs;
    
        const ParamStruct<float> threshold= {"threshold",.settable=true, .default_value=0.85f, .min=0.1f, .max=1.0f};
        const ParamStruct<float> attack=    {"attack",   .settable=true, .default_value=5.0f,  .min=0.1f, .max=50.0f};
        const ParamStruct<float> release=   {"release",  .settable=true, .default_value=60.0f, .min=5.0f, .max=500.0f};

        Param params[n_params] = { Param(&_threshold, threshold), Param(&_attackMs, attack), Param(&_releaseMs, release) };
    
        ParameterManager pm(params, n_params);
    }

    namespace Distortion{
        const int n_params = 1;

        float _drive;

        const ParamStruct<float> drive=     {"drive",  .settable=true, .default_value=6.0f,   .min=1.0f,  .max=40.0f};
    
        Param params[n_params] = { Param(&_drive, drive) };
        ParameterManager pm(params, n_params);
    }

    namespace Fuzz{
        const int n_params = 2;

        float _drive;
        float _asymmetry;

        const ParamStruct<float> drive=     {"drive",  .settable=true, .default_value=18.0f,  .min=1.0f,  .max=60.0f};
        const ParamStruct<float> asym=      {"asym",   .settable=true, .default_value=0.1f,   .min=-1.0f, .max=1.0f};
    
        Param params[n_params] = { Param(&_drive, drive), Param(&_asymmetry, asym) };
        ParameterManager pm(params, n_params);
    }

    namespace Phaser{
        const int n_params = 4;

        float _rateHz;
        float _depth;
        float _feedback;
        float _mix;

        const ParamStruct<float> rate=      {"rate",     .settable=true, .default_value=0.5f,  .min=0.05f, .max=5.0f};
        const ParamStruct<float> depth=     {"depth",    .settable=true, .default_value=0.6f,  .min=0.0f,  .max=1.0f};
        const ParamStruct<float> feedback=  {"feedback", .settable=true, .default_value=0.3f,  .min=-0.95f,.max=0.95f};
        const ParamStruct<float> mix=       {"mix",      .settable=true, .default_value=0.5f,  .min=0.0f,  .max=1.0f};
    
        Param params[n_params] = { Param(&_rateHz, rate), Param(&_depth, depth), Param(&_feedback, feedback), Param(&_mix, mix) };
        ParameterManager pm(params, n_params);
    }

    namespace Filter{
        const int n_params = 3;

        int _type;
        float _cutoff;
        float _q;

        const ParamStruct<int>   type=      {"type",     .settable=true, .default_value=0,       .min=0,     .max=3};
        const ParamStruct<float> cutoff=    {"cutoff",   .settable=true, .default_value=2000.0f, .min=20.0f, .max=20000.0f};
        const ParamStruct<float> q=         {"q",        .settable=true, .default_value=0.707f,  .min=0.1f,  .max=10.0f};
    
        Param params[n_params] = { Param(&_type, type), Param(&_cutoff, cutoff), Param(&_q, q) };
        ParameterManager pm(params, n_params);
    }

    namespace Delay{
        const int n_params = 3;

        float _timeMs;
        float _feedback;
        float _mix;

        const ParamStruct<float> time=      {"time",     .settable=true, .default_value=300.0f, .min=1.0f, .max=120.0f};
        const ParamStruct<float> feedback=  {"feedback", .settable=true, .default_value=0.35f,  .min=0.0f, .max=0.95f};
        const ParamStruct<float> mix=       {"mix",      .settable=true, .default_value=0.35f,  .min=0.0f, .max=1.0f};
    
        Param params[n_params] = { Param(&_timeMs, time), Param(&_feedback, feedback), Param(&_mix, mix) };
        ParameterManager pm(params, n_params);
    }

    namespace Reverb{
        const int n_params = 1;

        float _mix;

        const ParamStruct<float> mix=       {"mix",      .settable=true, .default_value=0.25f, .min=0.0f, .max=1.0f};
    
        Param params[n_params] = { Param(&_mix, mix) };
        ParameterManager pm(params, n_params);
    }
}