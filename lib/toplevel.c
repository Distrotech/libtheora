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
  last mod: $Id: toplevel.c,v 1.18 2003/05/12 22:23:03 mauricio Exp $

 ********************************************************************/

#include <stdlib.h>
#include <ogg/ogg.h>
#include <theora/theora.h>
#include "encoder_internal.h"
#include "toplevel_lookup.h"

#define VERSION_MAJOR 3
#define VERSION_MINOR 1
#define VERSION_SUB 0

#define VENDOR_STRING "Xiph.Org libTheora I 20030511 3 1 0"

#define A_TABLE_SIZE	    29
#define DF_CANDIDATE_WINDOW 5

static void EClearFragmentInfo(CP_INSTANCE * cpi){
  if(cpi->extra_fragments)
    _ogg_free(cpi->extra_fragments);
  if(cpi->FragmentLastQ)
    _ogg_free(cpi->FragmentLastQ);
  if(cpi->FragTokens)
    _ogg_free(cpi->FragTokens);
  if(cpi->FragTokenCounts)
    _ogg_free(cpi->FragTokenCounts);
  if(cpi->RunHuffIndices)
    _ogg_free(cpi->RunHuffIndices);
  if(cpi->LastCodedErrorScore)
    _ogg_free(cpi->LastCodedErrorScore);
  if(cpi->ModeList)
    _ogg_free(cpi->ModeList);
  if(cpi->MVList)
    _ogg_free(cpi->MVList);
  if(cpi->DCT_codes )
    _ogg_free( cpi->DCT_codes );
  if(cpi->DCTDataBuffer )
    _ogg_free( cpi->DCTDataBuffer);
  if(cpi->quantized_list)
    _ogg_free( cpi->quantized_list);
  if(cpi->OriginalDC)
    _ogg_free( cpi->OriginalDC);
  if(cpi->PartiallyCodedFlags)
    _ogg_free(cpi->PartiallyCodedFlags);
  if(cpi->PartiallyCodedMbPatterns)
    _ogg_free(cpi->PartiallyCodedMbPatterns);
  if(cpi->UncodedMbFlags)
    _ogg_free(cpi->UncodedMbFlags);
  
  if(cpi->BlockCodedFlags)
    _ogg_free(cpi->BlockCodedFlags);
  
  cpi->extra_fragments = 0;
  cpi->FragmentLastQ = 0;
  cpi->FragTokens = 0;
  cpi->FragTokenCounts = 0;
  cpi->RunHuffIndices = 0;
  cpi->LastCodedErrorScore = 0;
  cpi->ModeList = 0;
  cpi->MVList = 0;
  cpi->DCT_codes = 0;
  cpi->DCTDataBuffer = 0;
  cpi->quantized_list = 0;
  cpi->OriginalDC = 0;
  cpi->BlockCodedFlags = 0;
}

static void EInitFragmentInfo(CP_INSTANCE * cpi){
  
  /* clear any existing info */
  EClearFragmentInfo(cpi);
  
  /* Perform Fragment Allocations */
  cpi->extra_fragments =  
    _ogg_malloc(cpi->pb.UnitFragments*sizeof(unsigned char));
  
  /* A note to people reading and wondering why malloc returns aren't
     checked:

     lines like the following that implement a general strategy of
     'check the return of malloc; a zero pointer means we're out of
     memory!'...:

  if(!cpi->extra_fragments) { EDeleteFragmentInfo(cpi); return FALSE; }

     ...are not useful.  It's true that many platforms follow this
     malloc behavior, but many do not.  The more modern malloc
     strategy is only to allocate virtual pages, which are not mapped
     until the memory on that page is touched.  At *that* point, if
     the machine is out of heap, the page fails to be mapped and a
     SEGV is generated.  

     That means that is we want to deal with out of memory conditions,
     we *must* be prepared to process a SEGV.  If we implement the
     SEGV handler, there's no reason to to check malloc return; it is
     a waste of code. */

  cpi->FragmentLastQ = 
    _ogg_malloc(cpi->pb.UnitFragments*
		sizeof(*cpi->FragmentLastQ));
  cpi->FragTokens =  
    _ogg_malloc(cpi->pb.UnitFragments*
		sizeof(*cpi->FragTokens));
  cpi->OriginalDC =
    _ogg_malloc(cpi->pb.UnitFragments*
		sizeof(*cpi->OriginalDC));
  cpi->FragTokenCounts =  
    _ogg_malloc(cpi->pb.UnitFragments* 
		sizeof(*cpi->FragTokenCounts));
  cpi->RunHuffIndices =  
    _ogg_malloc(cpi->pb.UnitFragments*
		sizeof(*cpi->RunHuffIndices));
  cpi->LastCodedErrorScore =  
    _ogg_malloc(cpi->pb.UnitFragments*
		sizeof(*cpi->LastCodedErrorScore));
  cpi->BlockCodedFlags =  
    _ogg_malloc(cpi->pb.UnitFragments*
		sizeof(*cpi->BlockCodedFlags));
  cpi->ModeList =  
    _ogg_malloc(cpi->pb.UnitFragments*
		sizeof(*cpi->ModeList));
  cpi->MVList =  
    _ogg_malloc(cpi->pb.UnitFragments* 
		sizeof(cpi->MVList));
  cpi->DCT_codes =
    _ogg_malloc(64*
		sizeof(*cpi->DCT_codes));
  cpi->DCTDataBuffer =
    _ogg_malloc(64*
		sizeof(*cpi->DCTDataBuffer));
  cpi->quantized_list =
    _ogg_malloc(64*
		sizeof(*cpi->quantized_list));
  cpi->PartiallyCodedFlags = 
    _ogg_malloc(cpi->pb.MacroBlocks* 
		sizeof(*cpi->PartiallyCodedFlags));
  cpi->PartiallyCodedMbPatterns = 
    _ogg_malloc(cpi->pb.MacroBlocks* 
		sizeof(*cpi->PartiallyCodedMbPatterns));
  cpi->UncodedMbFlags = 
    _ogg_malloc(cpi->pb.MacroBlocks* 
		sizeof(*cpi->UncodedMbFlags));

}

void EClearFrameInfo(CP_INSTANCE * cpi) {
  if(cpi->ConvDestBuffer )
    _ogg_free(cpi->ConvDestBuffer );
  cpi->ConvDestBuffer = 0;
  
  if(cpi->yuv0ptr)
    _ogg_free(cpi->yuv0ptr);
  cpi->yuv0ptr = 0;

  if(cpi->yuv1ptr)
    _ogg_free(cpi->yuv1ptr);
  cpi->yuv1ptr = 0;

  if(cpi->OptimisedTokenListEb )
    _ogg_free(cpi->OptimisedTokenListEb);
  cpi->OptimisedTokenListEb = 0;
  
  if(cpi->OptimisedTokenList )
    _ogg_free(cpi->OptimisedTokenList);
  cpi->OptimisedTokenList = 0;

  if(cpi->OptimisedTokenListHi )
    _ogg_free(cpi->OptimisedTokenListHi);
  cpi->OptimisedTokenListHi = 0;

  if(cpi->OptimisedTokenListPl )
    _ogg_free(cpi->OptimisedTokenListPl);
  cpi->OptimisedTokenListPl = 0;
  
}

