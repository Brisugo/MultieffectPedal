#pragma once
#include "../Effect.h"

// ============================================================================
// Fuzz - hard clipping con leggera asimmetria (piu' "sporco" e squadrato
// della Distortion).
// ============================================================================
class FuzzEffect : public AudioEffect
{
public:
  using AudioEffect::AudioEffect;

  void begin() override { updateParams(); }

  inline void setDrive(float g)     { drive = g; }
  inline void setAsymmetry(float a) { asymmetry = a; }

  void updateParams() override
  {
    setDrive(Params::Fuzz::_drive);
    setAsymmetry(Params::Fuzz::_asymmetry);
  }

  float process(float in) override
  {
    if (!enabled) return in;
    float x = in * drive + asymmetry;
    if (x >  1.0f) x =  1.0f;
    if (x < -1.0f) x = -1.0f;
    return x;
  }

private:
  float drive;
  float asymmetry;
};