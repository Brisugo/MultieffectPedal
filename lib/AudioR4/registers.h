#pragma once

// ---------------------------------------------------------------------------
// MSTP - Module Stop Control Register D
// ---------------------------------------------------------------------------
#define MSTP          0x40040000
#define MSTP_MSTPCRD  ((volatile unsigned int *)(MSTP + 0x7008))
#define MSTPD16       16   // ADC140
#define MSTPD20       20   // DAC12

// ---------------------------------------------------------------------------
// ADC140 - convertitore A/D a 14 bit
// ---------------------------------------------------------------------------
#define ADCBASE          0x40050000
#define ADC140_ADCSR     ((volatile unsigned short *)(ADCBASE + 0xC000))
#define ADC140_ADANSA0   ((volatile unsigned short *)(ADCBASE + 0xC004))
#define ADC140_ADCER     ((volatile unsigned short *)(ADCBASE + 0xC00E))
#define ADC140_ADDR00    ((volatile unsigned short *)(ADCBASE + 0xC020)) // A1 (AN000)

// ---------------------------------------------------------------------------
// DAC12 - convertitore D/A a 12 bit (uscita fissa su A0)
// ---------------------------------------------------------------------------
#define DACBASE          0x40050000
#define DAC12_DADR0      ((volatile unsigned short *)(DACBASE + 0xE000))
#define DAC12_DACR       ((volatile unsigned char  *)(DACBASE + 0xE004))
#define DAC12_DADPR      ((volatile unsigned char  *)(DACBASE + 0xE005))
#define DAC12_DAADSCR    ((volatile unsigned char  *)(DACBASE + 0xE006))
#define DAC12_DAVREFCR   ((volatile unsigned char  *)(DACBASE + 0xE007))

// ---------------------------------------------------------------------------
// PFS - Pin Function Select (A0 in modalita' analogica)
// ---------------------------------------------------------------------------
#define PORTBASE     0x40040000
#define P000PFS      0x0800
#define PFS_P014PFS  ((volatile unsigned int *)(PORTBASE + P000PFS + (14 * 4))) // A0

// ---------------------------------------------------------------------------
// AGT0 - timer usato dal core Arduino per generare l'interrupt di millis()
// ---------------------------------------------------------------------------
#define AGTBASE      0x40084000
#define AGT0_AGTCR   ((volatile unsigned char *)(AGTBASE + 0x008))