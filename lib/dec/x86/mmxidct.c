/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function:
    last mod: $Id$

 ********************************************************************/

/*MMX acceleration of Theora's iDCT.
  Originally written by Rudolf Marek, based on code from On2's VP3.*/
#include <ogg/ogg.h>
#include "../dct.h"
#include "../idct.h"

#include "x86int.h"

#if defined(USE_ASM)

/*These are offsets into the table of constants below.*/
/*4 masks, in order: low word to high.*/
#define OC_MASK_OFFSET    (0)
/*7 rows of cosines, in order: pi/16 * (1 ... 7).*/
#define OC_COSINE_OFFSET (32)
/*A row of 8's.*/
#define OC_EIGHT_OFFSET  (88)



/*A table of constants used by the MMX routines.*/
ogg_uint16_t __attribute__((aligned(8),used)) OC_IDCT_CONSTS[(4+7+1)*4]={
  65535,    0,    0,    0,
      0,65535,    0,    0,
      0,    0,65535,    0,
      0,    0,    0,65535,
  (ogg_uint16_t)OC_C1S7,(ogg_uint16_t)OC_C1S7,
  (ogg_uint16_t)OC_C1S7,(ogg_uint16_t)OC_C1S7,
  (ogg_uint16_t)OC_C2S6,(ogg_uint16_t)OC_C2S6,
  (ogg_uint16_t)OC_C2S6,(ogg_uint16_t)OC_C2S6,
  (ogg_uint16_t)OC_C3S5,(ogg_uint16_t)OC_C3S5,
  (ogg_uint16_t)OC_C3S5,(ogg_uint16_t)OC_C3S5,
  (ogg_uint16_t)OC_C4S4,(ogg_uint16_t)OC_C4S4,
  (ogg_uint16_t)OC_C4S4,(ogg_uint16_t)OC_C4S4,
  (ogg_uint16_t)OC_C5S3,(ogg_uint16_t)OC_C5S3,
  (ogg_uint16_t)OC_C5S3,(ogg_uint16_t)OC_C5S3,
  (ogg_uint16_t)OC_C6S2,(ogg_uint16_t)OC_C6S2,
  (ogg_uint16_t)OC_C6S2,(ogg_uint16_t)OC_C6S2,
  (ogg_uint16_t)OC_C7S1,(ogg_uint16_t)OC_C7S1,
  (ogg_uint16_t)OC_C7S1,(ogg_uint16_t)OC_C7S1,
      8,    8,    8,    8
};

/*Converts the expression in the argument to a sting.*/
#define OC_M2STR(_s) #_s

