#pragma once
#include "../Effect.h"

// ============================================================================
// Phaser - cascata di filtri all-pass del primo ordine, con coefficiente
// modulato da un LFO condiviso tra gli stadi (effetto classico "swoosh").
// ============================================================================
class PhaserEffect : public AudioEffect {
public:
  using AudioEffect::AudioEffect;

  static const uint8_t NUM_STAGES = 4;

  void begin() override
  {
    fs = SAMPLE_RATE_HZ;
    lfo.begin(fs);
    for (uint8_t i = 0; i < NUM_STAGES; i++) z[i] = 0.0f;

    updateParams();
  }

  inline void setRateHz(float hz)   { lfo.setFrequency(hz); }
  inline void setDepth(float d)     { depth = d; }       // 0..1
  inline void setFeedback(float fb) { feedback = fb; }
  inline void setMix(float m)       { mix = m; }

  void updateParams() override
  {
    setRateHz(Params::Phaser::_rateHz);
    setDepth(Params::Phaser::_depth);
    setFeedback(Params::Phaser::_feedback);
    setMix(Params::Phaser::_mix);
  }

  float process(float in) override
  {
    if (!enabled) return in;

    float lfoVal = lfo.next();                    // [-1, +1]
    // coefficiente dell'allpass, mantenuto in un range che garantisce stabilita'
    float a = 0.1f + depth * 0.35f * (lfoVal + 1.0f) * 0.5f;

    float x = in + z[NUM_STAGES - 1] * feedback;
    for (uint8_t i = 0; i < NUM_STAGES; i++) {
      float y = -a * x + z[i];
      z[i] = x + a * y;
      x = y;
    }

    return in * (1.0f - mix) + x * mix;
  }

private:
  LFO lfo;
  float fs;
  float z[NUM_STAGES] = {0};
  float depth;
  float feedback;
  float mix;
};