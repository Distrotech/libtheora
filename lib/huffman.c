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
  last mod: $Id: huffman.c,v 1.6 2003/06/04 01:25:35 mauricio Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <ogg/ogg.h>
#include "encoder_internal.h"
#include "hufftables.h"

static void CreateHuffmanList(HUFF_ENTRY ** HuffRoot, 
			      ogg_uint32_t HIndex, ogg_uint32_t *FreqList ) {
  int i;
  HUFF_ENTRY *entry_ptr;
  HUFF_ENTRY *search_ptr;
  
  /* Create a HUFF entry for token zero. */
  HuffRoot[HIndex] = (HUFF_ENTRY *)_ogg_calloc(1,sizeof(*HuffRoot[HIndex]));
  
  HuffRoot[HIndex]->Previous = NULL;
  HuffRoot[HIndex]->Next = NULL;
  HuffRoot[HIndex]->ZeroChild = NULL;
  HuffRoot[HIndex]->OneChild = NULL;
  HuffRoot[HIndex]->Value = 0;
  HuffRoot[HIndex]->Frequency = FreqList[0];   
  
  if ( HuffRoot[HIndex]->Frequency == 0 )
    HuffRoot[HIndex]->Frequency = 1; 
    
  /* Now add entries for all the other possible tokens. */
  for ( i = 1; i < MAX_ENTROPY_TOKENS; i++ ) {
    entry_ptr = (HUFF_ENTRY *)_ogg_calloc(1,sizeof(*entry_ptr));

    entry_ptr->Value = i;
    entry_ptr->Frequency = FreqList[i];
    entry_ptr->ZeroChild = NULL;
    entry_ptr->OneChild = NULL;
    
    /* Force min value of 1. This prevents the tree getting too deep. */
    if ( entry_ptr->Frequency == 0 )
      entry_ptr->Frequency = 1;
    
    if ( entry_ptr->Frequency <= HuffRoot[HIndex]->Frequency ){
      entry_ptr->Next = HuffRoot[HIndex];  
      HuffRoot[HIndex]->Previous = entry_ptr;
      entry_ptr->Previous = NULL;   
      HuffRoot[HIndex] = entry_ptr;
    }else{  
      search_ptr = HuffRoot[HIndex];
      while ( (search_ptr->Next != NULL) && 
	      (search_ptr->Frequency < entry_ptr->Frequency) ){
	search_ptr = (HUFF_ENTRY *)search_ptr->Next;
      }
            
      if ( search_ptr->Frequency < entry_ptr->Frequency ){
	entry_ptr->Next = NULL;
	entry_ptr->Previous = search_ptr;   
	search_ptr->Next = entry_ptr;
      }else{
	entry_ptr->Next = search_ptr;
	entry_ptr->Previous = search_ptr->Previous; 
	search_ptr->Previous->Next = entry_ptr;
	search_ptr->Previous = entry_ptr;
      }
    }                                       
  }
}

static void CreateCodeArray( HUFF_ENTRY * HuffRoot, 
		      ogg_uint32_t *HuffCodeArray, 
		      unsigned char *HuffCodeLengthArray, 
		      ogg_uint32_t CodeValue, 
		      unsigned char CodeLength ) {    

  /* If we are at a leaf then fill in a code array entry. */
  if ( ( HuffRoot->ZeroChild == NULL ) && ( HuffRoot->OneChild == NULL ) ){
    HuffCodeArray[HuffRoot->Value] = CodeValue;
    HuffCodeLengthArray[HuffRoot->Value] = CodeLength;
  }else{
    /* Recursive calls to scan down the tree. */
    CreateCodeArray(HuffRoot->ZeroChild, HuffCodeArray, HuffCodeLengthArray, 
		    ((CodeValue << 1) + 0), CodeLength + 1);
    CreateCodeArray(HuffRoot->OneChild, HuffCodeArray, HuffCodeLengthArray, 
		    ((CodeValue << 1) + 1), CodeLength + 1);
  }
}

/*TODO note: not reentrant, scheduled for cleanup/optimization before beta1 */
static int huff_x;								///	rude!
static unsigned char huffset_bits[2230];		///	totally rude!

static void write_root(oggpack_buffer* opb, HUFF_ENTRY * HuffRoot) {
	if(HuffRoot->Value >= 0) {						///	if we have a leaf,
		oggpackB_write(opb, 1, 1);
		oggpackB_write(opb, HuffRoot->Value, 5);
	} else {
		oggpackB_write(opb, 0, 1);
		if(HuffRoot->ZeroChild) {
			write_root(opb, HuffRoot->ZeroChild);
		}
		if(HuffRoot->OneChild) {
			write_root(opb, HuffRoot->OneChild);
		}
	}
}


