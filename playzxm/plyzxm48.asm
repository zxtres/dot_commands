; API de ESXDOS.
include "esxdos.inc"
include "errors.inc"

; PLAYZXM : un comando para ESXDOS 0.8.5 que permite reproducir videos en formato
; ZXM en blanco y negro y color.

; Video: cabecera + secuencia lineal de frames.
; Cabecera (256 bytes):
;   Offset      Contenido
;     0            'Z'
;     1            'X'
;     2            'M'
;   16-17        Numero de frames que tiene el video (no usado en esta rutina)
;     18         Numero de sectores (de 512 bytes) que ocupa cada frame: 12 para BW, 14 para color
;   Resto de posiciones: reservado

; Cada frame: en blanco y negro, 6144 bytes en formato pantalla de Spectrum. Un pixel a 1 se muestra en negro. A 0, en blanco.
;             en color, 256 bytes de relleno + 6912 bytes en formato pantalla de Spectrum
;             NOTA: el relleno es simplemente para que cada frame ocupe un n�mero entero de sectores. El relleno se pone a 0
;                   o a lo que se quiera, ya que no se hace nada con �l.


;Para ensamblar con PASMO como archivo binario (no TAP)

BORDERCLR           equ 23624
PAPERCOLOR          equ 23693
BANKM               equ 7ffdh
PILA                equ 3deah   ;valor sugerido por Miguel �ngelo para poner la pila en el �rea de comando

                    org 2000h  ;comienzo de la ejecuci�n de los comandos ESXDOS.

Main                proc
                    ld a,h
                    or l
                    jr z,PrintUso  ;si no se ha especificado nombre de fichero, imprimir uso
                    call RecogerNFile

                    ;di
                    ;ld (BackupSP),sp
                    ;ld sp,PILA
                    ;ei
                    call PlayFichero
                    push af
                    call Cls
                    pop af
                    ;ld sp,(BackupSP)
                    ret

PrintUso            ld hl,Uso
BucPrintMsg         ld a,(hl)
                    or a
                    ret z
                    rst 10h
                    inc hl
                    jr BucPrintMsg
                    endp


RecogerNFile        proc   ;HL apunta a los argumentos (nombre del fichero)
                    ld de,BufferNFich
CheckCaracter       ld a,(hl)
                    or a
                    jr z,FinRecoger
                    cp " "
                    jr z,FinRecoger
                    cp ":"
                    jr z,FinRecoger
                    cp 13
                    jr z,FinRecoger
                    ldi
                    jr CheckCaracter
FinRecoger          xor a
                    ld (de),a
                    inc de   ;DE queda apuntando al buffer este que se necesita en OPEN, no s� pa qu�.
                    ret
                    endp


PlayFichero         proc
                    xor a
                    rst 08h
                    db M_GETSETDRV  ;A = unidad actual
                    ld b,FA_READ    ;B = modo de apertura
                    ld hl,BufferNFich   ;HL = Puntero al nombre del fichero (ASCIIZ)
                    rst 08h
                    db F_OPEN
                    ret c   ;Volver si hay error
                    ld (FHandle),a

                    call SetupVideoMemory
                    call ReadHeader
                    jr c,FinPlay

BucPlayVideo        ld bc,(LFrame)
                    ld a,b
                    cp 28   ;es color?
                    jr nz,NoColor

                    ld a,(FHandle)
                    ld bc,0
                    ld de,256
                    ld l,1
                    rst 08h
                    db F_SEEK
                    ld bc,6912

NoColor             ld hl,16384
                    ld a,(FHandle)
                    rst 08h
                    db F_READ
                    jr c,FinPlay  ;si error, fin de lectura
                    ld a,b
                    or c
                    jr z,FinPlay  ;si no hay m�s que leer, fin de lectura

                    ld bc,7ffeh
                    in a,(c)   ;Detectar si se ha pulsado SPACE
                    and 1
                    jr z,FinPlay

                    jr BucPlayVideo

FinPlay             ld a,(FHandle)
                    rst 08h
                    db F_CLOSE

                    or a   ;Volver sin errores a ESXDOS
                    ret
                    endp


ReadHeader          proc
                    ld a,(FHandle)
                    ld bc,256
                    ld hl,32768
                    rst 08h
                    db F_READ
                    ret c

                    ld hl,32768
                    ld a,'Z'
                    cpi
                    jr nz,NoZXM
                    ld a,'X'
                    cpi
                    jr nz,NoZXM
                    ld a,'M'
                    cpi
                    jr nz,NoZXM

                    ld hl,32768+18
                    ld a,(hl)
                    add a,a
                    ld b,a
                    ld c,0
                    ld (LFrame),bc

                    ld bc,16384  ;Principio buffer pantalla para BW
                    ld (StartScreen),bc
                    ld a,(hl)
                    cp 12   ;B/W ?
                    jr z,ZXMOk

                    ld bc,16384-256  ;Principio buffer pantalla para color
                    ld (StartScreen),bc

ZXMOk               or a
                    ret

NoZXM               scf
                    ret
                    endp


SetupVideoMemory    proc
                    di
                    ld hl,04000h + 6144
                    ld de,04000h + 6145
                    ld bc,767
                    ld (hl),64+56
                    ldir
                    xor a
                    out (254),a
                    ei
                    ret
                    endp

Cls                 proc
                    ld a,(BORDERCLR)
                    sra a
                    sra a
                    sra a
                    and 7
                    out (254),a
                    ld hl,16384
                    ld de,16385
                    ld bc,6143
                    ld (hl),l
                    ldir
                    inc hl
                    inc de
                    ld bc,767
                    ld a,(PAPERCOLOR)
                    ld (hl),a
                    ldir
                    ret
                    endp

                    ;   01234567890123456789012345678901
Uso                 db " playzxm moviefile.zxm",13,13
                    db "Plays a video file encoded in",13
                    db "ZXM (colour/BW) format.",13,0

FHandle             db 0
Banco               db 0
BackupSP            dw 0
BackupConfig        db 0
StartScreen         dw 0
LFrame              dw 0

BufferNFich         equ $   ;resto de la RAM para el nombre del fichero