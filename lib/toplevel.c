


#define CommentString "Xiph.Org libTheora I 20020916"

#define A_TABLE_SIZE	    29
#define DF_CANDIDATE_WINDOW 5

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
  cpi->TotKeyFrameBytes += oggpackB_bytes(&cpi->opb);
  
  /* reset keyframe context and calculate weighted average of last
     KEY_FRAME_CONTEXT keyframes */
  for( i = 0 ; i < KEY_FRAME_CONTEXT ; i ++ ) {
    if ( i < KEY_FRAME_CONTEXT -1) {
      cpi->PriorKeyFrameSize[i] = cpi->PriorKeyFrameSize[i+1];
      cpi->PriorKeyFrameDistance[i] = cpi->PriorKeyFrameDistance[i+1];
    } else {
      cpi->PriorKeyFrameSize[KEY_FRAME_CONTEXT - 1] = 
	oggpackB_bytes(&cpi->opb);
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
  cpi->LastKeyFrameSize=oggpackB_bytes(&cpi->opb);

}

static void CompressFirstFrame(CP_INSTANCE *cpi) {                  
  ogg_uint32_t  register i; 

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
  PickIntra( cpi, cpi->pb.YSBRows, cpi->pb.YSBCols, 
	     cpi->pb.HFragments%4, cpi->pb.VFragments%4, 
	     cpi->pb.Configuration.VideoFrameWidth);
  UpdateFrame(cpi);  
  
  /* Initialise the carry over rate targeting variables. */
  cpi->CarryOver = 0;
  
}

static void CompressKeyFrame(CP_INSTANCE *cpi){                  
  ogg_uint32_t  i;   
  
  /*  Average key frame frequency and size */
  ogg_int32_t AvKeyFrameFrequency = 
    (ogg_int32_t) (cpi->CurrentFrame / cpi->KeyFrameCount);  
  ogg_int32_t AvKeyFrameBytes = 
    (ogg_int32_t) (cpi->TotKeyFrameBytes / cpi->KeyFrameCount);

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
  PickIntra( cpi, cpi->pb.YSBRows, cpi->pb.YSBCols, 
	     cpi->pb.HFragments%4, cpi->pb.VFragments%4, 
	     cpi->pb.Configuration.VideoFrameWidth);
  UpdateFrame(cpi);  
  
}

static void CompressFrame( CP_INSTANCE *cpi, ogg_uint32_t FrameNumber ) {
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
    DropFrame = TRUE;
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
    SetScanParam( cpi->pp, SCP_CONFIGURE_PP, cpi->PreProcFilterLevel );
    
    /* Score / analyses the fragments. */ 
    cpi->MotionScore = YUVAnalyseFrame(cpi->pp, &KFIndicator );
    
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
    PickModes( cpi, cpi->pb.YSBRows, cpi->pb.YSBCols, cpi->pb.HFragments%4, 
	       cpi->pb.VFragments%4, cpi->pb.Configuration.VideoFrameWidth, 
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
  
void UpdateFrame(CP_INSTANCE *cpi){
  ogg_int32_t AvKeyFrameFrequency = 
    (ogg_int32_t) (cpi->CurrentFrame / cpi->KeyFrameCount);  
  ogg_int32_t AvKeyFrameBytes = 
    (ogg_int32_t) (cpi->TotKeyFrameBytes / cpi->KeyFrameCount);
  ogg_int32_t TotalWeight = 0;
  
  double CorrectionFactor;
  ogg_uint32_t  fragment_count = 0;                               
  
  ogg_uint32_t diff_tokens = 0;                  
  ogg_uint32_t  bits_per_token = 0;
  
  /* Reset the DC predictors. */
  cpi->pb.LastIntraDC = 0;
  cpi->pb.InvLastIntraDC = 0;
  cpi->pb.LastInterDC = 0;
  cpi->pb.InvLastInterDC = 0;
    
  /* Initialise bit packing mechanism. */
  InitAddBitsToBuffer(cpi);  
  
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
       DF_CANDIDATE_WINDOW) + oggpackB_bytes(&cpi->opb);
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
    CorrectionFactor = (double)oggpackB_bytes(&cpi->opb) / 
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
		       (ogg_int32_t)oggpackB_bytes(&cpi->opb));
  }
  cpi->TotalByteCount += oggpackB_bytes(&cpi->opb);
}

/********************** The toplevel ***********************/

const char *theora_encode_version(void){
  return CommentString;
}

