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
  last mod: $Id: reconstruct.c,v 1.2 2002/09/20 22:01:43 xiphmont Exp $

 ********************************************************************/

#include <ogg/ogg.h>
#include "encoder_internal.h"


static void SatUnsigned8( unsigned char * ResultPtr, ogg_int16_t * DataBlock, 
		   ogg_uint32_t ResultLineStep, ogg_uint32_t DataLineStep ) {
  int  i;
       
  for ( i = 0; i < BLOCK_HEIGHT_WIDTH; i++ ){
    ResultPtr[0] = clamp255(DataBlock[0]);
    ResultPtr[1] = clamp255(DataBlock[1]);
    ResultPtr[2] = clamp255(DataBlock[2]);
    ResultPtr[3] = clamp255(DataBlock[3]);
    ResultPtr[4] = clamp255(DataBlock[4]);
    ResultPtr[5] = clamp255(DataBlock[5]);
    ResultPtr[6] = clamp255(DataBlock[6]);
    ResultPtr[7] = clamp255(DataBlock[7]);
    
    DataBlock += DataLineStep;
    ResultPtr += ResultLineStep;
  }
}

void ReconIntra( PB_INSTANCE *pbi, unsigned char * ReconPtr, 
		 ogg_uint16_t * ChangePtr, ogg_uint32_t LineStep ) {
  ogg_uint32_t i;
  ogg_int16_t *TmpDataPtr = pbi->TmpDataBuffer;
  
  for ( i = 0; i < BLOCK_HEIGHT_WIDTH; i++ ){	
    /* Convert the data back to 8 bit unsigned */
    TmpDataPtr[0] = ( ChangePtr[0] + 128 );
    TmpDataPtr[1] = ( ChangePtr[1] + 128 );
    TmpDataPtr[2] = ( ChangePtr[2] + 128 );
    TmpDataPtr[3] = ( ChangePtr[3] + 128 );
    TmpDataPtr[4] = ( ChangePtr[4] + 128 );
    TmpDataPtr[5] = ( ChangePtr[5] + 128 );
    TmpDataPtr[6] = ( ChangePtr[6] + 128 );
    TmpDataPtr[7] = ( ChangePtr[7] + 128 );
    
    TmpDataPtr += BLOCK_HEIGHT_WIDTH;
    ChangePtr += BLOCK_HEIGHT_WIDTH;
  }
  
  /* Saturate the output to unsigend 8 bit values */
  SatUnsigned8( ReconPtr, pbi->TmpDataBuffer, LineStep, BLOCK_HEIGHT_WIDTH );
}

void ReconInter( PB_INSTANCE *pbi, unsigned char * ReconPtr, 
		 unsigned char * RefPtr, ogg_int16_t * ChangePtr, 
		 ogg_uint32_t LineStep ) {
  ogg_uint32_t i;
  ogg_int16_t *TmpDataPtr = pbi->TmpDataBuffer;
  
  for ( i = 0; i < BLOCK_HEIGHT_WIDTH; i++) {	
    TmpDataPtr[0] = (RefPtr[0] + ChangePtr[0]);
    TmpDataPtr[1] = (RefPtr[1] + ChangePtr[1]);
    TmpDataPtr[2] = (RefPtr[2] + ChangePtr[2]);
    TmpDataPtr[3] = (RefPtr[3] + ChangePtr[3]);
    TmpDataPtr[4] = (RefPtr[4] + ChangePtr[4]);
    TmpDataPtr[5] = (RefPtr[5] + ChangePtr[5]);
    TmpDataPtr[6] = (RefPtr[6] + ChangePtr[6]);
    TmpDataPtr[7] = (RefPtr[7] + ChangePtr[7]);
    
    ChangePtr += BLOCK_HEIGHT_WIDTH;
    TmpDataPtr += BLOCK_HEIGHT_WIDTH;
    RefPtr += LineStep; 
  }
  
  /* Saturate the output to unsigend 8 bit values */
  SatUnsigned8( ReconPtr, pbi->TmpDataBuffer, LineStep, BLOCK_HEIGHT_WIDTH );
  
}

void ReconInterHalfPixel2( PB_INSTANCE *pbi, unsigned char * ReconPtr, 
			   unsigned char * RefPtr1, unsigned char * RefPtr2, 
			   ogg_int16_t * ChangePtr, ogg_uint32_t LineStep ) {
  ogg_uint32_t  i;
  ogg_int16_t *TmpPtr = pbi->TmpDataBuffer;
  
  for ( i = 0; i < BLOCK_HEIGHT_WIDTH; i++ ){	
    TmpPtr[0] = ((((int)RefPtr1[0] + (int)RefPtr2[0]) >> 1) + ChangePtr[0] );
    TmpPtr[1] = ((((int)RefPtr1[1] + (int)RefPtr2[1]) >> 1) + ChangePtr[1] );
    TmpPtr[2] = ((((int)RefPtr1[2] + (int)RefPtr2[2]) >> 1) + ChangePtr[2] );
    TmpPtr[3] = ((((int)RefPtr1[3] + (int)RefPtr2[3]) >> 1) + ChangePtr[3] );
    TmpPtr[4] = ((((int)RefPtr1[4] + (int)RefPtr2[4]) >> 1) + ChangePtr[4] );
    TmpPtr[5] = ((((int)RefPtr1[5] + (int)RefPtr2[5]) >> 1) + ChangePtr[5] );
    TmpPtr[6] = ((((int)RefPtr1[6] + (int)RefPtr2[6]) >> 1) + ChangePtr[6] );
    TmpPtr[7] = ((((int)RefPtr1[7] + (int)RefPtr2[7]) >> 1) + ChangePtr[7] );

    ChangePtr += BLOCK_HEIGHT_WIDTH;
    TmpPtr += BLOCK_HEIGHT_WIDTH;
    RefPtr1 += LineStep; 
    RefPtr2 += LineStep; 
  }

  /* Saturate the output to unsigend 8 bit values */
  SatUnsigned8( ReconPtr, pbi->TmpDataBuffer, LineStep, BLOCK_HEIGHT_WIDTH );

}
