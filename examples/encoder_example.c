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

  function: example encoder application; makes an Ogg Theora/Vorbis 
            file from YUV4MPEG2 and WAV input
  last mod: $Id: encoder_example.c,v 1.6 2002/09/25 10:01:52 xiphmont Exp $

 ********************************************************************/

#define _GNU_SOURCE
#define _REENTRANT
#define _LARGEFILE_SOURCE 
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include "theora/theora.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisenc.h"

const char *optstring = "a:A:v:V:";
struct option options [] = {
  {"audio-rate-target",required_argument,NULL,'A'},
  {"video-rate-target",required_argument,NULL,'V'},
  {"audio-quality",required_argument,NULL,'a'},
  {"video-quality",required_argument,NULL,'v'},
  {NULL,0,NULL,0}
};

/* You'll go to Hell for using globals. */

FILE *audio=NULL;
FILE *video=NULL;

int audio_ch=0; 
int audio_hz=0; 

float audio_q=.1;
int audio_r=-1;

int video_x=0;
int video_y=0;
int video_x_adj=0;
int video_y_adj=0;
int video_hzn=0;
int video_hzd=0;
int video_an=0;
int video_ad=0;

int video_r=-1;
int video_q=16;

static void usage(void){
  fprintf(stderr,
	  "Usage: encoder_example [options] [audio_file] video_file\n\n"
	  "Options: \n\n"
	  "  -A --audio-rate-target <n>  bitrate target for Vorbis audio;\n"
	  "                              use -a and not -A if at all possible,\n"
	  "                              as -a gives higher quality for a given\n"
	  "                              bitrate.\n\n"
          "  -V --video-rate-target <n>  bitrate target for Theora video\n\n"
	  "  -a --audio-quality <n>      Vorbis quality selector from -1 to 10\n"
	  "                              (-1 yields smallest files but lowest\n"
	  "                              fidelity; 10 yields highest fidelity\n"
	  "                              but large files. '2' is a reasonable\n"
	  "                              default).\n\n"
	  "   -v --video-quality <n>     Theora quality selector fro 0 to 10\n"
	  "                              (0 yields smallest files but lowest\n"
	  "                              video quality. 10 yields highest\n"
	  "                              fidelity but large files).\n\n"
	  "encoder_example accepts only uncompressed RIFF WAV format audio and\n"
	  "YUV4MPEG2 uncompressed video.\n\n");
  exit(1);
}

