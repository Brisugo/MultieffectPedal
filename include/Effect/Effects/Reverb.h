#pragma once
#include "../Effect.h"

// ============================================================================
// Reverb - 4 comb filter in parallelo + 2 allpass in serie (Schroeder/
// Moorer semplificato). Le lunghezze dei ritardi sono calcolate a
// compile-time da SAMPLE_RATE_HZ (vedi AudioConfig.h), scalate a circa il
// 65% dei classici valori di Schroeder per restare in un budget di RAM
// ragionevole (~6.7KB invece di ~10KB): il risultato e' un riverbero di
// ambiente piccolo/medio, non una cattedrale. Se hai RAM libera e vuoi
// code piu' lunghe, alza il fattore 0.65 qui sotto verso 1.0.
// ============================================================================
class ReverbEffect : public AudioEffect
{
public:
  using AudioEffect::AudioEffect;
  
  void begin() override
  {
    comb1.begin(0.805f, 0.25f);
    comb2.begin(0.827f, 0.25f);
    comb3.begin(0.783f, 0.25f);
    comb4.begin(0.764f, 0.25f);
    allpass1.begin(0.7f);
    allpass2.begin(0.7f);

    updateParams();
  }

  void setMix(float m) { mix = m; }

  void updateParams() override
  {
    setMix(Params::Reverb::_mix);
  }

  float process(float in) override
  {
    if (!enabled) return in;

    float combSum = comb1.process(in) + comb2.process(in)
                   + comb3.process(in) + comb4.process(in);
    combSum *= 0.25f;

    float x = allpass2.process(allpass1.process(combSum));

    return in * (1.0f - mix) + x * mix;
  }

private:
  static constexpr float SCALE = 0.65f;   // vedi nota sopra sul budget RAM
  static constexpr uint32_t D1 = (uint32_t)(0.0297f * SCALE * SAMPLE_RATE_HZ);
  static constexpr uint32_t D2 = (uint32_t)(0.0371f * SCALE * SAMPLE_RATE_HZ);
  static constexpr uint32_t D3 = (uint32_t)(0.0411f * SCALE * SAMPLE_RATE_HZ);
  static constexpr uint32_t D4 = (uint32_t)(0.0437f * SCALE * SAMPLE_RATE_HZ);
  static constexpr uint32_t A1 = (uint32_t)(0.0050f * SAMPLE_RATE_HZ);
  static constexpr uint32_t A2 = (uint32_t)(0.0017f * SAMPLE_RATE_HZ);

  CombFilterI16<D1> comb1;
  CombFilterI16<D2> comb2;
  CombFilterI16<D3> comb3;
  CombFilterI16<D4> comb4;
  AllpassFilterI16<A1> allpass1;
  AllpassFilterI16<A2> allpass2;
  
  float mix;
};