/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: example encoder application; makes an Ogg Theora
            file from a sequence of png images
  last mod: $Id$
             based on code from Vegard Nossum
  
 ********************************************************************/

#define _GNU_SOURCE
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <png.h>
#include <ogg/ogg.h>
#include "theora/theora.h"


#define PROGRAM_NAME  "png2theora"
#define PROGRAM_VERSION  "1.0"

static const char *option_output = NULL;
static int video_fps_numerator = 24;
static int video_fps_denominator = 1;
static int video_aspect_numerator = 0;
static int video_aspect_denominator = 0;
static int video_rate = 0;
static int video_quality = 63;

static int theora_initialized = 0;

static FILE *ogg_fp = NULL;
static ogg_stream_state ogg_os;

static theora_state theora_td;
static theora_info theora_ti;

static char *input_filter;

const char *optstring = "o:h:v:V:s:S:f:F:";
struct option options [] = {
 {"output",required_argument,NULL,'o'},
 {"help",optional_argument,NULL,'h'},
 {"video-rate-target",required_argument,NULL,'V'},
 {"video-quality",required_argument,NULL,'v'},
 {"aspect-numerator",optional_argument,NULL,'s'},
 {"aspect-denominator",optional_argument,NULL,'S'},
 {"framerate-numerator",optional_argument,NULL,'f'},
 {"framerate-denominator",optional_argument,NULL,'F'},
 {NULL,0,NULL,0}
};

static void usage(void){
  fprintf(stderr,
          "%s %s\n"
          "Usage: %s [options] input\n\n"
          "Output is parsed by scanf and represents a list of files, i.e.\n"
          "  file-%%06d.png to look for files file000001.png to file9999999.png \n\n"
          "Options: \n\n"
          "  -o --output <filename.ogg>     file name for encoded output;\n"
          "                                 If this option is not given, the\n"
          "                                 compressed data is sent to stdout.\n\n"
          "  -V --video-rate-target <n>     bitrate target for Theora video\n\n"
          "  -v --video-quality <n>         Theora quality selector fro 0 to 10\n"
          "                                 (0 yields smallest files but lowest\n"
          "                                 video quality. 10 yields highest\n"
          "                                 fidelity but large files).\n\n"
          "   -s --aspect-numerator <n>     Aspect ratio numerator, default is 0\n"
          "   -S --aspect-denominator <n>   Aspect ratio denominator, default is 0\n"
          "   -f --framerate-numerator <n>  Frame rate numerator\n"
          "   -F --framerate-denominator <n>Frame rate denominator\n"
          "                                 The frame rate nominator divided by this\n"
          "                                 determinates the frame rate in units per tick\n"
          ,PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_NAME
  );
  exit(0);
}


static int
theora_open(const char *pathname)
{
  ogg_packet op;
  ogg_page og;
  theora_comment tc;

  ogg_fp = fopen(pathname, "wb");
  if(!ogg_fp) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "couldn't open output file");
    return 1;
  }

  if(ogg_stream_init(&ogg_os, rand())) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "couldn't create ogg stream state");
    return 1;
  }

  if(theora_encode_init(&theora_td, &theora_ti)) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "couldn't initialize theora encoding");
    return 1;
  }

  theora_encode_header(&theora_td, &op);
  ogg_stream_packetin(&ogg_os, &op);
  if(ogg_stream_pageout(&ogg_os, &og)) {
    fwrite(og.header, og.header_len, 1, ogg_fp);
    fwrite(og.body, og.body_len, 1, ogg_fp);
  }

  theora_comment_init(&tc);
  theora_encode_comment(&tc, &op);
  ogg_stream_packetin(&ogg_os, &op);
  if(ogg_stream_pageout(&ogg_os, &og)) {
    fwrite(og.header, og.header_len, 1, ogg_fp);
    fwrite(og.body, og.body_len, 1, ogg_fp);
  }

  theora_encode_tables(&theora_td, &op);
  ogg_stream_packetin(&ogg_os, &op);
  if(ogg_stream_pageout(&ogg_os, &og)) {
    fwrite(og.header, og.header_len, 1, ogg_fp);
    fwrite(og.body, og.body_len, 1, ogg_fp);
  }

  if(ogg_stream_flush(&ogg_os, &og)) {
    fwrite(og.header, og.header_len, 1, ogg_fp);
    fwrite(og.body, og.body_len, 1, ogg_fp);
  }

  return 0;
}

