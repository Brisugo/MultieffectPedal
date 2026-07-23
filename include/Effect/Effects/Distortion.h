#pragma once
#include "../Effect.h"

// ============================================================================
// Distortion - soft clipping (approssimazione razionale di tanh, Pade 3/3,
// piu' economica di una chiamata a tanhf() reale ma con curva simile).
// ============================================================================
class DistortionEffect : public AudioEffect
{
public:
  using AudioEffect::AudioEffect;

  void begin() override { updateParams(); }

  inline void setDrive(float g) { drive = g; }

  void updateParams() override
  {
    setDrive(Params::Distortion::_drive);
  }

  float process(float in) override
  {
    if (!enabled) return in;
    float x = in * drive;
    if (x >  3.0f) return  1.0f;
    if (x < -3.0f) return -1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
  }

private:  
  float drive;
};
