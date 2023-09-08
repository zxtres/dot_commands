/*
Compilar con:
sdcc -mz80 --reserve-regs-iy --opt-code-size --max-allocs-per-node 10000
--nostdlib --nostdinc --no-std-crt0 --code-loc 8192 loadpzx.c
*/

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

enum {IBLACK=0, IBLUE, IRED, IMAG, IGREEN, ICYAN, IYELLOW, IWHITE};
enum {PBLACK=0, PBLUE=8, PRED=16, PMAG=24, PGREEN=32, PCYAN=40, PYELLOW=48, PWHITE=56};
#define BRIGHT 64
#define FLASH 128

__sfr __at (0xfe) ULA;
__sfr __at (0xff) ATTR;
__sfr __at (0x1f) KEMPSTONADDR;
__sfr __at (0x7f) FULLERADDR;

__sfr __banked __at (0xf7fe) SEMIFILA1;
__sfr __banked __at (0xeffe) SEMIFILA2;
__sfr __banked __at (0xfbfe) SEMIFILA3;
__sfr __banked __at (0xdffe) SEMIFILA4;
__sfr __banked __at (0xfdfe) SEMIFILA5;
__sfr __banked __at (0xbffe) SEMIFILA6;
__sfr __banked __at (0xfefe) SEMIFILA7;
__sfr __banked __at (0x7ffe) SEMIFILA8;

#define ATTRP 23693
#define ATTRT 23695
#define BORDR 23624
#define LASTK 23560

#define WAIT_VRETRACE __asm halt __endasm
#define WAIT_HRETRACE while(ATTR!=0xff)
#define SETCOLOR(x) *(BYTE *)(ATTRP)=(x)
#define LASTKEY *(BYTE *)(LASTK)
#define ATTRPERMANENT *((BYTE *)(ATTRP))
#define ATTRTEMPORARY *((BYTE *)(ATTRT))
#define BORDERCOLOR *((BYTE *)(BORDR))

#define MAKEWORD(d,h,l) { ((BYTE *)&(d))[0] = (l) ; ((BYTE *)&(d))[1] = (h); }

__sfr __banked __at (0xfc3b) ZXUNOADDR;
__sfr __banked __at (0xfd3b) ZXUNODATA;

#define MASTERCONF 0
#define SCANCODE 4
#define KEYBSTAT 5
#define JOYCONF 6
#define SRAMDATA 0xf2
#define SRAMADDRINC 0xf1
#define SRAMADDR 0xf0
#define COREID 0xff

/* Some ESXDOS system calls */
#define HOOK_BASE   128
#define MISC_BASE   (HOOK_BASE+8)
#define FSYS_BASE   (MISC_BASE+16)
#define M_GETSETDRV (MISC_BASE+1)
#define F_OPEN      (FSYS_BASE+2)
#define F_CLOSE     (FSYS_BASE+3)
#define F_READ      (FSYS_BASE+5)
#define F_WRITE     (FSYS_BASE+6)
#define F_SEEK      (FSYS_BASE+7)
#define F_GETPOS    (FSYS_BASE+8)
#define M_TAPEIN    (MISC_BASE+3)
#define M_AUTOLOAD  (MISC_BASE+8)

#define FMODE_READ	     0x1 // Read access
#define FMODE_WRITE      0x2 // Write access
#define FMODE_OPEN_EX    0x0 // Open if exists, else error
#define FMODE_OPEN_AL    0x8 // Open if exists, if not create
#define FMODE_CREATE_NEW 0x4 // Create if not exists, if exists error
#define FMODE_CREATE_AL  0xc // Create if not exists, else open and truncate

#define SEEK_START       0
#define SEEK_CUR         1
#define SEEK_BKCUR       2

#define BUFSIZE 2048
BYTE errno;
WORD leido;
char buffer[BUFSIZE];

void __sdcc_enter_ix (void) __naked;
void cls (BYTE);

void puts (BYTE *);
void u16tohex (WORD n, char *s);
void u8tohex (BYTE n, char *s);
void print8bhex (BYTE n);
void print16bhex (WORD n);

void memset (BYTE *, BYTE, WORD);
void memcpy (BYTE *, BYTE *, WORD);

BYTE open (char *filename, BYTE mode);
void close (BYTE handle);
WORD read (BYTE handle, BYTE *buffer, WORD nbytes);
WORD write (BYTE handle, BYTE *buffer, WORD nbytes);
void seek (BYTE handle, WORD hioff, WORD looff, BYTE from);