static int
theora_write_frame(unsigned long w, unsigned long h, unsigned char *yuv)
{
  yuv_buffer yuv_buf;
  ogg_packet op;
  ogg_page og;

  unsigned long yuv_w;
  unsigned long yuv_h;

  unsigned char *yuv_y;
  unsigned char *yuv_u;
  unsigned char *yuv_v;

  unsigned int x;
  unsigned int y;
  
  /* Must hold: yuv_w >= w */
  yuv_w = (w + 15) & ~15;

  /* Must hold: yuv_h >= h */
  yuv_h = (h + 15) & ~15;

  yuv_y = malloc(yuv_w * yuv_h);
  yuv_u = malloc(yuv_w * yuv_h / 4);
  yuv_v = malloc(yuv_w * yuv_h / 4);

  yuv_buf.y_width = yuv_w;
  yuv_buf.y_height = yuv_h;
  yuv_buf.y_stride = yuv_w;
  yuv_buf.uv_width = yuv_w >> 1;
  yuv_buf.uv_height = yuv_h >> 1;
  yuv_buf.uv_stride = yuv_w >> 1;
  yuv_buf.y = yuv_y;
  yuv_buf.u = yuv_u;
  yuv_buf.v = yuv_v;

  for(y = 0; y < yuv_h; y++) {
    for(x = 0; x < yuv_w; x++) {
      yuv_y[x + y * yuv_w] = 0;
    }
  }

  for(y = 0; y < yuv_h; y += 2) {
    for(x = 0; x < yuv_w; x += 2) {
      yuv_u[(x >> 1) + (y >> 1) * (yuv_w >> 1)] = 0;
      yuv_v[(x >> 1) + (y >> 1) * (yuv_w >> 1)] = 0;
    }
  }

  for(y = 0; y < h; y++) {
    for(x = 0; x < w; x++) {
      yuv_y[x + y * yuv_w] = yuv[3 * (x + y * w) + 0];
    }
  }

  for(y = 0; y < h; y += 2) {
    for(x = 0; x < w; x += 2) {
      yuv_u[(x >> 1) + (y >> 1) * (yuv_w >> 1)] =
        yuv[3 * (x + y * w) + 1];
      yuv_v[(x >> 1) + (y >> 1) * (yuv_w >> 1)] =
        yuv[3 * (x + y * w) + 2];
    }
  }

  if(theora_encode_YUVin(&theora_td, &yuv_buf)) {
    fprintf(stderr, "%s: error: could not encode frame\n",
      option_output);
    return 1;
  }

  if(!theora_encode_packetout(&theora_td, 0, &op)) {
    fprintf(stderr, "%s: error: could not read packets\n",
      option_output);
    return 1;
  }

  ogg_stream_packetin(&ogg_os, &op);
  if(ogg_stream_pageout(&ogg_os, &og)) {
    fwrite(og.header, og.header_len, 1, ogg_fp);
    fwrite(og.body, og.body_len, 1, ogg_fp);
  }

  free(yuv_y);
  free(yuv_u);
  free(yuv_v);

  return 0;
}

static void
theora_close(void)
{
  ogg_packet op;
  ogg_page og;

  if (theora_initialized) {
    theora_encode_packetout(&theora_td, 1, &op);
    if(ogg_stream_pageout(&ogg_os, &og)) {
      fwrite(og.header, og.header_len, 1, ogg_fp);
      fwrite(og.body, og.body_len, 1, ogg_fp);
    }
  
    theora_info_clear(&theora_ti);
    theora_clear(&theora_td);
  
    fflush(ogg_fp);
    fclose(ogg_fp);
  }
  
  ogg_stream_clear(&ogg_os);
}

static unsigned char
clamp(double d)
{
  if(d < 0)
    return 0;

  if(d > 255)
    return 255;

  return d;
}

static void
rgb_to_yuv(png_bytep *png,
           unsigned char *yuv,
           unsigned int w, unsigned int h)
{
  unsigned int x;
  unsigned int y;

  for(y = 0; y < h; y++) {
    for(x = 0; x < w; x++) {
      png_byte r;
      png_byte g;
      png_byte b;

      r = png[y][3 * x + 0];
      g = png[y][3 * x + 1];
      b = png[y][3 * x + 2];

      /* XXX: Cringe. */
      yuv[3 * (x + w * y) + 0] = clamp(
        0.299 * r
        + 0.587 * g
        + 0.114 * b);
      yuv[3 * (x + w * y) + 1] = clamp((0.436 * 255
        - 0.14713 * r
        - 0.28886 * g
        + 0.436 * b) / 0.872);
      yuv[3 * (x + w * y) + 2] = clamp((0.615 * 255
        + 0.615 * r
        - 0.51499 * g
        - 0.10001 * b) / 1.230);
    }
  }
}

