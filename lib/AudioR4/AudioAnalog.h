#pragma once

#include "Arduino.h"
#include "registers.h"

namespace _ADC{

void setup_adc(void) {
    pinMode(A1, INPUT);
    
    *MSTP_MSTPCRD &= ~(0x01UL << MSTPD16);      // abilita il modulo ADC140
    *ADC140_ADCER   = 0x06;                     // modalita' 14 bit
    *ADC140_ADANSA0 |= (0x01 << 0);             // seleziona il canale AN000 = A1
}

inline uint16_t get_adc() {
    *ADC140_ADCSR |= (0x01 << 15);              // ADST = 1, avvia conversione
    while (*ADC140_ADCSR & (0x01 << 15)) { }    // attesa attiva fine conversione (~1us)

    uint16_t raw = *ADC140_ADDR00;              // campione grezzo, 0..16383

    return raw;
}

}

namespace _DAC{

void setup_dac(void) {
    *MSTP_MSTPCRD &= ~(0x01UL << MSTPD20);      // abilita il modulo DAC12
    *DAC12_DADPR    = 0x00;                     // formato right-justified
    *DAC12_DAADSCR  = 0x00;                     // nessuna sincronizzazione hw con l'ADC
    *DAC12_DAVREFCR = 0x00;                     // reset di sicurezza
    *DAC12_DADR0    = 0x0800;                   // preset a meta' scala
    delayMicroseconds(10);
    *DAC12_DAVREFCR = 0x01;                     // riferimento AVCC0/AVSS0 (0-5V)
    *DAC12_DACR     = 0x5F;                     // abilita l'uscita del DAC
    delayMicroseconds(5);

    *PFS_P014PFS  = 0x00000000;
    *PFS_P014PFS |= (0x1UL << 15);              // A0 in modalita' analogica
}

inline void set_dac(uint16_t value) {
    *DAC12_DADR0 = value;
}

}

inline void stop_delay_interrupt(){
    // Ferma AGT0: elimina l'interrupt periodico a 1kHz di millis()/delay(),
    *AGT0_AGTCR = 0;
}