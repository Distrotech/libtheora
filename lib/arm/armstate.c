/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2010                *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function:
    last mod: $Id: x86state.c 17344 2010-07-21 01:42:18Z tterribe $

 ********************************************************************/
#include "armint.h"

#if defined(OC_ARM_ASM)

# if defined(OC_ARM_ASM_NEON)
/*This table has been modified from OC_FZIG_ZAG by baking an 8x8 transpose into
   the destination.*/
static const unsigned char OC_FZIG_ZAG_NEON[128]={
   0, 8, 1, 2, 9,16,24,17,
  10, 3, 4,11,18,25,32,40,
  33,26,19,12, 5, 6,13,20,
  27,34,41,48,56,49,42,35,
  28,21,14, 7,15,22,29,36,
  43,50,57,58,51,44,37,30,
  23,31,38,45,52,59,60,53,
  46,39,47,54,61,62,55,63,
  64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64
};
# endif

void oc_state_accel_init_arm(oc_theora_state *_state){
  oc_state_accel_init_c(_state);
  _state->cpu_flags=oc_cpu_flags_get();
# if defined(OC_STATE_USE_VTABLE)
  _state->opt_vtable.frag_copy_list=oc_frag_copy_list_arm;
  _state->opt_vtable.frag_recon_intra=oc_frag_recon_intra_arm;
  _state->opt_vtable.frag_recon_inter=oc_frag_recon_inter_arm;
  _state->opt_vtable.frag_recon_inter2=oc_frag_recon_inter2_arm;
  _state->opt_vtable.idct8x8=oc_idct8x8_arm;
  /*Note: We _must_ set this function pointer, because the macro in armint.h
     calls it with different arguments, so the C version will segfault.*/
  _state->opt_vtable.state_loop_filter_frag_rows=
   (oc_state_loop_filter_frag_rows_func)oc_loop_filter_frag_rows_arm;
# endif
# if defined(OC_ARM_ASM_EDSP)
  if(_state->cpu_flags&OC_CPU_ARM_EDSP){
#  if defined(OC_STATE_USE_VTABLE)
    _state->opt_vtable.frag_copy_list=oc_frag_copy_list_edsp;
#  endif
  }
#  if defined(OC_ARM_ASM_MEDIA)
  if(_state->cpu_flags&OC_CPU_ARM_MEDIA){
#   if defined(OC_STATE_USE_VTABLE)
    _state->opt_vtable.frag_recon_intra=oc_frag_recon_intra_v6;
    _state->opt_vtable.frag_recon_inter=oc_frag_recon_inter_v6;
    _state->opt_vtable.frag_recon_inter2=oc_frag_recon_inter2_v6;
    _state->opt_vtable.idct8x8=oc_idct8x8_v6;
    _state->opt_vtable.loop_filter_init=oc_loop_filter_init_v6;
    _state->opt_vtable.state_loop_filter_frag_rows=
     (oc_state_loop_filter_frag_rows_func)oc_loop_filter_frag_rows_v6;
#   endif
  }
#   if defined(OC_ARM_ASM_NEON)
  if(_state->cpu_flags&OC_CPU_ARM_NEON){
#    if defined(OC_STATE_USE_VTABLE)
    _state->opt_vtable.frag_copy_list=oc_frag_copy_list_neon;
    _state->opt_vtable.frag_recon_intra=oc_frag_recon_intra_neon;
    _state->opt_vtable.frag_recon_inter=oc_frag_recon_inter_neon;
    _state->opt_vtable.frag_recon_inter2=oc_frag_recon_inter2_neon;
    _state->opt_vtable.loop_filter_init=oc_loop_filter_init_neon;
    _state->opt_vtable.state_loop_filter_frag_rows=
     (oc_state_loop_filter_frag_rows_func)oc_loop_filter_frag_rows_neon;
    _state->opt_vtable.idct8x8=oc_idct8x8_neon;
#    endif
    _state->opt_data.dct_fzig_zag=OC_FZIG_ZAG_NEON;
  }
#   endif
#  endif
# endif
}

#endif
