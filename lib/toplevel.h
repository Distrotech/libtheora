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
  last mod: $Id: toplevel.h,v 1.2 2003/06/07 23:34:56 giles Exp $

 ********************************************************************/

/*from dbm huffman pack patch*/

extern void write_Qtables(oggpack_buffer* opb);
extern void write_HuffmanSet(oggpack_buffer* opb, HUFF_ENTRY **hroot);
extern void read_Qtables(oggpack_buffer* opb);
extern void read_HuffmanSet(oggpack_buffer* opb);
extern void InitHuffmanSetPlay( PB_INSTANCE *pbi );
