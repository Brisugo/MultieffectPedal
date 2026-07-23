#pragma once
#include <stdint.h>
#include <math.h>

// ============================================================================
// LFO - oscillatore a bassa frequenza per la modulazione di flanger/phaser.
// Usa una tabella di lookup (calcolata una sola volta in begin(), fuori
// dal percorso critico) con interpolazione lineare, per evitare di
// chiamare sinf() ad ogni campione dentro l'ISR.
// ============================================================================
class LFO
{
public:
  static const uint16_t TABLE_SIZE = 256;   // potenza di 2

  void begin(float sampleRateHz)
  {
    fs = sampleRateHz;
    for (uint16_t i = 0; i < TABLE_SIZE; i++) {
      sineTable[i] = sinf(2.0f * (float)M_PI * (float)i / (float)TABLE_SIZE);
    }
  }

  void setFrequency(float hz)
  {
    // Incremento di fase a 32 bit: phaseInc = (hz / fs) * 2^32
    phaseInc = (uint32_t)((hz / fs) * 4294967296.0f);
  }

  // Ritorna il prossimo valore dell'LFO, in [-1, +1].
  inline float next()
  {
    phase += phaseInc;
    uint16_t idx  = phase >> 24;                          // 8 bit alti -> indice tabella
    uint16_t idx2 = (idx + 1) & (TABLE_SIZE - 1);
    float frac = (float)((phase >> 8) & 0xFFFF) / 65536.0f;
    return sineTable[idx] + frac * (sineTable[idx2] - sineTable[idx]);
  }

private:
  float sineTable[TABLE_SIZE];
  uint32_t phase = 0;
  uint32_t phaseInc = 0;
  float fs = 48000.0f;
};