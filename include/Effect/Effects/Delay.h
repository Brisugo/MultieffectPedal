#pragma once
#include "../Effect.h"

// ============================================================================
// Delay - eco singolo con feedback e mix regolabili.
// DELAY_MAX_SAMPLES = 4096 @ 32kHz -> ~128ms di ritardo massimo, 8KB di RAM.
// Alza/abbassa questa costante (potenza di 2!) in base al tempo di delay
// che ti serve e alla RAM che hai a disposizione.
// ============================================================================
class DelayEffect : public AudioEffect
{
public:
  using AudioEffect::AudioEffect;

  static const uint32_t DELAY_MAX_SAMPLES = 4096;

  void begin() override
  {
    fs = SAMPLE_RATE_HZ;
    delayLine.begin();

    updateParams();
  }

  void setTimeMs(float ms)
  {
    uint32_t d = (uint32_t)((ms * 0.001f) * fs);
    if (d >= DELAY_MAX_SAMPLES) d = DELAY_MAX_SAMPLES - 1;
    delaySamples = d;
  }
  void setFeedback(float fb) { feedback = fb; }
  void setMix(float m)       { mix = m; }

  void updateParams() override
  {
    setTimeMs(Params::Delay::_timeMs);
    setFeedback(Params::Delay::_feedback);
    setMix(Params::Delay::_mix);
  }

  float process(float in) override
  {
    if (!enabled) return in;
    float delayed = delayLine.read(delaySamples);
    delayLine.write(in + delayed * feedback);
    return in * (1.0f - mix) + delayed * mix;
  }

private:
  DelayLineI16<DELAY_MAX_SAMPLES> delayLine;
  float fs;
  uint32_t delaySamples;
  float feedback;
  float mix;
};