void EInitFrameInfo(CP_INSTANCE * cpi){
  int FrameSize = cpi->pb.ReconYPlaneSize + 2 * cpi->pb.ReconUVPlaneSize;

  /* clear any existing info */
  EClearFrameInfo(cpi);

  /* allocate frames */
  cpi->ConvDestBuffer = 
    _ogg_malloc(FrameSize*
		sizeof(*cpi->ConvDestBuffer));
  cpi->yuv0ptr = 
    _ogg_malloc(FrameSize*
		sizeof(*cpi->yuv0ptr));
  cpi->yuv1ptr = 
    _ogg_malloc(FrameSize*
		sizeof(*cpi->yuv1ptr));
  cpi->OptimisedTokenListEb = 
    _ogg_malloc(FrameSize*
		sizeof(*cpi->OptimisedTokenListEb));
  cpi->OptimisedTokenList = 
    _ogg_malloc(FrameSize*
		sizeof(*cpi->OptimisedTokenList));
  cpi->OptimisedTokenListHi = 
    _ogg_malloc(FrameSize*
		sizeof(*cpi->OptimisedTokenListHi));
  cpi->OptimisedTokenListPl = 
    _ogg_malloc(FrameSize*
		sizeof(*cpi->OptimisedTokenListPl));
}

static void SetupKeyFrame(CP_INSTANCE *cpi) {
  /* Make sure the "last frame" buffer contains the first frame data
     as well. */
  memcpy ( cpi->yuv0ptr, cpi->yuv1ptr, 
	   cpi->pb.ReconYPlaneSize + 2 * cpi->pb.ReconUVPlaneSize );

  /* Initialise the cpi->pb.display_fragments and other fragment
     structures for the first frame. */
  memset( cpi->pb.display_fragments, 1, cpi->pb.UnitFragments );
  memset( cpi->extra_fragments, 1, cpi->pb.UnitFragments );
  
  /* Set up for a BASE/KEY FRAME */
  SetFrameType( &cpi->pb,BASE_FRAME );
}

static void AdjustKeyFrameContext(CP_INSTANCE *cpi) {
  ogg_uint32_t i;
  ogg_uint32_t  AvKeyFrameFrequency = 
    (ogg_uint32_t) (cpi->CurrentFrame / cpi->KeyFrameCount);  
  ogg_uint32_t  AvKeyFrameBytes = 
    (ogg_uint32_t) (cpi->TotKeyFrameBytes / cpi->KeyFrameCount);
  ogg_uint32_t TotalWeight=0;
  ogg_int32_t AvKeyFramesPerSecond;
  ogg_int32_t MinFrameTargetRate;
  
  /* Update the frame carry over. */
  cpi->TotKeyFrameBytes += oggpackB_bytes(&cpi->oggbuffer);
  
  /* reset keyframe context and calculate weighted average of last
     KEY_FRAME_CONTEXT keyframes */
  for( i = 0 ; i < KEY_FRAME_CONTEXT ; i ++ ) {
    if ( i < KEY_FRAME_CONTEXT -1) {
      cpi->PriorKeyFrameSize[i] = cpi->PriorKeyFrameSize[i+1];
      cpi->PriorKeyFrameDistance[i] = cpi->PriorKeyFrameDistance[i+1];
    } else {
      cpi->PriorKeyFrameSize[KEY_FRAME_CONTEXT - 1] = 
	oggpackB_bytes(&cpi->oggbuffer);
      cpi->PriorKeyFrameDistance[KEY_FRAME_CONTEXT - 1] = 
	cpi->LastKeyFrame;
    }
    
    AvKeyFrameBytes += PriorKeyFrameWeight[i] * 
      cpi->PriorKeyFrameSize[i];
    AvKeyFrameFrequency += PriorKeyFrameWeight[i] * 
      cpi->PriorKeyFrameDistance[i];
    TotalWeight += PriorKeyFrameWeight[i];
  }
  AvKeyFrameBytes /= TotalWeight;
  AvKeyFrameFrequency /= TotalWeight;
  AvKeyFramesPerSecond =  100 * cpi->Configuration.OutputFrameRate / 
    AvKeyFrameFrequency ;

  /* Calculate a new target rate per frame allowing for average key
     frame frequency over newest frames . */
  if ( 100 * cpi->Configuration.TargetBandwidth > 
       AvKeyFrameBytes * AvKeyFramesPerSecond &&
       (100 * cpi->Configuration.OutputFrameRate - AvKeyFramesPerSecond )){
    cpi->frame_target_rate =  
      (ogg_int32_t)(100* cpi->Configuration.TargetBandwidth - 
		    AvKeyFrameBytes * AvKeyFramesPerSecond ) / 
      ( (100 * cpi->Configuration.OutputFrameRate - AvKeyFramesPerSecond ) );
  } else {
    /* don't let this number get too small!!! */
    cpi->frame_target_rate = 1;
  }

  /* minimum allowable frame_target_rate */
  MinFrameTargetRate = (cpi->Configuration.TargetBandwidth / 
			cpi->Configuration.OutputFrameRate) / 3;
  
  if(cpi->frame_target_rate < MinFrameTargetRate ) {
    cpi->frame_target_rate = MinFrameTargetRate;
  }

  cpi->LastKeyFrame = 1;
  cpi->LastKeyFrameSize=oggpackB_bytes(&cpi->oggbuffer);

}