int theora_encode_init( CP_INSTANCE *cpi , COMP_CONFIG *CompConfig ) {
  int i;
  
  InitCPInstance(cpi);
  
  for(i=0;i<=64;i++) {
    if(i<=1)cpi->pb.idct[i]=IDct1;
    else if(i<=10)cpi->pb.idct[i]=IDct10;
    else cpi->pb.idct[i]=IDctSlow;
  }
  cpi->pb.Configuration.HFragPixels = 8;
  cpi->pb.Configuration.VFragPixels = 8;
  
  /* set the version number */
  cpi->pb.Vp3VersionNo = CURRENT_ENCODE_VERSION;
  
  /* Set the video frame size. */
  cpi->pb.YPlaneSize = 0xFFF;
  cpi->pb.Configuration.VideoFrameHeight =0xFFF;
  cpi->pb.Configuration.VideoFrameHeight = 
    (CompConfig->FrameSize & 0x0000FFFF);
  cpi->pb.Configuration.VideoFrameWidth = 
    ((CompConfig->FrameSize & 0xFFFF0000) >> 16);
  
  /* Note the height and width in the pre-processor control structure. */
  cpi->ScanConfig.VideoFrameHeight = cpi->pb.Configuration.VideoFrameHeight;
  cpi->ScanConfig.VideoFrameWidth = cpi->pb.Configuration.VideoFrameWidth;

  /* Set data rate related variables. */
  cpi->Configuration.TargetBandwidth = (CompConfig->TargetBitRate * 1000) / 8;
  
  /* Set the target minimum key frame frequency */
  cpi->KeyFrameFrequency = CompConfig->KeyFrameFrequency;
  
  /* Set key frame data rate target */
  cpi->KeyFrameDataTarget = (CompConfig->KeyFrameDataTarget * 1000) / 8;
  
  /* Set the quality settings. */
  ConfigureQuality( cpi, CompConfig->Quality );

  /* Set the frame rate variables. */
  cpi->Configuration.OutputFrameRate = CompConfig->FrameRate;
  if ( cpi->Configuration.OutputFrameRate < 1 )
    cpi->Configuration.OutputFrameRate = 1;
  else if ( cpi->Configuration.OutputFrameRate > 30 )
    cpi->Configuration.OutputFrameRate = 30;
  cpi->frame_target_rate =  
    (cpi->Configuration.TargetBandwidth / cpi->Configuration.OutputFrameRate); 
  
  /* Initialise image format details */
  if(!InitFrameDetails(&cpi->pb)){
    return -1;
  }
  if(!EAllocateFragmentInfo(cpi)){
    DeleteFragmentInfo(&cpi->pb);
    DeleteFrameInfo(&cpi->pb);
    return -1;
  }

  if(!EAllocateFrameInfo(cpi)){
    DeleteFragmentInfo(&cpi->pb);
    DeleteFrameInfo(&cpi->pb);
    EDeleteFragmentInfo(cpi);
    return -1;
  }

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
  if(!ScanYUVInit(cpi->pp, &(cpi->ScanConfig))) {
    DeleteFragmentInfo(&cpi->pb);
    DeleteFrameInfo(&cpi->pb);
    EDeleteFragmentInfo(cpi);
    EDeleteFrameInfo(cpi);
    return -1;
  }

  /* Set encoder flags. */
  cpi->DropFramesAllowed = CompConfig->AllowDF;
  cpi->QuickCompress = CompConfig->QuickCompress;
  cpi->AutoKeyFrameEnabled = CompConfig->AutoKeyFrameEnabled;
  cpi->MinimumDistanceToKeyFrame = CompConfig->MinimumDistanceToKeyFrame;
  cpi->ForceKeyFrameEvery = CompConfig->ForceKeyFrameEvery;
  cpi->PreProcFilterLevel = CompConfig->NoiseSensitivity;
  cpi->AutoKeyFrameThreshold = CompConfig->AutoKeyFrameThreshold;
  cpi->Sharpness = CompConfig->Sharpness;
  
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
  
  /* Select the appropriate huffman set. */
  SelectHuffmanSet(&cpi->pb);

  /* This makes sure encoder version specific tables are initialised */
  InitQTables(&cpi->pb);  
  
  /* Indicate that the next frame to be compressed is the first in the
     current clip. */
  cpi->ThisIsFirstFrame = TRUE;
  
  return 0;
}

int theora_encode_framein( CP_INSTANCE *cpi, 
			   YUV_INPUT_BUFFER_CONFIG *YuvInputData){
  ogg_int32_t i;
  unsigned char *LocalDataPtr;
  unsigned char *InputDataPtr;

  /* If frame size has changed, abort out for now */
  if (YuvInputData->YHeight != (INT32)cpi->pb.Configuration.VideoFrameHeight ||
      YuvInputData->YWidth != (INT32)cpi->pb.Configuration.VideoFrameWidth )
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

  /* Special case for first frame */
  if ( cpi->ThisIsFirstFrame ){
    CompressFirstFrame(cpi);
    cpi->ThisIsFirstFrame = FALSE;
    cpi->ThisIsKeyFrame = FALSE;
  } else if ( cpi->ThisIsKeyFrame ) {
    CompressKeyFrame(cpi);
    cpi->ThisIsKeyFrame = FALSE;
  } else  {
    /* Compress the frame. */
    CompressFrame( cpi, (unsigned int) cpi->CurrentFrame );
  }

  /* Update stats variables. */
  cpi->LastFrameSize = oggpackB_bytes(&cpi->opb);
  cpi->CurrentFrame++;
  
  return 0;
}


  /* return whether or not we are a key frame */
  iskey=GetFrameType(&cpi->pb);
	if ( iskey == 0 )
		*is_key = 1;
	else 
		*is_key = 0;

    return ret_val;

}

/****************************************************************************
 * 
 *  ROUTINE       :     StopEncoder
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None .
 *
 *  FUNCTION      :     Stops the encoder and grabber
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
BOOL CCONV StopEncoder(CP_INSTANCE **cpi)
{

	if(*cpi)
	{

		DeleteFragmentInfo(&(*cpi)->pb);
		DeleteFrameInfo(&(*cpi)->pb);
		EDeleteFragmentInfo((*cpi));
		EDeleteFrameInfo((*cpi));
		
		/* Re-set Output buffer. */
		(*cpi)->BufferedOutputBytes = 0;
		
		DeleteCPInstance(cpi);

	}

	return TRUE;
}


