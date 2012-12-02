/* glulxdump.c: Glulx game file decompiler.
    Designed by Andrew Plotkin <erkyrath@eblong.com>
    http://eblong.com/zarf/glulx/index.html
*/

/* This program is very much a cheap hack. It is *not* a generic,
   neutral decompiler for any Glulx file in existence. It assumes
   that the Glulx file was generated by Glulx-Inform. (This, of
   course, is how it is able to display objects; there *are* no
   objects in the Glulx spec, but this program understands the
   tables that Inform generates.)

   We are making the following Inform-specific assumptions:

   1. ROM contains only the header, functions, and strings, plus
   some padding. (In that order.)
   2. All the functions and strings are in ROM, not RAM.
   3. When disassembling the contents of a function, a C0, C1,
   E0, or E1 byte signals the start of a new object. (This will be
   true unless we add opcodes that start with those bytes. Which
   may happen someday. In fact, an opcode such as add (10) could
   be represented as C0000010, which would also violate this
   assumption, but Inform does not do this.)
   4. The first object begins at the start of RAM.
   5. Objects contain seven bytes worth of attributes. (This is
   currently hardwired in Inform, but can be changed easily by
   changing a #define and recompiling.)
   6. Dictionary words are nine characters long, and have three
   two-byte fields appended.
   7. Probably other stuff I forgot.

   This whole situation could be improved by adding a "layout
   convention" field, at the start of ROM, which could contain
   compiler-specific information about how to decompile the file.
   Maybe someday.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* If your system doesn't have stdint.h, you'll have to edit the typedefs
   below to make unsigned and signed 32-bit integers. */
#include <stdint.h>
typedef uint32_t glui32;
typedef int32_t glsi32;

#include "opcodes.h"

/* We define our own TRUE and FALSE and NULL, because ANSI
    is a strange world. */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif

#define Read4(ptr)    \
  ( (glui32)(((unsigned char *)(ptr))[0] << 24)  \
  | (glui32)(((unsigned char *)(ptr))[1] << 16)  \
  | (glui32)(((unsigned char *)(ptr))[2] << 8)   \
  | (glui32)(((unsigned char *)(ptr))[3]))
#define Read2(ptr)    \
  ( (glui32)(((unsigned char *)(ptr))[0] << 8)  \
  | (glui32)(((unsigned char *)(ptr))[1]))
#define Read1(ptr)    \
  ((unsigned char)(((unsigned char *)(ptr))[0]))

#define Mem1(adr)  (Read1(memmap+(adr)))
#define Mem2(adr)  (Read2(memmap+(adr)))
#define Mem4(adr)  (Read4(memmap+(adr)))

/* The following #defines are copied directly from header.h in the inform6
   distribution. */