void UpdateFrame(CP_INSTANCE *cpi){
  
  double CorrectionFactor;
  
  /* Reset the DC predictors. */
  cpi->pb.LastIntraDC = 0;
  cpi->pb.InvLastIntraDC = 0;
  cpi->pb.LastInterDC = 0;
  cpi->pb.InvLastInterDC = 0;
    
  /* Initialise bit packing mechanism. */
  oggpackB_reset(&cpi->oggbuffer);

  /* mark as video frame */
  oggpackB_write(&cpi->oggbuffer,0,1);
  
  /* Write out the frame header information including size. */
  WriteFrameHeader(cpi);
  
  /* Copy back any extra frags that are to be updated by the codec 
     as part of the background cleanup task */
  CopyBackExtraFrags(cpi);
  
  /* Encode the data.  */
  EncodeData(cpi); 

  /* Adjust drop frame trigger. */
  if ( GetFrameType(&cpi->pb) != BASE_FRAME ) {
    /* Apply decay factor then add in the last frame size. */
    cpi->DropFrameTriggerBytes = 
      ((cpi->DropFrameTriggerBytes * (DF_CANDIDATE_WINDOW-1)) / 
       DF_CANDIDATE_WINDOW) + oggpackB_bytes(&cpi->oggbuffer);
  }else{
    /* Increase cpi->DropFrameTriggerBytes a little. Just after a key
       frame may actually be a good time to drop a frame. */
    cpi->DropFrameTriggerBytes = 
      (cpi->DropFrameTriggerBytes * DF_CANDIDATE_WINDOW) / 
      (DF_CANDIDATE_WINDOW-1);
  }

  /* Test for overshoot which may require a dropped frame next time
     around.  If we are already in a drop frame condition but the
     previous frame was not dropped then the threshold for continuing
     to allow dropped frames is reduced. */
  if ( cpi->DropFrameCandidate ) {
    if ( cpi->DropFrameTriggerBytes > 
	 (cpi->frame_target_rate * (DF_CANDIDATE_WINDOW+1)) )
      cpi->DropFrameCandidate = 1;
    else
      cpi->DropFrameCandidate = 0;
  } else {
    if ( cpi->DropFrameTriggerBytes > 
	 (cpi->frame_target_rate * ((DF_CANDIDATE_WINDOW*2)-2)) )
      cpi->DropFrameCandidate = 1;
    else
      cpi->DropFrameCandidate = 0;
  }
  
  /* Update the BpbCorrectionFactor variable according to whether or
     not we were close enough with our selection of DCT quantiser.  */
  if ( GetFrameType(&cpi->pb) != BASE_FRAME ) {
    /* Work out a size correction factor. */
    CorrectionFactor = (double)oggpackB_bytes(&cpi->oggbuffer) / 
      (double)cpi->ThisFrameTargetBytes;
  
    if ( (CorrectionFactor > 1.05) && 
	 (cpi->pb.ThisFrameQualityValue < 
	  cpi->pb.QThreshTable[cpi->Configuration.ActiveMaxQ]) ) {
      CorrectionFactor = 1.0 + ((CorrectionFactor - 1.0)/2);
      if ( CorrectionFactor > 1.5 )
	cpi->BpbCorrectionFactor *= 1.5;
      else
	cpi->BpbCorrectionFactor *= CorrectionFactor;
      
      /* Keep BpbCorrectionFactor within limits */
      if ( cpi->BpbCorrectionFactor > MAX_BPB_FACTOR )
	cpi->BpbCorrectionFactor = MAX_BPB_FACTOR;
    } else if ( (CorrectionFactor < 0.95) &&
		(cpi->pb.ThisFrameQualityValue > VERY_BEST_Q) ){
      CorrectionFactor = 1.0 - ((1.0 - CorrectionFactor)/2);
      if ( CorrectionFactor < 0.75 )
	cpi->BpbCorrectionFactor *= 0.75;
      else
	cpi->BpbCorrectionFactor *= CorrectionFactor;
      
      /* Keep BpbCorrectionFactor within limits */
      if ( cpi->BpbCorrectionFactor < MIN_BPB_FACTOR )
	cpi->BpbCorrectionFactor = MIN_BPB_FACTOR;
    }
  }

  /* Adjust carry over and or key frame context. */
  if ( GetFrameType(&cpi->pb) == BASE_FRAME ) {
    /* Adjust the key frame context unless the key frame was very small */
    AdjustKeyFrameContext(cpi);
  } else {
    /* Update the frame carry over */
    cpi->CarryOver += ((ogg_int32_t)cpi->frame_target_rate - 
		       (ogg_int32_t)oggpackB_bytes(&cpi->oggbuffer));
  }
  cpi->TotalByteCount += oggpackB_bytes(&cpi->oggbuffer);
}

static void CompressFirstFrame(CP_INSTANCE *cpi) {                  
  ogg_uint32_t i; 

  /* set up context of key frame sizes and distances for more local
     datarate control */
  for( i = 0 ; i < KEY_FRAME_CONTEXT ; i ++ ) {
    cpi->PriorKeyFrameSize[i] = cpi->Configuration.KeyFrameDataTarget;
    cpi->PriorKeyFrameDistance[i] = cpi->pb.info.keyframe_frequency_force;
  }
  
  /* Keep track of the total number of Key Frames Coded. */
  cpi->KeyFrameCount = 1;
  cpi->LastKeyFrame = 1;
  cpi->TotKeyFrameBytes = 0;
  
  /* A key frame is not a dropped frame there for reset the count of
     consequative dropped frames. */
  cpi->DropCount = 0;     
    
  SetupKeyFrame(cpi);

  /* Calculate a new target rate per frame allowing for average key
     frame frequency and size thus far. */
  if ( cpi->Configuration.TargetBandwidth > 
       ((cpi->Configuration.KeyFrameDataTarget * 
	 cpi->Configuration.OutputFrameRate)/
	cpi->pb.info.keyframe_frequency) ) {

    cpi->frame_target_rate =  
      (ogg_int32_t)((cpi->Configuration.TargetBandwidth - 
		     ((cpi->Configuration.KeyFrameDataTarget * 
		       cpi->Configuration.OutputFrameRate)/
		      cpi->pb.info.keyframe_frequency)) / 
		    cpi->Configuration.OutputFrameRate);
  }else 
    cpi->frame_target_rate = 1;

  /* Set baseline frame target rate. */
  cpi->BaseLineFrameTargetRate = cpi->frame_target_rate;

  /* A key frame is not a dropped frame there for reset the count of 
     consequative dropped frames. */
  cpi->DropCount = 0;     
  
  /* Initialise drop frame trigger to 5 frames worth of data. */
  cpi->DropFrameTriggerBytes = cpi->frame_target_rate * DF_CANDIDATE_WINDOW;

  /* Set a target size for this key frame based upon the baseline
     target and frequency */
  cpi->ThisFrameTargetBytes = cpi->Configuration.KeyFrameDataTarget;
  
  /* Get a DCT quantizer level for the key frame. */
  cpi->MotionScore = cpi->pb.UnitFragments;
  
  RegulateQ(cpi, cpi->pb.UnitFragments);
  
  cpi->pb.LastFrameQualityValue = cpi->pb.ThisFrameQualityValue;
  
  /* Initialise quantizer. */
  UpdateQC(cpi, cpi->pb.ThisFrameQualityValue );  
  
  /* Initialise the cpi->pb.display_fragments and other fragment
     structures for the first frame. */
  for ( i = 0; i < cpi->pb.UnitFragments; i ++ ) 
    cpi->FragmentLastQ[i] = cpi->pb.ThisFrameQualityValue;

  /* Compress and output the frist frame. */
  PickIntra( cpi,
	     cpi->pb.YSBRows, cpi->pb.YSBCols);
  UpdateFrame(cpi);  
  
  /* Initialise the carry over rate targeting variables. */
  cpi->CarryOver = 0;
  
}

