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
  last mod: $Id: encoder_internal.h,v 1.2 2002/09/17 07:01:33 xiphmont Exp $

 ********************************************************************/

#include <ogg/ogg.h>

typedef struct CONFIG_TYPE2{
  ogg_uint32_t TargetBandwidth;
  double       OutputFrameRate;
  ogg_uint32_t OutputFrameRateN;
  ogg_uint32_t OutputFrameRateD;
  
  ogg_uint32_t FirstFrameQ;
  ogg_uint32_t BaseQ;
  ogg_uint32_t MaxQ;            /* Absolute Max Q allowed. */
  ogg_uint32_t ActiveMaxQ;      /* Currently active Max Q */
  
} CONFIG_TYPE2;

typedef struct{
  unsigned char * Yuv0ptr;
  unsigned char * Yuv1ptr;
  unsigned char * SrfWorkSpcPtr;
  unsigned char * disp_fragments;
  
  ogg_uint32_t  * RegionIndex; /* Gives pixel index for top left of
                                 each block */
  ogg_uint32_t    VideoFrameHeight;
  ogg_uint32_t    VideoFrameWidth;
  unsigned char   HFragPixels;
  unsigned char   VFragPixels;

} SCAN_CONFIG_DATA;

typedef unsigned char YUV_BUFFER_ENTRY;

typedef struct{
  ogg_int32_t   x;
  ogg_int32_t   y;
} MOTION_VECTOR;

typedef ogg_int16_t     Q_LIST_ENTRY;
typedef Q_LIST_ENTRY    Q_LIST[64];