#define nop_gc 0
#define add_gc 1
#define sub_gc 2
#define mul_gc 3
#define div_gc 4
#define mod_gc 5
#define neg_gc 6
#define bitand_gc 7
#define bitor_gc 8
#define bitxor_gc 9
#define bitnot_gc 10
#define shiftl_gc 11
#define sshiftr_gc 12
#define ushiftr_gc 13
#define jump_gc 14
#define jz_gc 15
#define jnz_gc 16
#define jeq_gc 17
#define jne_gc 18
#define jlt_gc 19
#define jge_gc 20
#define jgt_gc 21
#define jle_gc 22
#define jltu_gc 23
#define jgeu_gc 24
#define jgtu_gc 25
#define jleu_gc 26
#define call_gc 27
#define return_gc 28
#define catch_gc 29
#define throw_gc 30
#define tailcall_gc 31
#define copy_gc 32
#define copys_gc 33
#define copyb_gc 34
#define sexs_gc 35
#define sexb_gc 36
#define aload_gc 37
#define aloads_gc 38
#define aloadb_gc 39
#define aloadbit_gc 40
#define astore_gc 41
#define astores_gc 42
#define astoreb_gc 43
#define astorebit_gc 44
#define stkcount_gc 45
#define stkpeek_gc 46
#define stkswap_gc 47
#define stkroll_gc 48
#define stkcopy_gc 49
#define streamchar_gc 50
#define streamnum_gc 51
#define streamstr_gc 52
#define gestalt_gc 53
#define debugtrap_gc 54
#define getmemsize_gc 55
#define setmemsize_gc 56
#define jumpabs_gc 57
#define random_gc 58
#define setrandom_gc 59
#define quit_gc 60
#define verify_gc 61
#define restart_gc 62
#define save_gc 63
#define restore_gc 64
#define saveundo_gc 65
#define restoreundo_gc 66
#define protect_gc 67
#define glk_gc 68
#define getstringtbl_gc 69
#define setstringtbl_gc 70
#define getiosys_gc 71
#define setiosys_gc 72
#define linearsearch_gc 73
#define binarysearch_gc 74
#define linkedsearch_gc 75
#define callf_gc 76
#define callfi_gc 77
#define callfii_gc 78
#define callfiii_gc 79
#define streamunichar_gc 80
#define mzero_gc 81
#define mcopy_gc 82
#define malloc_gc 83
#define mfree_gc 84
#define accelfunc_gc 85
#define accelparam_gc 86
#define numtof_gc 87
#define ftonumz_gc 88
#define ftonumn_gc 89
#define ceil_gc 90
#define floor_gc 91
#define fadd_gc 92
#define fsub_gc 93
#define fmul_gc 94
#define fdiv_gc 95
#define fmod_gc 96
#define sqrt_gc 97
#define exp_gc 98
#define log_gc 99
#define pow_gc 100
#define sin_gc 101
#define cos_gc 102
#define tan_gc 103
#define asin_gc 104
#define acos_gc 105
#define atan_gc 106
#define atan2_gc 107
#define jfeq_gc 108
#define jfne_gc 109
#define jflt_gc 110
#define jfle_gc 111
#define jfgt_gc 112
#define jfge_gc 113
#define jisnan_gc 114
#define jisinf_gc 115

typedef struct opcode_struct
{  
  char *name;        /* Lower case standard name */
  long code;         /* Opcode number */
  int flags;        /* Flags (see below) */
  int op_rules;     /* Any unusual operand rule applying (see below) */
  int no;           /* Number of operands */
} opcode_t;

/* The following #defines, and the opcodes_table, are copied directly from
   asm.c in the inform6 distribution. */

#define St      1     /* Store (last operand is store-mode) */
#define Br      2     /* Branch (last operand is a label) */
#define Rf      4     /* "Return flag": execution never continues after this
                         opcode (e.g., is a return or unconditional jump) */
#define St2 8     /* Store2 (second-to-last operand is store (Glulx)) */


#define GOP_Unicode      1   /* uses_unicode_features */
#define GOP_MemHeap      2   /* uses_memheap_features */
#define GOP_Acceleration 4   /* uses_acceleration_features */
#define GOP_Float        8   /* uses_float_features */

#define uchar char