static int
png_read(const char *pathname, unsigned int *w, unsigned int *h, unsigned char **yuv)
{
  FILE *fp;
  unsigned char header[8];
  png_structp png_ptr;
  png_infop info_ptr;
  png_infop end_ptr;
  png_bytep *row_pointers;

  fp = fopen(pathname, "rb");
  if(!fp) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, strerror(errno));
    return 1;
  }

  fprintf(stderr, "%s\n", pathname);

  fread(header, 1, 8, fp);
  if(png_sig_cmp(header, 0, 8)) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "not a PNG");
    return 1;
  }

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
    NULL, NULL, NULL);
  if(!png_ptr) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "couldn't create png read structure");
    return 1;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if(!info_ptr) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "couldn't create png info structure");
    /* XXX: cleanup */
    return 1;
  }

  end_ptr = png_create_info_struct(png_ptr);
  if(!end_ptr) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "couldn't create png info structure");
    /* XXX: cleanup */
    return 1;
  }

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16, NULL);

  row_pointers = png_get_rows(png_ptr, info_ptr);

  *w = png_get_image_width(png_ptr, info_ptr);
  *h = png_get_image_height(png_ptr, info_ptr);
  *yuv = malloc(*w * *h * 3);
  rgb_to_yuv(row_pointers, *yuv, *w, *h);

  png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);

  fclose(fp);
  return 0;
}

static int include_files (const struct dirent *de)
{
  char name[1024];
  int number = -1;
  sscanf(de->d_name, input_filter, &number);
  sprintf(name, input_filter, number);
  return !strcmp(name, de->d_name);
}

int
main(int argc, char *argv[])
{
  int c,long_option_index;
  int i, n;
  char *input_mask;
  char *input_directory;
  char *scratch;
  struct dirent **png_files;
  
  while(1) {

    c=getopt_long(argc,argv,optstring,options,&long_option_index);
    if(c == EOF)
      break;

    switch(c) {
      case 'h':
        usage();
        break;
      case 'o':
        option_output = optarg;
        break;;
      case 'v':
        video_quality=rint(atof(optarg)*6.3);
        if(video_quality<0 || video_quality>63){
          fprintf(stderr,"Illegal video quality (choose 0 through 10)\n");
          exit(1);
        }
        video_rate=0;
        break;
      case 'V':
        video_rate=rint(atof(optarg)*1000);
        if(video_rate<45000 || video_rate>2000000){
          fprintf(stderr,"Illegal video bitrate (choose 45kbps through 2000kbps)\n");
          exit(1);
        }
        video_quality=0;
       break;
     case 's':
       video_aspect_numerator=rint(atof(optarg));
       break;
     case 'S':
       video_aspect_denominator=rint(atof(optarg));
       break;
     case 'f':
       video_fps_numerator=rint(atof(optarg));
       break;
     case 'F':
       video_fps_denominator=rint(atof(optarg));   
     default:
        usage();
        break;
      }
  }

  if(argc < 3) {
    usage();
  }

  input_mask = argv[optind];
  /* dirname and basename must operate on scratch strings */
  scratch = strdup(input_mask);
  input_directory = strdup(dirname(scratch));
  free(scratch);
  scratch = strdup(input_mask);
  input_filter = strdup(basename(scratch));
  free(scratch);

#ifdef DEBUG
  fprintf(stderr, "scanning %s with filter '%s'\n",
	input_directory, input_filter);
#endif
  n = scandir (input_directory, &png_files, include_files, alphasort);
  for(i=0;i< n;i++) {
    unsigned int w;
    unsigned int h;
    unsigned char *yuv;
    char input_png[1024];

    sprintf(input_png, "%s/%s", input_directory, png_files[i]->d_name);
    
    if(png_read(input_png, &w, &h, &yuv)) {
      fprintf(stderr, "could not read %s\n", input_png);
      theora_close();
      exit(1);
    }
    
    if(!theora_initialized) {
      theora_info_init(&theora_ti);

      theora_ti.width = ((w + 15) >>4)<<4;
      theora_ti.height = ((h + 15)>>4)<<4;
      theora_ti.frame_width = w;
      theora_ti.frame_height = h;
      theora_ti.offset_x = 0;
      theora_ti.offset_y = 0;
      theora_ti.fps_numerator = video_fps_numerator;
      theora_ti.fps_denominator = video_fps_denominator;
      theora_ti.aspect_numerator = video_aspect_numerator;
      theora_ti.aspect_denominator = video_aspect_denominator;
      theora_ti.colorspace = OC_CS_UNSPECIFIED;
      theora_ti.pixelformat = OC_PF_420;
      theora_ti.target_bitrate = video_rate;
      theora_ti.quality = video_quality;

      theora_ti.dropframes_p = 0;
      theora_ti.quick_p = 1;
      theora_ti.keyframe_auto_p = 1;
      theora_ti.keyframe_frequency = 64;
      theora_ti.keyframe_frequency_force = 64;
      theora_ti.keyframe_data_target_bitrate = video_rate * 1.5;
      theora_ti.keyframe_mindistance = 8;
      theora_ti.noise_sensitivity = 1;

      if(theora_open(option_output)) {
        /* XXX: cleanup */
        return 1;
      }

      theora_initialized = 1;
    }

    if(theora_write_frame(w, h, yuv)) {
      theora_close();
      free(input_directory);
      free(input_filter);
      exit(1);
    }

    free(yuv);
  }

  theora_close();
  return 0;
}
