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
 * Modified by M. Piacentini http://www.tabuleiro.com
 * from the original Theora Alpha player_sample files 
 *
 * Modified to build on Windows and use PortAudio as the audio
 * and synchronization layer, calculating license.
 *
 * With SDL PortAudio it should be easy to compile on other platforms and
 * sound providers like DirectSound
 * just include the corresponding .c file (see PortAudio main documentation
 * for additional information)

 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <math.h>
#include <signal.h>
#include "theora/theora.h"
#include "vorbis/codec.h"
#include <SDL.h>
#include <windows.h>

#include "portaudio.h"
#include "pastreamio.h"

/* Ogg and codec state for demux/decode */
ogg_sync_state   oy; 
ogg_page         og;
ogg_stream_state vo;
ogg_stream_state to;
theora_info      ti;
theora_comment   tc;
theora_state     td;
vorbis_info      vi;
vorbis_dsp_state vd;
vorbis_block     vb;
vorbis_comment   vc;

int              theora_p=0;
int              vorbis_p=0;
int              stateflag=0;

FILE * infile = NULL;

/* SDL Video playback structures */
SDL_Surface *screen;
SDL_Overlay *yuv_overlay;
SDL_Rect rect;

/* single frame video buffering */
int          videobuf_ready=0;
ogg_int64_t  videobuf_granulepos=-1;
double       videobuf_time=0;

ogg_int64_t  audiobuf_granulepos=0; /* time position of last sample */

/*for portaudio  */
#define FRAMES_PER_BUFFER (256)
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)

PASTREAMIO_Stream  *aOutStream; /* our modified stream buffer*/
SAMPLE      *samples; /*local buffer for samples*/
double latency_sec = 0;

/*initial state of the audio stream*/
int isPlaying = 0;
PaError err;

DWORD initialticks, endticks;

/* Helper; just grab some more compressed bitstream and sync it for
   page extraction */
int buffer_data(ogg_sync_state *oy){
  char *buffer=ogg_sync_buffer(oy,4096);
  int bytes=fread(buffer,1,4096,infile);
  ogg_sync_wrote(oy,bytes);
  return(bytes);
}

static void usage(void){
  printf("Usage: splayer ogg_file\n\n"
	  "or drag and drop an ogg file over the .exe\n\n");
  exit(1);
}

