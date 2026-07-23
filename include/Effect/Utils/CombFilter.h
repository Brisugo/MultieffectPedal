#pragma once
#include <stdint.h>
#include <math.h>

template <uint32_t CAPACITY>
class CombFilterI16
{
public:
  void begin(float feedbackGain, float dampingCoeff)
  {
    feedback = feedbackGain;
    damping  = dampingCoeff;
    for (uint32_t i = 0; i < CAPACITY; i++) buf[i] = 0;
    idx = 0;
    lp = 0.0f;
  }

  inline float process(float in)
  {
    float delayed = (float)buf[idx] / 32767.0f;
    lp = delayed * (1.0f - damping) + lp * damping;   // damping = assorbimento HF nel feedback
    float x = in + lp * feedback;

    int32_t q = (int32_t)(x * 32767.0f);
    if (q >  32767) q =  32767;
    if (q < -32768) q = -32768;
    buf[idx] = (int16_t)q;

    idx = (idx + 1) % CAPACITY;
    return delayed;
  }

private:
  int16_t buf[CAPACITY];
  uint32_t idx = 0;
  float feedback = 0.5f;
  float damping = 0.2f;
  float lp = 0.0f;
};