typedef struct PP_INSTANCE {
  ogg_uint32_t  PrevFrameLimit;
  ogg_uint32_t *ScanPixelIndexTableAlloc;          
  signed char  *ScanDisplayFragmentsAlloc;
  
  signed char  *PrevFragmentsAlloc[MAX_PREV_FRAMES];
  
  ogg_uint32_t *FragScoresAlloc; /* The individual frame difference ratings. */
  signed char  *SameGreyDirPixelsAlloc;
  signed char  *BarBlockMapAlloc;
  
  /* Number of pixels changed by diff threshold in row of a fragment. */
  unsigned char  *FragDiffPixelsAlloc;  
  
  unsigned char  *PixelScoresAlloc;  
  unsigned char  *PixelChangedMapAlloc;
  unsigned char  *ChLocalsAlloc;
  ogg_int16_t    *yuv_differencesAlloc;  
  ogg_int32_t    *RowChangedPixelsAlloc;
  signed char    *TmpCodedMapAlloc;
  
  ogg_uint32_t   *ScanPixelIndexTable;               
  signed char    *ScanDisplayFragments;
  
  signed char    *PrevFragments[MAX_PREV_FRAMES];
  
  ogg_uint32_t   *FragScores; /* The individual frame difference ratings. */
  signed char    *SameGreyDirPixels;
  signed char    *BarBlockMap;
  
  /* Number of pixels changed by diff threshold in row of a fragment. */
  unsigned char  *FragDiffPixels;  

  unsigned char  *PixelScores;  
  unsigned char  *PixelChangedMap;
  unsigned char  *ChLocals;
  ogg_int16_t    *yuv_differences;  
  ogg_int32_t    *RowChangedPixels;
  signed char    *TmpCodedMap;
  
  /* Plane pointers and dimension variables */
  unsigned char  * YPlanePtr0;
  unsigned char  * YPlanePtr1;
  unsigned char  * UPlanePtr0;
  unsigned char  * UPlanePtr1;
  unsigned char  * VPlanePtr0;
  unsigned char  * VPlanePtr1;
  
  ogg_uint32_t    VideoYPlaneWidth;
  ogg_uint32_t    VideoYPlaneHeight;
  ogg_uint32_t    VideoUVPlaneWidth;
  ogg_uint32_t    VideoUVPlaneHeight;
  
  ogg_uint32_t    VideoYPlaneStride;
  ogg_uint32_t    VideoUPlaneStride;
  ogg_uint32_t    VideoVPlaneStride;
  
  /* Scan control variables. */
  unsigned char   HFragPixels;
  unsigned char   VFragPixels;
  
  ogg_uint32_t    ScanFrameFragments;
  ogg_uint32_t    ScanYPlaneFragments;
  ogg_uint32_t    ScanUVPlaneFragments;
  ogg_uint32_t    ScanHFragments;
  ogg_uint32_t    ScanVFragments;
  
  ogg_uint32_t    YFramePixels; 
  ogg_uint32_t    UVFramePixels; 
  
  ogg_uint32_t    SgcThresh;
  
  ogg_uint32_t    OutputBlocksUpdated;
  ogg_uint32_t    KFIndicator;
  
  /* The pre-processor scan configuration. */
  SCAN_CONFIG_DATA ScanConfig;
  
  ogg_int32_t   SRFGreyThresh;
  ogg_int32_t   SRFColThresh;
  ogg_int32_t   SgcLevelThresh;
  ogg_int32_t   SuvcLevelThresh;
  
  ogg_uint32_t  NoiseSupLevel;
  
  /* Block Thresholds. */
  ogg_uint32_t  PrimaryBlockThreshold;
  
  int   PAKEnabled;
  
  int   LevelThresh; 
  int   NegLevelThresh; 
  int   SrfThresh;
  int   NegSrfThresh;
  int   HighChange;
  int   NegHighChange;     
  
  /* Threshold lookup tables */
  unsigned char SrfPakThreshTable[512];
  unsigned char * SrfPakThreshTablePtr;
  unsigned char SrfThreshTable[512];
  unsigned char * SrfThreshTablePtr;
  unsigned char SgcThreshTable[512];
  unsigned char * SgcThreshTablePtr;
  
  /* Variables controlling S.A.D. break outs. */
  ogg_uint32_t GrpLowSadThresh;
  ogg_uint32_t GrpHighSadThresh;
  ogg_uint32_t ModifiedGrpLowSadThresh;
  ogg_uint32_t ModifiedGrpHighSadThresh;
  
  ogg_int32_t  PlaneHFragments;
  ogg_int32_t  PlaneVFragments;
  ogg_int32_t  PlaneHeight;
  ogg_int32_t  PlaneWidth;
  ogg_int32_t  PlaneStride;
  
  ogg_uint32_t BlockThreshold;
  ogg_uint32_t BlockSgcThresh;
  double UVBlockThreshCorrection;
  double UVSgcCorrection;
  
  double YUVPlaneCorrectionFactor;        
  double AbsDiff_ScoreMultiplierTable[256];
  unsigned char  NoiseScoreBoostTable[256];
  unsigned char  MaxLineSearchLen;
  
  ogg_int32_t YuvDiffsCircularBufferSize;
  ogg_int32_t ChLocalsCircularBufferSize;
  ogg_int32_t PixelMapCircularBufferSize;
  
} PP_INSTANCE;