static int open_audio(){
	/* this will open one circular audio stream*/
	/*build on top of portaudio routines*/
	/*implementation on fille pastreamio.c*/

	int        numSamples;
    int        numBytes;

	int minNumBuffers;
	int numFrames;

	minNumBuffers = 2 * Pa_GetMinNumBuffers( FRAMES_PER_BUFFER, vi.rate );
    numFrames = minNumBuffers * FRAMES_PER_BUFFER;
    numFrames = RoundUpToNextPowerOf2( numFrames );

    numSamples = numFrames * vi.channels;
    numBytes = numSamples * sizeof(SAMPLE);

    samples = (SAMPLE *) malloc( numBytes );

	/*store our latency calculation here*/
	latency_sec =  (double) numFrames / vi.rate / vi.channels;
	printf( "Latency: %.04f\n", latency_sec );

	err = OpenAudioStream( &aOutStream, vi.rate, PA_SAMPLE_TYPE,
                           (PASTREAMIO_WRITE | PASTREAMIO_STEREO) );
    if( err != paNoError ) goto error;
    return err;
error:
	CloseAudioStream( aOutStream );
    printf( "An error occured while opening the portaudio stream\n" );
    printf( "Error number: %d\n", err );
    printf( "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
	
}

static int start_audio(){
	err = StartAudioStream(aOutStream);
    if( err != paNoError ) goto error;
	
    return err;
error:
	CloseAudioStream( aOutStream );
    printf( "An error occured while opening the portaudio stream\n" );
    printf( "Error number: %d\n", err );
    printf( "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}

static int audio_close(void){
	err = CloseAudioStream( aOutStream );
	if( err != paNoError ) goto error;
	
	free(samples);
    return err;

error:
    Pa_Terminate();
    printf( "An error occured while closing the portaudio stream\n" );
    printf( "Error number: %d\n", err );
    printf( "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}


double get_time(){
  /*not entirely accurate with the WAVE OUT device, but good enough
  at this stage. Needs to be reworked to account for blank audio
  data written to the stream...*/
  double curtime = (double) (GetAudioStreamTime( aOutStream ) / vi.rate) -  latency_sec;
  if (curtime<0) curtime = 0;
  return  curtime  ;
}


static void open_video(void){
	/*taken from player_sample.c test file for theora alpha*/

  if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
    printf("Unable to init SDL: %s\n", SDL_GetError());
    exit(1);
  }
  
  screen = SDL_SetVideoMode(ti.width, ti.height, 0, SDL_SWSURFACE);
  if ( screen == NULL ) {
    printf("Unable to set %dx%d video: %s\n", 
	    ti.width,ti.height,SDL_GetError());
    exit(1);
  }
  
  yuv_overlay = SDL_CreateYUVOverlay(ti.width, ti.height,
				     SDL_YV12_OVERLAY,
				     screen);
  if ( yuv_overlay == NULL ) {
    printf("SDL: Couldn't create SDL_yuv_overlay: %s\n", 
	    SDL_GetError());
    exit(1);
  }
  rect.x = 0;
  rect.y = 0;
  rect.w = ti.width;
  rect.h = ti.height;

  SDL_DisplayYUVOverlay(yuv_overlay, &rect);
}

static void video_write(void){
	/*taken from player_sample.c test file for theora alpha*/
  int i;
  yuv_buffer yuv;
  theora_decode_YUVout(&td,&yuv);

  /* Lock SDL_yuv_overlay */
  if ( SDL_MUSTLOCK(screen) ) {
    if ( SDL_LockSurface(screen) < 0 ) return;
  }
  if (SDL_LockYUVOverlay(yuv_overlay) < 0) return;
  
  /* let's draw the data (*yuv[3]) on a SDL screen (*screen) */
  /* deal with border stride */
  /* reverse u and v for SDL */
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
  
}

/* dump the theora (or vorbis) comment header */
static int dump_comments(theora_comment *tc){
  int i, len;
  char *value;
  
  printf("Encoded by %s\n",tc->vendor);
  if(tc->comments){
    printf("theora comment header:\n");
    for(i=0;i<tc->comments;i++){
      if(tc->user_comments[i]){
        len=tc->comment_lengths[i];
      	value=malloc(len+1);
      	memcpy(value,tc->user_comments[i],len);
      	value[len]='\0';
      	printf("\t%s\n", value);
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
  if(vorbis_p)ogg_stream_pagein(&vo,&og);
  return 0;
}                                   


void parseHeaders(){
	/*extracted from player_sample.c test file for theora alpha*/
	ogg_packet op;
  /* Parse the headers */
  /* Only interested in Vorbis/Theora streams */
  while(!stateflag){
    int ret=buffer_data(&oy);
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
  
  /* we're expecting more header packets. */
  while((theora_p && theora_p<3) || (vorbis_p && vorbis_p<3)){
    int ret;
    
    /* look for further theora headers */
    while(theora_p && (theora_p<3) && (ret=ogg_stream_packetout(&to,&op))){
      if(ret<0){
      	printf("Error parsing Theora stream headers; corrupt stream?\n");
      	exit(1);
      }
      if(theora_p==1){
        if(theora_decode_comment(&tc,&op)){
          printf("Error parsing Theora stream headers; corrupt stream?\n");
          exit(1);
        }else{
          dump_comments(&tc);
          theora_p++;
          continue;
        }
      }
      if(theora_p==2){
        if(theora_decode_tables(&ti,&op)){
          printf("Error parsing Theora stream headers; corrupt stream?\n");
          exit(1);
        }
        theora_p++;
        /* fall through */
      }
      if(theora_p==3)break;
    }

    /* look for more vorbis header packets */  
    while(vorbis_p && (vorbis_p<3) && (ret=ogg_stream_packetout(&vo,&op))){
      if(ret<0){
	printf("Error parsing Vorbis stream headers; corrupt stream?\n");
	exit(1);
      }
      if(vorbis_synthesis_headerin(&vi,&vc,&op)){
	printf("Error parsing Vorbis stream headers; corrupt stream?\n");
	exit(1);
      }
      vorbis_p++;
      if(vorbis_p==3)break;
    }
    
    /* The header pages/packets will arrive before anything else we
       care about, or the stream is not obeying spec */
    
    if(ogg_sync_pageout(&oy,&og)>0){
      queue_page(&og); /* demux into the appropriate stream */
    }else{
      int ret=buffer_data(&oy);
      if(ret==0){
	fprintf(stderr,"End of file while searching for Vorbis headers.\n");
	exit(1);
      }
    }
  }

}

int main( int argc, char* argv[] ){
  
  int i,j, smresult;
  ogg_packet op;
  SDL_Event event;
  int hasdatatobuffer = 1;
  int playbackdone = 0;

  int frameNum=0;

  /*takes first argument as file to play*/
  /*this works better on Windows and is more convenient
  for drag and drop ogg files over the .exe*/

  if( argc != 2 )
  {
	usage();
	exit(0);
  }

  infile  = fopen( argv[1], "r" );

#ifdef _WIN32 /* We need to set the file to binary mode. */
   smresult = _setmode( _fileno(infile), _O_BINARY );
   if( smresult == -1 )
      printf( "Cannot set mode" );
   else
      printf( "Input file successfully changed to binary mode\n" );
#endif

  /* start up Ogg stream synchronization layer */
  ogg_sync_init(&oy);

  /* init supporting Vorbis structures needed in header parsing */
  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);

  /*parseHeaders*/
  parseHeaders();

  /* initialize decoders */
  if(theora_p){
    theora_decode_init(&td,&ti);
    printf("Ogg logical stream %x is Theora %dx%d %.02f fps video.\n",
	    to.serialno,ti.width,ti.height,
	    (double)ti.fps_numerator/ti.fps_denominator);
  }
  if(vorbis_p){
    vorbis_synthesis_init(&vd,&vi);
    vorbis_block_init(&vd,&vb);  
    printf("Ogg logical stream %x is Vorbis %d channel %d Hz audio.\n",
	    vo.serialno,vi.channels,vi.rate);
  }else{
    /* tear down the partial vorbis setup */
    vorbis_info_clear(&vi);
    vorbis_comment_clear(&vc);
  }
  /* open audio */
  if(vorbis_p)open_audio();
  /* open video */
  if(theora_p)open_video();

  initialticks = GetTickCount();
  /*our main loop*/
  while(1){

	SDL_Delay(5);

	if (playbackdone == 1 ) break;

	/*break out on SDL quit event*/
    if ( SDL_PollEvent ( &event ) )
    {
      if ( event.type == SDL_QUIT ) break ;
	  
    }

	/*get some audio data*/
    while(vorbis_p){
      int ret;
      float **pcm;
	  int count = 0;
	  int maxBytesToWrite;

	  /* is there pending audio? does it fit our circular buffer without blocking?*/
	  ret=vorbis_synthesis_pcmout(&vd,&pcm);
	  maxBytesToWrite = GetAudioStreamWriteable(aOutStream);

	  if (maxBytesToWrite<=FRAMES_PER_BUFFER)
	  {
		  /*break out until there is a significant amount of
		  data to avoid a series of small write operations*/
		  break;
      }
      /* if there's pending, decoded audio, grab it */
      if((ret>0)&&(maxBytesToWrite>0)){
	
	  for(i=0;i<ret && i<(maxBytesToWrite/vi.channels);i++)
	   for(j=0;j<vi.channels;j++){
		int val=(int)(pcm[j][i]*32767.f);
	    if(val>32767)val=32767;
	    if(val<-32768)val=-32768;
		samples[count]=val;
		count++;
	   }
	  WriteAudioStream( aOutStream, samples, i );
      vorbis_synthesis_read(&vd,i);

	  if(vd.granulepos>=0)
	   audiobuf_granulepos=vd.granulepos-ret+i;
	  else
	   audiobuf_granulepos+=i;

      }else{
	
	 /* no pending audio; is there a pending packet to decode? */
	  if(ogg_stream_packetout(&vo,&op)>0){
	    if(vorbis_synthesis(&vb,&op)==0) /* test for success! */
	     vorbis_synthesis_blockin(&vd,&vb);
	  }else	/* we need more data; break out to suck in another page */
	   break;
      }
    }/*end audio cycle*/
      
    while(theora_p && !videobuf_ready){
      /* get one video packet... */
      if(ogg_stream_packetout(&to,&op)>0){
	  theora_decode_packetin(&td,&op);

	  videobuf_granulepos=td.granulepos;
	  videobuf_time=theora_granule_time(&td,videobuf_granulepos);
	  /*update the frame counter*/
	  //printf("Frame\n");
	  frameNum++;

	  /*check if this frame time has not passed yet.
	  If the frame is late we need to decode additonal
	  ones and keep looping, since theora at
	  this stage needs to decode all frames*/

	   if(videobuf_time>= get_time())
		/*got a good frame, not late, ready to break out*/
		videobuf_ready=1;

      }else
	  /*already have a good frame in the buffer*/
	  {
		if (isPlaying == 1)
		{
	    printf("end\n");
	  	endticks = GetTickCount();
		isPlaying = 0;
		playbackdone = 1;
		}
	break;
	  }
    }
 
    if(stateflag && videobuf_ready && (videobuf_time<= get_time())){
		/*time to write our cached frame*/
	    video_write();
		videobuf_ready=0;

		/*if audio has not started (first frame) then start it*/
		if ((!isPlaying)&&(vorbis_p)){
			start_audio();
			isPlaying = 1;
		}
    }

	/*buffer compressed data every loop */
	//buffer_data(&oy);
	if (hasdatatobuffer)
	{
	  hasdatatobuffer=buffer_data(&oy);
      if(hasdatatobuffer==0){
		printf("Ogg buffering stopped, end of file reached.\n");

      }
	}


	if (ogg_sync_pageout(&oy,&og)>0){
		if(theora_p)ogg_stream_pagein(&to,&og);
  	    if(vorbis_p)ogg_stream_pagein(&vo,&og);
	}

  }

  /*show number of video frames decoded*/
  printf( "Frames decoded: %d\n", frameNum );
  printf( "Milliseconds: %d\n", endticks-initialticks);
  printf("Average fps (%.02f)\n", ((float) frameNum)/((endticks-initialticks)/1000.0f));

  /* tear it all down */
  fclose( infile );

  SDL_Quit();

  if(vorbis_p){
	audio_close();

    ogg_stream_clear(&vo);
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi); 
  }
  if(theora_p){
    ogg_stream_clear(&to);
    theora_clear(&td);
  }
  ogg_sync_clear(&oy);

  printf("\r                                                              "
	  "\nDone.\n");
  return(0);

}


