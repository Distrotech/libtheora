/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2009                *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function:
    last mod: $Id: x86int.h 15675 2009-02-06 09:43:27Z tterribe $

 ********************************************************************/

#if !defined(_x86_x86enc_H)
# define _x86_x86enc_H (1)
# include "../encint.h"
# include "x86int.h"

void oc_enc_vtable_init_x86(oc_enc_ctx *_enc);

void oc_enc_frag_sub_mmx(ogg_int16_t _diff[64],
 const unsigned char *_x,const unsigned char *_y,int _stride);
void oc_enc_frag_sub_128_mmx(ogg_int16_t _diff[64],
 const unsigned char *_x,int _stride);
unsigned oc_enc_frag_ssd_sse2(const unsigned char *_src,
 const unsigned char *_ref,int _ystride);
unsigned oc_enc_frag_border_ssd_sse2(const unsigned char *_src,
 const unsigned char *_ref,int _ystride,ogg_int64_t _mask);
unsigned oc_enc_frag_sad_mmxext(const unsigned char *_src,
 const unsigned char *_ref,int _ystride);
unsigned oc_enc_frag_sad_thresh_mmxext(const unsigned char *_src,
 const unsigned char *_ref,int _ystride,unsigned _thresh);
unsigned oc_enc_frag_sad2_thresh_mmxext(const unsigned char *_src,
 const unsigned char *_ref1,const unsigned char *_ref2,int _ystride,
 unsigned _thresh);
unsigned oc_enc_frag_satd_mmxext(unsigned *_dc,const unsigned char *_src,
 const unsigned char *_ref,int _ystride);
unsigned oc_enc_frag_satd_sse2(unsigned *_dc,const unsigned char *_src,
 const unsigned char *_ref,int _ystride);
unsigned oc_enc_frag_satd2_mmxext(unsigned *_dc,const unsigned char *_src,
 const unsigned char *_ref1,const unsigned char *_ref2,int _ystride);
unsigned oc_enc_frag_satd2_sse2(unsigned *_dc,const unsigned char *_src,
 const unsigned char *_ref1,const unsigned char *_ref2,int _ystride);
unsigned oc_enc_frag_intra_satd_sse2(unsigned *_dc,
 const unsigned char *_src,int _ystride);
unsigned oc_enc_frag_intra_satd_mmxext(unsigned *_dc,
 const unsigned char *_src,int _ystride);
void oc_enc_enquant_table_init_x86(void *_enquant,
 const ogg_uint16_t _dequant[64]);
void oc_enc_enquant_table_fixup_x86(void *_enquant[3][3][2],int _nqis);
int oc_enc_quantize_sse2(ogg_int16_t _qdct[64],const ogg_int16_t _dct[64],
 const ogg_uint16_t _dequant[64],const void *_enquant);
void oc_int_frag_copy2_mmxext(unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src1,const unsigned char *_src2,int _src_ystride);
void oc_enc_frag_copy2_mmxext(unsigned char *_dst,
 const unsigned char *_src1,const unsigned char *_src2,int _ystride);
void oc_enc_fdct8x8_mmx(ogg_int16_t _y[64],const ogg_int16_t _x[64]);

# if defined(OC_X86_64_ASM)
void oc_enc_fdct8x8_x86_64sse2(ogg_int16_t _y[64],const ogg_int16_t _x[64]);
# endif

#endif
