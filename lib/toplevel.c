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
  last mod: $Id: toplevel.c,v 1.5 2002/09/20 22:01:43 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <ogg/ogg.h>
#include <theora/theora.h>
#include "encoder_internal.h"
#include "toplevel_lookup.h"

#define A_TABLE_SIZE	    29
#define DF_CANDIDATE_WINDOW 5
#define VERSION_MAJOR 3
#define VERSION_MINOR 1
#define VERSION_SUB 0

#define CommentString "Xiph.Org libTheora I 20020916 0 0 0"

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
  
  // Set up for a BASE/KEY FRAME 
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

  /* if not AutoKeyframing cpi->ForceKeyFrameEvery = is frequency */
  if(!cpi->AutoKeyFrameEnabled) 
    cpi->ForceKeyFrameEvery = cpi->KeyFrameFrequency;

  /* set up context of key frame sizes and distances for more local
     datarate control */
  for( i = 0 ; i < KEY_FRAME_CONTEXT ; i ++ ) {
    cpi->PriorKeyFrameSize[i] = cpi->KeyFrameDataTarget;
    cpi->PriorKeyFrameDistance[i] = cpi->ForceKeyFrameEvery;
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
       ((cpi->KeyFrameDataTarget * cpi->Configuration.OutputFrameRate)/
	cpi->KeyFrameFrequency) ) {

    cpi->frame_target_rate =  
      (ogg_int32_t)((cpi->Configuration.TargetBandwidth - 
		     ((cpi->KeyFrameDataTarget * 
		       cpi->Configuration.OutputFrameRate)/
		      cpi->KeyFrameFrequency)) / 
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
  cpi->ThisFrameTargetBytes = cpi->KeyFrameDataTarget;
  
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
    ( (cpi->KeyFrameDataTarget - cpi->frame_target_rate) * 
      cpi->LastKeyFrame / cpi->ForceKeyFrameEvery );
  
  if ( cpi->ThisFrameTargetBytes > cpi->KeyFrameDataTarget )
    cpi->ThisFrameTargetBytes = cpi->KeyFrameDataTarget;
  
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
  if ( cpi->DropFramesAllowed && 
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
    ConfigurePP( &cpi->pp, cpi->PreProcFilterLevel );
    
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
	       cpi->pb.Configuration.VideoFrameWidth, 
	       &InterError, &IntraError );

    /* decide whether we really should have made this frame a key frame */

    if( cpi->AutoKeyFrameEnabled ) {
      if( ( ( 2* IntraError < 5 * InterError ) 
	    && ( KFIndicator >= (ogg_uint32_t) cpi->AutoKeyFrameThreshold) 
	    && ( cpi->LastKeyFrame > cpi->MinimumDistanceToKeyFrame) 
	    ) ||
	  (cpi->LastKeyFrame >= (ogg_uint32_t)cpi->ForceKeyFrameEvery) ) {
	
	CompressKeyFrame(cpi);  // Code a key frame
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
    if (cpi->NoDrops == 1){
      UpdateFrame(cpi);
    }
  }
}
  
/********************** The toplevel ***********************/

const char *theora_encode_version(void){
  return CommentString;
}

static int _ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

int theora_encode_init(theora_state *th, theora_info *c){
  int i;
  
  CP_INSTANCE *cpi;

  memset(th, 0, sizeof(*th));
  th->internal=cpi=_ogg_calloc(1,sizeof(*cpi));
  
  c->version_major=VERSION_MAJOR;
  c->version_minor=VERSION_MINOR;
  c->version_subminor=VERSION_SUB;

  InitTmpBuffers(&cpi->pb);
  InitPPInstance(&cpi->pp);

  /* Initialise Configuration structure to legal values */
  cpi->Configuration.BaseQ = 32;
  cpi->Configuration.FirstFrameQ = 32;
  cpi->Configuration.MaxQ = 32;
  cpi->Configuration.ActiveMaxQ = 32;
  
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
  cpi->QuickCompress = 1;
  cpi->MaxConsDroppedFrames = 1;
  cpi->Sharpness = 2;
  
  cpi->PreProcFilterLevel = 2;
  
  /* Set up default values for QTargetModifier[Q_TABLE_SIZE] table */
  for ( i = 0; i < Q_TABLE_SIZE; i++ ) 
    cpi->QTargetModifier[Q_TABLE_SIZE] = 1.0;
 
  /* Set up an encode buffer */
  oggpackB_writeinit(&cpi->oggbuffer);
  
  cpi->pb.Configuration.HFragPixels = 8;
  cpi->pb.Configuration.VFragPixels = 8;
  
  /* set the version number */
  cpi->pb.Vp3VersionNo = CURRENT_ENCODE_VERSION;
  
  /* Set the video frame size. */
  cpi->pb.YPlaneSize = 0xFFF;
  cpi->pb.Configuration.VideoFrameHeight = 0xFFF;
  cpi->pb.Configuration.VideoFrameHeight = c->height;
  cpi->pb.Configuration.VideoFrameWidth = c->width;
  
  /* Note the height and width in the pre-processor control structure. */
  cpi->ScanConfig.VideoFrameHeight = cpi->pb.Configuration.VideoFrameHeight;
  cpi->ScanConfig.VideoFrameWidth = cpi->pb.Configuration.VideoFrameWidth;

  /* Set data rate related variables. */
  cpi->Configuration.TargetBandwidth = (c->target_bitrate * 1000) / 8;
  
  /* Set the target minimum key frame frequency */
  cpi->KeyFrameFrequency = c->keyframe_frequency;
  
  /* Set key frame data rate target */
  cpi->KeyFrameDataTarget = (c->keyframe_data_target_bitrate * 1000) / 8;
  
  /* Set the quality settings. */
  ConfigureQuality( cpi, c->quality );

  /* Set the frame rate variables. */
  cpi->Configuration.OutputFrameRateN = c->fps_numerator;
  cpi->Configuration.OutputFrameRateD = c->fps_denominator;
  if ( cpi->Configuration.OutputFrameRateN < 1 )
    cpi->Configuration.OutputFrameRateN = 1;
  if ( cpi->Configuration.OutputFrameRateD < 1 )
    cpi->Configuration.OutputFrameRateD = 1;

  cpi->Configuration.OutputFrameRate =
    (double)( cpi->Configuration.OutputFrameRateN /
	      cpi->Configuration.OutputFrameRateD );

  cpi->frame_target_rate = cpi->Configuration.TargetBandwidth /
    cpi->Configuration.OutputFrameRate; 
  
  /* Initialise image format details */
  InitFrameDetails(&cpi->pb);
  EInitFragmentInfo(cpi);
  EInitFrameInfo(cpi);

  /* Set up pre-processor config pointers. */
  cpi->ScanConfig.Yuv0ptr = cpi->yuv0ptr;
  cpi->ScanConfig.Yuv1ptr = cpi->yuv1ptr;
  cpi->ScanConfig.SrfWorkSpcPtr = cpi->ConvDestBuffer;
  cpi->ScanConfig.disp_fragments = cpi->pb.display_fragments;
  cpi->ScanConfig.RegionIndex = cpi->pb.pixel_index_table;
  cpi->ScanConfig.HFragPixels = 
    (unsigned char)cpi->pb.Configuration.HFragPixels;
  cpi->ScanConfig.VFragPixels = 
    (unsigned char)cpi->pb.Configuration.VFragPixels;

  /* Initialise the pre-processor module. */
  ScanYUVInit(&cpi->pp, &(cpi->ScanConfig));

  /* Set encoder flags. */
  cpi->DropFramesAllowed = c->droppedframes_p;
  cpi->QuickCompress = c->quickcompress_p;
  cpi->AutoKeyFrameEnabled = 1;
  cpi->MinimumDistanceToKeyFrame = c->keyframe_mindistance;
  cpi->ForceKeyFrameEvery = c->keyframe_force_freq;
  cpi->PreProcFilterLevel = c->noise_sensitivity;
  cpi->AutoKeyFrameThreshold = c->keyframe_auto_threshold;
  cpi->Sharpness = c->sharpness;

  /* don't go too nuts on keyframe spacing; impose a high limit to
     make certain the granulepos encoding strategy works */
  if(cpi->ForceKeyFrameEvery>32768)cpi->ForceKeyFrameEvery=32768;
  if(cpi->MinimumDistanceToKeyFrame>32768)cpi->MinimumDistanceToKeyFrame=32768;

  {
    long maxPframes=(int)cpi->ForceKeyFrameEvery > 
      (int)cpi->MinimumDistanceToKeyFrame ?
      cpi->ForceKeyFrameEvery : cpi->MinimumDistanceToKeyFrame ;
    cpi->keyframe_granule_shift=_ilog(maxPframes-1);
  }  

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

int theora_encode_YUVin( CP_INSTANCE *cpi, 
			 YUV_INPUT_BUFFER_CONFIG *YuvInputData){
  ogg_int32_t i;
  unsigned char *LocalDataPtr;
  unsigned char *InputDataPtr;

  if(!cpi->readyflag)return -1;
  if(cpi->doneflag)return -1;

  /* If frame size has changed, abort out for now */
  if (YuvInputData->YHeight != (int)cpi->pb.Configuration.VideoFrameHeight ||
      YuvInputData->YWidth != (int)cpi->pb.Configuration.VideoFrameWidth )
    return(-1);


  /* Copy over input YUV to internal YUV buffers. */  
  /* First copy over the Y data */
  LocalDataPtr = cpi->yuv1ptr;
  InputDataPtr = (unsigned char *)YuvInputData->YBuffer;
  for ( i = 0; i < YuvInputData->YHeight; i++ ){
    memcpy( LocalDataPtr, InputDataPtr, YuvInputData->YWidth );
    LocalDataPtr += YuvInputData->YWidth;
    InputDataPtr += YuvInputData->YStride;
  }

  /* Now copy over the U data */
  LocalDataPtr = &cpi->yuv1ptr[(YuvInputData->YHeight * YuvInputData->YWidth)];
  InputDataPtr = (unsigned char *)YuvInputData->UBuffer;
  for ( i = 0; i < YuvInputData->UVHeight; i++ ){
    memcpy( LocalDataPtr, InputDataPtr, YuvInputData->UVWidth );
    LocalDataPtr += YuvInputData->UVWidth;
    InputDataPtr += YuvInputData->UVStride;
  }
  
  /* Now copy over the V data */
  LocalDataPtr = 
    &cpi->yuv1ptr[((YuvInputData->YHeight*YuvInputData->YWidth)*5)/4];
  InputDataPtr = (unsigned char *)YuvInputData->VBuffer;
  for ( i = 0; i < YuvInputData->UVHeight; i++ ){
    memcpy( LocalDataPtr, InputDataPtr, YuvInputData->UVWidth );
    LocalDataPtr += YuvInputData->UVWidth;
    InputDataPtr += YuvInputData->UVStride;
  }

  /* Mark this as a video frame */
  oggpackB_write(&cpi->oggbuffer,0,1);

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
  
  return 0;
}

int theora_encode_packetout( CP_INSTANCE *cpi, int last_p, ogg_packet *op){
  long bytes=oggpackB_bytes(&cpi->oggbuffer);
  
  if(!bytes)return(0);
  if(!cpi->packetflag)return(0);
  if(cpi->doneflag)return(-1);

  op->packet=oggpackB_get_buffer(&cpi->oggbuffer);
  op->bytes=bytes;
  op->b_o_s=0;
  op->e_o_s=last_p;
  
  op->packetno=cpi->CurrentFrame;

  op->granulepos=
    ((cpi->CurrentFrame-cpi->LastKeyFrame+1)<<cpi->keyframe_granule_shift)+ 
    cpi->LastKeyFrame-1;

  cpi->packetflag=0;
  if(last_p)cpi->doneflag=1;

  return 1;
}

int theora_encode_header(CP_INSTANCE *cpi, ogg_packet *op){
  /* width, height, fps, granule_shift, version */

  oggpackB_reset(&cpi->oggbuffer);
  oggpackB_write(&cpi->oggbuffer,0x80,8);
  oggpackB_write(&cpi->oggbuffer,'t',8);
  oggpackB_write(&cpi->oggbuffer,'h',8);
  oggpackB_write(&cpi->oggbuffer,'e',8);
  oggpackB_write(&cpi->oggbuffer,'o',8);
  oggpackB_write(&cpi->oggbuffer,'r',8);
  oggpackB_write(&cpi->oggbuffer,'a',8);
  
  oggpackB_write(&cpi->oggbuffer,cpi->ScanConfig.VideoFrameWidth,16);
  oggpackB_write(&cpi->oggbuffer,cpi->ScanConfig.VideoFrameHeight,16);
  oggpackB_write(&cpi->oggbuffer,cpi->Configuration.OutputFrameRateN,32);
  oggpackB_write(&cpi->oggbuffer,cpi->Configuration.OutputFrameRateD,32);

  oggpackB_write(&cpi->oggbuffer,cpi->keyframe_granule_shift,5);

  oggpackB_write(&cpi->oggbuffer,VERSION_MAJOR,8);
  oggpackB_write(&cpi->oggbuffer,VERSION_MINOR,8);
  oggpackB_write(&cpi->oggbuffer,VERSION_SUB,8);

  op->packet=oggpackB_get_buffer(&cpi->oggbuffer);
  op->bytes=oggpackB_bytes(&cpi->oggbuffer);

  op->b_o_s=1;
  op->e_o_s=0;
  
  op->packetno=0;
  
  op->granulepos=0;
  cpi->packetflag=0;

  return(0);
}

void theora_encode_clear(CP_INSTANCE *cpi){
  if(cpi){
    
    ClearHuffmanSet(&cpi->pb);
    ClearFragmentInfo(&cpi->pb);
    ClearFrameInfo(&cpi->pb);
    EClearFragmentInfo(cpi);
    EClearFrameInfo(cpi);		
    ClearTmpBuffers(&cpi->pb);
    ClearPPInstance(&cpi->pp);
    
  }
}