/*38 cycles*/
#define OC_IDCT_BEGIN \
 "  #OC_IDCT_BEGIN\n\t" \
 "  movq   "OC_I(3)",     %mm2\n\t" \
 "  movq   "OC_C(3)",     %mm6\n\t" \
 "  movq        %mm2,     %mm4\n\t" \
 "  movq   "OC_J(5)",     %mm7\n\t" \
 "  pmulhw      %mm6,     %mm4\n\t" \
 "  movq   "OC_C(5)",     %mm1\n\t" \
 "  pmulhw      %mm7,     %mm6\n\t" \
 "  movq        %mm1,     %mm5\n\t" \
 "  pmulhw      %mm2,     %mm1\n\t" \
 "  movq   "OC_I(1)",     %mm3\n\t" \
 "  pmulhw      %mm7,     %mm5\n\t" \
 "  movq   "OC_C(1)",     %mm0\n\t" \
 "  paddw       %mm2,     %mm4\n\t" \
 "  paddw       %mm7,     %mm6\n\t" \
 "  paddw       %mm1,     %mm2\n\t" \
 "  movq   "OC_J(7)",     %mm1\n\t" \
 "  paddw       %mm5,     %mm7\n\t" \
 "  movq        %mm0,     %mm5\n\t" \
 "  pmulhw      %mm3,     %mm0\n\t" \
 "  paddw       %mm7,     %mm4\n\t" \
 "  pmulhw      %mm1,     %mm5\n\t" \
 "  movq   "OC_C(7)",     %mm7\n\t" \
 "  psubw       %mm2,     %mm6\n\t" \
 "  paddw       %mm3,     %mm0\n\t" \
 "  pmulhw      %mm7,     %mm3\n\t" \
 "  movq   "OC_I(2)",     %mm2\n\t" \
 "  pmulhw      %mm1,     %mm7\n\t" \
 "  paddw       %mm1,     %mm5\n\t" \
 "  movq        %mm2,     %mm1\n\t" \
 "  pmulhw "OC_C(2)",     %mm2\n\t" \
 "  psubw       %mm5,     %mm3\n\t" \
 "  movq   "OC_J(6)",     %mm5\n\t" \
 "  paddw       %mm7,     %mm0\n\t" \
 "  movq        %mm5,     %mm7\n\t" \
 "  psubw       %mm4,     %mm0\n\t" \
 "  pmulhw "OC_C(2)",     %mm5\n\t" \
 "  paddw       %mm1,     %mm2\n\t" \
 "  pmulhw "OC_C(6)",     %mm1\n\t" \
 "  paddw       %mm4,     %mm4\n\t" \
 "  paddw       %mm0,     %mm4\n\t" \
 "  psubw       %mm6,     %mm3\n\t" \
 "  paddw       %mm7,     %mm5\n\t" \
 "  paddw       %mm6,     %mm6\n\t" \
 "  pmulhw "OC_C(6)",     %mm7\n\t" \
 "  paddw       %mm3,     %mm6\n\t" \
 "  movq        %mm4,"OC_I(1)"\n\t" \
 "  psubw       %mm5,     %mm1\n\t" \
 "  movq   "OC_C(4)",     %mm4\n\t" \
 "  movq        %mm3,     %mm5\n\t" \
 "  pmulhw      %mm4,     %mm3\n\t" \
 "  paddw       %mm2,     %mm7\n\t" \
 "  movq        %mm6,"OC_I(2)"\n\t" \
 "  movq        %mm0,     %mm2\n\t" \
 "  movq   "OC_I(0)",     %mm6\n\t" \
 "  pmulhw      %mm4,     %mm0\n\t" \
 "  paddw       %mm3,     %mm5\n\t" \
 "  movq   "OC_J(4)",     %mm3\n\t" \
 "  psubw       %mm1,     %mm5\n\t" \
 "  paddw       %mm0,     %mm2\n\t" \
 "  psubw       %mm3,     %mm6\n\t" \
 "  movq        %mm6,     %mm0\n\t" \
 "  pmulhw      %mm4,     %mm6\n\t" \
 "  paddw       %mm3,     %mm3\n\t" \
 "  paddw       %mm1,     %mm1\n\t" \
 "  paddw       %mm0,     %mm3\n\t" \
 "  paddw       %mm5,     %mm1\n\t" \
 "  pmulhw      %mm3,     %mm4\n\t" \
 "  paddw       %mm0,     %mm6\n\t" \
 "  psubw       %mm2,     %mm6\n\t" \
 "  paddw       %mm2,     %mm2\n\t" \
 "  movq   "OC_I(1)",     %mm0\n\t" \
 "  paddw       %mm6,     %mm2\n\t" \
 "  paddw       %mm3,     %mm4\n\t" \
 "  psubw       %mm1,     %mm2\n\t" \
 "#end OC_IDCT_BEGIN\n\t"