static opcode_t opcodes_table[] = 
{
  { (uchar *) "nop",        0x00,  0, 0, 0 },
  { (uchar *) "add",        0x10, St, 0, 3 },
  { (uchar *) "sub",        0x11, St, 0, 3 },
  { (uchar *) "mul",        0x12, St, 0, 3 },
  { (uchar *) "div",        0x13, St, 0, 3 },
  { (uchar *) "mod",        0x14, St, 0, 3 },
  { (uchar *) "neg",        0x15, St, 0, 2 },
  { (uchar *) "bitand",     0x18, St, 0, 3 },
  { (uchar *) "bitor",      0x19, St, 0, 3 },
  { (uchar *) "bitxor",     0x1A, St, 0, 3 },
  { (uchar *) "bitnot",     0x1B, St, 0, 2 },
  { (uchar *) "shiftl",     0x1C, St, 0, 3 },
  { (uchar *) "sshiftr",    0x1D, St, 0, 3 },
  { (uchar *) "ushiftr",    0x1E, St, 0, 3 },
  { (uchar *) "jump",       0x20, Br|Rf, 0, 1 },
  { (uchar *) "jz",     0x22, Br, 0, 2 },
  { (uchar *) "jnz",        0x23, Br, 0, 2 },
  { (uchar *) "jeq",        0x24, Br, 0, 3 },
  { (uchar *) "jne",        0x25, Br, 0, 3 },
  { (uchar *) "jlt",        0x26, Br, 0, 3 },
  { (uchar *) "jge",        0x27, Br, 0, 3 },
  { (uchar *) "jgt",        0x28, Br, 0, 3 },
  { (uchar *) "jle",        0x29, Br, 0, 3 },
  { (uchar *) "jltu",       0x2A, Br, 0, 3 },
  { (uchar *) "jgeu",       0x2B, Br, 0, 3 },
  { (uchar *) "jgtu",       0x2C, Br, 0, 3 },
  { (uchar *) "jleu",       0x2D, Br, 0, 3 },
  { (uchar *) "call",       0x30, St, 0, 3 },
  { (uchar *) "return",     0x31, Rf, 0, 1 },
  { (uchar *) "catch",      0x32, Br|St, 0, 2 },
  { (uchar *) "throw",      0x33, Rf, 0, 2 },
  { (uchar *) "tailcall",   0x34, Rf, 0, 2 },
  { (uchar *) "copy",       0x40, St, 0, 2 },
  { (uchar *) "copys",      0x41, St, 0, 2 },
  { (uchar *) "copyb",      0x42, St, 0, 2 },
  { (uchar *) "sexs",       0x44, St, 0, 2 },
  { (uchar *) "sexb",       0x45, St, 0, 2 },
  { (uchar *) "aload",      0x48, St, 0, 3 },
  { (uchar *) "aloads",     0x49, St, 0, 3 },
  { (uchar *) "aloadb",     0x4A, St, 0, 3 },
  { (uchar *) "aloadbit",   0x4B, St, 0, 3 },
  { (uchar *) "astore",     0x4C,  0, 0, 3 },
  { (uchar *) "astores",    0x4D,  0, 0, 3 },
  { (uchar *) "astoreb",    0x4E,  0, 0, 3 },
  { (uchar *) "astorebit",  0x4F,  0, 0, 3 },
  { (uchar *) "stkcount",   0x50, St, 0, 1 },
  { (uchar *) "stkpeek",    0x51, St, 0, 2 },
  { (uchar *) "stkswap",    0x52,  0, 0, 0 },
  { (uchar *) "stkroll",    0x53,  0, 0, 2 },
  { (uchar *) "stkcopy",    0x54,  0, 0, 1 },
  { (uchar *) "streamchar", 0x70,  0, 0, 1 },
  { (uchar *) "streamnum",  0x71,  0, 0, 1 },
  { (uchar *) "streamstr",  0x72,  0, 0, 1 },
  { (uchar *) "gestalt",    0x0100, St, 0, 3 },
  { (uchar *) "debugtrap",  0x0101, 0, 0, 1 },
  { (uchar *) "getmemsize",     0x0102, St, 0, 1 },
  { (uchar *) "setmemsize",     0x0103, St, 0, 2 },
  { (uchar *) "jumpabs",    0x0104, Rf, 0, 1 },
  { (uchar *) "random",     0x0110, St, 0, 2 },
  { (uchar *) "setrandom",  0x0111,  0, 0, 1 },
  { (uchar *) "quit",       0x0120, Rf, 0, 0 },
  { (uchar *) "verify",     0x0121, St, 0, 1 },
  { (uchar *) "restart",    0x0122,  0, 0, 0 },
  { (uchar *) "save",       0x0123, St, 0, 2 },
  { (uchar *) "restore",    0x0124, St, 0, 2 },
  { (uchar *) "saveundo",   0x0125, St, 0, 1 },
  { (uchar *) "restoreundo",    0x0126, St, 0, 1 },
  { (uchar *) "protect",    0x0127,  0, 0, 2 },
  { (uchar *) "glk",        0x0130, St, 0, 3 },
  { (uchar *) "getstringtbl",   0x0140, St, 0, 1 },
  { (uchar *) "setstringtbl",   0x0141, 0, 0, 1 },
  { (uchar *) "getiosys",   0x0148, St|St2, 0, 2 },
  { (uchar *) "setiosys",   0x0149, 0, 0, 2 },
  { (uchar *) "linearsearch",   0x0150, St, 0, 8 },
  { (uchar *) "binarysearch",   0x0151, St, 0, 8 },
  { (uchar *) "linkedsearch",   0x0152, St, 0, 7 },
  { (uchar *) "callf",      0x0160, St, 0, 2 },
  { (uchar *) "callfi",     0x0161, St, 0, 3 },
  { (uchar *) "callfii",    0x0162, St, 0, 4 },
  { (uchar *) "callfiii",   0x0163, St, 0, 5 },
  { (uchar *) "streamunichar", 0x73,  0, GOP_Unicode, 1 },
  { (uchar *) "mzero",      0x170,  0, GOP_MemHeap, 2 },
  { (uchar *) "mcopy",      0x171,  0, GOP_MemHeap, 3 },
  { (uchar *) "malloc",     0x178,  St, GOP_MemHeap, 2 },
  { (uchar *) "mfree",      0x179,  0, GOP_MemHeap, 1 },
  { (uchar *) "accelfunc",  0x180,  0, GOP_Acceleration, 2 },
  { (uchar *) "accelparam", 0x181,  0, GOP_Acceleration, 2 },
  { (uchar *) "numtof",     0x190,  St, GOP_Float, 2 },
  { (uchar *) "ftonumz",    0x191,  St, GOP_Float, 2 },
  { (uchar *) "ftonumn",    0x192,  St, GOP_Float, 2 },
  { (uchar *) "ceil",       0x198,  St, GOP_Float, 2 },
  { (uchar *) "floor",      0x199,  St, GOP_Float, 2 },
  { (uchar *) "fadd",       0x1A0,  St, GOP_Float, 3 },
  { (uchar *) "fsub",       0x1A1,  St, GOP_Float, 3 },
  { (uchar *) "fmul",       0x1A2,  St, GOP_Float, 3 },
  { (uchar *) "fdiv",       0x1A3,  St, GOP_Float, 3 },
  { (uchar *) "fmod",       0x1A4,  St|St2, GOP_Float, 4 },
  { (uchar *) "sqrt",       0x1A8,  St, GOP_Float, 2 },
  { (uchar *) "exp",        0x1A9,  St, GOP_Float, 2 },
  { (uchar *) "log",        0x1AA,  St, GOP_Float, 2 },
  { (uchar *) "pow",        0x1AB,  St, GOP_Float, 3 },
  { (uchar *) "sin",        0x1B0,  St, GOP_Float, 2 },
  { (uchar *) "cos",        0x1B1,  St, GOP_Float, 2 },
  { (uchar *) "tan",        0x1B2,  St, GOP_Float, 2 },
  { (uchar *) "asin",       0x1B3,  St, GOP_Float, 2 },
  { (uchar *) "acos",       0x1B4,  St, GOP_Float, 2 },
  { (uchar *) "atan",       0x1B5,  St, GOP_Float, 2 },
  { (uchar *) "atan2",      0x1B6,  St, GOP_Float, 3 },
  { (uchar *) "jfeq",       0x1C0,  Br, GOP_Float, 4 },
  { (uchar *) "jfne",       0x1C1,  Br, GOP_Float, 4 },
  { (uchar *) "jflt",       0x1C2,  Br, GOP_Float, 3 },
  { (uchar *) "jfle",       0x1C3,  Br, GOP_Float, 3 },
  { (uchar *) "jfgt",       0x1C4,  Br, GOP_Float, 3 },
  { (uchar *) "jfge",       0x1C5,  Br, GOP_Float, 3 },
  { (uchar *) "jisnan",     0x1C8,  Br, GOP_Float, 2 },
  { (uchar *) "jisinf",     0x1C9,  Br, GOP_Float, 2 },
};

