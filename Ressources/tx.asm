#include <xc.inc>

psect   txfunc,local,class=CODE,reloc=2

; -----------------------------------------------------------------
; GLOBALS
; | ---- asm ----- | ------------- C ----------------- |
; | _TX_64LEDS  <--|--> void TX_64LEDS(void)           |
; | _pC         <--|--> volatile const char * pC       |
; | _LED_MATRIX <--|--> volatile char LED_MATRIX [256] |
global _TX_64LEDS
global _pC
global _LED_MATRIX

; -----------------------------------------------------------------
; VARIABLES EN RAM (acces direct, bank 0)
; -----------------------------------------------------------------
psect   txdata,local,class=BANK0,space=1,noexec
tx_octet:   DS 1    ; octet en cours de transmission

psect   txfunc,local,class=CODE,reloc=2

; =================================================================
; TX_0 : envoie un bit 0 sur CMD_MATRIX (RB5)
;   Signal haut court  (~300 ns = 5 NOP a 64MHz)
;   Signal bas  long   (~900 ns = 14 NOP a 64MHz)
; =================================================================
TX_0:
    banksel LATB
    bsf     LATB, 5         ; RB5 = 1 (haut)
    nop
    nop
    nop
    nop
    nop
    bcf     LATB, 5         ; RB5 = 0 (bas)
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    return

; =================================================================
; TX_1 : envoie un bit 1 sur CMD_MATRIX (RB5)
;   Signal haut long   (~600 ns = 10 NOP a 64MHz)
;   Signal bas  court  (~600 ns = 9  NOP a 64MHz)
; =================================================================
TX_1:
    banksel LATB
    bsf     LATB, 5         ; RB5 = 1 (haut)
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    bcf     LATB, 5         ; RB5 = 0 (bas)
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    return

; =================================================================
; TX_1BIT : envoie le bit de poids fort de tx_octet
;           puis decale tx_octet d un cran vers la gauche
; =================================================================
TX_1BIT:
    banksel tx_octet
    btfsc   tx_octet, 7     ; si bit7 = 0, saute la ligne suivante
    goto    TX1BIT_send1
    call    TX_0
    goto    TX1BIT_shift
TX1BIT_send1:
    call    TX_1

TX1BIT_shift:
    banksel tx_octet
    ; bit7 <- bit6
    btfsc   tx_octet, 6
    goto    TX1BIT_b7_1
    bcf     tx_octet, 7
    goto    TX1BIT_b6
TX1BIT_b7_1:
    bsf     tx_octet, 7
TX1BIT_b6:
    ; bit6 <- bit5
    btfsc   tx_octet, 5
    goto    TX1BIT_b6_1
    bcf     tx_octet, 6
    goto    TX1BIT_b5
TX1BIT_b6_1:
    bsf     tx_octet, 6
TX1BIT_b5:
    ; bit5 <- bit4
    btfsc   tx_octet, 4
    goto    TX1BIT_b5_1
    bcf     tx_octet, 5
    goto    TX1BIT_b4
TX1BIT_b5_1:
    bsf     tx_octet, 5
TX1BIT_b4:
    ; bit4 <- bit3
    btfsc   tx_octet, 3
    goto    TX1BIT_b4_1
    bcf     tx_octet, 4
    goto    TX1BIT_b3
TX1BIT_b4_1:
    bsf     tx_octet, 4
TX1BIT_b3:
    ; bit3 <- bit2
    btfsc   tx_octet, 2
    goto    TX1BIT_b3_1
    bcf     tx_octet, 3
    goto    TX1BIT_b2
TX1BIT_b3_1:
    bsf     tx_octet, 3
TX1BIT_b2:
    ; bit2 <- bit1
    btfsc   tx_octet, 1
    goto    TX1BIT_b2_1
    bcf     tx_octet, 2
    goto    TX1BIT_b1
TX1BIT_b2_1:
    bsf     tx_octet, 2
TX1BIT_b1:
    ; bit1 <- bit0
    btfsc   tx_octet, 0
    goto    TX1BIT_b1_1
    bcf     tx_octet, 1
    goto    TX1BIT_b0
TX1BIT_b1_1:
    bsf     tx_octet, 1
TX1BIT_b0:
    ; bit0 <- 0
    bcf     tx_octet, 0
    return

; =================================================================
; READ_1BYTE : lit l octet pointe par FSR0 dans tx_octet
;              et incremente FSR0
; =================================================================
READ_1BYTE:
    movf    POSTINC0, 0, 0  ; WREG = *FSR0, FSR0++
    banksel tx_octet
    movwf   tx_octet        ; tx_octet = WREG
    return