typedef struct CP_INSTANCE {

  /* Compressor Configuration */
  SCAN_CONFIG_DATA ScanConfig;
  CONFIG_TYPE2     Configuration;
  int              QuickCompress;
  int              GoldenFrameEnabled;
  int              InterPrediction;
  int              MotionCompensation;
  int              AutoKeyFrameEnabled ;
  ogg_int32_t      ForceKeyFrameEvery ;
  ogg_int32_t      AutoKeyFrameThreshold ;
  ogg_uint32_t     LastKeyFrame ;
  ogg_uint32_t     MinimumDistanceToKeyFrame ;
  ogg_uint32_t     KeyFrameDataTarget ;  /* Data rate target for key frames */
  ogg_uint32_t     KeyFrameFrequency ;
  int              DropFramesAllowed ; 
  ogg_int32_t      DropCount ;
  ogg_int32_t      MaxConsDroppedFrames ;
  ogg_int32_t      DropFrameTriggerBytes;
  int              DropFrameCandidate;
  ogg_uint32_t     QualitySetting;
  ogg_uint32_t     Sharpness;
  ogg_uint32_t     PreProcFilterLevel;
  int              NoDrops;

  /* Compressor Statistics */
  double           TotErrScore;
  ogg_int64_t      KeyFrameCount; /* Count of key frames. */
  ogg_int64_t      TotKeyFrameBytes;
  ogg_uint32_t     LastKeyFrameSize;
  ogg_uint32_t     PriorKeyFrameSize[KEY_FRAME_CONTEXT];
  ogg_uint32_t     PriorKeyFrameDistance[KEY_FRAME_CONTEXT];
  ogg_int32_t      FrameQuality[6];
  int              DecoderErrorCode; /* Decoder error flag. */
  ogg_int32_t      ThreshMapThreshold;
  ogg_int32_t      TotalMotionScore;
  ogg_int64_t      TotalByteCount;
  ogg_int32_t      FixedQ;
	
  /* Frame Statistics  */
  signed char      InterCodeCount;
  ogg_int64_t      CurrentFrame;                                
  ogg_int64_t      CarryOver ;
  ogg_uint32_t     LastFrameSize;
  ogg_uint32_t     FrameBitCount;
  int              ThisIsFirstFrame;
  int              ThisIsKeyFrame;
	
  ogg_int32_t      MotionScore;   
  ogg_uint32_t     RegulationBlocks;
  ogg_int32_t      RecoveryMotionScore;   
  int              RecoveryBlocksAdded ;
  double           ProportionRecBlocks;
  double           MaxRecFactor ;
  
  /* Rate Targeting variables. */
  ogg_uint32_t     ThisFrameTargetBytes;
  double           BpbCorrectionFactor;
  
  /* Up regulation variables */
  ogg_uint32_t     FinalPassLastPos;  /* Used to regulate a final
					 unrestricted high quality
					 pass. */
  ogg_uint32_t     LastEndSB;	      /* Where we were in the loop
                                         last time. */
  ogg_uint32_t     ResidueLastEndSB;  /* Where we were in the residue
					 update loop last time. */
  
  /* Controlling Block Selection */
  ogg_uint32_t     MVChangeFactor;     
  ogg_uint32_t     FourMvChangeFactor;           
  ogg_uint32_t     MinImprovementForNewMV;   
  ogg_uint32_t     ExhaustiveSearchThresh;
  ogg_uint32_t     MinImprovementForFourMV;   
  ogg_uint32_t     FourMVThreshold;
  
  /* Module shared data structures. */            
  ogg_int32_t      frame_target_rate;
  ogg_int32_t      BaseLineFrameTargetRate;
  ogg_int32_t      min_blocks_per_frame;
  ogg_uint32_t     tot_bytes_old;
  
  //********************************************************************/
  /* Frames  Used in the selecetive convolution filtering of the Y plane. */
  unsigned char    *ConvDestBuffer;
  YUV_BUFFER_ENTRY *yuv0ptr;
  YUV_BUFFER_ENTRY *yuv1ptr;
  /*********************************************************************/

  /*********************************************************************/
  /* Token Buffers */
  ogg_uint32_t     *OptimisedTokenListEb; /* Optimised token list extra bits */
  unsigned char    *OptimisedTokenList;   /* Optimised token list. */
  unsigned char    *OptimisedTokenListHi; /* Optimised token list huffman
					     table index */
  
  unsigned char    *OptimisedTokenListPl; /* Plane to which the token
					     belongs Y = 0 or UV = 1 */
  ogg_int32_t       OptimisedTokenCount;	   /* Count of Optimized tokens */
  ogg_uint32_t      RunHuffIndex;         /* Huffman table in force at
					     the start of a run */
  ogg_uint32_t      RunPlaneIndex;        /* The plane (Y=0 UV=1) to
					     which the first token in
					     an EOB run belonged. */
  
  
  ogg_uint32_t      TotTokenCount;
  ogg_int32_t       TokensToBeCoded;
  ogg_int32_t       TokensCoded;
  /********************************************************************/

  /* SuperBlock, MacroBLock and Fragment Information */
  /* Coded flag arrays and counters for them */
  unsigned char    *PartiallyCodedFlags;
  unsigned char    *PartiallyCodedMbPatterns;
  unsigned char    *UncodedMbFlags;
  
  unsigned char    *extra_fragments;   /* extra updates not
					  recommended by pre-processor */
  ogg_int16_t      *OriginalDC;
  
  ogg_uint32_t     *FragmentLastQ;     /* Array used to keep track of
					  quality at which each
					  fragment was last
					  updated. */
  unsigned char    *FragTokens;
  ogg_uint32_t     *FragTokenCounts;   /* Number of tokens per fragment */
  
  ogg_uint32_t     *RunHuffIndices;
  ogg_uint32_t     *LastCodedErrorScore; 
  ogg_uint32_t     *ModeList;
  MOTION_VECTOR    *MVList;
  
  unsigned char    *BlockCodedFlags;
	
  ogg_uint32_t      MvListCount;
  ogg_uint32_t      ModeListCount;
  
  
  unsigned char    *DataOutputBuffer;
  /*********************************************************************/
	
  ogg_uint32_t      RunLength;
  ogg_uint32_t      MaxBitTarget;     /* Cut off target for rate capping */
  double            BitRateCapFactor; /* Factor relating normal frame target
					 to cut off target. */
	
  unsigned char     MBCodingMode;     /* Coding mode flags */
  
  ogg_int32_t       MVPixelOffsetY[MAX_SEARCH_SITES];
  ogg_uint32_t      InterTripOutThresh;
  unsigned char     MVEnabled;
  ogg_uint32_t      MotionVectorSearchCount;
  ogg_uint32_t      FrameMVSearcOunt;
  ogg_int32_t       MVSearchSteps;
  ogg_int32_t       MVOffsetX[MAX_SEARCH_SITES];
  ogg_int32_t       MVOffsetY[MAX_SEARCH_SITES];
  ogg_int32_t       HalfPixelRef2Offset[9]; /* Offsets for half pixel
                                               compensation */
  signed char       HalfPixelXOffset[9];    /* Half pixel MV offsets for X */
  signed char       HalfPixelYOffset[9];    /* Half pixel MV offsets for Y */
  
  ogg_uint32_t      bit_pattern ;
  unsigned char     bits_so_far ; 
  ogg_uint32_t      lastval ;
  ogg_uint32_t      lastrun ;
  
  Q_LIST_ENTRY     *quantized_list;  
  Q_LIST_ENTRY	   *quantized_listAlloc;
  
  MOTION_VECTOR     MVector;
  ogg_uint32_t      TempBitCount;
  ogg_int16_t      *DCT_codes; /* Buffer that stores the result of
                                  Forward DCT */
  ogg_int16_t      *DCTDataBuffer; /* Input data buffer for Forward DCT */
  
  /* Motion compensation related variables */
  ogg_uint32_t      MvMaxExtent;
  
  double            QTargetModifier[Q_TABLE_SIZE];

  /* instances (used for reconstructing buffers and to hold tokens etc.) */
  PP_INSTANCE      *pp;   /* preprocessor */
  PB_INSTANCE       pb;   /* playback */

  /* ogg bitpacker for use in packet coding, other API state */
  oggpack_buffer    oggbuffer;
  int               readyflag;
  int               packetflag;
  int               doneflag;

  /* how far do we shift the granulepos to seperate out P frame counts? */
  int               keyframe_granule_shift;
	
} CP_INSTANCE;