static void CompressKeyFrame(CP_INSTANCE *cpi){                  
  ogg_uint32_t  i;   
  
  /* Before we compress reset the carry over to the actual frame carry over */
  cpi->CarryOver = cpi->Configuration.TargetBandwidth * cpi->CurrentFrame  / 
    cpi->Configuration.OutputFrameRate - cpi->TotalByteCount;
  
  /* Keep track of the total number of Key Frames Coded */
  cpi->KeyFrameCount += 1;
  
  /* A key frame is not a dropped frame there for reset the count of  
     consequative dropped frames. */
  cpi->DropCount = 0;     
  
  SetupKeyFrame(cpi);
  
  /* set a target size for this frame */
  cpi->ThisFrameTargetBytes = (ogg_int32_t) cpi->frame_target_rate + 
    ( (cpi->Configuration.KeyFrameDataTarget - cpi->frame_target_rate) * 
      cpi->LastKeyFrame / cpi->pb.info.keyframe_frequency_force );
   
  if ( cpi->ThisFrameTargetBytes > cpi->Configuration.KeyFrameDataTarget )
    cpi->ThisFrameTargetBytes = cpi->Configuration.KeyFrameDataTarget;
  
  /* Get a DCT quantizer level for the key frame. */
  cpi->MotionScore = cpi->pb.UnitFragments;
  
  RegulateQ(cpi, cpi->pb.UnitFragments);
  
  cpi->pb.LastFrameQualityValue = cpi->pb.ThisFrameQualityValue;
  
  /* Initialise DCT tables. */
  UpdateQC(cpi, cpi->pb.ThisFrameQualityValue );  
  
  /* Initialise the cpi->pb.display_fragments and other fragment
     structures for the first frame. */
  for ( i = 0; i < cpi->pb.UnitFragments; i ++ )
    cpi->FragmentLastQ[i] = cpi->pb.ThisFrameQualityValue;
  
  
  /* Compress and output the frist frame. */
  PickIntra( cpi,
	     cpi->pb.YSBRows, cpi->pb.YSBCols);
  UpdateFrame(cpi);  
  
}

static void CompressFrame( CP_INSTANCE *cpi) {
  ogg_int32_t min_blocks_per_frame;
  ogg_uint32_t	i; 
  int DropFrame = 0;
  ogg_uint32_t  ResidueBlocksAdded=0;
  ogg_uint32_t  KFIndicator = 0;
  
  double QModStep;
  double QModifier = 1.0;
  
  /* Clear down the macro block level mode and MV arrays. */
  for ( i = 0; i < cpi->pb.UnitFragments; i++ ) {
    cpi->pb.FragCodingMethod[i] = CODE_INTER_NO_MV;  /* Default coding mode */
    cpi->pb.FragMVect[i].x = 0;
    cpi->pb.FragMVect[i].y = 0;
  }
  
  /* Default to normal frames. */
  SetFrameType( &cpi->pb, NORMAL_FRAME );  
  
  /* Clear down the difference arrays for the current frame. */                     
  memset( cpi->pb.display_fragments, 0, cpi->pb.UnitFragments );
  memset( cpi->extra_fragments, 0, cpi->pb.UnitFragments );
  
  /* Calculate the target bytes for this frame. */ 
  cpi->ThisFrameTargetBytes = cpi->frame_target_rate;
  
  /* Correct target to try and compensate for any overall rate error
     that is developing */
  
  /* Set the max allowed Q for this frame based upon carry over
     history.  First set baseline worst Q for this frame */
  cpi->Configuration.ActiveMaxQ = cpi->Configuration.MaxQ + 10;
  if ( cpi->Configuration.ActiveMaxQ >= Q_TABLE_SIZE )
    cpi->Configuration.ActiveMaxQ = Q_TABLE_SIZE - 1;
  
  /* Make a further adjustment based upon the carry over and recent
   history..  cpi->Configuration.ActiveMaxQ reduced by 1 for each 1/2
   seconds worth of -ve carry over up to a limit of 6.  Also
   cpi->Configuration.ActiveMaxQ reduced if frame is a
   "DropFrameCandidate".  Remember that if we are behind the bit
   target carry over is -ve. */
  if ( cpi->CarryOver < 0 ) {
    if ( cpi->DropFrameCandidate ) {
      cpi->Configuration.ActiveMaxQ -= 4;
    }
    
    if ( cpi->CarryOver < 
	 -((ogg_int32_t)cpi->Configuration.TargetBandwidth*3) )
      cpi->Configuration.ActiveMaxQ -= 6;
    else
      cpi->Configuration.ActiveMaxQ += 
	(ogg_int32_t) ((cpi->CarryOver*2) / 
		       (ogg_int32_t)cpi->Configuration.TargetBandwidth);
    
    /* Check that we have not dropped quality too far */
    if ( cpi->Configuration.ActiveMaxQ < cpi->Configuration.MaxQ )
      cpi->Configuration.ActiveMaxQ = cpi->Configuration.MaxQ;
  }

  /* Calculate the Q Modifier step size required to cause a step down
     from full target bandwidth to 40% of target between max Q and
     best Q */
  QModStep = 0.5 / (double)((Q_TABLE_SIZE - 1) - 
			    cpi->Configuration.ActiveMaxQ); 

  /* Set up the cpi->QTargetModifier[] table. */
  for ( i = 0; i < cpi->Configuration.ActiveMaxQ; i++ ) {
    cpi->QTargetModifier[i] = QModifier;
  } 
  for ( i = cpi->Configuration.ActiveMaxQ; i < Q_TABLE_SIZE; i++ ) {
    cpi->QTargetModifier[i] = QModifier;
    QModifier -= QModStep;
  }
  
  /* if we are allowed to drop frames and are falling behind (eg more
     than x frames worth of bandwidth) */
  if ( cpi->pb.info.dropframes_p && 
       ( cpi->DropCount < cpi->MaxConsDroppedFrames) && 
       ( cpi->CarryOver < 
	 -((ogg_int32_t)cpi->Configuration.TargetBandwidth)) &&
       ( cpi->DropFrameCandidate) ) {
    /* (we didn't do this frame so we should have some left over for
       the next frame) */
    cpi->CarryOver += cpi->frame_target_rate;
    DropFrame = 1;
    cpi->DropCount ++;
    
    /* Adjust DropFrameTriggerBytes to account for the saving achieved. */
    cpi->DropFrameTriggerBytes = 
      (cpi->DropFrameTriggerBytes * 
       (DF_CANDIDATE_WINDOW-1))/DF_CANDIDATE_WINDOW;        

    /* Even if we drop a frame we should account for it when
        considering key frame seperation. */
    cpi->LastKeyFrame++;                                    
  } else if ( cpi->CarryOver < 
	      -((ogg_int32_t)cpi->Configuration.TargetBandwidth * 2) ) {
    /* Reduce frame bit target by 1.75% for each 1/10th of a seconds
       worth of -ve carry over down to a minimum of 65% of its
       un-modified value. */

    cpi->ThisFrameTargetBytes = 
      (ogg_uint32_t)(cpi->ThisFrameTargetBytes * 0.65); 
  } else if ( cpi->CarryOver < 0 ) {
    /* Note that cpi->CarryOver is a -ve here hence 1.0 "+" ... */
    cpi->ThisFrameTargetBytes = 
      (ogg_uint32_t)(cpi->ThisFrameTargetBytes * 
		     (1.0 + ( ((cpi->CarryOver * 10)/
			       ((ogg_int32_t)cpi->
				Configuration.TargetBandwidth)) * 0.0175) ));
  }

  if ( !DropFrame ) {
    /*  pick all the macroblock modes and motion vectors */
    ogg_uint32_t InterError;
    ogg_uint32_t IntraError;
    
    
    /* Set Baseline filter level. */
    ConfigurePP( &cpi->pp, cpi->pb.info.noise_sensitivity);
    
    /* Score / analyses the fragments. */ 
    cpi->MotionScore = YUVAnalyseFrame(&cpi->pp, &KFIndicator );
    
    /* Get the baseline Q value */
    RegulateQ( cpi, cpi->MotionScore );
    
    /* Recode blocks if the error score in last frame was high. */
    ResidueBlocksAdded  = 0;
    for ( i = 0; i < cpi->pb.UnitFragments; i++ ){
      if ( !cpi->pb.display_fragments[i] ){
	if ( cpi->LastCodedErrorScore[i] >= 
	     ResidueErrorThresh[cpi->pb.FrameQIndex] ) {
	  cpi->pb.display_fragments[i] = 1; /* Force block update */
	  cpi->extra_fragments[i] = 1;      /* Insures up to date
                                               pixel data is used. */
	  ResidueBlocksAdded ++;
	}
      }
    }
    
    /* Adjust the motion score to allow for residue blocks
       added. These are assumed to have below average impact on
       bitrate (Hence ResidueBlockFactor). */
    cpi->MotionScore = cpi->MotionScore + 
      (ResidueBlocksAdded / ResidueBlockFactor[cpi->pb.FrameQIndex]);
    
    /* Estimate the min number of blocks at best Q */
    min_blocks_per_frame = 
      (ogg_int32_t)(cpi->ThisFrameTargetBytes / 
		    GetEstimatedBpb( cpi, VERY_BEST_Q ));
    if ( min_blocks_per_frame == 0 )
      min_blocks_per_frame = 1;
    
    /* If we have less than this number then consider adding in some
       extra blocks */
    if ( cpi->MotionScore < min_blocks_per_frame ) {
      min_blocks_per_frame = 
	cpi->MotionScore + 
	(ogg_int32_t)(((min_blocks_per_frame - cpi->MotionScore) * 4) / 3 );
      UpRegulateDataStream( cpi, VERY_BEST_Q, min_blocks_per_frame );
    }else{
      /* Reset control variable for best quality final pass. */
      cpi->FinalPassLastPos = 0;
    }

    /* Get the modified Q prediction taking into account extra blocks added. */
    RegulateQ( cpi, cpi->MotionScore );
    
    /* Unless we are already well ahead (4 seconds of data) of the
       projected bitrate */
    if ( cpi->CarryOver < 
	 (ogg_int32_t)(cpi->Configuration.TargetBandwidth * 4) ){
      /* Look at the predicted Q (pbi->FrameQIndex).  Adjust the
	 target bits for this frame based upon projected Q and
	 re-calculate.  The idea is that if the Q is better than a
	 given (good enough) level then we will try and save some bits
	 for use in more difficult segments. */
      cpi->ThisFrameTargetBytes = 
	(ogg_int32_t) (cpi->ThisFrameTargetBytes * 
		       cpi->QTargetModifier[cpi->pb.FrameQIndex]);

      /* Recalculate Q again */
      RegulateQ( cpi, cpi->MotionScore );
    }


    /* Select modes and motion vectors for each of the blocks : return
       an error score for inter and intra */
    PickModes( cpi, cpi->pb.YSBRows, cpi->pb.YSBCols, 
	       cpi->pb.info.width, 
	       &InterError, &IntraError );

    /* decide whether we really should have made this frame a key frame */

    if( cpi->pb.info.keyframe_auto_p){
      if( ( ( 2* IntraError < 5 * InterError ) 
	    && ( KFIndicator >= (ogg_uint32_t) 
		 cpi->pb.info.keyframe_auto_threshold)
	    && ( cpi->LastKeyFrame > cpi->pb.info.keyframe_mindistance)
	    ) ||
	  (cpi->LastKeyFrame >= (ogg_uint32_t)
	   cpi->pb.info.keyframe_frequency_force) ){
	
	CompressKeyFrame(cpi);  /* Code a key frame */
	return;
      }
      
    }
    
    /* Increment the frames since last key frame count */
    cpi->LastKeyFrame++;
    
    if ( cpi->MotionScore > 0 ){
      cpi->DropCount = 0;
      
      /* Proceed with the frame update. */
      UpdateFrame(cpi);  
      
      /* Note the Quantizer used for each block coded. */
      for ( i = 0; i < cpi->pb.UnitFragments; i++ ){
	if ( cpi->pb.display_fragments[i] ){
	  cpi->FragmentLastQ[i] = cpi->pb.ThisFrameQualityValue;
	}
      }
      
    }
  }else{
    /* even if we 'drop' a frame, a placeholder must be written as we
       currently assume fixed frame rate timebase as Ogg mapping
       invariant */
    UpdateFrame(cpi);
  }
}
 
