#pragma once
#include <stdint.h>
#include <math.h>

// ============================================================================
// CombFilterI16<CAPACITY> / AllpassFilterI16<CAPACITY> - building block del
// Reverb (algoritmo di Schroeder/Moorer semplificato: comb filter in
// parallelo con damping nel feedback, seguiti da allpass in serie per la
// diffusione). Storage a 16 bit per risparmiare RAM.
// CAPACITY = lunghezza del ritardo in campioni (qui capacita' = ritardo,
// nessuno spazio extra: e' un reverb, non serve modulare il tempo).
// ============================================================================

template <uint32_t CAPACITY>
class AllpassFilterI16
{
public:
  void begin(float g)
  {
    gain = g;
    for (uint32_t i = 0; i < CAPACITY; i++) buf[i] = 0;
    idx = 0;
  }

  inline float process(float in)
  {
    float delayed = (float)buf[idx] / 32767.0f;
    float x   = in + delayed * gain;
    float out = -x * gain + delayed;

    int32_t q = (int32_t)(x * 32767.0f);
    if (q >  32767) q =  32767;
    if (q < -32768) q = -32768;
    buf[idx] = (int16_t)q;

    idx = (idx + 1) % CAPACITY;
    return out;
  }

private:
  int16_t buf[CAPACITY];
  uint32_t idx = 0;
  float gain = 0.5f;
};