static int read_header(FILE *fl);
static int findopcode(int opnum);
void print_string(glui32 pos);
void print_proptable(glui32 pos);
void dump_ram(void);
void dump_objs(void);
void dump_action_table(void);
void dump_dict_table(void);
void dump_grammar_table(void);

int dumpfuncs = FALSE;
int dumpstrings = FALSE;
int dumpobjs = FALSE;
int dumpheader = FALSE;
int dumpactiontbl = FALSE;
int dumpdicttbl = FALSE;
int dumpgrammartbl = FALSE;
glui32 posactiontbl = 0;
glui32 posdicttbl = 0;
glui32 posgrammartbl = 0;
unsigned char *memmap = NULL;

glui32 version;
glui32 ramstart;
glui32 endgamefile;
glui32 endmem;
glui32 stacksize;
glui32 startfuncaddr;
glui32 stringtable;
glui32 checksum;

int main(int argc, char *argv[])
{
  FILE *fl;
  int ix;
  glui32 val;
  char *filename = NULL;

  if (argc < 2) {
    printf("Usage: glulxdump file\n");
    exit(1);
  }

  for (ix=1; ix<argc; ix++) {
    if (!strcmp(argv[ix], "-f"))
      dumpfuncs = TRUE;
    else if (!strcmp(argv[ix], "-s"))
      dumpstrings = TRUE;
    else if (!strcmp(argv[ix], "-o"))
      dumpobjs = TRUE;
    else if (!strcmp(argv[ix], "-h"))
      dumpheader = TRUE;
    else if (!strcmp(argv[ix], "-a")) {
      ix++;
      if (ix >= argc || (val = strtol(argv[ix], NULL, 16)) == 0) {
        printf("-a must be followed by address.\n");
        exit(1);
      }
      posactiontbl = val;
      dumpactiontbl = TRUE;
    }
    else if (!strcmp(argv[ix], "-d")) {
      ix++;
      if (ix >= argc || (val = strtol(argv[ix], NULL, 16)) == 0) {
        printf("-d must be followed by address.\n");
        exit(1);
      }
      posdicttbl = val;
      dumpdicttbl = TRUE;
    }
    else if (!strcmp(argv[ix], "-g")) {
      ix++;
      if (ix >= argc || (val = strtol(argv[ix], NULL, 16)) == 0) {
        printf("-g must be followed by address.\n");
        exit(1);
      }
      posgrammartbl = val;
      dumpgrammartbl = TRUE;
    }
    else
      filename = argv[ix];
  }

  if (!filename) {
    printf("No file given\n");
    exit(1);
  }

  if (!dumpfuncs && !dumpstrings && !dumpobjs && !dumpheader 
    && !dumpactiontbl && !dumpdicttbl && !dumpgrammartbl) {
    dumpfuncs = TRUE;
    dumpstrings = TRUE;
    dumpobjs = TRUE;
    dumpheader = TRUE;
  }

  fl = fopen(filename, "r");
  if (!fl) {
    perror("unable to open file");
    exit(1);
  }

  ix = read_header(fl);
  if (!ix) {
    fclose(fl);
    exit(1);
  }

  memmap = (unsigned char *)malloc(endgamefile);
  rewind(fl);
  ix = fread(memmap, 1, endgamefile, fl);
  if (ix != endgamefile) {
    printf("File too short.\n");
    fclose(fl);
    exit(1);
  }

  fclose(fl);
  fl = NULL;

  if (dumpheader) {
    printf("Version:   %08lx\n", (long)version);
    printf("RAMSTART:  %08lx\n", (long)ramstart);
    printf("ENDGAME:   %08lx\n", (long)endgamefile);
    printf("ENDMEM:    %08lx\n", (long)endmem);
    printf("STACKSIZE: %08lx\n", (long)stacksize);
    printf("StartFunc: %08lx\n", (long)startfuncaddr);
    printf("StringTbl: %08lx\n", (long)stringtable);
    printf("CheckSum:  %08lx\n", (long)checksum);
  }

  if (dumpstrings || dumpfuncs)
    dump_ram();
  if (dumpobjs)
    dump_objs();

  if (dumpactiontbl) 
    dump_action_table();
  if (dumpdicttbl) 
    dump_dict_table();
  if (dumpgrammartbl) 
    dump_grammar_table();

  exit(0);
}