static int _ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

 
/********************** The toplevel: encode ***********************/

const char *theora_version_string(void){
  return VENDOR_STRING;
}

ogg_uint32_t theora_version_number(void){
  return (VERSION_MAJOR<<16) + (VERSION_MINOR<<8) + (VERSION_SUB);
}

int theora_encode_init(theora_state *th, theora_info *c){
  int i;
  
  CP_INSTANCE *cpi;

  memset(th, 0, sizeof(*th));
  th->internal_encode=cpi=_ogg_calloc(1,sizeof(*cpi));
  
  c->version_major=VERSION_MAJOR;
  c->version_minor=VERSION_MINOR;
  c->version_subminor=VERSION_SUB;

  InitTmpBuffers(&cpi->pb);
  InitPPInstance(&cpi->pp);

  /* Initialise Configuration structure to legal values */
  if(c->quality>63)c->quality=63;
  if(c->quality<0)c->quality=32;
  cpi->Configuration.BaseQ = c->quality;
  cpi->Configuration.FirstFrameQ = c->quality;
  cpi->Configuration.MaxQ = c->quality;
  cpi->Configuration.ActiveMaxQ = c->quality;
  
  cpi->MVChangeFactor    =    14;     
  cpi->FourMvChangeFactor =   8;           
  cpi->MinImprovementForNewMV = 25;   
  cpi->ExhaustiveSearchThresh = 2500;
  cpi->MinImprovementForFourMV = 100;   
  cpi->FourMVThreshold = 10000;
  cpi->BitRateCapFactor = 1.50;    
  cpi->InterTripOutThresh = 5000;
  cpi->MVEnabled = 1;
  cpi->InterCodeCount = 127;
  cpi->BpbCorrectionFactor = 1.0;
  cpi->GoldenFrameEnabled = 1;
  cpi->InterPrediction = 1;
  cpi->MotionCompensation = 1;
  cpi->ThreshMapThreshold = 5;
  cpi->MaxConsDroppedFrames = 1;

  /* Set encoder flags. */
  /* if not AutoKeyframing cpi->ForceKeyFrameEvery = is frequency */
  if(!c->keyframe_auto_p)
    c->keyframe_frequency_force = c->keyframe_frequency;

  /* Set the frame rate variables. */
  if ( c->fps_numerator < 1 )
    c->fps_numerator = 1;
  if ( c->fps_denominator < 1 )
    c->fps_denominator = 1;

  /* don't go too nuts on keyframe spacing; impose a high limit to
     make certain the granulepos encoding strategy works */
  if(c->keyframe_frequency_force>32768)c->keyframe_frequency_force=32768;
  if(c->keyframe_mindistance>32768)c->keyframe_mindistance=32768;
  if(c->keyframe_mindistance>c->keyframe_frequency_force)
    c->keyframe_mindistance=c->keyframe_frequency_force;
  cpi->pb.keyframe_granule_shift=_ilog(c->keyframe_frequency_force-1);

  /* copy in config */
  memcpy(&cpi->pb.info,c,sizeof(*c));
  th->i=&cpi->pb.info;
  th->granulepos=-1;

  /* Set up default values for QTargetModifier[Q_TABLE_SIZE] table */
  for ( i = 0; i < Q_TABLE_SIZE; i++ ) 
    cpi->QTargetModifier[i] = 1.0;
 
  /* Set up an encode buffer */
  oggpackB_writeinit(&cpi->oggbuffer);  

  /* Set data rate related variables. */
  cpi->Configuration.TargetBandwidth = (c->target_bitrate) / 8;
  
  cpi->Configuration.OutputFrameRate =
    (double)( c->fps_numerator /
	      c->fps_denominator );

  cpi->frame_target_rate = cpi->Configuration.TargetBandwidth /
    cpi->Configuration.OutputFrameRate; 
  
  /* Set key frame data rate target; this is nominal keyframe size */
  cpi->Configuration.KeyFrameDataTarget = (c->keyframe_data_target_bitrate * 
					   c->fps_numerator /
					   c->fps_denominator ) / 8;

  /* Note the height and width in the pre-processor control structure. */
  cpi->ScanConfig.VideoFrameHeight = cpi->pb.info.height;
  cpi->ScanConfig.VideoFrameWidth = cpi->pb.info.width;

  InitFrameDetails(&cpi->pb);
  EInitFragmentInfo(cpi);
  EInitFrameInfo(cpi);

  /* Set up pre-processor config pointers. */
  cpi->ScanConfig.Yuv0ptr = cpi->yuv0ptr;
  cpi->ScanConfig.Yuv1ptr = cpi->yuv1ptr;
  cpi->ScanConfig.SrfWorkSpcPtr = cpi->ConvDestBuffer;
  cpi->ScanConfig.disp_fragments = cpi->pb.display_fragments;
  cpi->ScanConfig.RegionIndex = cpi->pb.pixel_index_table;

  /* Initialise the pre-processor module. */
  ScanYUVInit(&cpi->pp, &(cpi->ScanConfig));

  /* Initialise Motion compensation */
  InitMotionCompensation(cpi);
  
  /* Initialise the compression process. */
  /* We always start at frame 1 */                 
  cpi->CurrentFrame = 1;   
  
  /* Reset the rate targeting correction factor. */
  cpi->BpbCorrectionFactor = 1.0;
  
  cpi->TotalByteCount = 0;
  cpi->TotalMotionScore = 0;
  
  /* Up regulation variables. */
  cpi->FinalPassLastPos = 0;  /* Used to regulate a final unrestricted pass. */
  cpi->LastEndSB = 0;	      /* Where we were in the loop last time.  */
  cpi->ResidueLastEndSB = 0;  /* Where we were in the residue update
                                 loop last time. */
  
  InitHuffmanSet(&cpi->pb);

  /* This makes sure encoder version specific tables are initialised */
  InitQTables(&cpi->pb);  
  
  /* Indicate that the next frame to be compressed is the first in the
     current clip. */
  cpi->ThisIsFirstFrame = 1;
  cpi->readyflag = 1;
  
  return 0;
}

