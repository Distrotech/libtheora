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
  last mod: $Id: encoder_example.c,v 1.1 2002/09/23 09:15:03 xiphmont Exp $

 ********************************************************************/

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

float audio_q=-99;
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
int video_q=-1;

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
  char buffer[80];
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

int main(int argc,char *argv[]){
  int c,long_option_index,ret;

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

  yuv_buffer       yuv;
  
  
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
      audio_q=atof(optarg)*.1;
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
  ogg_stream_init(&to,rand());

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
  ti.target_bitrate=video_r;
  ti.quality=video_r;
  
  ti.droppedframes_p=0;
  ti.quickcompress_p=1;
  ti.keyframe_auto_p=1;
  ti.keyframe_frequency=128;
  ti.keyframe_frequency_force=128;
  ti.keyframe_data_target_bitrate=video_r*1.5;
  ti.keyframe_auto_threshold=80;
  ti.keyframe_mindistance=8;
  ti.noise_sensitivity=1;

  theora_encode_init(&td,&ti);

  /* initialize Vorbis too, assuming we have audio to compress. */
  if(audio){
    vorbis_info_init(&vi);
    if(audio_q>-99)
      ret = vorbis_encode_init_vbr(&vi,2,44100,.4);
    else
      ret = vorbis_encode_init(&vi,2,44100,-1,128000,-1);
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

  /* setup complete.  Raw processing loop! */
  while(1){

    /* is there an audio page ready to flush?  If not, work until there is. */



    /* is there a video page ready to flush?  If not, work until there is. */


    /* no pages of either?  Must be end of stream. */
    if(ogg_stream_pageout(&to,&og)<=0 && ogg_stream_pageout(&vo,&og)<=0)break; 

    /* which is earlier; the end of the audio page or the end of the
       video page? Flush the earlier to stream */

  }

  fprintf(stderr,
	  "\r                                                            \n"
	  "done.\n\n");

  return(0);

}