static void id_file(char *f){
  FILE *test;
  unsigned char buffer[80];
  int ret;

  /* open it, look for magic */

  if(!strcmp(f,"-")){
    /* stdin */
    test=stdin;
  }else{
    test=fopen(f,"rb");
    if(!test){
      fprintf(stderr,"Unable to open file %s.\n",f);
      exit(1);
    }
  }

  ret=fread(buffer,1,4,test);
  if(ret<4){
    fprintf(stderr,"EOF determining file type of file %s.\n",f);
    exit(1);
  }

  if(!memcmp(buffer,"RIFF",4)){
    /* possible WAV file */

    if(audio){
      /* umm, we already have one */
      fprintf(stderr,"Multiple RIFF WAVE files specified on command line.\n");
      exit(1);
    }

    /* Parse the rest of the header */

    ret=fread(buffer,1,4,test);
    ret=fread(buffer,1,4,test);
    if(ret<4)goto riff_err;
    if(!memcmp(buffer,"WAVE",4)){
      
      while(!feof(test)){
	ret=fread(buffer,1,4,test);
	if(ret<4)goto riff_err;
	if(!memcmp("fmt",buffer,3)){

	  /* OK, this is our audio specs chunk.  Slurp it up. */

	  ret=fread(buffer,1,20,test);
	  if(ret<20)goto riff_err;

	  if(memcmp(buffer+4,"\001\000",2)){
	    fprintf(stderr,"The WAV file %s is in a compressed format; "
		    "can't read it.\n",f);
	    exit(1);
	  }

	  audio=test;
	  audio_ch=buffer[6]+(buffer[7]<<8);
	  audio_hz=buffer[8]+(buffer[9]<<8)+
	    (buffer[10]<<16)+(buffer[11]<<24);

	  if(buffer[18]+(buffer[19]<<8)!=16){
	    fprintf(stderr,"Can only read 16 bit WAV files for now.\n",f);
	    exit(1);
	  }
	  
	  /* Now, align things to the beginning of the data */
	  /* Look for 'dataxxxx' */
	  while(!feof(test)){
	    ret=fread(buffer,1,4,test);
	    if(ret<4)goto riff_err;
	    if(!memcmp("data",buffer,4)){
	      /* We're there.  Ignore the declared size for now. */
	      ret=fread(buffer,1,4,test);
	      if(ret<4)goto riff_err;

	      fprintf(stderr,"File %s is 16 bit %d channel %d Hz RIFF WAV audio.\n",
		      f,audio_ch,audio_hz);

	      return;
	    }
	  }
	}
      }
    }
    
    fprintf(stderr,"Couldn't find WAVE data in RIFF file %s.\n",f);
    exit(1);

  }
  if(!memcmp(buffer,"YUV4",4)){
    /* possible YUV2MPEG2 format file */
    /* read until newline, or 80 cols, whichever happens first */
    int i;
    for(i=0;i<79;i++){
      ret=fread(buffer+i,1,1,test);
      if(ret<1)goto yuv_err;
      if(buffer[i]=='\n')break;
    }
    if(i==79){
      fprintf(stderr,"Error parsing %s header; not a YUV2MPEG2 file?\n",f);
    }
    buffer[i]='\0';

    if(!memcmp(buffer,"MPEG",4)){
      char interlace;
      int aspectn;
      int aspectd;

      if(video){
	/* umm, we already have one */
	fprintf(stderr,"Multiple video files specified on command line.\n");
	exit(1);
      }

      if(buffer[4]!='2'){
	fprintf(stderr,"Incorrect YUV input file version; YUV4MPEG2 required.\n");
      }
      
      ret=sscanf(buffer,"MPEG2 W%d H%d F%d:%d I%c A%d:%d",
		 &video_x,&video_y,&video_hzn,&video_hzd,&interlace,
		 &video_an,&video_ad);
      if(ret<7){
	fprintf(stderr,"Error parsing YUV4MPEG2 header in file %s.\n",f);
	exit(1);
      }

      if(interlace!='p'){
	fprintf(stderr,"Input video is interlaced; Theora handles only progressive scan\n",f);
	exit(1);
      }

      video=test;
      
      fprintf(stderr,"File %s is %dx%d %.02f fps YUV12 video.\n",
	      f,video_x,video_y,(double)video_hzn/video_hzd);
      
      return;
    }
  }
  fprintf(stderr,"Input file %s is niether a WAV nor YUV4MPEG2 file.\n",f);
  exit(1);

 riff_err:
  fprintf(stderr,"EOF parsing RIFF file %s.\n",f);
  exit(1);
 yuv_err:
  fprintf(stderr,"EOF parsing YUV4MPEG2 file %s.\n",f);
  exit(1);
  
}

int spinner=0;
char *spinascii="|/-\\";
void spinnit(void){
  spinner++;
  if(spinner==4)spinner=0;
  fprintf(stderr,"\r%c",spinascii[spinner]);
}

int fetch_and_process_audio(FILE *audio,ogg_page *audiopage,
			    ogg_stream_state *vo,
			    vorbis_dsp_state *vd,
			    vorbis_block *vb,
			    int audioflag){
  ogg_packet op;
  int i,j;

  while(audio && !audioflag){
    /* process any audio already buffered */
    spinnit();
    if(ogg_stream_pageout(vo,audiopage)>0) return 1;
    if(ogg_stream_eos(vo))return 0;

    {
      /* read and process more audio */
      signed char readbuffer[4096];
      int toread=4096/2/audio_ch;
      int bytesread=fread(readbuffer,1,toread*2*audio_ch,audio);
      int sampread=bytesread/2/audio_ch;
      float **vorbis_buffer;
      int count=0;
      
      if(bytesread<=0){
	/* end of file.  this can be done implicitly, but it's
	   easier to see here in non-clever fashion.  Tell the
	   library we're at end of stream so that it can handle the
	   last frame and mark end of stream in the output properly */
	vorbis_analysis_wrote(vd,0);
      }else{
	vorbis_buffer=vorbis_analysis_buffer(vd,sampread);
	/* uninterleave samples */
	for(i=0;i<sampread;i++){
	  for(j=0;j<audio_ch;j++){
	    vorbis_buffer[j][i]=((readbuffer[count+1]<<8)|
				 (0x00ff&(int)readbuffer[count]))/32768.f;
	    count+=2;
	  }
	}
	
	vorbis_analysis_wrote(vd,sampread);
	
      }
      
      while(vorbis_analysis_blockout(vd,vb)==1){
	
	/* analysis, assume we want to use bitrate management */
	vorbis_analysis(vb,NULL);
	vorbis_bitrate_addblock(vb);
	
	/* weld packets into the bitstream */
	while(vorbis_bitrate_flushpacket(vd,&op))
	  ogg_stream_packetin(vo,&op);
	
      }
    }
  }
  
  return audioflag;
}

