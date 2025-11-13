/* --------------------------------------------------------------
 * Fichier     :   main.c
 * Auteur(s)   :
 * Description :
 * -------------------------------------------------------------- */

#include <xc.h>

// Configuration materielle du PIC :
#pragma config FEXTOSC = OFF           // Pas de source d'horloge externe
#pragma config RSTOSC = HFINTOSC_64MHZ // Horloge interne de 64 MHz
#pragma config WDTE = OFF              // Désactiver le watchdog

#define _XTAL_FREQ 64000000 // Frequence d'horloge - necessaire aux macros de delay (_delay(N) ; __delay_us(N) ; __delay_ms(N)))

// Définition des masques, macros, etc. :
// TODO


// Déclaration de fonctions et variables globales permettant au code C et à l'asm de les partager
// Une même fonction ou variable côté asm est préfixée par un underscore, et ne l'est pas côté C
// Avec ce formalisme, elles sont utilisables de façon intercangeable et transparente :
// | ---- asm ----- | ------------- C ----------------- |
// | _TX_64LEDS  <--|--> void TX_64LEDS(void)           |
// | _pC         <--|--> volatile const char * pC       |
// | _LED_MATRIX <--|--> volatile char LED_MATRIX [256] |

// Définition des fonctions relatives à la matrice de LEDs:
extern void TX_64LEDS(void); // Fonction définie dans tx.asm ; Fonction permettant d'envoyer la commande pour piloter les 64 LEDs, telle que décrite dans LED_MATRIX

// Définition des constantes / variables relatives à la matrice de LEDs :
volatile char LED_MATRIX [256] ; // Definition d'une matrice de 64 x 4 octets contenant les composantes R/G/B/W de chaque LED (1 octet/couleur/LED)
volatile const char * pC = LED_MATRIX; // Pointeur vers LED_MATRIX


// - Fonction main ----------------------------------------------------------------------
void main(void) {
    /* Configuration des entrées / sorties */
    // TODO

    /* Corps du programme */
    // TODO

    /* Code pour vérification du bon fonctionnement de la partir uC (à retirer par la suite) : === DEMO CODE */

    // Initialisation des LEDs =================================================================== DEMO CODE
    TRISB &= 0xEF; // LED_MASTER : OUTPUT -------------------------------------------------------- DEMO CODE
    TRISC &= 0x00; // LED0-7     : OUTPUT -------------------------------------------------------- DEMO CODE

    LATB &= 0xEF; // Eteindre LEDM   ------------------------------------------------------------- DEMO CODE
    LATC  = 0x00; // Eteindre LED0-7 ------------------------------------------------------------- DEMO CODE

    // Blink sur LEDM : ========================================================================== DEMO CODE
    LATB |= 0x10;    // Allumer LEDM   ----------------------------------------------------------- DEMO CODE
    __delay_ms(500); // Macro de délai ----------------------------------------------------------- DEMO CODE
    LATB &= 0xEF;    // Eteindre LEDM  ----------------------------------------------------------- DEMO CODE
    __delay_ms(500); // Macro de délai ----------------------------------------------------------- DEMO CODE
    LATB |= 0x10;    // Allumer LEDM   ----------------------------------------------------------- DEMO CODE

    // Chenillard : ============================================================================== DEMO CODE
    while(1){ //---------------------------------------------------------------------------------- DEMO CODE
        for (int i=0; i<8; i++){ // -------------------------------------------------------------- DEMO CODE
            LATC = 0x01 << i;    // Commander les LEDs de test sur le PORTC ---------------------- DEMO CODE
            __delay_ms(125);     // Macro de délai ----------------------------------------------- DEMO CODE
        } // ------------------------------------------------------------------------------------- DEMO CODE
    } // ----------------------------------------------------------------------------------------- DEMO CODE

    return;
}