/*38+8=46 cycles.*/
#define OC_ROW_IDCT __asm__ __volatile__( \
 "  #OC_ROW_IDCT\n" \
 OC_IDCT_BEGIN \
 "  movq   "OC_I(2)",     %mm3\n\t"  /* r3 = D. */ \
 "  psubw       %mm7,     %mm4\n\t"  /* r4 = E. = E - G */ \
 "  paddw       %mm1,     %mm1\n\t"  /* r1 = H. + H. */ \
 "  paddw       %mm7,     %mm7\n\t"  /* r7 = G + G */ \
 "  paddw       %mm2,     %mm1\n\t"  /* r1 = R1 = A.. + H. */ \
 "  paddw       %mm4,     %mm7\n\t"  /* r7 = G. = E + G */ \
 "  psubw       %mm3,     %mm4\n\t"  /* r4 = R4 = E. - D. */ \
 "  paddw       %mm3,     %mm3\n\t" \
 "  psubw       %mm5,     %mm6\n\t"  /* r6 = R6 = F. - B.. */ \
 "  paddw       %mm5,     %mm5\n\t" \
 "  paddw       %mm4,     %mm3\n\t"  /* r3 = R3 = E. + D. */ \
 "  paddw       %mm6,     %mm5\n\t"  /* r5 = R5 = F. + B.. */ \
 "  psubw       %mm0,     %mm7\n\t"  /* r7 = R7 = G. - C. */ \
 "  paddw       %mm0,     %mm0\n\t" \
 "  movq        %mm1,"OC_I(1)"\n\t"  /* save R1 */ \
 "  paddw       %mm7,     %mm0\n\t" /* r0 = R0 = G. + C. */ \
 "#end OC_ROW_IDCT\n\t" \
)

/* The following macro does two 4x4 transposes in place.
   At entry, we assume:
     r0 = a3 a2 a1 a0
   I(1) = b3 b2 b1 b0
     r2 = c3 c2 c1 c0
     r3 = d3 d2 d1 d0

     r4 = e3 e2 e1 e0
     r5 = f3 f2 f1 f0
     r6 = g3 g2 g1 g0
     r7 = h3 h2 h1 h0

   At exit, we have:
   I(0) = d0 c0 b0 a0
   I(1) = d1 c1 b1 a1
   I(2) = d2 c2 b2 a2
   I(3) = d3 c3 b3 a3

   J(4) = h0 g0 f0 e0
   J(5) = h1 g1 f1 e1
   J(6) = h2 g2 f2 e2
   J(7) = h3 g3 f3 e3

   I(0) I(1) I(2) I(3) is the transpose of r0 I(1) r2 r3.
   J(4) J(5) J(6) J(7) is the transpose of r4   r5 r6 r7.

   Since r1 is free at entry, we calculate the Js first.*/
/*19 cycles.*/
#define OC_TRANSPOSE __asm__ __volatile__( \
 "  #OC_TRANSPOSE\n\t" \
 "  movq           %mm4,     %mm1\n\t" \
 "  punpcklwd      %mm5,     %mm4\n\t" \
 "  movq           %mm0,"OC_I(0)"\n\t" \
 "  punpckhwd      %mm5,     %mm1\n\t" \
 "  movq           %mm6,     %mm0\n\t" \
 "  punpcklwd      %mm7,     %mm6\n\t" \
 "  movq           %mm4,     %mm5\n\t" \
 "  punpckldq      %mm6,     %mm4\n\t" \
 "  punpckhdq      %mm6,     %mm5\n\t" \
 "  movq           %mm1,     %mm6\n\t" \
 "  movq           %mm4,"OC_J(4)"\n\t" \
 "  punpckhwd      %mm7,     %mm0\n\t" \
 "  movq           %mm5,"OC_J(5)"\n\t" \
 "  punpckhdq      %mm0,     %mm6\n\t" \
 "  movq      "OC_I(0)",     %mm4\n\t" \
 "  punpckldq      %mm0,     %mm1\n\t" \
 "  movq      "OC_I(1)",     %mm5\n\t" \
 "  movq           %mm4,     %mm0\n\t" \
 "  movq           %mm6,"OC_J(7)"\n\t" \
 "  punpcklwd      %mm5,     %mm0\n\t" \
 "  movq           %mm1,"OC_J(6)"\n\t" \
 "  punpckhwd      %mm5,     %mm4\n\t" \
 "  movq           %mm2,     %mm5\n\t" \
 "  punpcklwd      %mm3,     %mm2\n\t" \
 "  movq           %mm0,     %mm1\n\t" \
 "  punpckldq      %mm2,     %mm0\n\t" \
 "  punpckhdq      %mm2,     %mm1\n\t" \
 "  movq           %mm4,     %mm2\n\t" \
 "  movq           %mm0,"OC_I(0)"\n\t" \
 "  punpckhwd      %mm3,     %mm5\n\t" \
 "  movq           %mm1,"OC_I(1)"\n\t" \
 "  punpckhdq      %mm5,     %mm4\n\t" \
 "  punpckldq      %mm5,     %mm2\n\t" \
 "  movq           %mm4,"OC_I(3)"\n\t" \
 "  movq           %mm2,"OC_I(2)"\n\t" \
 "#end OC_TRANSPOSE\n\t" \
)

