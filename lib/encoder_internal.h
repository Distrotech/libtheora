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
  last mod: $Id: encoder_internal.h,v 1.1 2002/09/16 07:10:02 xiphmont Exp $

 ********************************************************************/

#include <ogg/ogg.h>

typedef struct CONFIG_TYPE2{
  UINT32 TargetBandwidth;
  UINT32 OutputFrameRate;
  
  UINT32 FirstFrameQ;
  UINT32 BaseQ;
  UINT32 MaxQ;            /* Absolute Max Q allowed. */
  UINT32 ActiveMaxQ;      /* Currently active Max Q */
  
} CONFIG_TYPE2;

typedef struct CP_INSTANCE {

  /* Compressor Configuration */
  SCAN_CONFIG_DATA ScanConfig;
  CONFIG_TYPE2 Configuration;
  BOOL   QuickCompress;
  BOOL   GoldenFrameEnabled;
  BOOL   InterPrediction;
  BOOL   MotionCompensation;
  BOOL   AutoKeyFrameEnabled ;
  INT32  ForceKeyFrameEvery ;
  INT32  AutoKeyFrameThreshold ;
  UINT32 LastKeyFrame ;
  UINT32 MinimumDistanceToKeyFrame ;
  UINT32 KeyFrameDataTarget ;        /* Data rate target for key frames */
  UINT32 KeyFrameFrequency ;
  BOOL   DropFramesAllowed ; 
  INT32  DropCount ;
  INT32  MaxConsDroppedFrames ;
  INT32  DropFrameTriggerBytes;
  BOOL   DropFrameCandidate;
  UINT32 QualitySetting;
  UINT32 Sharpness;
  UINT32 PreProcFilterLevel;
  BOOL   NoDrops;

  /* Compressor Statistics */
  double TotErrScore;
  INT64  KeyFrameCount; /* Count of key frames. */
  INT64  TotKeyFrameBytes;
  UINT32 LastKeyFrameSize;
  UINT32 PriorKeyFrameSize[KEY_FRAME_CONTEXT];
  UINT32 PriorKeyFrameDistance[KEY_FRAME_CONTEXT];
  INT32  FrameQuality[6];
  int    DecoderErrorCode; /* Decoder error flag. */
  INT32  ThreshMapThreshold;
  INT32  TotalMotionScore;
  INT64  TotalByteCount;
  INT32  FixedQ;
	
  /* Frame Statistics  */
  INT8   InterCodeCount;
  INT64  CurrentFrame;                                
  INT64  CarryOver ;
  UINT32 LastFrameSize;
  /*UINT32 ThisFrameSize; use the oggpackB primitive */
  /*UINT32 BufferedOutputBytes; */
  UINT32 FrameBitCount;
  BOOL   ThisIsFirstFrame;
  BOOL   ThisIsKeyFrame;
	
  INT32  MotionScore;   
  UINT32 RegulationBlocks;
  INT32  RecoveryMotionScore;   
  BOOL   RecoveryBlocksAdded ;
  double ProportionRecBlocks;
  double MaxRecFactor ;
  
  /* Rate Targeting variables. */
  UINT32 ThisFrameTargetBytes;
  double BpbCorrectionFactor;
  
  /* Up regulation variables */
  UINT32 FinalPassLastPos;  /* Used to regulate a final unrestricted
                               high quality pass. */
  UINT32 LastEndSB;	    /* Where we were in the loop last time. */
  UINT32 ResidueLastEndSB;  /* Where we were in the residue update
                               loop last time. */
  
  /* Controlling Block Selection */
  UINT32 MVChangeFactor;     
  UINT32 FourMvChangeFactor;           
  UINT32 MinImprovementForNewMV;   
  UINT32 ExhaustiveSearchThresh;
  UINT32 MinImprovementForFourMV;   
  UINT32 FourMVThreshold;
  
  /* Module shared data structures. */            
  INT32  frame_target_rate;
  INT32  BaseLineFrameTargetRate;
  INT32  min_blocks_per_frame;
  UINT32 tot_bytes_old;
  
  //********************************************************************/
  /* Frames  Used in the selecetive convolution filtering of the Y plane. */
  UINT8 *ConvDestBuffer;
  YUV_BUFFER_ENTRY *yuv0ptr;
  YUV_BUFFER_ENTRY *yuv1ptr;
  UINT8 *ConvDestBufferAlloc;
  YUV_BUFFER_ENTRY *yuv0ptrAlloc;
  YUV_BUFFER_ENTRY *yuv1ptrAlloc;
  /*********************************************************************/

  /*********************************************************************/
  /* Token Buffers */
  UINT32 *OptimisedTokenListEb;    /* Optimised token list extra bits */
  UINT8  *OptimisedTokenList;      /* Optimised token list. */
  UINT8  *OptimisedTokenListHi;    /* Optimised token list huffman
                                      table index */
  
  UINT32 *OptimisedTokenListEbAlloc;    /* Optimised token list extra bits */
  UINT8  *OptimisedTokenListAlloc;      /* Optimised token list. */
  UINT8  *OptimisedTokenListHiAlloc;    /* Optimised token list
                                           huffman table index */
  UINT8  *OptimisedTokenListPlAlloc;    /* Optimised token list
                                           huffman table index */
  
  UINT8  *OptimisedTokenListPl;    /* Plane to which the token belongs
                                      Y = 0 or UV = 1 */
  INT32  OptimisedTokenCount;	   /* Count of Optimized tokens */
  UINT32 RunHuffIndex;             /* Huffman table in force at the
                                      start of a run */
  UINT32 RunPlaneIndex;            /* The plane (Y=0 UV=1) to which
                                      the first token in an EOB run
                                      belonged. */
  
  
  UINT32 TotTokenCount;
  INT32  TokensToBeCoded;
  INT32  TokensCoded;
  /********************************************************************/

  /* SuperBlock, MacroBLock and Fragment Information */
  /* Coded flag arrays and counters for them */
  UINT8  *PartiallyCodedFlags;
  UINT8  *PartiallyCodedMbPatterns;
  UINT8  *UncodedMbFlags;
  
  UINT8  *extra_fragments;   /* extra updates not recommended by
                                pre-processor */
  INT16  *OriginalDC;
  
  UINT32 *FragmentLastQ;     /* Array used to keep track of quality at
                                which each fragment was last
                                updated. */
  UINT8  *FragTokens;
  UINT32 *FragTokenCounts;   /* Number of tokens per fragment */
  
  UINT32 *RunHuffIndices;
  UINT32 *LastCodedErrorScore; 
  UINT32 *ModeList;
  MOTION_VECTOR *MVList;
  
  UINT8  *BlockCodedFlags;
	
  UINT32 MvListCount;
  UINT32 ModeListCount;
  
  
  UINT8  *DataOutputBuffer;
  /*********************************************************************/
	
  UINT32 RunLength;
  UINT32 MaxBitTarget;        /* Cut off target for rate capping */
  double BitRateCapFactor;    /* Factor relating normal frame target
                                 to cut off target. */
	
  UINT8  MBCodingMode;	      /* Coding mode flags */
  
  INT32  MVPixelOffsetY[MAX_SEARCH_SITES];
  UINT32  InterTripOutThresh;
  UINT8  MVEnabled;
  UINT32 MotionVectorSearchCount;
  UINT32 FrameMVSearcOunt;
  INT32  MVSearchSteps;
  INT32  MVOffsetX[MAX_SEARCH_SITES];
  INT32  MVOffsetY[MAX_SEARCH_SITES];
  INT32  HalfPixelRef2Offset[9];    /* Offsets for half pixel compensation */
  INT8   HalfPixelXOffset[9];       /* Half pixel MV offsets for X */
  INT8   HalfPixelYOffset[9];       /* Half pixel MV offsets for Y */
  
  UINT32 bit_pattern ;
  UINT8  bits_so_far ; 
  UINT32 lastval ;
  UINT32 lastrun ;
  
  Q_LIST_ENTRY    *quantized_list;  
  Q_LIST_ENTRY	*quantized_listAlloc;
  
  MOTION_VECTOR   MVector;
  UINT32 TempBitCount;
  INT16  *DCT_codes;	    /* Buffer that stores the result of Forward DCT */
  INT16  *DCTDataBuffer;    /* Input data buffer for Forward DCT */
  
  /* Motion compensation related variables */
  UINT32  MvMaxExtent;
  
  /* copied from cbitman.c */
  INT32 byte_bit_offset;     
  UINT32 DataBlock;  
  UINT32 mybits; 
  UINT32 ByteBitsLeft;

  double QTargetModifier[Q_TABLE_SIZE];

  /* instances (used for reconstructing buffers and to hold tokens etc.) */
  PP_INSTANCE *pp;	   /* preprocessor */
  PB_INSTANCE pb;  /* playback */

  /* ogg bitpacker for use in packet coding */
  oggpack_buffer oggbuffer;
	
} CP_INSTANCE;
