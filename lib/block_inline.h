/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: 
  last mod: $Id: block_inline.h,v 1.1 2002/09/20 09:30:31 xiphmont Exp $

 ********************************************************************/

static ogg_int32_t MBOrderMap[4] = { 0, 2, 3, 1 };
static ogg_int32_t BlockOrderMap1[4][4] =	
{ { 0, 1, 3, 2 },
  { 0, 2, 3, 1 },       
  { 0, 2, 3, 1 },
  { 3, 2, 0, 1 }
};

static ogg_int32_t BlockOrderMap2[4][4] =	
{ { 0, 1, 2, 3 },
  { 0, 1, 2, 3 },       
  { 0, 1, 2, 3 },
  { 0, 1, 2, 3 }
};
       
/* The following hard wired tables and masks are used to decode block
   pattern tokens */
static unsigned char BlockPatternMask[4] = { 
  0x08, 0x04, 0x02, 0x01 };

static unsigned char BlockDecode1[2][8] = {	
  { 0x01, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x07, 0x0B, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00 } };

static unsigned char BlockDecode2[2][16] = {	
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 
    0x0A, 0x03, 0x0E, 0x07, 0x0D, 0x05, 0x0C, 0x02 },  
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
    0x05, 0x03, 0x02, 0x0C, 0x04, 0x0A, 0x08, 0x0D } };

static unsigned char BlockDecode3[2][32] = {	
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x09, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x09, 0x06, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, } };

static ogg_uint32_t BPPredictor[15] = { 
  0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1  };	

         
static ogg_int32_t QuadMapToIndex1( ogg_int32_t	(*BlockMap)[4][4], 
				    ogg_uint32_t SB, ogg_uint32_t MB, 
				    ogg_uint32_t B ){
  return BlockMap[SB][MBOrderMap[MB]][BlockOrderMap1[MB][B]];
}

static ogg_int32_t QuadMapToIndex2( ogg_int32_t	(*BlockMap)[4][4], 
				    ogg_uint32_t SB, ogg_uint32_t MB, 
				    ogg_uint32_t B ){
  return BlockMap[SB][MBOrderMap[MB]][BlockOrderMap2[MB][B]];
}

static ogg_int32_t QuadMapToMBTopLeft( ogg_int32_t (*BlockMap)[4][4], 
				       ogg_uint32_t SB, ogg_uint32_t MB ){
  return BlockMap[SB][MBOrderMap[MB]][0];
}

