#pragma once
#include "../Effect.h"

// ============================================================================
// Flanger - linea di ritardo breve modulata da un LFO, con feedback e mix
// regolabili. Il ritardo di base + la profondita' vengono espressi in
// millisecondi e convertiti internamente in campioni in base al sample rate.
// ============================================================================
class FlangerEffect : public AudioEffect
{
public:
  using AudioEffect::AudioEffect;

  void begin() override
  {
    fs = SAMPLE_RATE_HZ;
    delayLine.begin();
    lfo.begin(fs);
    lfo.setFrequency(0.25f);
    setDepthMs(3.0f);
    setBaseDelayMs(4.0f);

    updateParams();
  }

  inline void setRateHz(float hz)      { lfo.setFrequency(hz); }
  inline void setDepthMs(float ms)     { depthSamples     = (ms * 0.001f) * fs; }
  inline void setBaseDelayMs(float ms) { baseDelaySamples = (ms * 0.001f) * fs; }
  inline void setFeedback(float fb)    { feedback = fb; }
  inline void setMix(float m)          { mix = m; }

  void updateParams() override
  {
    setRateHz(Params::Flanger::_rateHz);
    setDepthMs(Params::Flanger::_depthMs);
    setBaseDelayMs(Params::Flanger::_baseDelayMs);
    setFeedback(Params::Flanger::_feedback);
    setMix(Params::Flanger::_mix);
  }

  float process(float in) override
  {
    if (!enabled) return in;

    float lfoVal = lfo.next();                                 // [-1, +1]
    float delaySamples = baseDelaySamples + depthSamples * lfoVal;
    float delayed = delayLine.read(delaySamples);

    delayLine.write(in + delayed * feedback);

    return in * (1.0f - mix) + delayed * mix;
  }

private:
  DelayLineI16<1024> delayLine;   // fino a ~21ms di ritardo massimo @48kHz
  LFO lfo;
  float fs = 48000.0f;
  float baseDelaySamples;
  float depthSamples;
  float feedback;
  float mix;
};