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
  last mod: $Id: player_example.c,v 1.2 2002/09/24 11:18:22 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include "theora/theora.h"
#include "vorbis/codec.h"
#include <SDL/SDL.h>

/* yes, this makes us OSS-specific for now */
#include <sys/soundcard.h>
#include <sys/ioctl.h>


int buffer_data(ogg_sync_state *oy){
  char *buffer=ogg_sync_buffer(oy,4096);
  int bytes=fread(buffer,1,4096,stdin);
  ogg_sync_wrote(oy,bytes);
  return(bytes);
}

/* never forget that globals are a one-way ticket to Hell */

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


SDL_Surface *screen;
SDL_Overlay *yuv_overlay;
SDL_Rect rect;

int videobuf_fill=0;
ogg_int64_t videobuf_granulepos=0;
double videobuf_time=0;

int audiobuf_fill=0;
int audiobuf_written=0;
ogg_int16_t audiobuf[4096];
ogg_int64_t audiobuf_granulepos=0; /* time position of last sample */
double audiobuf_time=0; /* time position of last sample */
long audio_totalsize=-1;
int  audiofd=-1;
int audio_buffstate=0;

int got_sigint=0;
static void sigint_handler (int signal) {
  got_sigint = 1;
}

static ogg_int64_t cali_time;
void calibrate_timer(int bytes){
  struct timeval tv;
  ogg_int64_t samples;
  gettimeofday(&tv,0);
  cali_time=tv.tv_sec*1000+tv.tv_usec/1000;

  samples=audiobuf_granulepos-
    audiobuf_fill+
    audiobuf_written-
    (bytes/2/vi.channels);
  
  cali_time-=1000.*samples/vi.rate;
  fprintf(stderr,"cali_time: %ld\n",(long)cali_time);
}

double get_time(){
  static int init=0;
  static ogg_int64_t start; 
  
  if(!init){
    struct timeval tv;
    gettimeofday(&tv,0);
    start=tv.tv_sec*1000+tv.tv_usec/1000;
    init=1;
  }

  {
    struct timeval tv;
    ogg_int64_t now;
    gettimeofday(&tv,0);
    now=tv.tv_sec*1000+tv.tv_usec/1000;

    if(audiofd<0) return(now-start*.001);

    return (now-cali_time)*.001;
  }
}

void audio_write_nonblocking(void){

  if(audiobuf_fill){
    audio_buf_info info;
    long bytes;

    ioctl(audiofd,SNDCTL_DSP_GETOSPACE,&info);
    bytes=info.bytes;
    if(bytes){
      if(audio_buffstate || bytes==audio_totalsize){
	/* *just* bank switched a fragment.  For a split second, 
	   the OSPACE is accurate for timing */
	calibrate_timer(audio_totalsize-bytes);
      }
      audio_buffstate=0;
      if(bytes>audiobuf_fill-audiobuf_written)
	bytes=audiobuf_fill-audiobuf_written;

      bytes=write(audiofd,audiobuf+audiobuf_written,bytes);
      if(bytes>0){
	audiobuf_written+=bytes;
	if(audiobuf_written>=audiobuf_fill){
	  audiobuf_fill=0;
	  audiobuf_written=0;
	}
      }
    }else audio_buffstate=1;
  } 
}