int fetch_and_process_video(FILE *video,ogg_page *videopage,
			    ogg_stream_state *to,
			    theora_state *td,
			    int videoflag){
  /* You'll go to Hell for using static variables */
  static int          state=-1;
  static signed char *yuvframe[2];
  yuv_buffer          yuv;
  ogg_packet          op;
  int i;

  if(state==-1){
    /* initialize the double frame buffer */
    yuvframe[0]=malloc(video_x*video_y*3/2);
    yuvframe[1]=malloc(video_x*video_y*3/2);

    state=0;
  }

  /* is there a video page flushed?  If not, work until there is. */
  while(!videoflag){
    spinnit();
    
    if(ogg_stream_pageout(to,videopage)>0) return 1;
    if(ogg_stream_eos(to)) return 0;

    {
      /* read and process more video */
      /* video strategy reads one frame ahead so we know when we're
	 at end of stream and can mark last video frame as such
	 (vorbis audio has to flush one frame past last video frame
	 due to overlap and thus doesn't need this extra work */
      
      /* have two frame buffers full (if possible) before
	 proceeding.  after first pass and until eos, one will
	 always be full when we get here */

      for(i=state;i<2;i++){
	char frame[6];
	int ret=fread(frame,1,6,video);
	
	if(ret<6)break;
	if(memcmp(frame,"FRAME\n",6)){
	  fprintf(stderr,"Loss of framing in YUV input data\n");
	  exit(1);
	}

	ret=fread(yuvframe[i],1,video_x*video_y*3/2,video);
	if(ret!=video_x*video_y*3/2)break;
	
	state++;
      }

      if(state<1){
	/* can't get here unless YUV4MPEG stream has no video */
	fprintf(stderr,"Video input contains no frames.\n");
	exit(1);
      }
      
      /* Theora is a one-frame-in,one-frame-out system; subit a frame
         for compression and pull out the packet */
      
      /* center crop the input to a /16 size */
      {
	int x_adj= (video_x-video_x_adj)/2;
	int y_adj= (video_y-video_y_adj)/2;

	yuv.y_width=video_x_adj;
	yuv.y_height=video_y_adj;
	yuv.y_stride=video_x;
	
	yuv.uv_width=video_x_adj/2;
	yuv.uv_height=video_y_adj/2;
	yuv.uv_stride=video_x/2;
	
	yuv.y= yuvframe[0]+ 
	  video_x*y_adj + x_adj;
	yuv.u= yuvframe[0]+ video_x*video_y +
	  (video_x/2)*(y_adj/2) + x_adj/2;
	yuv.v= yuvframe[0]+ video_x*video_y*5/4 +
	  (video_x/2)*(y_adj/2) + x_adj/2;
      }
      
      theora_encode_YUVin(td,&yuv);

      /* if there's only one frame, it's the last in the stream */
      if(state<2)
	theora_encode_packetout(td,1,&op);
      else
	theora_encode_packetout(td,0,&op);
      
      ogg_stream_packetin(to,&op);
      
      {
	signed char *temp=yuvframe[0];
	yuvframe[0]=yuvframe[1];
	yuvframe[1]=temp;
	state--;
      }
      
    }
  }
  return videoflag;
}