static int read_header(FILE *fl)
{
  unsigned char buf[4 * 9];
  int res;

  /* Read in all the size constants from the game file header. */

  res = fread(buf, 1, 4 * 9, fl);
  if (res != 4 * 9) {
    printf("This file is too short.\n");
    return FALSE;
  }
  
  if (buf[0] != 'G' || buf[1] != 'l' || buf[2] != 'u' || buf[3] != 'l') {
    printf("This does not appear to be a Glulx file.\n");
    return FALSE;
  }

  version = Read4(buf+4);
  ramstart = Read4(buf+8);
  endgamefile = Read4(buf+12);
  endmem = Read4(buf+16);
  stacksize = Read4(buf+20);
  startfuncaddr = Read4(buf+24);
  stringtable = Read4(buf+28);
  checksum = Read4(buf+32);

  return TRUE;
}

void dump_ram()
{
  glui32 pos, startpos;
  unsigned char ch;
  int jx;

  pos = 4 * (9+5);

  while (pos < ramstart) {
    startpos = pos;
    ch = Mem1(pos);
    
    if (ch == 0) {
      /* skip padding */
      pos++;
    }
    else if (ch == 0xE0) {
      if (dumpstrings) {
        printf("String (%08lx): ", (long)startpos);
        pos++;
        while (1) {
          ch = Mem1(pos);
          pos++;
          if (ch == '\0')
            break;
          putchar(ch);
        }
        printf("\n");
      }
      else {
        while (1) {
          ch = Mem1(pos);
          pos++;
          if (ch == '\0')
            break;
        }
      }
    }
    else if (ch == 0xC0 || ch == 0xC1) {
      int loctype, locnum;
      int opcode;
      opcode_t *opco;
      glui32 exstart;
      int opmodes[16];

      printf("Function (%08lx), ", (long)startpos);
      if (ch == 0xC0)
        printf("stack-called:");
      else
        printf("locals-called:");

      pos++;

      while (1) {
        loctype = Mem1(pos);
        pos++;
        locnum = Mem1(pos);
        pos++;
        if (loctype == 0)
          break;
        printf(" %d %d-byte local%s,", locnum, loctype,
          ((locnum == 1) ? "" : "s"));
      }
      printf("\n");

      ch = Mem1(pos);
      pos++;
      exstart = pos;

      while (ch != 0xC0 && ch != 0xC1 && ch != 0xE0) {
        /* wrong, but the hell with it */

        /* Get the opcode */
        if ((ch & 0x80) == 0) {
          opcode = ch;
        }
        else if ((ch & 0x40) == 0) {
          opcode = (ch & 0x7F);
          ch = Mem1(pos); pos++;
          opcode = (opcode << 8) | ch;
        }
        else {
          opcode = (ch & 0x3F);
          ch = Mem1(pos); pos++;
          opcode = (opcode << 8) | ch;
          ch = Mem1(pos); pos++;
          opcode = (opcode << 8) | ch;
          ch = Mem1(pos); pos++;
          opcode = (opcode << 8) | ch;
        }
        opco = &opcodes_table[findopcode(opcode)];
        printf("  %08lx: %12s ", (long)(pos-exstart), opco->name);

        for (jx=0; jx<opco->no; jx+=2) {
          ch = Mem1(pos); pos++;
          opmodes[jx+0] = (ch & 0x0F);
          opmodes[jx+1] = ((ch >> 4) & 0x0F);
        }

        for (jx=0; jx<opco->no; jx++) {
          int val = 0;
          printf(" ");
          switch (opmodes[jx]) {
          case 0:
            printf("zero");
            break;
          case 8:
            printf("stackptr");
            break;
          case 1:
          case 5:
          case 9:
          case 13:
            if ((opmodes[jx] & 0x0C) == 4)
              printf("*");
            else if ((opmodes[jx] & 0x0C) == 8)
              printf("Fr:");
            else if ((opmodes[jx] & 0x0C) == 12)
              printf("*R:");
            ch = Mem1(pos); pos++;
            val = ch;
            if (val & 0x80)
              val |= 0xFFFFFF00;
            printf("%02x", val & 0xFF);
            break;
          case 2:
          case 6:
          case 10:
          case 14:
            if ((opmodes[jx] & 0x0C) == 4)
              printf("*");
            else if ((opmodes[jx] & 0x0C) == 8)
              printf("Fr:");
            else if ((opmodes[jx] & 0x0C) == 12)
              printf("*R:");
            ch = Mem1(pos); pos++;
            val = ch;
            if (val & 0x80)
              val |= 0xFFFFFF00;
            ch = Mem1(pos); pos++;
            val = (val << 8) | ch;
            printf("%04x", val & 0xFFFF);
            break;
          case 3:
          case 7:
          case 11:
          case 15:
            if ((opmodes[jx] & 0x0C) == 4)
              printf("*");
            else if ((opmodes[jx] & 0x0C) == 8)
              printf("Fr:");
            else if ((opmodes[jx] & 0x0C) == 12)
              printf("*R:");
            ch = Mem1(pos); pos++;
            val = ch;
            ch = Mem1(pos); pos++;
            val = (val << 8) | ch;
            ch = Mem1(pos); pos++;
            val = (val << 8) | ch;
            ch = Mem1(pos); pos++;
            val = (val << 8) | ch;
            printf("%08x", val);
            break;
          }
          if ((opco->flags & Br) && (jx == opco->no-1)) {
            if (val == 0) {
              printf(" (rfalse)");
            }
            else if (val == 1) {
              printf(" (rtrue)");
            }
            else {
              printf(" (%08lx)", (long)((pos-exstart)+val-2+1));
            }
          }
        }
        printf("\n");
        
        ch = Mem1(pos); pos++;
      }

      pos--;
    }
    else {
      printf("Unknown thing.\n");
      pos++;
    }
  }
}