static int read_root_bit() {
	int i = huff_x >> 3;
	int r = huff_x & 7;
	int m = 0x80 >> r;
	int b = huffset_bits[i];
	huff_x++;
	if(b & m) b = 1;
	else b = 0;
	return b;
}

static int read_root_(int cnt) {
	int v=0;
	int x;
	for(x=0; x< cnt; x++) {
		v <<= 1;
		v += read_root_bit();
	}
	return v;
}

static void read_root( HUFF_ENTRY * HuffRoot) {
	if(!read_root_bit()) {
//    entry_ptr = (HUFF_ENTRY *)_ogg_calloc(1,sizeof(*entry_ptr));
		HuffRoot->ZeroChild = (HUFF_ENTRY *)_ogg_calloc(1,sizeof(HUFF_ENTRY));
		read_root(HuffRoot->ZeroChild);
		HuffRoot->OneChild = (HUFF_ENTRY *)_ogg_calloc(1,sizeof(HUFF_ENTRY));
		read_root(HuffRoot->OneChild);
		HuffRoot->Value = -1;
	} else {
		HuffRoot->ZeroChild = NULL;
		HuffRoot->OneChild = NULL;
		HuffRoot->Value = read_root_(5);
	}
}

static void read_hufftree(HUFF_ENTRY ** huffarray, int index) {
    huffarray[index] = (HUFF_ENTRY *)_ogg_calloc(1,sizeof(HUFF_ENTRY));
	read_root(huffarray[index]);
}

static void  BuildHuffmanTree( HUFF_ENTRY **HuffRoot, 
			ogg_uint32_t *HuffCodeArray, 
			unsigned char *HuffCodeLengthArray, 
			ogg_uint32_t HIndex, 
			ogg_uint32_t *FreqList ){   

  HUFF_ENTRY *entry_ptr;
  HUFF_ENTRY *search_ptr;

  /* First create a sorted linked list representing the frequencies of
     each token. */
  CreateHuffmanList( HuffRoot, HIndex, FreqList );
    
  /* Now build the tree from the list. */
     
  /* While there are at least two items left in the list. */ 
  while ( HuffRoot[HIndex]->Next != NULL ){                              
    /* Create the new node as the parent of the first two in the list. */
    entry_ptr = (HUFF_ENTRY *)_ogg_calloc(1,sizeof(*entry_ptr));
    entry_ptr->Value = -1;
    entry_ptr->Frequency = HuffRoot[HIndex]->Frequency + 
      HuffRoot[HIndex]->Next->Frequency ;
    entry_ptr->ZeroChild = HuffRoot[HIndex];
    entry_ptr->OneChild = HuffRoot[HIndex]->Next;  
    
    /* If there are still more items in the list then insert the new
       node into the list. */
    if (entry_ptr->OneChild->Next != NULL ){   
      /* Set up the provisional 'new root' */ 
      HuffRoot[HIndex] = entry_ptr->OneChild->Next;
      HuffRoot[HIndex]->Previous = NULL;
            
      /* Now scan through the remaining list to insert the new entry
         at the appropriate point. */
      if ( entry_ptr->Frequency <= HuffRoot[HIndex]->Frequency ){ 
	entry_ptr->Next = HuffRoot[HIndex];  
	HuffRoot[HIndex]->Previous = entry_ptr;
	entry_ptr->Previous = NULL;   
	HuffRoot[HIndex] = entry_ptr;
      }else{  
	search_ptr = HuffRoot[HIndex];
	while ( (search_ptr->Next != NULL) && 
		(search_ptr->Frequency < entry_ptr->Frequency) ){
	  search_ptr = search_ptr->Next;
	}
	
	if ( search_ptr->Frequency < entry_ptr->Frequency ){
	  entry_ptr->Next = NULL;
	  entry_ptr->Previous = search_ptr;   
	  search_ptr->Next = entry_ptr;
	}else{
	  entry_ptr->Next = search_ptr;
	  entry_ptr->Previous = search_ptr->Previous; 
	  search_ptr->Previous->Next = entry_ptr;
	  search_ptr->Previous = entry_ptr;
	}
      }                                     
    }else{   
      /* Build has finished. */ 
      entry_ptr->Next = NULL;
      entry_ptr->Previous = NULL;
      HuffRoot[HIndex] = entry_ptr;
    }
    
    /* Delete the Next/Previous properties of the children (PROB NOT NEC). */ 
    entry_ptr->ZeroChild->Next = NULL;
    entry_ptr->ZeroChild->Previous = NULL;
    entry_ptr->OneChild->Next = NULL;
    entry_ptr->OneChild->Previous = NULL;
    
  } 
    
  /* Now build a code array from the tree. */
  CreateCodeArray( HuffRoot[HIndex], HuffCodeArray, 
		   HuffCodeLengthArray, 0, 0);
}

