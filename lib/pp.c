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
  last mod: $Id: pp.c,v 1.1 2002/09/18 08:56:57 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <ogg/ogg.h>
#include "encoder_internal.h"

void PClearFrameInfo(PP_INSTANCE * ppi){
  int i;

  if(ppi->ScanPixelIndexTable) _ogg_free(ppi->ScanPixelIndexTable);
  ppi->ScanPixelIndexTable=0;

  if(ppi->ScanDisplayFragments) _ogg_free(ppi->ScanDisplayFragments);
  ppi->ScanDisplayFragments=0;

  for(i = 0 ; i < MAX_PREV_FRAMES ; i ++) 
    if(ppi->PrevFragments[i]){
      _ogg_free(ppi->PrevFragments[i]);
      ppi->PrevFragments[i]=0;
    }

  if(ppi->FragScores) _ogg_free(ppi->FragScores);
  ppi->FragScores=0;

  if(ppi->SameGreyDirPixels) _ogg_free(ppi->SameGreyDirPixels);
  ppi->SameGreyDirPixels=0;

  if(ppi->FragDiffPixels) _ogg_free(ppi->FragDiffPixels);
  ppi->FragDiffPixels=0;

  if(ppi->BarBlockMap) _ogg_free(ppi->BarBlockMap);
  ppi->BarBlockMap=0;

  if(ppi->TmpCodedMap) _ogg_free(ppi->TmpCodedMap);
  ppi->TmpCodedMap=0;

  if(ppi->RowChangedPixels) _ogg_free(ppi->RowChangedPixels);
  ppi->RowChangedPixels=0;

  if(ppi->PixelScores) _ogg_free(ppi->PixelScores);
  ppi->PixelScores=0;

  if(ppi->PixelChangedMap) _ogg_free(ppi->PixelChangedMap);
  ppi->PixelChangedMap=0;

  if(ppi->ChLocals) _ogg_free(ppi->ChLocals);
  ppi->ChLocals=0;

  if(ppi->yuv_differences) _ogg_free(ppi->yuv_differences);
  ppi->yuv_differences=0;
  
}

void PAllocateFrameInfo(PP_INSTANCE * ppi){
  int i;
  PClearFrameInfo(ppi);

  ppi->ScanPixelIndexTable = 
    _ogg_malloc(ppi->ScanFrameFragments*sizeof(*ppi->ScanPixelIndexTable));

  ppi->ScanDisplayFragments = 
    _ogg_malloc(ppi->ScanFrameFragments*sizeof(*ppi->ScanDisplayFragments));

  for(i = 0 ; i < MAX_PREV_FRAMES ; i ++) 
    ppi->PrevFragments[i] = 
      _ogg_malloc(ppi->ScanFrameFragments*sizeof(*ppi->PrevFragments));
   
  ppi->FragScores = 
    _ogg_malloc(ppi->ScanFrameFragments*sizeof(*ppi->FragScores));

  ppi->SameGreyDirPixels = 
    _ogg_malloc(ppi->ScanFrameFragments*sizeof(*ppi->SameGreyDirPixels));

  ppi->FragDiffPixels = 
    _ogg_malloc(ppi->ScanFrameFragments*sizeof(*ppi->FragScores));
  
  ppi->BarBlockMap= 
    _ogg_malloc(3 * ppi->ScanHFragments*sizeof(*ppi->BarBlockMap));

  ppi->TmpCodedMap = 
    _ogg_malloc(ppi->ScanHFragments*sizeof(*ppi->TmpCodedMap));

  ppi->RowChangedPixels = 
    _ogg_malloc(3 * ppi->ScanConfig.VideoFrameHeight*
		sizeof(*ppi->RowChangedPixels));

  ppi->PixelScores = 
    _ogg_malloc(ppi->ScanConfig.VideoFrameWidth*
		sizeof(*ppi->PixelScores) * PSCORE_CB_ROWS);

  ppi->PixelChangedMap = 
    _ogg_malloc(ppi->ScanConfig.VideoFrameWidth*
		sizeof(*ppi->PixelChangedMap) * PMAP_CB_ROWS);

  ppi->ChLocals = 
    _ogg_malloc(ppi->ScanConfig.VideoFrameWidth*
		sizeof(*ppi->ChLocals) * CHLOCALS_CB_ROWS);

  ppi->yuv_differences = 
    _ogg_malloc(ppi->ScanConfig.VideoFrameWidth*
		sizeof(*ppi->yuv_differences) * YDIFF_CB_ROWS);
}

void ClearPPInstance(PP_INSTANCE *ppi){
  PClearFrameInfo(ppi);
}


void InitPPInstance(PP_INSTANCE *ppi){

  memset(ppi,0,sizeof(*ppi));

  /* Initializations */
  ppi->PrevFrameLimit = 3; /* Must not exceed MAX_PREV_FRAMES (Note
			      that this number includes the current
			      frame so "1 = no effect") */

  /* Scan control variables. */
  ppi->HFragPixels = 8;
  ppi->VFragPixels = 8;
  
  ppi->SRFGreyThresh = 4;
  ppi->SRFColThresh = 5;
  ppi->NoiseSupLevel = 3;
  ppi->SgcLevelThresh = 3;
  ppi->SuvcLevelThresh = 4;
  
  /* Variables controlling S.A.D. breakouts. */
  ppi->GrpLowSadThresh = 10;
  ppi->GrpHighSadThresh = 64;
  ppi->PrimaryBlockThreshold = 5;
  ppi->SgcThresh = 16;  /* (Default values for 8x8 blocks). */
  
  ppi->UVBlockThreshCorrection = 1.25;
  ppi->UVSgcCorrection = 1.5;
  
  ppi->MaxLineSearchLen = MAX_SEARCH_LINE_LEN;
}