; =================================================================
; TX_1BYTE : envoie 8 bits d un octet (MSB en premier)
; =================================================================
TX_1BYTE:
    call    READ_1BYTE      ; charge *FSR0 dans tx_octet, FSR0++
    call    TX_1BIT         ; bit 7
    call    TX_1BIT         ; bit 6
    call    TX_1BIT         ; bit 5
    call    TX_1BIT         ; bit 4
    call    TX_1BIT         ; bit 3
    call    TX_1BIT         ; bit 2
    call    TX_1BIT         ; bit 1
    call    TX_1BIT         ; bit 0
    return

; =================================================================
; TX_1LED : envoie les 4 octets R, G, B, W d une LED
; =================================================================
TX_1LED:
    call    TX_1BYTE        ; R
    call    TX_1BYTE        ; G
    call    TX_1BYTE        ; B
    call    TX_1BYTE        ; W
    return

; =================================================================
; _TX_64LEDS : envoie les 64 LEDs de LED_MATRIX
; =================================================================
_TX_64LEDS:
    ; --- Test : allume LED6 (RC6) pour verifier que la fonction est appelee ---
    banksel TRISC
    bcf     TRISC, 6        ; RC6 en sortie
    banksel LATC
    bsf     LATC, 6         ; RC6 = 1 -> LED6 allumee

    ; --- Configure RB5 (CMD_MATRIX) en sortie numerique ---
    banksel TRISB
    bcf     TRISB, 5        ; RB5 en sortie
    banksel ANSELB
    bcf     ANSELB, 5       ; RB5 numerique

    ; --- Charge l adresse de LED_MATRIX dans FSR0 via _pC ---
    MOVFF   _pC + 0, WREG
    MOVWF   FSR0L, 0
    MOVFF   _pC + 1, WREG
    MOVWF   FSR0H, 0

    ; --- Envoie les 64 LEDs (boucle deroulee) ---
    call    TX_1LED     ; LED  1
    call    TX_1LED     ; LED  2
    call    TX_1LED     ; LED  3
    call    TX_1LED     ; LED  4
    call    TX_1LED     ; LED  5
    call    TX_1LED     ; LED  6
    call    TX_1LED     ; LED  7
    call    TX_1LED     ; LED  8
    call    TX_1LED     ; LED  9
    call    TX_1LED     ; LED 10
    call    TX_1LED     ; LED 11
    call    TX_1LED     ; LED 12
    call    TX_1LED     ; LED 13
    call    TX_1LED     ; LED 14
    call    TX_1LED     ; LED 15
    call    TX_1LED     ; LED 16
    call    TX_1LED     ; LED 17
    call    TX_1LED     ; LED 18
    call    TX_1LED     ; LED 19
    call    TX_1LED     ; LED 20
    call    TX_1LED     ; LED 21
    call    TX_1LED     ; LED 22
    call    TX_1LED     ; LED 23
    call    TX_1LED     ; LED 24
    call    TX_1LED     ; LED 25
    call    TX_1LED     ; LED 26
    call    TX_1LED     ; LED 27
    call    TX_1LED     ; LED 28
    call    TX_1LED     ; LED 29
    call    TX_1LED     ; LED 30
    call    TX_1LED     ; LED 31
    call    TX_1LED     ; LED 32
    call    TX_1LED     ; LED 33
    call    TX_1LED     ; LED 34
    call    TX_1LED     ; LED 35
    call    TX_1LED     ; LED 36
    call    TX_1LED     ; LED 37
    call    TX_1LED     ; LED 38
    call    TX_1LED     ; LED 39
    call    TX_1LED     ; LED 40
    call    TX_1LED     ; LED 41
    call    TX_1LED     ; LED 42
    call    TX_1LED     ; LED 43
    call    TX_1LED     ; LED 44
    call    TX_1LED     ; LED 45
    call    TX_1LED     ; LED 46
    call    TX_1LED     ; LED 47
    call    TX_1LED     ; LED 48
    call    TX_1LED     ; LED 49
    call    TX_1LED     ; LED 50
    call    TX_1LED     ; LED 51
    call    TX_1LED     ; LED 52
    call    TX_1LED     ; LED 53
    call    TX_1LED     ; LED 54
    call    TX_1LED     ; LED 55
    call    TX_1LED     ; LED 56
    call    TX_1LED     ; LED 57
    call    TX_1LED     ; LED 58
    call    TX_1LED     ; LED 59
    call    TX_1LED     ; LED 60
    call    TX_1LED     ; LED 61
    call    TX_1LED     ; LED 62
    call    TX_1LED     ; LED 63
    call    TX_1LED     ; LED 64

    ; --- Reset : RB5 bas pendant > 80 us pour valider la trame ---
    banksel LATB
    bcf     LATB, 5         ; RB5 = 0
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop  ; 10
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop  ; 20
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop  ; 30
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop  ; 40
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop  ; 50
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop  ; 60
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop  ; 70
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop  ; 80
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop  ; 90
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop  ; 100

    return

    END