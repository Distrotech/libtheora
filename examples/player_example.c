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

  function: example SDL player application; plays Ogg Theora files (with
            optional Vorbis audio second stream)
  last mod: $Id: player_example.c,v 1.1 2002/09/24 05:05:49 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include "theora/theora.h"
#include "vorbis/codec.h"
#include <SDL/SDL.h>

/*
int open_audio(){}
double get_audio_time(){}
double get_time(){}
*/

int buffer_data(ogg_sync_state *oy){
  char *buffer=ogg_sync_buffer(oy,4096);
  int bytes=fread(buffer,1,4096,stdin);
  ogg_sync_wrote(oy,bytes);
  return(bytes);
}

int main(void){
  
  ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
  ogg_page         og;
  ogg_stream_state vo;
  ogg_stream_state to;
  theora_info      ti;
  theora_state     td;
  vorbis_info      vi;
  vorbis_dsp_state vd;
  vorbis_block     vb;
  vorbis_comment   vc;

  int theora_p=0;
  int vorbis_p=0;
  int stateflag=0;

#ifdef _WIN32 /* We need to set stdin/stdout to binary mode. Damn windows. */
  /* Beware the evil ifdef. We avoid these where we can, but this one we 
     cannot. Don't add any more, you'll probably go to hell if you do. */
  _setmode( _fileno( stdin ), _O_BINARY );
#endif

  ogg_sync_init(&oy);
  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);

  /* Ogg file on stdin; parse the headers */
  /* Only interested in Vorbis/Theora streams */
  while(!stateflag){
    int ret=buffer_data(&oy);
    if(ret==0)break;
    while(ogg_sync_pageout(&oy,&og)>0){
      ogg_stream_state test;
      ogg_packet       op;
      
      /* is this a mandated initial header? If not, stop parsing */
      if(!ogg_page_bos(&og)){
	/* don't leak the page; get it into the appropriate stream */
	/* this can be done blindly; a stream won't accept a page
             that doesn't bewlong to it */
	if(theora_p)ogg_stream_pagein(&to,&og);
	if(vorbis_p)ogg_stream_pagein(&vo,&og);
	stateflag=1;
	break;
      }
      
      ogg_stream_init(&test,ogg_page_serialno(&og));
      ogg_stream_pagein(&test,&og);
      ogg_stream_packetout(&test,&op);
      
      /* identify the codec: try theora */
      if(!theora_p && theora_decode_header(&ti,&op)>=0){
	/* it is theora */
	memcpy(&to,&test,sizeof(test));
	theora_p=1;
      }else if(!vorbis_p && vorbis_synthesis_headerin(&vi,&vc,&op)>=0){
	/* it is vorbis */
	memcpy(&vo,&test,sizeof(test));
	/* there will be more vorbis headers later... */
	vorbis_p=1;
      }else{
	/* whatever it is, we don't care about it */
	ogg_stream_clear(&test);
      }
    }
  }
  
  if(!theora_p){
    fprintf(stderr,"Input stream contains no Theora video (only checked "
	    "first link).\n");
    exit(1);
  }
  
  /* we're expecting more vorbis header packets. */
  if(vorbis_p && vorbis_p<3){
    int ret;
    ogg_packet       op;
    while((ret=ogg_stream_packetout(&vo,&op))){
      if(ret<0){
	fprintf(stderr,"Error parsing Vorbis stream headers; corrupt stream?\n");
	exit(1);
      }

      if(vorbis_synthesis_headerin(&vi,&vc,&op)){
	fprintf(stderr,"Error parsing Vorbis stream headers; corrupt stream?\n");
	exit(1);
      }
      vorbis_p++;
      if(vorbis_p==3)break;

    }

    /* The header pages/packets will arrive before anything else we
       care about, or the stream is not obeying spec */

    if(ogg_sync_pageout(&oy,&og)>0){
      ogg_stream_pagein(&vo,&og); /* the vorbis stream will accept
                                     only its own */
    }else{
      int ret=buffer_data(&oy);
      if(ret==0){
	fprintf(stderr,"End of file while searching for Vorbis headers.\n");
	exit(1);
      }
    }
  }

  /* and now we have it all.  initialize decoders */
  if(theora_p){
    theora_decode_init(&td,&ti);
    fprintf(stderr,"Ogg logical stream %x is Theora %dx%d %.02f fps video.\n",
	    to.serialno,ti.width,ti.height,
	    (double)ti.fps_numerator/ti.fps_denominator);
  }
  if(vorbis_p){
    vorbis_synthesis_init(&vd,&vi);
    vorbis_block_init(&vd,&vb);  
    fprintf(stderr,"Ogg logical stream %x is Vorbis %d channel %d Hz audio.\n",
	    vo.serialno,vi.channels,vi.rate);
  }else{
    /* tear down the partial vorbis setup */
    vorbis_info_clear(&vi);
    vorbis_comment_clear(&vc);
  }

  /* on to the main decode loop */




#if 0
    if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
      fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
      exit(1);
    }




    SDL_Quit();
#endif
}