int theora_encode_YUVin(theora_state *t, 
			 yuv_buffer *yuv){
  ogg_int32_t i;
  unsigned char *LocalDataPtr;
  unsigned char *InputDataPtr;
  CP_INSTANCE *cpi=(CP_INSTANCE *)(t->internal_encode);

  if(!cpi->readyflag)return OC_EINVAL;
  if(cpi->doneflag)return OC_EINVAL;

  /* If frame size has changed, abort out for now */
  if (yuv->y_height != (int)cpi->pb.info.height ||
      yuv->y_width != (int)cpi->pb.info.width )
    return(-1);


  /* Copy over input YUV to internal YUV buffers. */  
  /* First copy over the Y data */
  LocalDataPtr = cpi->yuv1ptr;
  InputDataPtr = yuv->y;
  for ( i = 0; i < yuv->y_height; i++ ){
    memcpy( LocalDataPtr, InputDataPtr, yuv->y_width );
    LocalDataPtr += yuv->y_width;
    InputDataPtr += yuv->y_stride;
  }

  /* Now copy over the U data */
  LocalDataPtr = &cpi->yuv1ptr[(yuv->y_height * yuv->y_width)];
  InputDataPtr = yuv->u;
  for ( i = 0; i < yuv->uv_height; i++ ){
    memcpy( LocalDataPtr, InputDataPtr, yuv->uv_width );
    LocalDataPtr += yuv->uv_width;
    InputDataPtr += yuv->uv_stride;
  }
  
  /* Now copy over the V data */
  LocalDataPtr = 
    &cpi->yuv1ptr[((yuv->y_height*yuv->y_width)*5)/4];
  InputDataPtr = yuv->v;
  for ( i = 0; i < yuv->uv_height; i++ ){
    memcpy( LocalDataPtr, InputDataPtr, yuv->uv_width );
    LocalDataPtr += yuv->uv_width;
    InputDataPtr += yuv->uv_stride;
  }

  /* Special case for first frame */
  if ( cpi->ThisIsFirstFrame ){
    CompressFirstFrame(cpi);
    cpi->ThisIsFirstFrame = 0;
    cpi->ThisIsKeyFrame = 0;
  } else if ( cpi->ThisIsKeyFrame ) {
    CompressKeyFrame(cpi);
    cpi->ThisIsKeyFrame = 0;
  } else  {
    /* Compress the frame. */
    CompressFrame( cpi );
  }

  /* Update stats variables. */
  cpi->LastFrameSize = oggpackB_bytes(&cpi->oggbuffer);
  cpi->CurrentFrame++;
  cpi->packetflag=1;

  t->granulepos=
    ((cpi->CurrentFrame-cpi->LastKeyFrame-1)<<cpi->pb.keyframe_granule_shift)+ 
    cpi->LastKeyFrame-1;
  
  return 0;
}

