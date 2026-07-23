#pragma once
#include <stdint.h>
#include <math.h>

// ============================================================================
// DelayLine<N> - linea di ritardo circolare in virgola mobile, con lettura
// a ritardo frazionario (interpolazione lineare). N deve essere potenza di 2.
// Componente riusabile: alla base di flanger, chorus, echo, ecc.
// ============================================================================
template <uint16_t MAX_SAMPLES>   // MAX_SAMPLES deve essere una potenza di 2
class DelayLine
{
public:
  void begin()
  {
    for (uint16_t i = 0; i < MAX_SAMPLES; i++) buf[i] = 0.0f;
    writeIdx = 0;
  }

  inline void write(float sample)
  {
    buf[writeIdx] = sample;
    writeIdx = (writeIdx + 1) & (MAX_SAMPLES - 1);
  }

  // delaySamples puo' essere frazionario, deve stare in [0, MAX_SAMPLES)
  inline float read(float delaySamples)
  {
    float readPos = (float)writeIdx - delaySamples;
    while (readPos < 0.0f) readPos += (float)MAX_SAMPLES;

    uint16_t idx0 = ((uint16_t)readPos) & (MAX_SAMPLES - 1);
    uint16_t idx1 = (idx0 + 1) & (MAX_SAMPLES - 1);
    float frac = readPos - (float)((uint16_t)readPos);

    return buf[idx0] + frac * (buf[idx1] - buf[idx0]);
  }

private:
  float buf[MAX_SAMPLES];
  uint16_t writeIdx = 0;
};

// ============================================================================
// DelayLineI16<N> - come DelayLine<N> ma con storage a 16 bit invece di
// float (meta' della RAM), pensata per gli echi lunghi del Delay. Ritardo
// sempre intero in campioni (nessuna interpolazione: per un semplice
// delay/echo non serve, a differenza del flanger/chorus modulati).
// N deve essere potenza di 2.
// ============================================================================
template <uint32_t MAX_SAMPLES>
class DelayLineI16
{
public:
  void begin()
  {
    for (uint32_t i = 0; i < MAX_SAMPLES; i++) buf[i] = 0;
    writeIdx = 0;
  }

  inline void write(float sample)
  {
    int32_t q = (int32_t)(sample * 32767.0f);
    if (q >  32767) q =  32767;
    if (q < -32768) q = -32768;
    buf[writeIdx] = (int16_t)q;
    writeIdx = (writeIdx + 1) & (MAX_SAMPLES - 1);
  }

  inline float read(uint32_t delaySamples)
  {
    uint32_t idx = (writeIdx - delaySamples) & (MAX_SAMPLES - 1);
    return (float)buf[idx] / 32767.0f;
  }

  static constexpr uint32_t capacity() { return MAX_SAMPLES; }

private:
  int16_t buf[MAX_SAMPLES];
  uint32_t writeIdx = 0;
};

// Delay line con compressione IMA ADPCM (4 bit/campione, 4:1 vs PCM16).
// Random-access in lettura tramite keyframe periodici dello stato encoder.
// Nessuna allocazione dinamica, nessuna FPU richiesta per il codec.

template <uint32_t CAPACITY_SAMPLES, uint32_t KEYFRAME_INTERVAL = 8>
class DelayLineADPCM {
    static_assert(CAPACITY_SAMPLES % 2 == 0, "CAPACITY_SAMPLES deve essere pari");
    static_assert(CAPACITY_SAMPLES % KEYFRAME_INTERVAL == 0, "CAPACITY_SAMPLES multiplo di KEYFRAME_INTERVAL");

public:
    void begin() {
        memset(_data, 0, sizeof(_data));
        memset(_keyPredictor, 0, sizeof(_keyPredictor));
        memset(_keyIndex, 0, sizeof(_keyIndex));
        _writePos = 0;
        _encPredictor = 0;
        _encIndex = 0;
    }

    void write(float sample) {
        // salva lo stato dell'encoder all'inizio di ogni blocco, PRIMA di codificare
        if ((_writePos % KEYFRAME_INTERVAL) == 0) {
            uint32_t k = _writePos / KEYFRAME_INTERVAL;
            _keyPredictor[k] = (int16_t)_encPredictor;
            _keyIndex[k] = (int8_t)_encIndex;
        }

        int16_t pcm = floatToPCM16(sample);
        uint8_t nibble = encodeNibble(pcm);
        storeNibble(_writePos, nibble);

        _writePos = (_writePos + 1) % CAPACITY_SAMPLES;
    }