/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */
BYTE main (char *p);
void getcoreid(BYTE *s);
void usage (void);
BYTE commandlinemode (char *p);
void getfilename (char *p, char *fname);
BYTE printheader (char *fname);
void readtap (BYTE handle, BYTE nopausa);
WORD readword (BYTE handle);
BYTE readbyte (BYTE handle);
void copyblock (BYTE handle, WORD hicopy, WORD locopy);
void rewindsram (void);
void writesram (BYTE v);
void incaddrsram (void);
void copysram (BYTE *p, WORD l);

void init (void) __naked
{
     __asm
     xor a
     ld (#_errno),a
     push hl
     call _main
     inc sp
     inc sp
     ld a,l
     or a
     jr z,preparaload
     cp #255
     jr z,noload
     scf
     ret
noload:
     or a
     ret
preparaload:
     ;Cierra TAPE.IN
     ld b,#1
     rst #8
     .db #M_TAPEIN

     ;Auto LOAD ""
     xor a
     rst #8
     .db #M_AUTOLOAD
     
     or a
     ret

;Codigo antiguo para hacer LOAD ""
;     ld bc,#3
;     ld hl,(#23641)
;     push hl
;     rst #0x18
;     .dw 0x1655
;     ld hl,#comando_load
;     pop de
;     ld bc,#3
;     ldir
;     ld hl,#0x12cf
;     .db 0xc3, 0xfb, 0x1f

;comando_load:
;     .db 239,34,34
     __endasm;
}

BYTE main (char *p)
{
  if (!p)
  {
     usage();
     return 255;
  }
  else
      return commandlinemode(p);
}

BYTE commandlinemode (char *p)
{
    char fname[32];
    BYTE handle;
    BYTE noautoload, nopausa;

    nopausa = 0;
    noautoload = 0;
    while (*p==' ')
      p++;
    if (*p=='-')
    {
      p++;
      if (*p=='n')
      {
        noautoload = 255;
        p++;
      }
      else if (*p=='p')
      {
        nopausa = 1;
        p++;
      }
    }
    while (*p==' ')
      p++;

    getfilename (p, fname);
    handle = open (fname, FMODE_READ);
    if (handle==0xff)
       return errno;

    printheader(fname);
    readtap(handle, nopausa);

    close (handle);
    
    if (noautoload == 255)
      puts ("\xd\xdType LOAD \"\" and press PLAY\xd");
    else
    {
        ZXUNOADDR = 0xf3;
        ZXUNODATA = 0x1;  // software assisted PLAY press
        ZXUNODATA = 0x0;
    }

    return noautoload;
}

void usage (void)
{
        // 01234567890123456789012345678901
    puts (" LOADTAP [-n][-p] file.tap\xd\xd"
          "Loads a TAP file into the PZX\xd"
          "player.\xd"
          "-n : do not autoplay.\xd"
          "-p : do not insert pauses\xd"
          "     between blocks.\xd");
}

void getfilename (char *p, char *fname)
{
    while (*p!=':' && *p!=0xd && *p!=' ')
          *fname++ = *p++;
    *fname = '\0';
}

BYTE printheader (char *filename)
{
    puts ("Loading: ");
    puts(filename);
    puts ("\xd");
    return 1;
}

void readtap (BYTE handle, BYTE nopausa)
{
    BYTE pulsh[] = {0x02,0x08,0x00,0x00,0x00,0x7f,0x9f,0x78,0x08,0x9b,0x02,0xdf,0x02};
    BYTE pulsd[] = {0x02,0x08,0x00,0x00,0x00,0x97,0x8c,0x78,0x08,0x9b,0x02,0xdf,0x02};
    BYTE data[] = {0x03,0x00,0x00,0x00,0x00,0x98,0x00,0x00,0x80,0xB1,0x03,0x02,0x02,
                     0x57,0x03,0x57,0x03,0xAE,0x06,0xAE,0x06};
    BYTE pausa[] = {0x02,0x6,0,0,0,0x1,0x80,0x35,0x80,0xe0,0x67};
    BYTE fullstop[] = {0,0,0,0,0};
    WORD tam;
    DWORD tamb;
    BYTE scandblctrl, flag;

    ZXUNOADDR = 0xb;
    scandblctrl = ZXUNODATA;
    ZXUNODATA = scandblctrl | 0xc0;

    rewindsram();
    while(1)
    {
      *((BYTE *)(23692)) = 0xff;  // para evitar el mensaje de scroll
      tam = readword (handle); if (!leido) break;
      flag = readbyte (handle); if (!leido) break;
      puts ("P");
      if (flag < 128)
        copysram (pulsh, sizeof pulsh);
      else
        copysram (pulsd, sizeof pulsd);

      puts ("D");
      tam += 16;
      //puts("\xd");print16bhex (tam); puts("\xd");
      data[1] = tam & 0xFF;
      data[2] = (tam>>8) & 0xFF;
      tam -= 16;
      tamb = tam;
      tamb = tamb * 8;
      data[5] = tamb & 0xFF;
      data[6] = (tamb>>8) & 0xFF;
      data[7] = (tamb>>16) & 0xFF;
      data[8] = 0x80 | ((tamb>>24) & 0xFF);
      copysram (data, sizeof data);
      writesram (flag);
      incaddrsram();
      tam--;
      copyblock (handle, 0, tam); if (!leido) break;
      
      if (nopausa == 0)
      {
        puts ("A");
        copysram (pausa, sizeof pausa);
      }
    }
    // pequeña pausa de 20ms para evitar errores de carga
    // a causa de la conmutación brusca de la señal de EAR
    // del player virtual a la entrada real

    // Add full stop mark to the tape
    copysram (fullstop, sizeof fullstop);

    ZXUNOADDR = 0xb;
    ZXUNODATA = scandblctrl;
}

WORD readword (BYTE handle)
{
    WORD res;
  
    errno = 0;
    read (handle, (BYTE *)&res, 2);
    return res;
}

BYTE readbyte (BYTE handle)
{
  BYTE res;

  errno = 0;
  read (handle, &res, 1);
  return res;
}

void copyblock (BYTE handle, WORD hicopy, WORD locopy)
{
  errno = 0;
  //print16bhex(hicopy);
  //print16bhex(locopy); puts("\xd");
  while (hicopy!=0 || locopy>=BUFSIZE)
  {
      read (handle, buffer, BUFSIZE);
      copysram (buffer, BUFSIZE);
      if ((locopy-BUFSIZE)>locopy)
         hicopy--;
      locopy -= BUFSIZE;
      //print16bhex(hicopy);
      //print16bhex(locopy);  puts("\xd");
  }
  if (locopy>0)
  {
      //print16bhex(hicopy);
      //print16bhex(locopy);  puts("\xd");
      read (handle, buffer, locopy);
      copysram (buffer, locopy);
  }
}

void rewindsram (void)
{
    ZXUNOADDR = SRAMADDR;
    ZXUNODATA = 0;
    ZXUNODATA = 0;
    ZXUNODATA = 0;
}

void writesram (BYTE v)
{
    ZXUNOADDR = SRAMDATA;
    ZXUNODATA = v;
}

void incaddrsram (void)
{
    ZXUNOADDR = SRAMADDRINC;
    ZXUNODATA = 0;
}

void copysram (BYTE *p, WORD l)
{
    while (l--)
    {
        ZXUNOADDR = SRAMDATA;
        ZXUNODATA = *p++;
        ZXUNOADDR = SRAMADDRINC;
        ZXUNODATA = 0;
    }
}

void getcoreid(BYTE *s)
{
  BYTE cont;
  volatile BYTE letra;

  ZXUNOADDR = COREID;
  cont=0;

  do
  {
    letra = ZXUNODATA;
    *(s++) = letra;
    cont++;
  }
  while (letra!=0 && cont<32);
  *s='\0';
}


/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */
#pragma disable_warning 85
#pragma disable_warning 59
void memset (BYTE *dir, BYTE val, WORD nby)
{
  __asm
  push bc
  push de
  ld l,4(ix)
  ld h,5(ix)
  ld a,6(ix)
  ld c,7(ix)
  ld b,8(ix)
  ld d,h
  ld e,l
  inc de
  dec bc
  ld (hl),a
  ldir
  pop de
  pop bc
  __endasm;
}

void memcpy (BYTE *dst, BYTE *fue, WORD nby)
{
  __asm
  push bc
  push de
  ld e,4(ix)
  ld d,5(ix)
  ld l,6(ix)
  ld h,7(ix)
  ld c,8(ix)
  ld b,9(ix)
  ldir
  pop de
  pop bc
  __endasm;
}

void puts (BYTE *str)
{
  __asm
  push bc
  push de
  ld a,(#ATTRT)
  push af
  ld a,(#ATTRP)
  ld (#ATTRT),a
  ld l,4(ix)
  ld h,5(ix)
buc_print:
  ld a,(hl)
  or a
  jp z,fin_print
  cp #4
  jr nz,no_attr
  inc hl
  ld a,(hl)
  ld (#ATTRT),a
  inc hl
  jr buc_print
no_attr:
  rst #16
  inc hl
  jp buc_print

fin_print:
  pop af
  ld (#ATTRT),a
  pop de
  pop bc
  __endasm;
}

/* void u16tohex (WORD n, char *s)*/
/* {*/
/*   u8tohex((n>>8)&0xFF,s);*/
/*   u8tohex(n&0xFF,s+2);*/
/* }*/
 
/* void u8tohex (BYTE n, char *s)*/
/* {*/
/*   BYTE i=1;*/
/*   BYTE resto;*/
 
/*   resto=n&0xF;*/
/*   s[1]=(resto>9)?resto+55:resto+48;*/
/*   resto=n>>4;*/
/*   s[0]=(resto>9)?resto+55:resto+48;*/
/*   s[2]='\0';*/
/* }*/

/* void print8bhex (BYTE n)*/
/* {*/
/*     char s[3];*/
 
/*     u8tohex(n,s);*/
/*     puts(s);*/
/* }*/
 
/* void print16bhex (WORD n)*/
/* {*/
/*     char s[5];*/
 
/*     u16tohex(n,s);*/
/*     puts(s);*/
/* }*/


void __sdcc_enter_ix (void) __naked
{
    __asm
    pop	hl	; return address
    push ix	; save frame pointer
    ld ix,#0
    add	ix,sp	; set ix to the stack frame
    jp (hl)	; and return
    __endasm;
}

BYTE open (char *filename, BYTE mode)
{
    __asm
    push bc
    push de
    xor a
    rst #8
    .db #M_GETSETDRV   ;Default drive in A
    ld l,4(ix)  ;Filename pointer
    ld h,5(ix)  ;in HL
    ld b,6(ix)  ;Open mode in B
    rst #8
    .db #F_OPEN
    jr nc,open_ok
    ld (#_errno),a
    ld a,#0xff
open_ok:
    ld l,a
    pop de
    pop bc
    __endasm;
}

void close (BYTE handle)
{
    __asm
    push bc
    push de
    ld a,4(ix)  ;Handle
    rst #8
    .db #F_CLOSE
    pop de
    pop bc
    __endasm;
}

WORD read (BYTE handle, BYTE *buffer, WORD nbytes)
{
    __asm
    push bc
    push de
    ld a,4(ix)  ;File handle in A
    ld l,5(ix)  ;Buffer address
    ld h,6(ix)  ;in HL
    ld c,7(ix)
    ld b,8(ix)  ;Buffer length in BC
    rst #8
    .db #F_READ
    ld (#_leido),bc
    jr nc,read_ok
    ld (#_errno),a
    ld bc,#65535
read_ok:
    ld h,b
    ld l,c
    pop de
    pop bc
    __endasm;
}

WORD write (BYTE handle, BYTE *buffer, WORD nbytes)
{
    __asm
    push bc
    push de
    ld a,4(ix)  ;File handle in A
    ld l,5(ix)  ;Buffer address
    ld h,6(ix)  ;in HL
    ld c,7(ix)
    ld b,8(ix)  ;Buffer length in BC
    rst #8
    .db #F_WRITE
    jr nc,write_ok
    ld (#_errno),a
    ld bc,#65535
write_ok:
    ld h,b
    ld l,c
    pop de
    pop bc
    __endasm;
}

/*void seek (BYTE handle, WORD hioff, WORD looff, BYTE from)*/
/*{*/
/*    __asm*/
/*    push bc*/
/*    push de*/
/*    ld a,4(ix)  ;File handle in A*/
/*    ld c,5(ix)  ;Hiword of offset in BC*/
/*    ld b,6(ix)*/
/*    ld e,7(ix)  ;Loword of offset in DE*/
/*    ld d,8(ix)*/
/*    ld l,9(ix)  ;From where: 0: start, 1:forward current pos, 2: backwards current pos*/
/*    rst #8*/
/*    .db #F_SEEK*/
/*    pop de*/
/*    pop bc*/
/*    __endasm;*/
/*}*/