int main(void){
  
  int theora_p=0;
  int vorbis_p=0;
  int stateflag=0;

  if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
    exit(1);
  }

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

  /* open audio */
  if(vorbis_p){
    audio_buf_info info;
    int format=AFMT_S16_NE;
    int channels=vi.channels;
    int rate=vi.rate;
    int ret;

    audiofd=open("/dev/dsp",O_RDWR);
    if(audiofd<0){
      fprintf(stderr,"Could not open audio device /dev/dsp.\n");
      exit(1);
    }

    ret=ioctl(audiofd,SNDCTL_DSP_SETFMT,&format);
    if(ret){
      fprintf(stderr,"Could not set 16 bit host-endian playback\n");
      exit(1);
    }
    
    ret=ioctl(audiofd,SNDCTL_DSP_CHANNELS,&vi.channels);
    if(ret){
      fprintf(stderr,"Could not set %d channel playback\n",channels);
      exit(1);
    }

    ioctl(audiofd,SNDCTL_DSP_SPEED,&rate);
    if(ret){
      fprintf(stderr,"Could not set %d Hz playback\n",rate);
      exit(1);
    }

    ioctl(audiofd,SNDCTL_DSP_GETOSPACE,&info);
    audio_totalsize=info.fragstotal*info.fragsize;

  }

  /* open video */

  screen = SDL_SetVideoMode(ti.width, ti.height, 0, SDL_SWSURFACE);
  if ( screen == NULL ) {
    fprintf(stderr, "Unable to set 640x480 video: %s\n", SDL_GetError());
    exit(1);
  }

  yuv_overlay = SDL_CreateYUVOverlay(ti.width, ti.height,
				     SDL_YV12_OVERLAY,
				     screen);
  if ( yuv_overlay == NULL ) {
    fprintf(stderr, "SDL: Couldn't create SDL_yuv_overlay: %s\n", SDL_GetError());
    exit(1);
  }
  rect.x = 0;
  rect.y = 0;
  rect.w = ti.width;
  rect.h = ti.height;

  SDL_DisplayYUVOverlay(yuv_overlay, &rect);
  signal (SIGINT, sigint_handler);


  /* on to the main decode loop.  We assume in this example that audio
     and video start roughly together, and don't begin playback until
     we have a start frame for both */
  {
    int i,j;
    ogg_packet op;

    while(!got_sigint){
      
      /* we want a video and audio frame ready to go at all times.  If
	 we have to buffer incoming, buffer the compressed data (ie, let
	 ogg do the buffering) */
      while(vorbis_p && !audiobuf_fill){
	int ret;
	float **pcm;

	/* if there's pending, decoded audio, grab it */
	if((ret=vorbis_synthesis_pcmout(&vd,&pcm))>0){
	  int count=0;
	  int maxsamples=4096/vi.channels;
	  for(i=0;i<ret && i<maxsamples;i++)
	    for(j=0;j<vi.channels;j++){
	      int val=rint(pcm[j][i]*32767.f);
	      if(val>32767)val=32767;
	      if(val<-32768)val=-32768;
	      audiobuf[count++]=val;
	    }
	  vorbis_synthesis_read(&vd,i);
	  audiobuf_fill=i*vi.channels*2;

	  if(vd.granulepos>=0)
	    audiobuf_granulepos=vd.granulepos-ret+i;
	  else
	    audiobuf_granulepos+=i;


	  audiobuf_time=vorbis_granule_time(&vd,audiobuf_granulepos);
	}else{
	  
	  /* no pending audio; is there a pending packet to decode? */
	  if(ogg_stream_packetout(&vo,&op)>0){
	    if(vorbis_synthesis(&vb,&op)==0) /* test for success! */
	      vorbis_synthesis_blockin(&vd,&vb);
	  }else	/* we need more data; break out to suck in another page */
	    break;
	}
      }
      
      if(!videobuf_fill){
	/* theora is one in, one out... */
	if(ogg_stream_packetout(&to,&op)>0){
	  theora_decode_packetin(&td,&op);
	  videobuf_fill=1;

	  if(op.granulepos>=0)
	    videobuf_granulepos=op.granulepos;
	  else
	    videobuf_granulepos++;
	    
	  videobuf_time=theora_granule_time(&td,videobuf_granulepos);

	}
      }

      if(!videobuf_fill && !audiobuf_fill && feof(stdin))break;
      
      if(!videobuf_fill || !audiobuf_fill){
	/* no data yet for somebody.  Grab another page */
	int ret=buffer_data(&oy);
	while(ogg_sync_pageout(&oy,&og)>0){
	  if(ogg_stream_pagein(&to,&og))
	    if(vorbis_p)ogg_stream_pagein(&vo,&og);
	}
      }
      
      /* Top audio buffer off immediately; nonblocking write */
      audio_write_nonblocking();
      
      /* are we at or past time for this video frame? */
      if(videobuf_fill && videobuf_time<=get_time()){
	yuv_buffer yuv;
	theora_decode_YUVout(&td,&yuv);

	/* Lock SDL_yuv_overlay */
	if ( SDL_MUSTLOCK(screen) ) {
	  if ( SDL_LockSurface(screen) < 0 ) break;
	}
	if (SDL_LockYUVOverlay(yuv_overlay) < 0) break;

	/* let's draw the data (*yuv[3]) on a SDL screen (*screen) */
	/* deal with border stride */
	for(i=0;i<yuv.y_height;i++)
	  memcpy(yuv_overlay->pixels[0]+yuv.y_width*i, 
		 yuv.y+yuv.y_stride*i, 
		 yuv.y_width);
	for(i=0;i<yuv.uv_height;i++){
	  memcpy(yuv_overlay->pixels[1]+yuv.uv_width*i, 
		 yuv.v+yuv.uv_stride*i, 
		 yuv.uv_width);
	  memcpy(yuv_overlay->pixels[2]+yuv.uv_width*i, 
		 yuv.u+yuv.uv_stride*i, 
		 yuv.uv_width);
	}

	/* Unlock SDL_yuv_overlay */
	if ( SDL_MUSTLOCK(screen) ) {
	  SDL_UnlockSurface(screen);
	}
	SDL_UnlockYUVOverlay(yuv_overlay);


	/* Show, baby, show! */
	SDL_DisplayYUVOverlay(yuv_overlay, &rect);
	videobuf_fill=0;

      }

      
      /* block if there's nothing else to do */
      if(audiobuf_fill && videobuf_fill){
	
	


      }

    }

  }

  SDL_Quit();
}