int theora_encode_packetout( theora_state *t, int last_p, ogg_packet *op){
  CP_INSTANCE *cpi=(CP_INSTANCE *)(t->internal_encode);
  long bytes=oggpackB_bytes(&cpi->oggbuffer);
  
  if(!bytes)return(0);
  if(!cpi->packetflag)return(0);
  if(cpi->doneflag)return(-1);

  op->packet=oggpackB_get_buffer(&cpi->oggbuffer);
  op->bytes=bytes;
  op->b_o_s=0;
  op->e_o_s=last_p;
  
  op->packetno=cpi->CurrentFrame;
  op->granulepos=t->granulepos;

  cpi->packetflag=0;
  if(last_p)cpi->doneflag=1;

  return 1;
}

static void _tp_readbuffer(oggpack_buffer *opb, char *buf, const long len)
{
  long i;
  
  for (i = 0; i < len; i++)
    *buf++=oggpack_read(opb,8);
}

static void _tp_writebuffer(oggpack_buffer *opb, const char *buf, const long len)
{
  long i;
  
  for (i = 0; i < len; i++)
    oggpack_write(opb, *buf++, 8);
}
 
/* build the initial short header for stream recognition and format */
int theora_encode_header(theora_state *t, ogg_packet *op){
  CP_INSTANCE *cpi=(CP_INSTANCE *)(t->internal_encode);

  oggpackB_reset(&cpi->oggbuffer);
  oggpackB_write(&cpi->oggbuffer,0x80,8);
  _tp_writebuffer(&cpi->oggbuffer, "theora", 6);
  
  oggpackB_write(&cpi->oggbuffer,VERSION_MAJOR,8);
  oggpackB_write(&cpi->oggbuffer,VERSION_MINOR,8);
  oggpackB_write(&cpi->oggbuffer,VERSION_SUB,8);

  oggpackB_write(&cpi->oggbuffer,cpi->pb.info.width>>4,16);
  oggpackB_write(&cpi->oggbuffer,cpi->pb.info.height>>4,16);
  oggpackB_write(&cpi->oggbuffer,cpi->pb.info.fps_numerator,32);
  oggpackB_write(&cpi->oggbuffer,cpi->pb.info.fps_denominator,32);
  oggpackB_write(&cpi->oggbuffer,cpi->pb.info.aspect_numerator,24);
  oggpackB_write(&cpi->oggbuffer,cpi->pb.info.aspect_denominator,24);

  oggpackB_write(&cpi->oggbuffer,cpi->pb.keyframe_granule_shift,5);

  oggpackB_write(&cpi->oggbuffer,cpi->pb.info.target_bitrate,24);
  oggpackB_write(&cpi->oggbuffer,cpi->pb.info.quality,6);

  op->packet=oggpackB_get_buffer(&cpi->oggbuffer);
  op->bytes=oggpackB_bytes(&cpi->oggbuffer);

  op->b_o_s=1;
  op->e_o_s=0;
  
  op->packetno=0;
  
  op->granulepos=0;
  cpi->packetflag=0;

  return(0);
}

/* build the comment header packet from the passed metadata */
int theora_encode_comment(theora_comment *tc, ogg_packet *op)
{
  const char *vendor = theora_version_string();
  const int vendor_length = strlen(vendor);
  oggpack_buffer opb;
  
  oggpack_writeinit(&opb);
  oggpack_write(&opb, 0x81, 8);
  _tp_writebuffer(&opb, "theora", 6);
  
  oggpack_write(&opb, vendor_length, 32);
  _tp_writebuffer(&opb, vendor, vendor_length);
  
  oggpack_write(&opb, tc->comments, 32);
  if(tc->comments){
    int i;
    for(i=0;i<tc->comments;i++){
      if(tc->user_comments[i]){
        oggpack_write(&opb,tc->comment_lengths[i],32);
        _tp_writebuffer(&opb,tc->user_comments[i],tc->comment_lengths[i]);
      }else{
        oggpack_write(&opb,0,32);
      }
    }
  }
  {
    int bytes=oggpack_bytes(&opb);
    op->packet=malloc(bytes);
    memcpy(op->packet, oggpack_get_buffer(&opb), bytes);
    op->bytes=bytes;
  }
  oggpack_writeclear(&opb);

  op->b_o_s=0;
  op->e_o_s=0;
  
  op->packetno=0;
  op->granulepos=0;
  
  return (0);
}

/* build the final header packet with the tables required
   for decode */
int theora_encode_tables(theora_state *t, ogg_packet *op){
  CP_INSTANCE *cpi=(CP_INSTANCE *)(t->internal_encode);

  oggpackB_reset(&cpi->oggbuffer);
  oggpackB_write(&cpi->oggbuffer,0x82,8);
  _tp_writebuffer(&cpi->oggbuffer,"theora",6);
  
  write_Qtables(&cpi->oggbuffer);
  write_FrequencyCounts(&cpi->oggbuffer);

  op->packet=oggpackB_get_buffer(&cpi->oggbuffer);
  op->bytes=oggpackB_bytes(&cpi->oggbuffer);

  op->b_o_s=0;
  op->e_o_s=0;
  
  op->packetno=0;
  
  op->granulepos=0;
  cpi->packetflag=0;

  return(0);
}

void theora_clear(theora_state *t){
  if(t){
    CP_INSTANCE *cpi=(CP_INSTANCE *)(t->internal_encode);
    PB_INSTANCE *pbi=(PB_INSTANCE *)(t->internal_decode);
    
    if(cpi){
      
      ClearHuffmanSet(&cpi->pb);
      ClearFragmentInfo(&cpi->pb);
      ClearFrameInfo(&cpi->pb);
      EClearFragmentInfo(cpi);
      EClearFrameInfo(cpi);		
      ClearTmpBuffers(&cpi->pb);
      ClearPPInstance(&cpi->pp);

	  _ogg_free(t->internal_encode);
      
    }

    if(pbi){

      ClearHuffmanSet(pbi);
      ClearFragmentInfo(pbi);
      ClearFrameInfo(pbi);
      ClearPBInstance(pbi);

      _ogg_free(t->internal_decode);
    }

    t->internal_encode=NULL;
    t->internal_decode=NULL;
  }
}

