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
  last mod: $Id: decode.c,v 1.1 2002/09/20 09:30:32 xiphmont Exp $

 ********************************************************************/

#include <ogg/ogg.h>
#include "encoder_internal.h"

int GetFrameType(PB_INSTANCE *pbi){
  return pbi->FrameType; 
}

int PbBuildBitmapHeader( PB_INSTANCE *pbi, 
			 ogg_uint32_t ImageWidth, 
			 ogg_uint32_t ImageHeight ){
  if(!InitFrameDetails(pbi))return 0;
  return 1;
}

int LoadFrame(PB_INSTANCE *pbi){ 
  
  /* Load the frame header (including the frame size). */
  if ( LoadFrameHeader(pbi) ){
    /* Read in the updated block map */
    QuadDecodeDisplayFragments( pbi );
    return 1;
  }
  
  return 0;
}

int LoadFrameHeader(PB_INSTANCE *pbi){
  unsigned char  VersionByte0;    /* Must be 0 for VP30b and later */
  unsigned char  DctQMask;
  unsigned char  SpareBits;       /* Spare cfg bits */
  unsigned char  Unused;
  
  /* Is the frame and inter frame or a key frame */
  pbi->FrameType = oggpackB_read(&pbi->opb,1);
    
  /* unused bit */
  Unused = oggpackB_read(&pbi->opb,1);
  
  /* Quality (Q) index */
  DctQMask = oggpackB_read( &pbi->opb,   6 );

  /* If the frame was a base frame then read the frame dimensions and
     build a bitmap structure. */
  if ( (pbi->FrameType == BASE_FRAME) ){
    /* Read the frame dimensions bytes (0,0 indicates vp31 or later) */
    VersionByte0 = oggpackB_read( &pbi->opb,   8 );
    pbi->Vp3VersionNo = oggpackB_read( &pbi->opb,   5 );
    
    /* we don't decode version 0 either */
    if(pbi->Vp3VersionNo > CURRENT_ENCODE_VERSION) return 0;
    if(pbi->Vp3VersionNo == 0) return 0;
    
    /* Initialise version specific quantiser values */
    InitQTables( pbi );

    /* Read the type / coding method for the key frame. */
    pbi->KeyFrameType = oggpackB_read( &pbi->opb,   1 );
    
    SpareBits = oggpackB_read( &pbi->opb,   2 );
    
    InitHuffmanSet( pbi );
    
  }
	
  /* Set this frame quality value from Q Index */
  pbi->ThisFrameQualityValue = pbi->QThreshTable[DctQMask];
  
  return 1;
}

void SetFrameType( PB_INSTANCE *pbi,unsigned char FrType ){ 
  /* Set the appropriate frame type according to the request. */
  switch ( FrType ){  
    
  case BASE_FRAME:
    pbi->FrameType = FrType;
    break;
    
  default:
    pbi->FrameType = FrType;
    break;
  }
}