static int findopcode(int opnum)
{
  switch (opnum) {
  case op_nop: return nop_gc;
  case op_add: return add_gc;
  case op_sub: return sub_gc;
  case op_mul: return mul_gc;
  case op_div: return div_gc;
  case op_mod: return mod_gc;
  case op_neg: return neg_gc;
  case op_bitand: return bitand_gc;
  case op_bitor: return bitor_gc;
  case op_bitxor: return bitxor_gc;
  case op_bitnot: return bitnot_gc;
  case op_shiftl: return shiftl_gc;
  case op_sshiftr: return sshiftr_gc;
  case op_ushiftr: return ushiftr_gc;
  case op_jump: return jump_gc;
  case op_jz: return jz_gc;
  case op_jnz: return jnz_gc;
  case op_jeq: return jeq_gc;
  case op_jne: return jne_gc;
  case op_jlt: return jlt_gc;
  case op_jge: return jge_gc;
  case op_jgt: return jgt_gc;
  case op_jle: return jle_gc;
  case op_call: return call_gc;
  case op_return: return return_gc;
  case op_catch: return catch_gc;
  case op_throw: return throw_gc;
  case op_copy: return copy_gc;
  case op_copys: return copys_gc;
  case op_copyb: return copyb_gc;
  case op_sexs: return sexs_gc;
  case op_sexb: return sexb_gc;
  case op_aload: return aload_gc;
  case op_aloads: return aloads_gc;
  case op_aloadb: return aloadb_gc;
  case op_aloadbit: return aloadbit_gc;
  case op_astore: return astore_gc;
  case op_astores: return astores_gc;
  case op_astoreb: return astoreb_gc;
  case op_astorebit: return astorebit_gc;
  case op_stkcount: return stkcount_gc;
  case op_stkpeek: return stkpeek_gc;
  case op_stkswap: return stkswap_gc;
  case op_stkroll: return stkroll_gc;
  case op_stkcopy: return stkcopy_gc;
  case op_streamchar: return streamchar_gc;
  case op_streamunichar: return streamunichar_gc;
  case op_streamnum: return streamnum_gc;
  case op_streamstr: return streamstr_gc;
  case op_gestalt: return gestalt_gc;
  case op_random: return random_gc;
  case op_setrandom: return setrandom_gc;
  case op_quit: return quit_gc;
  case op_verify: return verify_gc;
  case op_restart: return restart_gc;
  case op_save: return save_gc;
  case op_restore: return restore_gc;
  case op_saveundo: return saveundo_gc;
  case op_restoreundo: return restoreundo_gc;
  case op_protect: return protect_gc;
  case op_glk: return glk_gc;
  default: 
    printf("Unknown opcode %02x\n", opnum);
    return nop_gc;
  }
}

