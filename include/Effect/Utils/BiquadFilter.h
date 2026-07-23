#pragma once
#include <stdint.h>
#include <math.h>

// ============================================================================
// FilterType / BiquadFilter - filtro biquad RBJ (Robert Bristow-Johnson
// "Audio EQ Cookbook"), riusabile per lowpass/highpass/bandpass/notch.
// I coefficienti si ricalcolano SOLO quando cambi tipo/cutoff/Q (chiamate
// a sinf/cosf), MAI dentro process(): nel percorso critico process() e'
// solo una somma pesata di pochi float.
// ============================================================================
enum class FilterType : uint8_t { LOWPASS, HIGHPASS, BANDPASS, NOTCH };

class BiquadFilter
{
public:
  void begin(float sampleRateHz)
  {
    fs = sampleRateHz;
    x1 = x2 = y1 = y2 = 0.0f;
    recompute();
  }

  void setType(FilterType t)   { type = t;   recompute(); }
  void setCutoff(float hz)     { cutoff = hz; recompute(); }
  void setQ(float q)           { Q = q;       recompute(); }

  inline float process(float in)
  {
    float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = in;
    y2 = y1; y1 = out;
    return out;
  }

private:
  void recompute()
  {
    float w0    = 2.0f * (float)M_PI * cutoff / fs;
    float cosw0 = cosf(w0);
    float alpha = sinf(w0) / (2.0f * Q);
    float a0;

    switch (type) {
      case FilterType::LOWPASS:
        b0 = (1.0f - cosw0) * 0.5f;
        b1 =  1.0f - cosw0;
        b2 = (1.0f - cosw0) * 0.5f;
        a0 =  1.0f + alpha;
        a1 = -2.0f * cosw0;
        a2 =  1.0f - alpha;
        break;
      case FilterType::HIGHPASS:
        b0 =  (1.0f + cosw0) * 0.5f;
        b1 = -(1.0f + cosw0);
        b2 =  (1.0f + cosw0) * 0.5f;
        a0 =  1.0f + alpha;
        a1 = -2.0f * cosw0;
        a2 =  1.0f - alpha;
        break;
      case FilterType::BANDPASS:
        b0 =  alpha;
        b1 =  0.0f;
        b2 = -alpha;
        a0 =  1.0f + alpha;
        a1 = -2.0f * cosw0;
        a2 =  1.0f - alpha;
        break;
      case FilterType::NOTCH:
      default:
        b0 =  1.0f;
        b1 = -2.0f * cosw0;
        b2 =  1.0f;
        a0 =  1.0f + alpha;
        a1 = -2.0f * cosw0;
        a2 =  1.0f - alpha;
        break;
    }
    b0 /= a0; b1 /= a0; b2 /= a0;
    a1 /= a0; a2 /= a0;
  }

  float fs = 48000.0f;
  float cutoff = 1000.0f;
  float Q = 0.707f;
  FilterType type = FilterType::LOWPASS;
  float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
  float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
};