    // delaySamples: distanza in campioni dall'ultimo write(). Deve essere < capacity().
    float read(uint32_t delaySamples) {
        if (delaySamples >= CAPACITY_SAMPLES) delaySamples = CAPACITY_SAMPLES - 1;

        uint32_t target = (_writePos + CAPACITY_SAMPLES - 1 - delaySamples) % CAPACITY_SAMPLES;
        uint32_t block = target / KEYFRAME_INTERVAL;
        uint32_t blockStart = block * KEYFRAME_INTERVAL;

        int32_t predictor = _keyPredictor[block];
        int32_t index = _keyIndex[block];

        int16_t pcm = (int16_t)predictor;
        for (uint32_t pos = blockStart; pos <= target; pos++) {
            uint8_t nibble = loadNibble(pos);
            pcm = decodeNibble(nibble, predictor, index);
        }

        return pcm / 32768.0f;
    }

    uint32_t capacity() const { return CAPACITY_SAMPLES; }

private:
    uint8_t _data[CAPACITY_SAMPLES / 2]; // 4 bit/campione

    static const uint32_t NUM_KEYFRAMES = CAPACITY_SAMPLES / KEYFRAME_INTERVAL;
    int16_t _keyPredictor[NUM_KEYFRAMES];
    int8_t  _keyIndex[NUM_KEYFRAMES];

    uint32_t _writePos = 0;
    int32_t  _encPredictor = 0; // stato encoder, avanza ad ogni write()
    int32_t  _encIndex = 0;

    static const int16_t stepTable[89];
    static const int8_t  indexTable[16];

    static int16_t floatToPCM16(float s) {
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        return (int16_t)(s * 32767.0f);
    }

    void storeNibble(uint32_t pos, uint8_t nibble) {
        uint32_t b = pos / 2;
        if (pos & 1) _data[b] = (_data[b] & 0x0F) | (nibble << 4);
        else         _data[b] = (_data[b] & 0xF0) | (nibble & 0x0F);
    }

    uint8_t loadNibble(uint32_t pos) const {
        uint32_t b = pos / 2;
        return (pos & 1) ? ((_data[b] >> 4) & 0x0F) : (_data[b] & 0x0F);
    }

    uint8_t encodeNibble(int16_t sample) {
        int32_t diff = sample - _encPredictor;
        uint8_t nibble = 0;
        if (diff < 0) { nibble = 8; diff = -diff; }

        int32_t step = stepTable[_encIndex];
        int32_t diffq = step >> 3;
        if (diff >= step) { nibble |= 4; diff -= step; diffq += step; }
        step >>= 1;
        if (diff >= step) { nibble |= 2; diff -= step; diffq += step; }
        step >>= 1;
        if (diff >= step) { nibble |= 1; diffq += step; }

        _encPredictor += (nibble & 8) ? -diffq : diffq;
        if (_encPredictor > 32767)  _encPredictor = 32767;
        if (_encPredictor < -32768) _encPredictor = -32768;

        _encIndex += indexTable[nibble];
        if (_encIndex < 0)  _encIndex = 0;
        if (_encIndex > 88) _encIndex = 88;

        return nibble;
    }

    static int16_t decodeNibble(uint8_t nibble, int32_t &predictor, int32_t &index) {
        int32_t step = stepTable[index];
        int32_t diffq = step >> 3;
        if (nibble & 4) diffq += step;
        if (nibble & 2) diffq += step >> 1;
        if (nibble & 1) diffq += step >> 2;

        predictor += (nibble & 8) ? -diffq : diffq;
        if (predictor > 32767)  predictor = 32767;
        if (predictor < -32768) predictor = -32768;

        index += indexTable[nibble];
        if (index < 0)  index = 0;
        if (index > 88) index = 88;

        return (int16_t)predictor;
    }
};

template <uint32_t C, uint32_t K>
const int16_t DelayLineADPCM<C,K>::stepTable[89] = {
    7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,
    66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,
    371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,
    1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,
    6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,
    20350,22385,24623,27086,29794,32767
};

template <uint32_t C, uint32_t K>
const int8_t DelayLineADPCM<C,K>::indexTable[16] = {
    -1,-1,-1,-1,2,4,6,8, -1,-1,-1,-1,2,4,6,8
};