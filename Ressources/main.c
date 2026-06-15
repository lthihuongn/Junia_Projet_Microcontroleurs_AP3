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

// select_channel() : selectionne le canal ADC
void select_channel(char channel) {
    ADPCH = channel;
    __delay_ms(2);            // laisse le canal se stabiliser
}

//ad_read_max() : echantillonne 200 fois et garde le maximum
char ad_read_max(void) {
    unsigned char max_val = 0;
    for (int i = 0; i < 200; i++) {
        ADCON0 |= 0x01;            // ADGO = bit 0
        while (ADCON0 & 0x01) {}  // attend fin de conversion

        unsigned char v = (unsigned char)ADRESH;
        if (v > max_val) {
            max_val = v;
        }
        __delay_us(50);
    }
    return (char)max_val;
}


// get_thermometre() : Calcule le nombre de LEDs à allumer (de 0 à 8)
// Convertit l'amplitude audio brute en un masque binaire.
#define SEUIL_REPOS   105   // Valeur ADC lue dans le silence total (bruit de fond)
#define DIVISEUR_SENS  15   // Reglage de la sensibilité (plus la valeur est petite, plus ça monte vite)

char get_thermometre(char adc_val) {
    // On force la valeur en "non signé" (0 à 255) pour éviter les bugs de bits de signe
    unsigned char uval = (unsigned char)adc_val;
    // On la convertit en entier (int)
    int val = (int)uval;

    // suppression du bruit de fond (calibrage du point 0)
    // On calcule la vraie puissance du son en retirant la tension de repos
    int amplitude = val - SEUIL_REPOS;
    
    // si le résultat est négatif (bruit électrique parasite), on force à 0
    // Cela garantit que les LEDs restent éteintes quand il n'y a pas de musique
    if (amplitude < 0) amplitude = 0;

    // mise à l'échelle (Mapping de l'amplitude vers 8 LEDs)
    // On multiplie par 8 (le nombre de LEDs par ligne) puis on divise par notre sensibilité
    int niveau = (amplitude * 8) / DIVISEUR_SENS;
    
    // même si le son est extrêmement fort, 
    // on ne doit pas essayer d'allumer plus de 8 LEDs
    if (niveau > 8) niveau = 8;

    // masque binaire (format "thermomètre")
    // Initialisation à 00000000 en binaire (toutes les LEDs sont éteintes)
    char code = 0;
    
    // On boucle autant de fois qu'il y a de LEDs à allumer
    for (char i = 0; i < niveau; i++) {
        // Opération bit à bit : 
        // (code << 1) décale tous les bits d'un cran vers la gauche
        // | 0x01 ajoute un "1" tout à droite
        // Exemple : 00000000 -> 00000001 -> 00000011 -> 00000111...
        code = (char)((code << 1) | 0x01);
    }
    
    // On renvoie l'octet final (ex: 00001111 si 4 LEDs doivent s'allumer)
    return code;
}


