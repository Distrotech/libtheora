#ifndef _PASTREAMIO_H
#define _PASTREAMIO_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * $Id: pastreamio.h,v 1.1 2003/05/23 14:10:17 mauricio Exp $
 * PASTREAMIO.h
 * Modified version of Portable Audio Blocking read/write utility.
 *
 * Modified by Mauricio Piacentini http://www.tabuleiro.com
 * from the original PABLIO files
 * Modified to support only playback buffers, direct access
 * to the underlying stream time and remove blocking operations
 *
 * Original Author: Phil Burk, http://www.softsynth.com/portaudio/
 *
 * Include file for PASTREAMIO.
 * Built on top of PortAudio, the Portable Audio Library.
 * For more information see: http://www.audiomulch.com/portaudio/
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "portaudio.h"
#include "ringbuffer.h"
#include <string.h>

typedef struct
{
    RingBuffer   outFIFO;
    PortAudioStream *stream;
    int          bytesPerFrame;
    int          samplesPerFrame;
}
PASTREAMIO_Stream;

/* Values for flags for OpenAudioStream(). */
/* Keep PABLIO ones*/

#define PASTREAMIO_READ     (1<<0)
#define PASTREAMIO_WRITE    (1<<1)
#define PASTREAMIO_READ_WRITE    (PABLIO_READ|PABLIO_WRITE)
#define PASTREAMIO_MONO     (1<<2)
#define PASTREAMIO_STEREO   (1<<3)

/************************************************************
 * Write data to ring buffer.
 * Will not return until all the data has been written.
 */
long WriteAudioStream( PASTREAMIO_Stream *aStream, void *data, long numFrames );

/************************************************************
 * Read data from ring buffer.
 * Will not return until all the data has been read.
 */
long ReadAudioStream( PASTREAMIO_Stream *aStream, void *data, long numFrames );

/************************************************************
 * Return the number of frames that could be written to the stream without
 * having to wait.
 */
long GetAudioStreamWriteable( PASTREAMIO_Stream *aStream );

/************************************************************
 * Return the number of frames that are available to be read from the
 * stream without having to wait.
 */
long GetAudioStreamReadable( PASTREAMIO_Stream *aStream );

/************************************************************
 * Opens a PortAudio stream with default characteristics.
 * Allocates PASTREAMIO_Stream structure.
 *
 * flags parameter can be an ORed combination of:
 *    PASTREAMIO_WRITE,
 *    and either PASTREAMIO_MONO or PASTREAMIO_STEREO
 */
PaError OpenAudioStream( PASTREAMIO_Stream **aStreamPtr, double sampleRate,
                         PaSampleFormat format, long flags );

PaError StartAudioStream( PASTREAMIO_Stream *aStream);

PaTimestamp GetAudioStreamTime( PASTREAMIO_Stream *aStream );

PaError CloseAudioStream( PASTREAMIO_Stream *aStream );

unsigned long RoundUpToNextPowerOf2( unsigned long n );

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PASTREAMIO_H */