void dump_objs()
{
  glui32 startpos, pos, nextstartpos, proptablepos;
  unsigned char ch;
  int ix, jx;

  startpos = ramstart;
  while (startpos) {

    ch = Mem1(startpos);
    if (ch != 0x70) {
      printf("Non-object in object list (%08lx)\n", (long)startpos);
      return;
    }
    pos = startpos+1;

    printf("Object (%08lx):\n", (long)startpos);
    printf("    attrs:");
    for (ix=0; ix<7; ix++) {
      ch = Mem1(pos); pos++; 
      printf(" ");
      for (jx=0; jx<8; jx++) {
        printf("%c", (ch&1) ? '1' : '0');
        ch >>= 1;
      }
    }
    printf("\n");

    for (ix=0; ix<6; ix++) {
      static char *labellist[6] = {
        "next", "name", "props", "parent", "sibling", "child"
      };
      long val;
      ch = Mem1(pos); pos++;
      val = ch;
      ch = Mem1(pos); pos++;
      val = (val << 8) | ch;
      ch = Mem1(pos); pos++;
      val = (val << 8) | ch;
      ch = Mem1(pos); pos++;
      val = (val << 8) | ch;
      printf("  %7s: ", labellist[ix]);
      printf("%08lx", val);
      switch (ix) {
      case 0:
        nextstartpos = val;
        break;
      case 1:
        printf("  ");
        print_string(val);
        break;
      case 2:
        proptablepos = val;
        break;
      }
      printf("\n");
    }

    print_proptable(proptablepos);
    
    startpos = nextstartpos;
    printf("\n");
  }
}