/*38+19=57 cycles.*/
#define OC_COLUMN_IDCT __asm__ __volatile__( \
 "  #OC_COLUMN_IDCT\n" \
 OC_IDCT_BEGIN \
 "  paddw     "OC_8",     %mm2\n\t" \
 "  paddw       %mm1,     %mm1\n\t"  /* r1 = H. + H. */ \
 "  paddw       %mm2,     %mm1\n\t"  /* r1 = R1 = A.. + H. */ \
 "  psraw         $4,     %mm2\n\t"  /* r2 = NR2 */ \
 "  psubw       %mm7,     %mm4\n\t"  /* r4 = E. = E - G */ \
 "  psraw         $4,     %mm1\n\t"  /* r1 = NR1 */ \
 "  movq   "OC_I(2)",     %mm3\n\t"  /* r3 = D. */ \
 "  paddw       %mm7,     %mm7\n\t"  /* r7 = G + G */ \
 "  movq        %mm2,"OC_I(2)"\n\t"  /* store NR2 at I2 */ \
 "  paddw       %mm4,     %mm7\n\t"  /* r7 = G. = E + G */ \
 "  movq        %mm1,"OC_I(1)"\n\t"  /* store NR1 at I1 */ \
 "  psubw       %mm3,     %mm4\n\t"  /* r4 = R4 = E. - D. */ \
 "  paddw     "OC_8",     %mm4\n\t" \
 "  paddw       %mm3,     %mm3\n\t"  /* r3 = D. + D. */ \
 "  paddw       %mm4,     %mm3\n\t"  /* r3 = R3 = E. + D. */ \
 "  psraw         $4,     %mm4\n\t"  /* r4 = NR4 */ \
 "  psubw       %mm5,     %mm6\n\t"  /* r6 = R6 = F. - B.. */ \
 "  psraw         $4,     %mm3\n\t"  /* r3 = NR3 */ \
 "  paddw     "OC_8",     %mm6\n\t" \
 "  paddw       %mm5,     %mm5\n\t"  /* r5 = B.. + B.. */ \
 "  paddw       %mm6,     %mm5\n\t"  /* r5 = R5 = F. + B.. */ \
 "  psraw         $4,     %mm6\n\t"  /* r6 = NR6 */ \
 "  movq        %mm4,"OC_J(4)"\n\t"  /* store NR4 at J4 */ \
 "  psraw         $4,     %mm5\n\t"  /* r5 = NR5 */ \
 "  movq        %mm3,"OC_I(3)"\n\t"  /* store NR3 at I3 */ \
 "  psubw       %mm0,     %mm7\n\t"  /* r7 = R7 = G. - C. */ \
 "  paddw     "OC_8",     %mm7\n\t" \
 "  paddw       %mm0,     %mm0\n\t"  /* r0 = C. + C. */ \
 "  paddw       %mm7,     %mm0\n\t"  /* r0 = R0 = G. + C. */ \
 "  psraw         $4,     %mm7\n\t"  /* r7 = NR7 */ \
 "  movq        %mm6,"OC_J(6)"\n\t"  /* store NR6 at J6 */ \
 "  psraw         $4,     %mm0\n\t"  /* r0 = NR0 */ \
 "  movq        %mm5,"OC_J(5)"\n\t"  /* store NR5 at J5 */ \
 "  movq        %mm7,"OC_J(7)"\n\t"  /* store NR7 at J7 */ \
 "  movq        %mm0,"OC_I(0)"\n\t"  /* store NR0 at I0 */ \
 "  #end OC_COLUMN_IDCT\n\t" \
)

