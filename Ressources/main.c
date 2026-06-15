/* --------------------------------------------------------------
 * Fichier     :   main.c
 * Auteur(s)   :
 * Description :   VU-metre audio -> matrice LED 8x8 avec mode dégradé
 * Calibre pour un signal faible (val ~ 100-115)
 * -------------------------------------------------------------- */
#include <xc.h>

#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINTOSC_64MHZ
#pragma config WDTE = OFF

#define _XTAL_FREQ 64000000

// Masques LEDs (debug, port C)
#define MASK_LED0 0x01
#define MASK_LED1 0x02
#define MASK_LED2 0x04
#define MASK_LED3 0x08
#define MASK_LED4 0x10
#define MASK_LED5 0x20
#define MASK_LED6 0x40
#define MASK_LED7 0x80

// Canaux ADC (cf. schematic)
#define CH_ENV1 0x05  // RA5
#define CH_ENV2 0x04  // RA4
#define CH_ENV3 0x03  // RA3
#define CH_ENV4 0x02  // RA2

// Intensite globale des LEDs (0 = eteint, 255 = max)
#define INTENSITE 60

// Couleurs RGBW pour le mode classique
#define ENV1_R INTENSITE
#define ENV1_G 0
#define ENV1_B 0
#define ENV1_W 0

#define ENV2_R 0
#define ENV2_G INTENSITE
#define ENV2_B 0
#define ENV2_W 0

#define ENV3_R 0
#define ENV3_G 0
#define ENV3_B INTENSITE
#define ENV3_W 0

#define ENV4_R (INTENSITE/2)
#define ENV4_G (INTENSITE/2)
#define ENV4_B (INTENSITE/2)
#define ENV4_W INTENSITE

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
    __delay_ms(2);            // laisse le temps au canal de se stabiliser
}

// ------------------------------------------------------------------
// ad_read_max() : echantillonne 200 fois et garde le maximum
// ------------------------------------------------------------------
char ad_read_max(void) {
    unsigned char max_val = 0;
    for (int i = 0; i < 200; i++) {
        ADCON0 |= 0x01;            // ADGO = bit 0 (PIC18F25K40)
        while (ADCON0 & 0x01) {}  // attend fin de conversion

        unsigned char v = (unsigned char)ADRESH;
        if (v > max_val) {
            max_val = v;
        }
        __delay_us(50);
    }
    return (char)max_val;
}

// ------------------------------------------------------------------
// get_thermometre() : Calcule le nombre de LEDs a allumer (0..8)
// ------------------------------------------------------------------
#define SEUIL_REPOS   105   // valeur ADC au repos (a ajuster si besoin)
#define DIVISEUR_SENS  15   // plus petit = plus sensible

char get_thermometre(char adc_val) {
    unsigned char uval = (unsigned char)adc_val;
    int val = (int)uval;

    int amplitude = val - SEUIL_REPOS;
    if (amplitude < 0) amplitude = 0;

    int niveau = (amplitude * 8) / DIVISEUR_SENS;
    if (niveau > 8) niveau = 8;

    char code = 0;
    for (char i = 0; i < niveau; i++) {
        code = (char)((code << 1) | 0x01);
    }
    return code;
}

