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
    last mod: $Id$

 ********************************************************************/

#include <limits.h>
#if !defined(_decint_H)
# define _decint_H (1)
# include "theora/theoradec.h"
# include "internal.h"
# include "bitpack.h"

typedef struct th_setup_info         oc_setup_info;
typedef struct oc_dec_opt_vtable     oc_dec_opt_vtable;
typedef struct oc_dec_pipeline_state oc_dec_pipeline_state;
typedef struct th_dec_ctx            oc_dec_ctx;

# include "huffdec.h"
# include "dequant.h"

/*Constants for the packet-in state machine specific to the decoder.*/

/*Next packet to read: Data packet.*/
#define OC_PACKET_DATA (0)



struct th_setup_info{
  /*The Huffman codes.*/
  ogg_int16_t   *huff_tables[TH_NHUFFMAN_TABLES];
  /*The quantization parameters.*/
  th_quant_info  qinfo;
};



/*Decoder specific functions with accelerated variants.*/
struct oc_dec_opt_vtable{
  void (*dc_unpredict_mcu_plane)(oc_dec_ctx *_dec,
   oc_dec_pipeline_state *_pipe,int _pli);
};



struct oc_dec_pipeline_state{
  int                 bounding_values[256];
  ptrdiff_t           ti[3][64];
  ptrdiff_t           ebi[3][64];
  ptrdiff_t           eob_runs[3][64];
  const ptrdiff_t    *coded_fragis[3];
  const ptrdiff_t    *uncoded_fragis[3];
  ptrdiff_t           ncoded_fragis[3];
  ptrdiff_t           nuncoded_fragis[3];
  const ogg_uint16_t *dequant[3][3][2];
  int                 fragy0[3];
  int                 fragy_end[3];
  int                 pred_last[3][3];
  int                 mcu_nvfrags;
  int                 loop_filter;
  int                 pp_level;
};


struct th_dec_ctx{
  /*Shared encoder/decoder state.*/
  oc_theora_state      state;
  /*Whether or not packets are ready to be emitted.
    This takes on negative values while there are remaining header packets to
     be emitted, reaches 0 when the codec is ready for input, and goes to 1
     when a frame has been processed and a data packet is ready.*/
  int                  packet_state;
  /*Buffer in which to assemble packets.*/
  oc_pack_buf          opb;
  /*Huffman decode trees.*/
  ogg_int16_t         *huff_tables[TH_NHUFFMAN_TABLES];
  /*The index of the first token in each plane for each coefficient.*/
  ptrdiff_t            ti0[3][64];
  /*The number of outstanding EOB runs at the start of each coefficient in each
     plane.*/
  ptrdiff_t            eob_runs[3][64];
  /*The DCT token lists.*/
  unsigned char       *dct_tokens;
  /*The extra bits associated with DCT tokens.*/
  unsigned char       *extra_bits;
  /*The number of dct tokens unpacked so far.*/
  int                  dct_tokens_count;
  /*The out-of-loop post-processing level.*/
  int                  pp_level;
  /*The DC scale used for out-of-loop deblocking.*/
  int                  pp_dc_scale[64];
  /*The sharpen modifier used for out-of-loop deringing.*/
  int                  pp_sharp_mod[64];
  /*The DC quantization index of each block.*/
  unsigned char       *dc_qis;
  /*The variance of each block.*/
  int                 *variances;
  /*The storage for the post-processed frame buffer.*/
  unsigned char       *pp_frame_data;
  /*Whether or not the post-processsed frame buffer has space for chroma.*/
  int                  pp_frame_state;
  /*The buffer used for the post-processed frame.
    Note that this is _not_ guaranteed to have the same strides and offsets as
     the reference frame buffers.*/
  th_ycbcr_buffer      pp_frame_buf;
  /*The striped decode callback function.*/
  th_stripe_callback   stripe_cb;
  oc_dec_pipeline_state pipe;
  /*Table for decoder acceleration functions.*/
  oc_dec_opt_vtable    opt_vtable;
# if defined(HAVE_CAIRO)
  /*Output metrics for debugging.*/
  int                  telemetry;
  int                  telemetry_mbmode;
  int                  telemetry_mv;
  int                  telemetry_qi;
  int                  telemetry_bits;
  int                  telemetry_frame_bytes;
  int                  telemetry_coding_bytes;
  int                  telemetry_mode_bytes;
  int                  telemetry_mv_bytes;
  int                  telemetry_qi_bytes;
  int                  telemetry_dc_bytes;
  unsigned char       *telemetry_frame_data;
# endif
};

/*Decoder-specific accelerated functions.*/
# if defined(OC_C64X_ASM)
#  include "c64x/c64xdec.h"
# endif

# if !defined(oc_dec_dc_unpredict_mcu_plane)
#  define oc_dec_dc_unpredict_mcu_plane(_dec,_pipe,_pli) \
 ((*(_dec)->opt_vtable.dc_unpredict_mcu_plane)(_dec,_pipe,_pli))
# endif

/*Default pure-C implementations.*/
void oc_dec_vtable_init_c(oc_dec_ctx *_dec);

void oc_dec_dc_unpredict_mcu_plane_c(oc_dec_ctx *_dec,
 oc_dec_pipeline_state *_pipe,int _pli);

#endif
