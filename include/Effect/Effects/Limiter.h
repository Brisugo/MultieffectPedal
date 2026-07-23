#pragma once
#include "../Effect.h"

// ============================================================================
// Limiter - limiter feed-forward con envelope follower (attack/release a
// singolo polo). Pensato come ULTIMO effetto della catena, per contenere
// eventuali picchi generati dagli effetti precedenti (distorsione, feedback
// di delay/riverbero) prima della scrittura sul DAC.
// ============================================================================
class LimiterEffect : public AudioEffect
{
public:

  using AudioEffect::AudioEffect;

  void begin() override
  {
    fs = SAMPLE_RATE_HZ;
    envelope = 0.0f;

    updateParams();
  }

  inline void setThreshold(float t) { threshold = t; }
  inline void setAttackMs(float ms)  { attackCoeff  = expf(-1.0f / (0.001f * ms * fs)); }
  inline void setReleaseMs(float ms) { releaseCoeff = expf(-1.0f / (0.001f * ms * fs)); }

  void updateParams() override
  {
    setThreshold(Params::Limiter::_threshold);
    setAttackMs(Params::Limiter::_attackMs);
    setReleaseMs(Params::Limiter::_releaseMs);
  }
  
  float process(float in) override
  {
    if (!enabled) return in;


    float rectified = fabsf(in);
    if (rectified > envelope) envelope = attackCoeff  * envelope + (1.0f - attackCoeff)  * rectified;
    else                      envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * rectified;

    float gain = (envelope > threshold) ? (threshold / envelope) : 1.0f;

    float out = in * gain;
    if (out >  1.0f) out =  1.0f;    // clip di sicurezza finale, non dovrebbe mai scattare
    if (out < -1.0f) out = -1.0f;
    return out;
  }

private:
  float fs = 48000.0f;
  float threshold;
  float attackCoeff;
  float releaseCoeff;
  float envelope = 0.0f;
};