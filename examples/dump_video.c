/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2003             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: example dumpvid application; dumps  Theora streams 
  last mod: $Id: dump_video.c,v 1.1 2003/06/02 19:07:08 mauricio Exp $

 ********************************************************************/

/* By Mauricio Piacentini (mauricio at xiph.org) */
/*  simply dump decoded YUV data, for verification of theora bitstream */

#define _GNU_SOURCE
#define _REENTRANT
#define _LARGEFILE_SOURCE 
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
/*#include <sys/time.h>*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include "theora/theora.h"



/* Helper; just grab some more compressed bitstream and sync it for
   page extraction */
int buffer_data(FILE *in,ogg_sync_state *oy){
  char *buffer=ogg_sync_buffer(oy,4096);
  int bytes=fread(buffer,1,4096,in);
  ogg_sync_wrote(oy,bytes);
  return(bytes);
}

/* never forget that globals are a one-way ticket to Hell */
/* Ogg and codec state for demux/decode */
ogg_sync_state   oy; 
ogg_page         og;
ogg_stream_state vo;
ogg_stream_state to;
theora_info      ti;
theora_comment   tc;
theora_state     td;

int              theora_p=0;
//int              vorbis_p=0;
int              stateflag=0;

/* single frame video buffering */
int          videobuf_ready=0;
ogg_int64_t  videobuf_granulepos=-1;
double       videobuf_time=0;

FILE* outfile = NULL;

int got_sigint=0;
static void sigint_handler (int signal) {
  got_sigint = 1;
}

static void open_video(void){

}

static void video_write(void){
  int i;
		
  yuv_buffer yuv;
  theora_decode_YUVout(&td,&yuv);

  for(i=0;i<yuv.y_height;i++)
    fwrite(yuv.y+yuv.y_stride*i, 1, yuv.y_width, outfile);
 for(i=0;i<yuv.uv_height;i++){
	fwrite(yuv.v+yuv.uv_stride*i, 1, yuv.uv_width, outfile);
	fwrite(yuv.u+yuv.uv_stride*i, 1, yuv.uv_width, outfile);
	}
  
}
/* dump the theora (or vorbis) comment header */
static int dump_comments(theora_comment *tc){
  int i, len;
  char *value;
  FILE *out=stdout;
  
  fprintf(out,"Encoded by %s\n",tc->vendor);
  if(tc->comments){
    fprintf(out, "theora comment header:\n");
    for(i=0;i<tc->comments;i++){
      if(tc->user_comments[i]){
        len=tc->comment_lengths[i];
      	value=malloc(len+1);
      	memcpy(value,tc->user_comments[i],len);
      	value[len]='\0';
      	fprintf(out, "\t%s\n", value);
      	free(value);
      }
    }
  }
  return(0);
}

/* helper: push a page into the appropriate steam */
/* this can be done blindly; a stream won't accept a page
                that doesn't belong to it */
static int queue_page(ogg_page *page){
  if(theora_p)ogg_stream_pagein(&to,&og);
  return 0;
}                                   

static void usage(void){
  fprintf(stderr,
          "Usage: dumpvid <file.ogg> > outfile\n"
          "input is read from stdin if no file is passed on the command line\n"
          "\n"
  );
}

int main(int argc,char *argv[]){
  
  int i,j;
  ogg_packet op;
  
  FILE *infile = stdin;
  outfile = stdout;

#ifdef _WIN32 /* We need to set stdin/stdout to binary mode. Damn windows. */
  /* Beware the evil ifdef. We avoid these where we can, but this one we 
     cannot. Don't add any more, you'll probably go to hell if you do. */
  _setmode( _fileno( stdin ), _O_BINARY );
  _setmode( _fileno( stdout ), _O_BINARY );
#endif

  /* open the input file if any */
  if(argc==2){
    infile=fopen(argv[1],"rb");
    if(infile==NULL){
      fprintf(stderr,"Unable to open '%s' for extraction.\n", argv[1]);
      exit(1);
    }
  }
  if(argc>2){
      usage();
      exit(1);
  }
  
  /* start up Ogg stream synchronization layer */
  ogg_sync_init(&oy);

  /* init supporting Vorbis structures needed in header parsing */
//  vorbis_info_init(&vi);
//  vorbis_comment_init(&vc);
  theora_comment_init(&tc);

  /* Ogg file open; parse the headers */
  /* Only interested in Vorbis/Theora streams */
  while(!stateflag){
    int ret=buffer_data(infile,&oy);
    if(ret==0)break;
    while(ogg_sync_pageout(&oy,&og)>0){
      ogg_stream_state test;
      
      /* is this a mandated initial header? If not, stop parsing */
      if(!ogg_page_bos(&og)){
	/* don't leak the page; get it into the appropriate stream */
	queue_page(&og);
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
      }else{
	/* whatever it is, we don't care about it */
	ogg_stream_clear(&test);
      }
    }
    /* fall through to non-bos page parsing */
  }
  
  /* we're expecting more header packets. */
  while(theora_p && theora_p<3){
    int ret;
    
    /* look for further theora headers */
    while(theora_p && (theora_p<3) && (ret=ogg_stream_packetout(&to,&op))){
      if(ret<0){
      	fprintf(stderr,"Error parsing Theora stream headers; corrupt stream?\n");
      	exit(1);
      }
      if(theora_p==1){
        if(theora_decode_comment(&tc,&op)){
          fprintf(stderr,"Error parsing Theora stream headers; corrupt stream?\n");
          exit(1);
        }else{
		  /*not interested in comments now*/
          /*dump_comments(&tc);*/
          theora_p++;
          continue;
        }
      }
      if(theora_p==2){
        if(theora_decode_tables(&ti,&op)){
          fprintf(stderr,"Error parsing Theora stream headers; corrupt stream?\n");
          exit(1);
        }
        theora_p++;
        /* fall through */
      }
      if(theora_p==3)break;
    }

    
    /* The header pages/packets will arrive before anything else we
       care about, or the stream is not obeying spec */
    
    if(ogg_sync_pageout(&oy,&og)>0){
      queue_page(&og); /* demux into the appropriate stream */
    }else{
      int ret=buffer_data(infile,&oy); /* someone needs more data */
      if(ret==0){
	fprintf(stderr,"End of file while searching for codec headers.\n");
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
  

  /* open video */
  if(theora_p)open_video();

  /* install signal handler */
  signal (SIGINT, sigint_handler);

  /* on to the main decode loop.*/

  stateflag=0; /* playback has not begun */
  while(!got_sigint){
      
    while(theora_p && !videobuf_ready){
      /* theora is one in, one out... */
      if(ogg_stream_packetout(&to,&op)>0){
   
	theora_decode_packetin(&td,&op);
	videobuf_granulepos=td.granulepos;
	videobuf_time=theora_granule_time(&td,videobuf_granulepos);
	videobuf_ready=1;
		
      }else
	break;
    }
    
    if(!videobuf_ready  && feof(infile))break;
    
    if(!videobuf_ready ){
      /* no data yet for somebody.  Grab another page */
      int ret=buffer_data(infile,&oy);
      while(ogg_sync_pageout(&oy,&og)>0){
      	queue_page(&og);
      }
    }

    /* dumpvideo frame, and get new one */
      video_write();
      videobuf_ready=0;
  }

  /* close everything */

  if(theora_p){
    ogg_stream_clear(&to);
    theora_clear(&td);
  }
  ogg_sync_clear(&oy);

  if(infile && infile!=stdin)fclose(infile);
  
  fprintf(stderr,
	  "\r                                                              "
	  "\nDone.\n");
  return(0);

}