int main(int argc,char *argv[]){
  int c,long_option_index,ret,i,j;

  ogg_stream_state to; /* take physical pages, weld into a logical
                           stream of packets */
  ogg_stream_state vo; /* take physical pages, weld into a logical
                           stream of packets */
  ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet       op; /* one raw packet of data for decode */

  theora_state     td;
  theora_info      ti;
  
  vorbis_info      vi; /* struct that stores all the static vorbis bitstream
                          settings */
  vorbis_comment   vc; /* struct that stores all the user comments */

  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */

  int audioflag=0;
  int videoflag=0;
  int akbps=0;
  int vkbps=0;

  ogg_int64_t audio_bytesout=0;
  ogg_int64_t video_bytesout=0;
  double timebase;

#ifdef _WIN32 /* We need to set stdin/stdout to binary mode. Damn windows. */
  /* if we were reading/writing a file, it would also need to in
     binary mode, eg, fopen("file.wav","wb"); */
  /* Beware the evil ifdef. We avoid these where we can, but this one we 
     cannot. Don't add any more, you'll probably go to hell if you do. */
  _setmode( _fileno( stdin ), _O_BINARY );
  _setmode( _fileno( stdout ), _O_BINARY );
#endif

  while((c=getopt_long(argc,argv,optstring,options,&long_option_index))!=EOF){
    switch(c){
    case 'a':
      audio_q=atof(optarg)*.099;
      if(audio_q<-.1 || audio_q>1){
	fprintf(stderr,"Illegal audio quality (choose -1 through 10)\n");
	exit(1);
      }
      audio_r=-1;
      break; 
      
    case 'v':
      video_q=rint(atof(optarg)*6.3);
      if(video_q<0 || video_q>63){
	fprintf(stderr,"Illegal video quality (choose 0 through 10)\n");
	exit(1);
      }
      video_r=0;
      break;
     
    case 'A':
      audio_r=atof(optarg)*1000;
      if(audio_q<0){
	fprintf(stderr,"Illegal audio quality (choose > 0 please)\n");
	exit(1);
      }
      audio_q=-99;
      break; 

    case 'V':
      video_r=rint(atof(optarg)*1000);
      if(video_r<45000 || video_r>2000000){
	fprintf(stderr,"Illegal video bitrate (choose 45kbps through 2000kbps)\n");
	exit(1);
      }
      video_q=0;
     break;
    default:
      usage();
    }
  }

  while(optind<argc){
    /* assume that anything following the options must be a filename */
    id_file(argv[optind]);
    optind++;
  }

  /* yayness.  Set up Ogg output stream */
  srand(time(NULL));
  ogg_stream_init(&vo,rand());
  ogg_stream_init(&to,rand()); /* oops, add one ot the above */

  /* Set up Theora encoder */
  if(!video){
    fprintf(stderr,"No video files submitted for compression?\n");
    exit(1);
  }
  /* Theora has a divisible-by-sixteen restriction for encoding */
  video_x_adj=(video_x>>4)<<4;
  video_y_adj=(video_y>>4)<<4;
  
  ti.width=video_x_adj;
  ti.height=video_y_adj;
  ti.fps_numerator=video_hzn;
  ti.fps_denominator=video_hzd;
  ti.aspect_numerator=video_an;
  ti.aspect_denominator=video_ad;
  ti.target_bitrate=video_r;
  ti.quality=video_q;
  
  ti.dropframes_p=0;
  ti.quick_p=1;
  ti.keyframe_auto_p=1;
  ti.keyframe_frequency=64;
  ti.keyframe_frequency_force=64;
  ti.keyframe_data_target_bitrate=video_r*1.5;
  ti.keyframe_auto_threshold=80;
  ti.keyframe_mindistance=8;
  ti.noise_sensitivity=1;

  theora_encode_init(&td,&ti);

  /* initialize Vorbis too, assuming we have audio to compress. */
  if(audio){
    vorbis_info_init(&vi);
    if(audio_q>-99)
      ret = vorbis_encode_init_vbr(&vi,audio_ch,audio_hz,audio_q);
    else
      ret = vorbis_encode_init(&vi,audio_ch,audio_hz,-1,audio_r,-1);
    if(ret){
      fprintf(stderr,"The Vorbis encoder could not set up a mode according to\n"
	      "the requested quality or bitrate.\n\n");
      exit(1);
    }
    
    vorbis_comment_init(&vc);
    vorbis_analysis_init(&vd,&vi);
    vorbis_block_init(&vd,&vb);
  }

  /* get the bitstream header pages; one for theora, three for vorbis */
  theora_encode_header(&td,&op);
  ogg_stream_packetin(&to,&op); /* first packet, so it's flushed
                                   immediately */
  if(ogg_stream_pageout(&to,&og)!=1){
    /*can't get here unless Ogg is borked */
    fprintf(stderr,"Internal Ogg library error.\n");
    exit(1);
  }
  fwrite(og.header,1,og.header_len,stdout);
  fwrite(og.body,1,og.body_len,stdout);

  if(audio){
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;
    
    vorbis_analysis_headerout(&vd,&vc,&header,&header_comm,&header_code);
    ogg_stream_packetin(&vo,&header); /* automatically placed in its own
                                         page */
    ogg_stream_packetin(&vo,&header_comm);
    ogg_stream_packetin(&vo,&header_code);
    
    /* This ensures the actual
     * audio data will start on a new page, as per spec
     */
    while(1){
      int result=ogg_stream_flush(&vo,&og);
      if(result<0){
	/* can't get here */
	fprintf(stderr,"Internal Ogg library error.\n");
	exit(1);
      }
      if(result==0)break;
      fwrite(og.header,1,og.header_len,stdout);
      fwrite(og.body,1,og.body_len,stdout);
    }
  }

  /* setup complete.  Raw processing loop */
  fprintf(stderr,"Compressing....\n");
  while(1){
    ogg_page audiopage;
    ogg_page videopage;

    /* is there an audio page flushed?  If not, fetch one if possible */
    audioflag=fetch_and_process_audio(audio,&audiopage,&vo,&vd,&vb,audioflag);

    /* is there a video page flushed?  If not, fetch one if possible */
    videoflag=fetch_and_process_video(video,&videopage,&to,&td,videoflag);
    
    /* no pages of either?  Must be end of stream. */
    if(!audioflag && !videoflag)break; 

    /* which is earlier; the end of the audio page or the end of the
       video page? Flush the earlier to stream */
    {
      int audio_or_video=-1;
      double audiotime=
	audioflag?vorbis_granule_time(&vd,ogg_page_granulepos(&audiopage)):-1;
      double videotime=
	videoflag?theora_granule_time(&td,ogg_page_granulepos(&videopage)):-1;

      if(!audioflag){
	audio_or_video=1;
      } else if(!videoflag) {
	audio_or_video=0;
      } else {
	if(audiotime<videotime)
	  audio_or_video=0;
	else
	  audio_or_video=1;
      }

      if(audio_or_video==1){
	/* flush a video page */
	video_bytesout+=fwrite(videopage.header,1,videopage.header_len,stdout);
	video_bytesout+=fwrite(videopage.body,1,videopage.body_len,stdout);
	videoflag=0;
	timebase=videotime;
	
      }else{
	/* flush an audio page */
	audio_bytesout+=fwrite(audiopage.header,1,audiopage.header_len,stdout);
	audio_bytesout+=fwrite(audiopage.body,1,audiopage.body_len,stdout);
	audioflag=0;
	timebase=audiotime;
      }
      {
	int hundredths=timebase*100-(long)timebase*100;
	int seconds=(long)timebase%60;
	int minutes=((long)timebase/60)%60;
	int hours=(long)timebase/3600;
	
	if(audio_or_video)
	  vkbps=rint(video_bytesout*8./timebase*.001);
	else
	  akbps=rint(audio_bytesout*8./timebase*.001);
	
	fprintf(stderr,
		"\r      %d:%02d:%02d.%02d audio: %dkbps video: %dkbps                 ",
		hours,minutes,seconds,hundredths,akbps,vkbps);
      }
    }
	    
  }

  /* clear out state */

  if(audio){
    ogg_stream_clear(&vo);
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
  }
  if(video){
    ogg_stream_clear(&to);
    theora_clear(&td);
  }

  fprintf(stderr,"\r   \ndone.\n\n");

  return(0);

}