static void  DestroyHuffTree(HUFF_ENTRY *root_ptr){
  if (root_ptr){
    if ( root_ptr->ZeroChild )
      DestroyHuffTree(root_ptr->ZeroChild);
    
    if ( root_ptr->OneChild )
      DestroyHuffTree(root_ptr->OneChild);
    
    _ogg_free(root_ptr);
  }
}

void ClearHuffmanSet( PB_INSTANCE *pbi ){    
  int i;
  
  if (pbi->HuffRoot_VP3x){
    for ( i = 0; i < NUM_HUFF_TABLES; i++ )
      if (pbi->HuffRoot_VP3x[i]) DestroyHuffTree(pbi->HuffRoot_VP3x[i]);
    _ogg_free(pbi->HuffRoot_VP3x);
  }

  if (pbi->HuffCodeArray_VP3x){
    for ( i = 0; i < NUM_HUFF_TABLES; i++ )
      if (pbi->HuffCodeArray_VP3x[i]) 
	_ogg_free (pbi->HuffCodeArray_VP3x[i]);
    _ogg_free (pbi->HuffCodeArray_VP3x);
  }

  if (pbi->HuffCodeLengthArray_VP3x) {
    for ( i = 0; i < NUM_HUFF_TABLES; i++ )
      if (pbi->HuffCodeLengthArray_VP3x[i]) 
	_ogg_free (pbi->HuffCodeLengthArray_VP3x[i]);
    _ogg_free (pbi->HuffCodeLengthArray_VP3x);
  }  
  
  pbi->HuffRoot_VP3x=NULL;
  pbi->HuffCodeArray_VP3x=NULL;
  pbi->HuffCodeLengthArray_VP3x=NULL;
}

void InitHuffmanSet( PB_INSTANCE *pbi ){
  int i;

  ClearHuffmanSet(pbi);

  pbi->HuffRoot_VP3x = 
    _ogg_calloc(NUM_HUFF_TABLES,sizeof(*pbi->HuffRoot_VP3x));
  pbi->HuffCodeArray_VP3x = 
    _ogg_calloc(NUM_HUFF_TABLES,sizeof(*pbi->HuffCodeArray_VP3x));
  pbi->HuffCodeLengthArray_VP3x = 
    _ogg_calloc(NUM_HUFF_TABLES,sizeof(*pbi->HuffCodeLengthArray_VP3x));
  pbi->ExtraBitLengths_VP3x = ExtraBitLengths_VP31;
  
  for ( i = 0; i < NUM_HUFF_TABLES; i++ ){
    pbi->HuffCodeArray_VP3x[i] = 
      _ogg_calloc(MAX_ENTROPY_TOKENS,
		  sizeof(*pbi->HuffCodeArray_VP3x[i]));
    pbi->HuffCodeLengthArray_VP3x[i] = 
      _ogg_calloc(MAX_ENTROPY_TOKENS,
		  sizeof(*pbi->HuffCodeLengthArray_VP3x[i]));
    BuildHuffmanTree( pbi->HuffRoot_VP3x, 
		      pbi->HuffCodeArray_VP3x[i],
		      pbi->HuffCodeLengthArray_VP3x[i],
		      i, FrequencyCounts_VP3[i]);
  }
}

void InitHuffmanSetPlay( PB_INSTANCE *pbi ){
  int i;

  ClearHuffmanSet(pbi);

  pbi->HuffRoot_VP3x = 
    _ogg_calloc(NUM_HUFF_TABLES,sizeof(*pbi->HuffRoot_VP3x));
  pbi->HuffCodeArray_VP3x = 
    _ogg_calloc(NUM_HUFF_TABLES,sizeof(*pbi->HuffCodeArray_VP3x));
  pbi->HuffCodeLengthArray_VP3x = 
    _ogg_calloc(NUM_HUFF_TABLES,sizeof(*pbi->HuffCodeLengthArray_VP3x));
  pbi->ExtraBitLengths_VP3x = ExtraBitLengths_VP31;
  
  huff_x = 0;
  for ( i = 0; i < NUM_HUFF_TABLES; i++ ){
	read_hufftree(pbi->HuffRoot_VP3x,i);
  }
}

void write_HuffmanSet(oggpack_buffer* opb, HUFF_ENTRY **hroot) {
  int x;

  for(x=0; x<NUM_HUFF_TABLES; x++) {
    write_root(opb, hroot[x]);
  }
}

void read_HuffmanSet(oggpack_buffer* opb) {
  int x;
  for(x=0; x<2230; x++) {
    huffset_bits[x]=oggpackB_read(opb, 8);
  }
}