// ------------------------------------------------------------------
// fill_row() : Mode Classique
// ------------------------------------------------------------------
void fill_row(unsigned char row, char therm,
              char R, char G, char B, char W) {
    unsigned char col;
    for (col = 0; col < 8; col++) {
        unsigned int idx = (unsigned int)(row * 8 + col) * 4;

        if (therm & (1 << col)) {
            LED_MATRIX[idx + 0] = G;  // Vert en premier (SK6812)
            LED_MATRIX[idx + 1] = R;  // Rouge en second
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
// fill_row_gradient() : Mode Dégradé (Vert -> Jaune -> Orange -> Rouge)
// ------------------------------------------------------------------
void fill_row_gradient(unsigned char row, char therm) {
    unsigned char col;
    for (col = 0; col < 8; col++) {
        unsigned int idx = (unsigned int)(row * 8 + col) * 4;

        if (therm & (1 << col)) {
            // Dégradé basé sur la position de la LED (colonne 0 à 7)
            if (col < 2) {
                // Base : VERT (Leds 0 et 1)
                LED_MATRIX[idx + 0] = INTENSITE; // G
                LED_MATRIX[idx + 1] = 0;         // R
                LED_MATRIX[idx + 2] = 0;         // B
                LED_MATRIX[idx + 3] = 0;         // W
            } else if (col < 4) {
                // Montée : JAUNE (Leds 2 et 3) - Mélange Vert + Rouge
                LED_MATRIX[idx + 0] = INTENSITE;
                LED_MATRIX[idx + 1] = INTENSITE;
                LED_MATRIX[idx + 2] = 0;
                LED_MATRIX[idx + 3] = 0;
            } else if (col < 6) {
                // Fort : ORANGE (Leds 4 et 5) - Moins de Vert, plein de Rouge
                LED_MATRIX[idx + 0] = INTENSITE / 3; 
                LED_MATRIX[idx + 1] = INTENSITE;
                LED_MATRIX[idx + 2] = 0;
                LED_MATRIX[idx + 3] = 0;
            } else {
                // Max : ROUGE (Leds 6 et 7)
                LED_MATRIX[idx + 0] = 0;
                LED_MATRIX[idx + 1] = INTENSITE;
                LED_MATRIX[idx + 2] = 0;
                LED_MATRIX[idx + 3] = 0;
            }
        } else {
            // Eteint
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
    TRISA  |= 0x3C;   // RA2,RA3,RA4,RA5 en entree
    ANSELA |= 0x3C;   // RA2..RA5 analogiques

    // --- Configuration Boutons (Exemple: RB0 pour bp0, RB1 pour bp1) ---
    // A adapter selon ton branchement !
    TRISB |= 0x03;    // RB0 et RB1 en entree
    ANSELB &= ~0x03;  // RB0 et RB1 en numerique (desactiver l'analogique)
    // WPUB |= 0x03;  // Decommente si tu as besoin des pull-ups internes

    // Configuration ADC
    ADCON1 = 0x00;
    ADCON0 = 0b10010000;
    
    // Configuration du PORTB pour le bouton BP1 et BP0
    TRISAbits.TRISA6 = 1;  
    ANSELAbits.ANSELA6 = 0;
    TRISAbits.TRISA7 = 1;  
    ANSELAbits.ANSELA7 = 0;
    


    __delay_ms(100);   // laisse l ADC se stabiliser

    char therm;
    char mode = 0; // 0 = Classique, 1 = Dégradé

    while (1) {
        // --- Lecture des boutons (Anti-rebond simple) ---
        // On suppose que le bouton relie la broche a GND (actif a 0)
        // On verifie si le bouton est à 0 (appuyé)
        if (PORTAbits.RA6 == 0) { 
            __delay_ms(20); // Anti-rebond
            if (PORTAbits.RA6 == 0) mode = 1; // bp0 -> Mode dégradé
        }

        if (PORTAbits.RA7 == 0) { 
            __delay_ms(20);
            if (PORTAbits.RA7 == 0) mode = 0; // bp1 -> Mode classique
        }

        // --- ENV1 -> lignes 0 et 1 ---
        select_channel(CH_ENV1);
        therm = get_thermometre(ad_read_max());
        if (mode == 0) {
            fill_row(0, therm, ENV1_R, ENV1_G, ENV1_B, ENV1_W);
            fill_row(1, therm, ENV1_R, ENV1_G, ENV1_B, ENV1_W);
        } else {
            fill_row_gradient(0, therm);
            fill_row_gradient(1, therm);
        }

        // --- ENV2 -> lignes 2 et 3 ---
        select_channel(CH_ENV2);
        therm = get_thermometre(ad_read_max());
        if (mode == 0) {
            fill_row(2, therm, ENV2_R, ENV2_G, ENV2_B, ENV2_W);
            fill_row(3, therm, ENV2_R, ENV2_G, ENV2_B, ENV2_W);
        } else {
            fill_row_gradient(2, therm);
            fill_row_gradient(3, therm);
        }

        // --- ENV3 -> lignes 4 et 5 ---
        select_channel(CH_ENV3);
        therm = get_thermometre(ad_read_max());
        if (mode == 0) {
            fill_row(4, therm, ENV3_R, ENV3_G, ENV3_B, ENV3_W);
            fill_row(5, therm, ENV3_R, ENV3_G, ENV3_B, ENV3_W);
        } else {
            fill_row_gradient(4, therm);
            fill_row_gradient(5, therm);
        }

        // --- ENV4 -> lignes 6 et 7 ---
        select_channel(CH_ENV4);
        therm = get_thermometre(ad_read_max());
        if (mode == 0) {
            fill_row(6, therm, ENV4_R, ENV4_G, ENV4_B, ENV4_W);
            fill_row(7, therm, ENV4_R, ENV4_G, ENV4_B, ENV4_W);
        } else {
            fill_row_gradient(6, therm);
            fill_row_gradient(7, therm);
        }

        // Envoi vers la matrice LED
        TX_64LEDS();
    }
}