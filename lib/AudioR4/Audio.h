#pragma once

#include "AudioAnalog.h"
#include "FspTimer.h"

#include "Config.h"

class Audio{
public:
    inline void init(){
        _ADC::setup_adc();
        _DAC::setup_dac();
    }

    // ---------------------------------------------------------------------------
    // Parametri audio
    // ---------------------------------------------------------------------------
    constexpr static float ADC_MID = 8192.0f;           // meta' scala 14 bit (centro = DC offset +1.6V)
    constexpr static float DAC_MID  = 2048.0f;          // meta' scala 12 bit

    // ---------------------------------------------------------------------------
    // Timer hardware periodico, interrupt a SAMPLE_RATE_HZ.
    // ---------------------------------------------------------------------------
    bool begin(float (*process)(float))
    {
        _process = process;

        init();

        uint8_t timer_type = GPT_TIMER;
        int8_t tindex = FspTimer::get_available_timer(timer_type);
        if (tindex < 0) tindex = FspTimer::get_available_timer(timer_type, true);
        if (tindex < 0) return false;

        FspTimer::force_use_of_pwm_reserved_timer();

        
        if (!audioTimer.begin(TIMER_MODE_PERIODIC, timer_type, tindex, SAMPLE_RATE_HZ, 0.0f, audio_isr)) return false;
        if (!audioTimer.setup_overflow_irq()) return false;
        if (!audioTimer.open())  return false;
        if (!audioTimer.start()) return false;
        return true;
    }

    // ---------------------------------------------------------------------------
    // ISR audio: chiamata a SAMPLE_RATE_HZ.
    // ---------------------------------------------------------------------------
    static void audio_isr(timer_callback_args_t __attribute__((unused)) *arg)
    {
        uint16_t raw = _ADC::get_adc();

        float sample = ((float)raw - ADC_MID) / ADC_MID;   // normalizza a [-1, +1]

        sample = _process(sample);                     // catena di effetti configurabile
        
        if (sample >  1.0f) sample =  1.0f;                // clip di sicurezza
        if (sample < -1.0f) sample = -1.0f;

        uint16_t out = (uint16_t)(sample * DAC_MID + DAC_MID);  // torna a 0..4095

        _DAC::set_dac(out);
    }

private:
    FspTimer audioTimer;

    static inline float (*_process)(float) = NULL;
};