#if (defined(__amd64__) || defined(__x86_64__))
# define OC_MID_REG "%rcx"
# define OC_Y_REG   "%rdx"
#else
# define OC_MID_REG "%ecx"
# define OC_Y_REG   "%edx"
#endif
#define OC_MID(_m,_i) OC_M2STR(_m+(_i)*8)"("OC_MID_REG")"
#define OC_M(_i)      OC_MID(OC_MASK_OFFSET,_i)
#define OC_C(_i)      OC_MID(OC_COSINE_OFFSET,_i-1)
#define OC_8          OC_MID(OC_EIGHT_OFFSET,0)

void oc_idct8x8_mmx(ogg_int16_t _y[64]){
/*This routine accepts an 8x8 matrix, but in transposed form.
  Every 4x4 submatrix is transposed.*/
  __asm__ __volatile__(
   ""
   :
   :"d" (_y),
    "c" (OC_IDCT_CONSTS)
  );
#define OC_I(_k)      OC_M2STR((_k*16))"("OC_Y_REG")"
#define OC_J(_k)      OC_M2STR(((_k-4)*16)+8)"("OC_Y_REG")"
  OC_ROW_IDCT;
  OC_TRANSPOSE;
#undef  OC_I
#undef  OC_J
#define OC_I(_k)      OC_M2STR((_k*16)+64)"("OC_Y_REG")"
#define OC_J(_k)      OC_M2STR(((_k-4)*16)+72)"("OC_Y_REG")"
  OC_ROW_IDCT;
  OC_TRANSPOSE;
#undef  OC_I
#undef  OC_J
#define OC_I(_k)      OC_M2STR((_k*16))"("OC_Y_REG")"
#define OC_J(_k)      OC_I(_k)
  OC_COLUMN_IDCT;
#undef  OC_I
#undef  OC_J
#define OC_I(_k)      OC_M2STR((_k*16)+8)"("OC_Y_REG")"
#define OC_J(_k)      OC_I(_k)
  OC_COLUMN_IDCT;
#undef  OC_I
#undef  OC_J
  __asm__ __volatile__(
   " emms\n\t"
  );
}

/*25 cycles.*/
#define OC_IDCT_BEGIN_10 \
 "  #OC_IDCT_BEGIN_10\n\t" \
 "  movq   "OC_I(3)",     %mm2\n\t" \
 "  nop\n\t" \
 "  movq   "OC_C(3)",     %mm6\n\t" \
 "  movq        %mm2,     %mm4\n\t" \
 "  movq   "OC_C(5)",     %mm1\n\t" \
 "  pmulhw      %mm6,     %mm4\n\t" \
 "  movq   "OC_I(1)",     %mm3\n\t" \
 "  pmulhw      %mm2,     %mm1\n\t" \
 "  movq   "OC_C(1)",     %mm0\n\t" \
 "  paddw       %mm2,     %mm4\n\t" \
 "  pxor        %mm6,     %mm6\n\t" \
 "  paddw       %mm1,     %mm2\n\t" \
 "  movq   "OC_I(2)",     %mm5\n\t" \
 "  pmulhw      %mm3,     %mm0\n\t" \
 "  movq        %mm5,     %mm1\n\t" \
 "  paddw       %mm3,     %mm0\n\t" \
 "  pmulhw "OC_C(7)",     %mm3\n\t" \
 "  psubw       %mm2,     %mm6\n\t" \
 "  pmulhw "OC_C(2)",     %mm5\n\t" \
 "  psubw       %mm4,     %mm0\n\t" \
 "  movq   "OC_I(2)",     %mm7\n\t" \
 "  paddw       %mm4,     %mm4\n\t" \
 "  paddw       %mm5,     %mm7\n\t" \
 "  paddw       %mm0,     %mm4\n\t" \
 "  pmulhw "OC_C(6)",     %mm1\n\t" \
 "  psubw       %mm6,     %mm3\n\t" \
 "  movq        %mm4,"OC_I(1)"\n\t" \
 "  paddw       %mm6,     %mm6\n\t" \
 "  movq   "OC_C(4)",     %mm4\n\t" \
 "  paddw       %mm3,     %mm6\n\t" \
 "  movq        %mm3,     %mm5\n\t" \
 "  pmulhw      %mm4,     %mm3\n\t" \
 "  movq        %mm6,"OC_I(2)"\n\t" \
 "  movq        %mm0,     %mm2\n\t" \
 "  movq   "OC_I(0)",     %mm6\n\t" \
 "  pmulhw      %mm4,     %mm0\n\t" \
 "  paddw       %mm3,     %mm5\n\t" \
 "  paddw       %mm0,     %mm2\n\t" \
 "  psubw       %mm1,     %mm5\n\t" \
 "  pmulhw      %mm4,     %mm6\n\t" \
 "  paddw  "OC_I(0)",     %mm6\n\t" \
 "  paddw       %mm1,     %mm1\n\t" \
 "  movq        %mm6,     %mm4\n\t" \
 "  paddw       %mm5,     %mm1\n\t" \
 "  psubw       %mm2,     %mm6\n\t" \
 "  paddw       %mm2,     %mm2\n\t" \
 "  movq   "OC_I(1)",     %mm0\n\t" \
 "  paddw       %mm6,     %mm2\n\t" \
 "  psubw       %mm1,     %mm2\n\t" \
 "  nop\n\t" \
 "  #end OC_IDCT_BEGIN_10\n\t"

