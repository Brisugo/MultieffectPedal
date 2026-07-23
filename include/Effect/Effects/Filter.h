#pragma once
#include "../Effect.h"

// ============================================================================
// Filter - wrapper AudioEffect attorno a BiquadFilter.
// ============================================================================
class FilterEffect : public AudioEffect
{
public:
  using AudioEffect::AudioEffect;

  void begin() override
  {
    biquad.begin(SAMPLE_RATE_HZ);

    updateParams();
  }

  inline void setType(FilterType t) { biquad.setType(t); }
  inline void setCutoff(float hz)   { biquad.setCutoff(hz); }
  inline void setQ(float q)         { biquad.setQ(q); }

  void updateParams() override
  {
    setType((FilterType)Params::Filter::_type);
    setCutoff(Params::Filter::_cutoff);
    setQ(Params::Filter::_q);
  }

  float process(float in) override
  {
    if (!enabled) return in;
    return biquad.process(in);
  }

private:  
  BiquadFilter biquad;
};