// Mode Classique
void fill_row(unsigned char row, char therm,
              char R, char G, char B, char W) {
    unsigned char col;
    for (col = 0; col < 8; col++) {
        unsigned int idx = (unsigned int)(row * 8 + col) * 4;

        if (therm & (1 << col)) {
            LED_MATRIX[idx + 0] = G;
            LED_MATRIX[idx + 1] = R;
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

// mode Degrade (Vert - Jaune - Orange - Rouge)
void fill_row_gradient(unsigned char row, char therm) {
    unsigned char col;
    for (col = 0; col < 8; col++) {
        unsigned int idx = (unsigned int)(row * 8 + col) * 4;

        if (therm & (1 << col)) {
            // Degrade base sur la position de la LED (colonne 0 à 7)
            if (col < 2) {
                // Base : VERT (Leds 0 et 1)
                LED_MATRIX[idx + 0] = INTENSITE; // G
                LED_MATRIX[idx + 1] = 0;         // R
                LED_MATRIX[idx + 2] = 0;         // B
                LED_MATRIX[idx + 3] = 0;         // W
            } else if (col < 4) {
                // Montée : JAUNE (Leds 2 et 3)
                LED_MATRIX[idx + 0] = INTENSITE;
                LED_MATRIX[idx + 1] = INTENSITE;
                LED_MATRIX[idx + 2] = 0;
                LED_MATRIX[idx + 3] = 0;
            } else if (col < 6) {
                // Fort : ORANGE (Leds 4 et 5)
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

// PIXEL ART : La Baleine
const char image_baleine[64] = {
    0, 1, 0, 1, 0, 0, 0, 0,  // ligne 0 (haut)
    1, 0, 1, 0, 1, 0, 0, 0,  // ligne 1
    0, 0, 1, 0, 0, 0, 0, 0,  // ligne 2
    0, 2, 2, 2, 0, 0, 2, 0,  // ligne 3
    2, 2, 0, 2, 2, 0, 2, 2,  // ligne 4 (œil : 0 au milieu)
    3, 3, 2, 2, 2, 0, 2, 0,  // ligne 5
    3, 3, 3, 2, 2, 2, 2, 0,  // ligne 6
    0, 3, 3, 3, 2, 2, 0, 0   // ligne 7 (bas)
};

void draw_whale(void) {
    for (int i = 0; i < 64; i++) {
        int idx = i * 4; // 4 octets par LED (G, R, B, W)
        
        switch (image_baleine[i]) {
            case 0: // eteint : oeil noir
                LED_MATRIX[idx + 0] = 0; 
                LED_MATRIX[idx + 1] = 0; 
                LED_MATRIX[idx + 2] = 0; 
                LED_MATRIX[idx + 3] = 0; 
                break;
            case 1: // bleu Fonce (Jet d'eau)
                LED_MATRIX[idx + 0] = 0; 
                LED_MATRIX[idx + 1] = 0; 
                LED_MATRIX[idx + 2] = INTENSITE; 
                LED_MATRIX[idx + 3] = 0; 
                break;
            case 2: // bleu Clair (corps)
                LED_MATRIX[idx + 0] = INTENSITE / 2; 
                LED_MATRIX[idx + 1] = 0; 
                LED_MATRIX[idx + 2] = INTENSITE; 
                LED_MATRIX[idx + 3] = 0; 
                break;
            case 3: // gris (ventre)
                LED_MATRIX[idx + 0] = 0; 
                LED_MATRIX[idx + 1] = 0; 
                LED_MATRIX[idx + 2] = 0; 
                LED_MATRIX[idx + 3] = INTENSITE / 4; 
                break;
        }
    }
}

void main(void) {
    // RA2..RA5 en entrees analogiques
    TRISA  |= 0x3C;   // RA2,RA3,RA4,RA5 en entree
    ANSELA |= 0x3C;   // RA2..RA5 analogiques

    // config ADC
    ADCON1 = 0x00;
    ADCON0 = 0b10010000;
    
    // config PORTB pour le bouton BP1 et BP0
    TRISAbits.TRISA6 = 1;  
    ANSELAbits.ANSELA6 = 0;
    TRISAbits.TRISA7 = 1;  
    ANSELAbits.ANSELA7 = 0;
    
    __delay_ms(100);   // laisse ADC se stabiliser

    char therm;
    char mode = 0; // 0 = Classique, 1 = Dégradé, 2 = Baleine

    while (1) {
        //lecture des boutons
        char btn0_appuye = (PORTAbits.RA6 == 0);
        char btn1_appuye = (PORTAbits.RA7 == 0);

        // Priorité 1 : deux boutons appuyés
        if (btn0_appuye && btn1_appuye) {
            __delay_ms(20); // Anti-rebond
            if ((PORTAbits.RA6 == 0) && (PORTAbits.RA7 == 0)) {
                mode = 2; // Mode Baleine
            }
        } 
        // Priorité 2 : Bouton BP0 (Dégradé)
        else if (btn0_appuye) { 
            __delay_ms(20); 
            if (PORTAbits.RA6 == 0) mode = 1; 
        }
        // Priorité 3 : Bouton BP1 (Classique)
        else if (btn1_appuye) { 
            __delay_ms(20);
            if (PORTAbits.RA7 == 0) mode = 0; 
        }

        //AFFICHAGE SELON LE MODE

        if (mode == 2) {
            // Affichage de la baleine en image fixe
            draw_whale();
            TX_64LEDS();
            __delay_ms(50);
            continue;       // Repart directement au début du while(1) sans lire l'ADC
        }

        // si on n'est pas en mode baleine, on lit l'audio (Mode 0 ou 1)

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