/*25+8=33 cycles.*/
#define OC_ROW_IDCT_10 __asm__ __volatile__( \
 "  #OC_ROW_IDCT_10\n\t" \
 OC_IDCT_BEGIN_10 \
 "  movq    "OC_I(2)",     %mm3\n\t" /* r3 = D. */ \
 "  psubw        %mm7,     %mm4\n\t" /* r4 = E. = E - G */ \
 "  paddw        %mm1,     %mm1\n\t" /* r1 = H. + H. */ \
 "  paddw        %mm7,     %mm7\n\t" /* r7 = G + G */ \
 "  paddw        %mm2,     %mm1\n\t" /* r1 = R1 = A.. + H. */ \
 "  paddw        %mm4,     %mm7\n\t" /* r7 = G. = E + G */ \
 "  psubw        %mm3,     %mm4\n\t" /* r4 = R4 = E. - D. */ \
 "  paddw        %mm3,     %mm3\n\t" \
 "  psubw        %mm5,     %mm6\n\t" /* r6 = R6 = F. - B.. */ \
 "  paddw        %mm5,     %mm5\n\t" \
 "  paddw        %mm4,     %mm3\n\t" /* r3 = R3 = E. + D. */ \
 "  paddw        %mm6,     %mm5\n\t" /* r5 = R5 = F. + B.. */ \
 "  psubw        %mm0,     %mm7\n\t" /* r7 = R7 = G. - C. */ \
 "  paddw        %mm0,     %mm0\n\t" \
 "  movq         %mm1,"OC_I(1)"\n\t" /* save R1 */ \
 "  paddw        %mm7,     %mm0\n\t" /* r0 = R0 = G. + C. */ \
 "#end OC_ROW_IDCT_10\n\t" \
)

