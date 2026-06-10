/* --------------------------------------------------------------
 * Fichier     :   main.c
 * Auteur(s)   :
 * Description :
 * -------------------------------------------------------------- */

#include <xc.h>

#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINTOSC_64MHZ
#pragma config WDTE = OFF

#define _XTAL_FREQ 64000000

#define MASK_LED0 0x01
#define MASK_LED1 0x02
#define MASK_LED2 0x04
#define MASK_LED3 0x08
#define MASK_LED4 0x10
#define MASK_LED5 0x20
#define MASK_LED6 0x40
#define MASK_LED7 0x80

#define MASK_BP0 0x01
#define MASK_BP1 0x02

// Canaux ADC
#define CH_ENV1 0x05  // RA5
#define CH_ENV2 0x04  // RA4
#define CH_ENV3 0x03  // RA3
#define CH_ENV4 0x02  // RA2

// Couleurs RGBW
#define ENV1_R 255
#define ENV1_G   0
#define ENV1_B   0
#define ENV1_W   0

#define ENV2_R   0
#define ENV2_G 255
#define ENV2_B   0
#define ENV2_W   0

#define ENV3_R   0
#define ENV3_G   0
#define ENV3_B 255
#define ENV3_W   0

#define ENV4_R 128
#define ENV4_G 128
#define ENV4_B 128
#define ENV4_W 255

// Interface C <-> ASM
extern void TX_64LEDS(void);
extern void TX_1LEDS(void);
extern void TX_1BYTE(void);
extern void TX_1BIT(void);
extern void READ_1BYTE(void);

volatile char        LED_MATRIX[256];
volatile const char *pC = LED_MATRIX;

// ------------------------------------------------------------------
// select_channel() : selectionne le canal ADC
// ------------------------------------------------------------------
void select_channel(char channel) {
    ADPCH = channel;
    __delay_us(10);
}

// ------------------------------------------------------------------
// ad_read() : lance une conversion et retourne ADRESH
// ------------------------------------------------------------------
char ad_read(void) {
    ADCON0 |= 0x02;           // ADGO = 1
    while (ADCON0 & 0x02) {} // attend fin de conversion
    return ADRESH;
}

// ------------------------------------------------------------------
// ad_read_max() : lit 200 fois et retourne le maximum
//                pour capturer le pic du signal audio
// ------------------------------------------------------------------
char ad_read_max(void) {
    char max_val = 0;
    char val;
    int i;
    for (i = 0; i < 200; i++) {
        val = ad_read();
        if ((unsigned char)val > (unsigned char)max_val) {
            max_val = val;
        }
        __delay_us(50);
    }
    return max_val;
}

// ------------------------------------------------------------------
// get_thermometre() : convertit valeur ADC (128-255) en code
//                    thermometre sur 8 bits
// ------------------------------------------------------------------
char get_thermometre(char adc_val) {
    int val    = (unsigned char)adc_val;  // 128..255
    int niveau = (val - 128) * 8 / 127;  // 0..8
    char code  = 0;
    char i;
    for (i = 0; i < niveau; i++) {
        code = (char)((code << 1) | 0x01);
    }
    return code;
}

// ------------------------------------------------------------------
// fill_row() : remplit une ligne de 8 LEDs dans LED_MATRIX
// ------------------------------------------------------------------
void fill_row(unsigned char row, char therm,
              char R, char G, char B, char W) {
    unsigned char col;
    for (col = 0; col < 8; col++) {
        unsigned int idx = (unsigned int)(row * 8 + col) * 4;
        if (therm & (1 << col)) {
            LED_MATRIX[idx + 0] = R;
            LED_MATRIX[idx + 1] = G;
            LED_MATRIX[idx + 2] = B;
            LED_MATRIX[idx + 3] = W;
        } else {
            LED_MATRIX[idx + 0] = 0;
            LED_MATRIX[idx + 1] = 0;
            LED_MATRIX[idx + 2] = 0;
            LED_MATRIX[idx + 3] = 0;
        }
    }
}

// ------------------------------------------------------------------
// main()
// ------------------------------------------------------------------
void main(void) {
    // RA2..RA5 en entrees analogiques
    TRISA  = 0x3C;   // RA2,RA3,RA4,RA5 en entree
    ANSELA = 0x3C;   // RA2..RA5 analogiques

    // LED de test RC1 en sortie
    TRISC &= ~MASK_LED1;

    // Configuration ADC
    ADCON1 = 0b01100000;  // Fosc/64, Vref = AVdd/AVss
    ADCON0 = 0b00000001;  // ADON=1, ADGO=0

    char therm;

    while (1) {
        // ENV1 -> lignes 0 et 1 (rouge)
        select_channel(CH_ENV1);
        therm = get_thermometre(ad_read_max());
        fill_row(0, therm, ENV1_R, ENV1_G, ENV1_B, ENV1_W);
        fill_row(1, therm, ENV1_R, ENV1_G, ENV1_B, ENV1_W);

        // ENV2 -> lignes 2 et 3 (vert)
        select_channel(CH_ENV2);
        therm = get_thermometre(ad_read_max());
        fill_row(2, therm, ENV2_R, ENV2_G, ENV2_B, ENV2_W);
        fill_row(3, therm, ENV2_R, ENV2_G, ENV2_B, ENV2_W);

        // ENV3 -> lignes 4 et 5 (bleu)
        select_channel(CH_ENV3);
        therm = get_thermometre(ad_read_max());
        fill_row(4, therm, ENV3_R, ENV3_G, ENV3_B, ENV3_W);
        fill_row(5, therm, ENV3_R, ENV3_G, ENV3_B, ENV3_W);

        // ENV4 -> lignes 6 et 7 (blanc)
        select_channel(CH_ENV4);
        therm = get_thermometre(ad_read_max());
        fill_row(6, therm, ENV4_R, ENV4_G, ENV4_B, ENV4_W);
        fill_row(7, therm, ENV4_R, ENV4_G, ENV4_B, ENV4_W);

        // Envoi vers la matrice LED
        TX_64LEDS();

        __delay_ms(50);
    }
}