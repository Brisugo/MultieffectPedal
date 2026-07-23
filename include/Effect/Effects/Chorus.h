#pragma once
#include "../Effect.h"

// ============================================================================
// Chorus - come il Flanger ma con base delay piu' lunga, poco/nessun
// feedback, e piu' voci moltiplicate a rate leggermente diversi tra loro
// (da qui il caratteristico effetto di "raddoppio" e ampiezza dello
// strumento). Riusa la classe DelayLine<N> gia' definita in AudioEffect.h.
// ============================================================================
class ChorusEffect : public AudioEffect
{
public:
  static const uint8_t NUM_VOICES = 2;

  using AudioEffect::AudioEffect;

  void begin() override
  {
    fs = SAMPLE_RATE_HZ;
    
    delayLine.begin();

    for (uint8_t i = 0; i < NUM_VOICES; i++) {
      lfo[i].begin(fs);
    }

    updateParams();
  }

  inline void setVoiceRateHz(uint8_t voice, float hz) { if (voice < NUM_VOICES) lfo[voice].setFrequency(hz); }
  inline void setDepthMs(float ms)                    { depthSamples = (ms * 0.001f) * fs; }
  inline void setBaseDelayMs(float ms)                { baseDelaySamples = (ms * 0.001f) * fs; }
  inline void setMix(float m)                         { mix = m; }

  void updateParams() override
  {
    setVoiceRateHz(0, Params::Chorus::_voice1RateHz);
    setVoiceRateHz(1, Params::Chorus::_voice2RateHz);
    setDepthMs(Params::Chorus::_depthMs);
    setBaseDelayMs(Params::Chorus::_baseDelayMs);
    setMix(Params::Chorus::_mix);
  }

  float process(float in) override
  {
    if (!enabled) return in;

    delayLine.write(in);

    float wet = 0.0f;
    for (uint8_t i = 0; i < NUM_VOICES; i++) {
      float lfoVal = lfo[i].next();
      float delaySamples = baseDelaySamples + depthSamples * lfoVal;
      wet += delayLine.read(delaySamples);
    }
    wet /= (float)NUM_VOICES;

    return in * (1.0f - mix) + wet * mix;
  }

private:
  DelayLineI16<1024> delayLine;   // >= baseDelay+depth in campioni (26ms @32kHz -> 832 campioni, ok)
  LFO lfo[NUM_VOICES];

  float fs;
  float baseDelaySamples;
  float depthSamples;
  float mix;
};