int theora_decode_header(theora_info *c, ogg_packet *op){
  int ret;
  oggpack_buffer opb;
  oggpackB_readinit(&opb,op->packet,op->bytes);
  
  if(!op->b_o_s)return(OC_BADHEADER);
  {
    char id[6];
    int i;
    int typeflag=oggpackB_read(&opb,8);

    if(typeflag!=0x80)return(OC_NOTFORMAT);
    
    for(i=0;i<6;i++)
      id[i]=(char)oggpackB_read(&opb,8);
    
    if(memcmp(id,"theora",6))return(OC_NOTFORMAT);

    memset(c,0,sizeof(*c));

    c->version_major=oggpackB_read(&opb,8);
    c->version_minor=oggpackB_read(&opb,8);
    c->version_subminor=oggpackB_read(&opb,8);

    if(c->version_major!=VERSION_MAJOR)return(OC_VERSION);
    if(c->version_minor>VERSION_MINOR)return(OC_VERSION);
  }

  c->width=oggpackB_read(&opb,16)<<4;
  c->height=oggpackB_read(&opb,16)<<4;
  c->fps_numerator=oggpackB_read(&opb,32);
  c->fps_denominator=oggpackB_read(&opb,32);
  c->aspect_numerator=oggpackB_read(&opb,24);
  c->aspect_denominator=oggpackB_read(&opb,24);

  c->keyframe_frequency_force=1<<oggpackB_read(&opb,5);

  c->target_bitrate=oggpackB_read(&opb,24);
  c->quality=ret=oggpackB_read(&opb,6);

  if(ret==-1)return(OC_BADHEADER);

  return(0);
}

int theora_decode_comment(theora_comment *tc, ogg_packet *op){
  oggpack_buffer opb;
  oggpack_readinit(&opb,op->packet,op->bytes);

  {
    char id[6];
    int typeflag=oggpack_read(&opb,8);

    if(typeflag!=0x81)return(OC_NOTFORMAT);
    
    _tp_readbuffer(&opb, id, 6);
    if(memcmp(id,"theora",6))return(OC_NOTFORMAT);
  }
  
  {
    int i;
    int len = oggpack_read(&opb,32);
    if(len<0)return(OC_BADHEADER);
    tc->vendor=_ogg_calloc(1,len+1);
    _tp_readbuffer(&opb,tc->vendor, len);
    tc->vendor[len]='\0';
  
    tc->comments=oggpack_read(&opb,32);
    if(tc->comments<0)goto parse_err;
    tc->user_comments=_ogg_calloc(tc->comments,sizeof(*tc->user_comments));
    tc->comment_lengths=_ogg_calloc(tc->comments,sizeof(*tc->comment_lengths));
    for(i=0;i<tc->comments;i++){
      len=oggpack_read(&opb,32);
      if(len<0)goto parse_err;
      if(len){
        tc->user_comments[i]=_ogg_calloc(1,len+1);
        _tp_readbuffer(&opb,tc->user_comments[i],len);
        tc->user_comments[i][len]='\0';
        tc->comment_lengths[i]=len;
      }
    }
  }
  return(0);

parse_err:
  theora_comment_clear(tc);
  return(OC_BADHEADER);
}

int theora_decode_tables(theora_info *c, ogg_packet *op){
  oggpack_buffer opb;
  oggpackB_readinit(&opb,op->packet,op->bytes);

  {  
    char id[6];
    int i;
    int typeflag=oggpackB_read(&opb,8);

    if(typeflag!=0x82)return(OC_NOTFORMAT);
    
    for(i=0;i<6;i++)
      id[i]=(char)oggpackB_read(&opb,8);
    
    if(memcmp(id,"theora",6))return(OC_NOTFORMAT);
  }
  
  /* todo: check for error */
  read_Qtables(&opb);
  read_FrequencyCounts(&opb);

  return(0);
}

int theora_decode_init(theora_state *th, theora_info *c){
  PB_INSTANCE *pbi;
  
  th->internal_decode=pbi=_ogg_calloc(1,sizeof(*pbi));
  
  InitPBInstance(pbi);
  memcpy(&pbi->info,c,sizeof(*c));
  th->i=&pbi->info;
  th->granulepos=-1;
        
  InitFrameDetails(pbi);

  pbi->keyframe_granule_shift=_ilog(c->keyframe_frequency_force-1);

  pbi->LastFrameQualityValue = 0;
  pbi->DecoderErrorCode = 0;

  /* Clear down the YUVtoRGB conversion skipped list. */
  memset(pbi->skipped_display_fragments, 0, pbi->UnitFragments );

  /* Initialise version specific quantiser values */
  InitQTables( pbi );

  /* Huffman setup */
  InitHuffmanSet( pbi );
    

  return(0);

}

int theora_decode_packetin(theora_state *th,ogg_packet *op){
  int ret;
  PB_INSTANCE *pbi=(PB_INSTANCE *)(th->internal_decode);
  
  pbi->DecoderErrorCode = 0;
  oggpackB_readinit(&pbi->opb,op->packet,op->bytes);

  /* verify that this is a video frame */
  if(oggpackB_read(&pbi->opb,1)==0){
    ret=LoadAndDecode(pbi);

    if(ret)return ret;
    
    if(pbi->PostProcessingLevel)
      PostProcess(pbi);

    if(op->granulepos>-1)
      th->granulepos=op->granulepos;
    else{
      if(th->granulepos==-1){
	th->granulepos=0;
      }else{
	if(pbi->FrameType==BASE_FRAME){
	  long frames= th->granulepos & ((1<<pbi->keyframe_granule_shift)-1);
	  th->granulepos>>=pbi->keyframe_granule_shift;
	  th->granulepos+=frames+1;
	  th->granulepos<<=pbi->keyframe_granule_shift;
	}else
	  th->granulepos++;
      }
    }
           
    return(0);
  }

  return OC_BADPACKET;
}

int theora_decode_YUVout(theora_state *th,yuv_buffer *yuv){
  PB_INSTANCE *pbi=(PB_INSTANCE *)(th->internal_decode);

  yuv->y_width = pbi->info.width;
  yuv->y_height = pbi->info.height;
  yuv->y_stride = pbi->YStride;
  
  yuv->uv_width = pbi->info.width / 2;
  yuv->uv_height = pbi->info.height / 2;
  yuv->uv_stride = pbi->UVStride;
  
  if(pbi->PostProcessingLevel){
    yuv->y = &pbi->PostProcessBuffer[pbi->ReconYDataOffset];
    yuv->u = &pbi->PostProcessBuffer[pbi->ReconUDataOffset];
    yuv->v = &pbi->PostProcessBuffer[pbi->ReconVDataOffset];
  }else{
    yuv->y = &pbi->LastFrameRecon[pbi->ReconYDataOffset];
    yuv->u = &pbi->LastFrameRecon[pbi->ReconUDataOffset];
    yuv->v = &pbi->LastFrameRecon[pbi->ReconVDataOffset];
  }

  return 0;
}

/* returns, in seconds, absolute time of current packet in given
   logical stream */
double theora_granule_time(theora_state *th,ogg_int64_t granulepos){
  CP_INSTANCE *cpi=(CP_INSTANCE *)(th->internal_encode);
  PB_INSTANCE *pbi=(PB_INSTANCE *)(th->internal_decode);
  
  if(cpi)pbi=&cpi->pb;

  if(granulepos>=0){
    ogg_int64_t iframe=granulepos>>pbi->keyframe_granule_shift;
    ogg_int64_t pframe=granulepos-(iframe<<pbi->keyframe_granule_shift);

    return (iframe+pframe)*
      ((double)pbi->info.fps_denominator/pbi->info.fps_numerator);

  }

  return(-1);
}