void print_string(glui32 pos)
{
  unsigned char ch;
  ch = Mem1(pos);
  if (ch != 0xE0) {
    printf("<nonstring %08lx>", (long)pos);
    return;
  }
  pos++;
  while (1) {
    ch = Mem1(pos);
    pos++;
    if (ch == '\0')
      return;
    putchar(ch);
  }
}

void print_proptable(glui32 pos)
{
  int ix, jx;
  int numprops;
  
  numprops = Mem4(pos);
  pos += 4;

  printf("%d properties:\n", numprops);

  for (ix=0; ix<numprops; ix++) {
    int propnum, proplen, propflags;
    glui32 addr;
    propnum = Mem2(pos);
    pos += 2;
    proplen = Mem2(pos);
    pos += 2;
    addr = Mem4(pos);
    pos += 4;
    propflags = Mem2(pos);
    pos += 2;
    printf("  num=%d, len=%d, flags=%d, addr=%08lx\n",
      propnum, proplen, propflags, (long)addr);
    printf("  :");
    for (jx=0; jx<proplen; jx++) {
      glsi32 val = Mem4(addr);
      addr += 4;
      printf(" %lx", (long)val);
    }
    printf("\n");
  }
}

void dump_action_table()
{
  glui32 lx, len, val;

  len = Mem4(posactiontbl);

  printf("Action table at %08lx: %ld entries\n", (long)posactiontbl, (long)len);
  for (lx=0; lx<len; lx++) {
    val = Mem4(posactiontbl + 4 + lx*4);
    printf("%ld: %08lx\n", (long)lx, (long)val);
  }
}

void dump_dict_table()
{
  glui32 lx, len, val, addr;
  char ch;
  int ix;

  len = Mem4(posdicttbl);

  printf("Dictionary table at %08lx: %ld entries\n", (long)posdicttbl, (long)len);
  for (lx=0; lx<len; lx++) {
    addr = posdicttbl + 4 + lx*16;
    printf("%08lx: ", (long)addr);
    for (ix=0; ix<9; ix++) {
      ch = Mem1(addr+1+ix);
      if (ch == '\0')
        ch = ' ';
      printf("%c", ch);
    }
    printf(" : ");
    val = Mem2(addr+10);
    printf("flags=%04lx", (long)val);
    val = Mem2(addr+12);
    printf(", verbnum=%04lx", (long)val);
    val = Mem2(addr+14);
    printf(", filler=%lx", (long)val);
    printf("\n");
  }
}

void dump_grammar_table()
{
  glui32 lx, len, val, addr;
  int numlines;
  int jx;

  len = Mem4(posgrammartbl);

  printf("Grammar table at %08lx: %ld entries\n", (long)posgrammartbl, (long)len);

  for (lx=0; lx<len; lx++) {
    addr = posgrammartbl + 4 + lx*4;
    addr = Mem4(addr);
    printf("%03ld: %08lx: ", (long)lx, (long)addr);
    numlines = Mem1(addr);
    addr++;
    printf("%d lines:\n", numlines);
    for (jx=0; jx<numlines; jx++) {
      glui32 action, toktype, tokdata;
      action = Mem2(addr);
      addr += 2;
      val = Mem1(addr);
      addr++;
      printf("    ac %04lx; fl %02lx :", (long)action, (long)val);
      while (1) {
        toktype = Mem1(addr);
        addr++;
        if (toktype == 15) {
          printf(" .\n");
          break;
        }
        tokdata = Mem4(addr);
        addr += 4;
        printf(" %02lx(%04lx)", (long)toktype, (long)tokdata);
      }
    }
  }
}