/*25+19=44 cycles.*/
#define OC_COLUMN_IDCT_10 __asm__ __volatile__( \
 "  #OC_COLUMN_IDCT_10\n\t" \
 OC_IDCT_BEGIN_10 \
 "  paddw     "OC_8",     %mm2\n\t" \
 "  paddw       %mm1,     %mm1\n\t" /* r1 = H. + H. */ \
 "  paddw       %mm2,     %mm1\n\t" /* r1 = R1 = A.. + H. */ \
 "  psraw         $4,     %mm2\n\t" /* r2 = NR2 */ \
 "  psubw       %mm7,     %mm4\n\t" /* r4 = E. = E - G */ \
 "  psraw         $4,     %mm1\n\t" /* r1 = NR1 */ \
 "  movq   "OC_I(2)",     %mm3\n\t" /* r3 = D. */ \
 "  paddw       %mm7,     %mm7\n\t" /* r7 = G + G */ \
 "  movq        %mm2,"OC_I(2)"\n\t" /* store NR2 at I2 */ \
 "  paddw       %mm4,     %mm7\n\t" /* r7 = G. = E + G */ \
 "  movq        %mm1,"OC_I(1)"\n\t" /* store NR1 at I1 */ \
 "  psubw       %mm3,     %mm4\n\t" /* r4 = R4 = E. - D. */ \
 "  paddw     "OC_8",     %mm4\n\t" \
 "  paddw       %mm3,     %mm3\n\t" /* r3 = D. + D. */ \
 "  paddw       %mm4,     %mm3\n\t" /* r3 = R3 = E. + D. */ \
 "  psraw         $4,     %mm4\n\t" /* r4 = NR4 */ \
 "  psubw       %mm5,     %mm6\n\t" /* r6 = R6 = F. - B.. */ \
 "  psraw         $4,     %mm3\n\t" /* r3 = NR3 */ \
 "  paddw     "OC_8",     %mm6\n\t" \
 "  paddw       %mm5,     %mm5\n\t" /* r5 = B.. + B.. */ \
 "  paddw       %mm6,     %mm5\n\t" /* r5 = R5 = F. + B.. */ \
 "  psraw         $4,     %mm6\n\t" /* r6 = NR6 */ \
 "  movq        %mm4,"OC_J(4)"\n\t" /* store NR4 at J4 */ \
 "  psraw         $4,     %mm5\n\t" /* r5 = NR5 */ \
 "  movq        %mm3,"OC_I(3)"\n\t" /* store NR3 at I3 */ \
 "  psubw       %mm0,     %mm7\n\t" /* r7 = R7 = G. - C. */ \
 "  paddw     "OC_8",     %mm7\n\t" \
 "  paddw       %mm0,     %mm0\n\t" /* r0 = C. + C. */ \
 "  paddw       %mm7,     %mm0\n\t" /* r0 = R0 = G. + C. */ \
 "  psraw         $4,     %mm7\n\t" /* r7 = NR7 */ \
 "  movq        %mm6,"OC_J(6)"\n\t" /* store NR6 at J6 */ \
 "  psraw         $4,     %mm0\n\t" /* r0 = NR0 */ \
 "  movq        %mm5,"OC_J(5)"\n\t" /* store NR5 at J5 */ \
 "  movq        %mm7,"OC_J(7)"\n\t" /* store NR7 at J7 */ \
 "  movq        %mm0,"OC_I(0)"\n\t" /* store NR0 at I0 */ \
 "  #end OC_COLUMN_IDCT_10\n\t" \
)

void oc_idct8x8_10_mmx(ogg_int16_t _y[64]){
  __asm__ __volatile__(
   ""
   :
   :"d" (_y),
   "c" (OC_IDCT_CONSTS)
  );
#define OC_I(_k) OC_M2STR((_k*16))"("OC_Y_REG")"
#define OC_J(_k) OC_M2STR(((_k-4)*16)+8)"("OC_Y_REG")"
  /*Done with dequant, descramble, and partial transpose.
    Now do the iDCT itself.*/
  OC_ROW_IDCT_10;
  OC_TRANSPOSE;
#undef  OC_I
#undef  OC_J
#define OC_I(_k) OC_M2STR((_k*16))"("OC_Y_REG")"
#define OC_J(_k) OC_I(_k)
  OC_COLUMN_IDCT_10;
#undef  OC_I
#undef  OC_J
#define OC_I(_k) OC_M2STR((_k*16)+8)"("OC_Y_REG")"
#define OC_J(_k) OC_I(_k)
  OC_COLUMN_IDCT_10;
#undef  OC_I
#undef  OC_J
  __asm__ __volatile__(
   " emms\n\t"
  );
}
#endif
