##_Theora Bitstream Specification and Reference Decoder -- Theora 1.0

#(c) 2003 Xiph Foundation, Dan Miller

#special thanks to various people at On2 Technologies for donating VP3, and to the folks at Xiph.org Foundation for
#doing good things with it

#This documentation is released under the yada yada open free gnu/BDS license etc.

#Theora is a video codec. This is the spec. 


#Introduction
#About this document

#This document is The Theora Bitstream Specification and Reference Decoder. It is both an english language
#description of the Theora video bitstream, and an interpretable program in the Python programming language, which
#can be executed using a version 2.0 or later Python interpreter. More information about Python can be gleaned at
#www.python.org. When run as a Python script, this document will decode compliant Theora bitstreams, producing
#uncompressed YUV files.

#To invoke this document as a Python program: at the command prompt, type:
#[prompt]> python spec.py testspec.ogg

#The raw file will be saved as "out.raw"

#Formatting Conventions

#There are two versions of this document: HTML, and Python.  Both are created from the same source file.  

#In the HTML version, non-code sections are generally formatted using a variable-width font to distinguish them from
#code. Code and non-code can be interspersed, even within Python routines. In general, non-code blocks of text within
#routines will be italicized.

#The Python version is a simple ascii text file intended for viewing in a source code editor with a fixed-width font.
#Non-code sections are formatted as legal Python comments.

#Overview

#This document represents part of the overall specification package. The specification package includes the following
#elements:

# * Theora Bitstream Specification and Reference Decoder (this document) 

# * Verification Bitstreams (compressed data) 

# * Verification Output Streams (uncompressed YUV data)

#A decoding application is Theora compliant IFF it can decode any Theora bitstream that is decodable with the
#Reference Decoder (this document), producing output that is bytewise equivalent. Several Verification Bitstreams and
#Verification Output Streams are included in the Specification Package for reference; the Verification Output Streams
#were produced using this document as a Python script. However, it is understood that a compliant decoder must be
#capable of decoding any legal Theora bitstream, not just those included in the Specification Package. A legal Theora
#bitstream is any bitstream that can be decoded by this document.

#Please note: the Python code herein is written solely to facilitate the definition and compliance testing of Theora
#bitstreams. It is by design an extremely inefficient and poorly structured piece of code. Do not use this as a
#template for real-world player applications. Instead, start with the C-based Theora decoder from CVS.

#The Theora Bitstream Specification does not cover encoders, except for the following sentence:

#An encoder is Theora compliant if it can produce a compressed bitstream that the Verification Decoder can decode.

#Note that this could be an application that produces nothing but black frames. The quality and scope of a Theora
#compliant encoder are entirely defined by the application domain for that product. If it produces a syntactically
#correct Theora bitstream, it is by definition a Theora encoder. The rest is up to you.


#Theora Bitstream Specification
#A Theora bitstream consists of header packet(s), followed by video packets. A decoder must receive all valid header
#packets before playing video. (note that this means if you wish to play a Theora stream from an arbitrary point, you
#need a mechanism to acquire the header information for that stream before commencing playback).

#Before we define our first routine, a little housekeeping for Python:

from array import array
from os import abort

#some globals & useful functions:

oggfile = file("testspec.ogg","rb")
oggstring = oggfile.read()                            #NOTE limited by memory constraints -- should use file I/O
oggindex = 0
oggbyte = 0
bitmask = 0
oggbuf = array('B',oggstring)                         #convert to an array of unsigned bytes
huffs = []                                            #this will contain list of huffman trees

#Bitstream parsing routines

#bit & byte ordering: For historical reasons, Ogg is generally little-endian, while Theora is big-endian when reading
#values larger than 8 bits directly from the stream. For byte reads (on byte boundaries), it doesn't matter; both
#schemes use the same bitpacking within a byte. These routines are intended primarily for reading the actual Theora
#bitstream. However for testing purposes, we have to implement a bit of Ogg since the test files are Ogg formatted.
#Note readlong() which is implemented to facilitate little-endian longword reads in the Ogg-aware routines.

#this routine just grabs a byte from the input stream:

def readbyte():                                       #note: this is a low-level function to read 
                                                      #a byte-aligned octet from the datastream.
                                                      #To read an arbitrarily aligned byte, use readbits(8)
  global oggindex
  byte = oggbuf[oggindex]
  oggindex += 1
  return byte

#These are used during the bulk of Theora stream parsing:

def readbit():
  global bitmask, oggbyte
  if bitmask == 0:
    bitmask = 0x80
    oggbyte = readbyte()
  if oggbyte & bitmask:
    bit = 1
  else:
    bit = 0
  bitmask >>= 1
  return bit

def flushpage():                                      #Ogg pages are byte-aligned; need to flush after using
  global bitmask
  bitmask = 0

#readbits: our workhorse.  Gets up to 32 bits from the stream 
#(note -- hi bit is first bit read! use readlong() or build up values through sequential byte reads for Ogg purposes)

def readbits(x):
  ret = 0
  for i in range(x):
    ret <<= 1
    ret += readbit()
  return ret

#readstring reads a string of 8-bit unsigned chars:

def readstring(x):
  s = ''
  for i in range(x):
    s += chr(readbits(8))
  return s

#readlong reads a longword Ogg style:

def readlong():                                       #different than readbits(32): byte order is reversed

  return readbits(8) + (readbits(8) << 8) + (readbits(8) << 16) + (readbits(8) << 24)

#entropy coding routines

#Certain values in Theora (such as DCT coefficients) are encoded using a context-sensitive Huffman scheme based on 32
#possible token values. Each token value has an associated set of extra bits that are bitpacked immediately following
#the primary huffman string. The binary decision trees (80 of them) necessary for decoding are in the table header.

#Set up the Huffman tables

#Huffman tables are encoded in compressed form using the following algorithm:

#Note how this function is called recursively for each possible branch in the tree until all branches have bottomed
#out with complete bitstrings:

def read_hufftable(table):
  if readbit():                                       #if bit==1, this bitstring is complete
    table.append( readbits(5) )                       #next 5 bits = token number for this string
  else:                                               #if bit was zero, we have two more entries defining
                                                      #the zero and one case for the next bit:
    table.append([])                                  #add another pair of tables
    table.append([])
    read_hufftable(table[0])                          #with an entry for zero
    read_hufftable(table[1])                          #and one for one

#read a token

#Again, we use recursion to parse the bits until we have a complete string:

def readtoken(huf):
  if type(huf[0]) == type(0):                         #integer means we have a value
    return huf[0]                                     #return token value
  else:
    if readbit():                                     #read a bit, recurse into subtable 0 or 1
      return readtoken(huf[1])                        #case for bit=1
    else:
      return readtoken(huf[0])                        #case for bit=0

#define an array of information tables for each token

#Each table contains the following five entries in this order:

#  base run length
#  number of extra run length bits (0 - 12)
#  base value
#  number of extra value bits (0 - 9)
#  number of extra sign bits (0 or 1)

token_array = [    
[1, 0, 'eob', 0, 0], 
[2, 0, 'eob', 0, 0], 
[3, 0, 'eob', 0, 0], 
[4, 2, 'eob', 0, 0], 
[8, 3, 'eob', 0, 0], 
[16, 4, 'eob', 0, 0], 
[0, 12, 'eob', 0, 0], 
[0, 3, 0, 0, 0], 

[0, 6, 0, 0, 0], 
[0, 0, 1, 0, 0], 
[0, 0, -1, 0, 0], 
[0, 0, 2, 0, 0], 
[0, 0, -2, 0, 0], 
[0, 0, 3, 0, 1], 
[0, 0, 4, 0, 1], 
[0, 0, 5, 0, 1], 

[0, 0, 6, 0, 1], 
[0, 0, 7, 1, 1], 
[0, 0, 9, 2, 1], 
[0, 0, 13, 3, 1], 
[0, 0, 21, 4, 1], 
[0, 0, 37, 5, 1], 
[0, 0, 69, 9, 1], 
[1, 0, 1, 0, 1], 

[2, 0, 1, 0, 1], 
[3, 0, 1, 0, 1], 
[4, 0, 1, 0, 1], 
[5, 0, 1, 0, 1], 
[6, 2, 1, 0, 1], 
[10, 3, 1, 0, 1], 
[1, 0, 2, 1, 1], 
[2, 1, 2, 1, 1] ]

#parse the tokens

#this function returns a run length & value based on the token & extended bits:

def parsetoken(huf):
  global token_array                                  #(not strictly necessary for read-only in Python)

  token = readtoken(huf)                              #read a token from the stream
  table = token_array[token]                          #get our table of parameters for this token
  run = table[0]                                      #base run length
  run_extra = table[1]                                #number of extra bits for run length
  value = table[2]                                    #actual value
  val_extra = table[3]                                #number of extra value bits
  sign_extra = table[4]                               #number of sign bits

  sign = 1
  if sign_extra:
    if readbit():
      sign = -1                                       #if there's a sign bit, get it.  1 means negative
                                                      #note that value may be negative to begin with, in
                                                      #which case there are no extra value or sign bits
  if val_extra:
    value += readbits(val_extra)                      #get extra value bits

  if run_extra:
    run += readbits(run_extra)                        #get extra run bits
  
  return [run, value * sign]                          #return run length and value
                                                      #note: string * 1 = string, so 'eob', 'zrl' are OK


#routines to read & parse headers

#simple Ogg page header parsing routine:

def read_page_header():
  global oggindex
  flushpage()
  oggs = readstring(4)                                #get the putated 4-byte Ogg identifier
  if oggs != "OggS":
    print "invalid page data"
    abort()
  oggindex += 10                                      #serialnum at offset 14
  serialno = readlong()
  oggindex += 8                                       #segment count at offset 26
  segments = readbits(8)
  bytes = 0
  for i in range(segments):
    bytes += readbits(8)

#routine to parse the Theora info header:
#[note: next three routines need to be rewritten so headers can come in any order]

def read_info_header():
  global huffs, encoded_width, encoded_height, decode_width, decode_height, offset_x, offset_y
  global fps_numerator, fps_denominator, aspect_numerator, aspect_denominator, quality, bitrate
  global version_major, version_minor, version_subminor, colorspace

  typeflag = readbits(8)

  if typeflag != 0x80:                                #first bit = 1; next 7 bits define header type.
                                                      #for info header, 7 bit value = zero, so we have 0x80
    print "expected info header?"
    abort()

  cid = readstring(6)                                 #codec ID
  if cid != "theora":
    print "Not a Theora header packet"
    abort()

  version_major = readbits(8)                         #major & minor version must be exact match
  if version_major != 3:
    abort()
  version_minor = readbits(8)
  if version_minor != 2:
    abort()
  version_subminor = readbits(8)

  encoded_width = readbits(16) << 4                   #encoded width & height are in block units of 16x16
  encoded_height = readbits(16) << 4

  decode_width = readbits(24)                         #decode width & height are in actual pixels
  decode_height = readbits(24)
  offset_x = readbits(8)                              #offset for cropping if decode != full encoded frame
  offset_y = readbits(8)

  fps_numerator = readbits(32)                        #frames per second encoded as a fraction
  fps_denominator = readbits(32)
  aspect_numerator = readbits(24)                     #aspect not used now
  aspect_denominator = readbits(24)
  readbits(5)                                         #force keyframe frequency flag -- not used for decode
  colorspace = readbits(8)                            #colorspace flag defines YUV to RGB mapping
  bitrate = readbits(24)                              #target bitrate; not used for decode
  quality = readbits(6)                               #target quality also not used for decode


#parse the comment header:

def read_comment_header():
  global vendor_string, vendor_string_len, comment_string, comment_string_len

  typeflag = readbits(8)

  if typeflag != 0x81:                                #header type 1 = comment
    print "expected comment header? typeflag =",typeflag
    abort()
  cid = readstring(6)
  if cid != "theora":
    print "corrupted comment header?", cid
    abort()
    
  vendor_string_len = readlong()
  vendor_string = readstring(vendor_string_len)
  comment_string_len = readlong()
  comment_string = readstring(comment_string_len)


#read & parse the table header:

def read_table_header():
  global scale_table_AC, scale_table_DC, Y_quantizer, UV_quantizer, IF_quantizer, frequency_counts

  typeflag = readbits(8)

  if typeflag != 0x82:                                            #type 2 = table header
    print "expected table header? type = ", hex(typeflag)
    abort()

  cid = readstring(6)
  if cid != "theora":
    print "corrupted table header?", cid
    abort()

  scale_table_AC = []                                             #64 possible quantizer scalers for AC coeffs
  for x in range(64):    scale_table_AC.append(readbits(16))

  scale_table_DC = []                                             #64 possible quantizer scalers for DC coeffs
                                                                  #Note this is unrelated to 64 coeffs in an 8x8 block!
  for x in range(64):    scale_table_DC.append(readbits(16))

  Y_quantizer = []                                                #quantizers for intra Y coeff (this IS about 8x8 blocks!)
  for x in range(64):    Y_quantizer.append(readbits(8))

  UV_quantizer = []                                               #quantizers for intra U or V coeff
  for x in range(64):    UV_quantizer.append(readbits(8))

  IF_quantizer = []                                               #quantizers for interframe coeffs (Y, U, or V)
  for x in range(64):    IF_quantizer.append(readbits(8))

  for x in range(80):                                             #Read in huffman tables
    huffs.append([])
    read_hufftable(huffs[x])


# Routines that decode video

#[NOTE: for now, these routines only handle keyframes.  We may modify or add routines to support interframe data]
#[NOTE: each frame of video resides in a single Ogg page.]
#[NOTE: that's crap.  each frame of video resides in a logical Ogg packet.  pagination is irrelevant.]

#Hilbert ordering

#All data in Theora is organized into 8x8 blocks. When encoding the data, these blocks are further grouped into
#'super-blocks' of 16 blocks each, and encoded in 'Hilbert' order. Each super-block consists of up to 16 blocks
#encoded in the following order:

#      X -> X    X -> X
#           |    ^
#           v    |
#      X <- X    X <- X
#      |              ^
#      v              |
#      X    X -> X    X
#      |    ^    |    ^
#      v    |    v    |
#      X -> X    X -> X

#(thanks to Mike Melanson for the diagram)

#If any block is not coded (due to clipping, for instance -- encoded images can include partial super-blocks) the
#pattern continues until the next coded block is hit. Each pixel plane -- Y, V, and U -- are encoded using their own
#pattern of superblocks.

#By way of example: if a plane consisted of 32x16 pixels, only the top half of the Hilbert pattern would be used. If
#the 8 8x8 blocks in this example are labled in this way:

#    A B C D
#    E F G H

#then they will be encoded in the following order: A, B, F, E, H, G, C, D.


#the following array & function are used to 'de-hilbertize' the data. [note: this is a keyframe-only routine right
#now]

hilbert = [    
[0,0], [1,0], [1,1], [0,1], 
[0,2], [0,3], [1,3], [1,2], 
[2,2], [2,3], [3,3], [3,2], 
[3,1], [2,1], [2,0], [3,0] ]

def de_hilbert(w, h, colist):                               #width, height, coefficient list
  sbw = int( (w+3) / 4)                                     #super-block width (width in sb's)
  sbh = int( (h+3) / 4)                                     #super-block height (height in sb's)
  ii = 0 
  comap = []

  for x in range(w):                                        #initialize coefficient map
    comap.append([])
    for y in range(h):
      comap[x].append([])

  for y in range(sbh):
    for x in range(sbw):
      for i in range(16):
        p, q = hilbert[i]                                   #nice Python syntax
        xx = x*4 + p
        yy = y*4 + q
        if (xx < w) & (yy < h):                                   #skip stuff out of range
          comap[xx][yy] = colist[ii]                        #if in range, get a coeff
          ii += 1
  return comap

#decoding the DC coefficients

#The DC values are simply the zero-order coefficients of each 8x8 block. These values tend to have more entropy than
#most AC components, so in addition to quantization, it is desirable to use delta coding to reduce the data
#requirement of encoding them.

#Theora uses a scheme where each encoded DC value is in fact a difference between a predicted value and the actual
#value. Since the blocks are coded in raster order, the predicted value can be any combination of DC values of blocks
#to the left, up, upper left, and upper right.

#routine to do DC prediction on a single color plane:

def DCpredict(comap, w, h):

#first row is straight delta coding:

  l = 0                                                     #l = DC coeff to my left; init to zero
  for x in range(w):
    l += comap[x][0][0]
    comap[x][0][0] = l
      
#now the rest:

  for y in range(h):
    for x in range(w):
      if y>0:                                               #already got the first row
        if x == 0:                                          #left column?
          u = comap[0][y-1][0]                              #u = upper
          ur = comap[1][y-1][0]                             #ur = upper-right
          p = u                                             #[Shouldn't this be u*75 + ur*53??? but this works!]
          comap[0][y][0] += p                               #add predictor to decoded value

        else:                                               #general case -- neither left column nor top row
          l = comap[x-1][y][0]                              #l = left
          ul = comap[x-1][y-1][0]                           #ul = upper-left
          u = comap[x][y-1][0]                              #u = upper
          p = (l*29 + u*29 - ul*26)                         #compute weighted predictor
          if p < 0:
            p += 31                                         #round towards zero
          p >>= 5                                           #shift by weight multiplier
          if abs(p-u) > 128:                                #range checking
            p = u;
          if abs(p-l) > 128:
            p = l;
          if abs(p-ul) > 128:
            p = ul;

          comap[x][y][0] += p                               #add predictor to decoded value


#Dequantization logic

#(thanks again to Mike Melanson for this exposition)

#Setting Up The Dequantizers

#Theora has three static tables for dequantizing fragments-- one for intra Y fragments, one for intra C fragments,
#and one for inter Y or C fragments. In the following code, these tables are loaded as Y_quantizer[], UV_quantizer[],
#and IF_quantizer[] (see definition for read_table_header below). However, these tables are adjusted according to the
#value of quality_index.

#quality_index is an index into 2 64-element tables: scale_table_DC[] and scale_table_AC[]. Each dequantizer from the
#three dequantization tables is adjusted by the appropriate scale factor according to this formula:

#    Scale dequantizers:
     
#        dequantizer * sf
#        ----------------
#              100
     
#    where sf = scale_table_DC[quality_index] for DC dequantizer
#               scale_table_AC[quality_index] for AC dequantizer

#Note that the dequantize routine also multiplies each coefficient by 4.  This is to facilitate the iDCT later on.

def dequant(data, scaleDC, scaleAC, dqtable):
  mul = int( (scaleDC * dqtable[0]) / 100 ) * 4
  for x in range(64):
    if x>0:
      mul = int( (scaleAC * dqtable[x]) / 100 ) * 4
    data[x] *= mul

#ZIG-ZAG order

#The coefficients in each 8x8 DCT coded block are layed out in 'zig-zag' order. The following table shows the order
#in which the 64 coefficients are coded:

zigzag=[  
  0,  1,  5,  6,  14, 15, 27, 28,
  2,  4,  7,  13, 16, 26, 29, 42,
  3,  8,  12, 17, 25, 30, 41, 43,
  9,  11, 18, 24, 31, 40, 44, 53,
  10, 19, 23, 32, 39, 45, 52, 54,
  20, 22, 33, 38, 46, 51, 55, 60,
  21, 34, 37, 47, 50, 56, 59, 61,
  35, 36, 48, 49, 57, 58, 62, 63]

#this routine remaps the coefficients to their original order:

def unzig(data,un):
  for x in range(64):
    un[x] = data[zigzag[x]]

#inverse DCT (iDCT)

#Once coefficients are re-ordered and dequantized, the iDCT is performed on the 8x8 matrix to produce the actual
#pixel values (or differentials for predicted blocks).

#Theora's particular choice of iDCT computation involves intermediate values that are calculated using 32 bit,
#fixed-point arithmetic. Multiplication is defined as follows (assuming 32-bit integer parameters):

def Mul_fix(a, b):
  return (a * b) >> 16

#addition and subtraction can be performed normally.

#First we define a one-dimensional iDCT:

def idct_1D(data, i, stride):

#[where did these varnames come from?  somebody make them instructive please]

  A = Mul_fix(64277, data[i+stride]) + Mul_fix(12785, data[i+7*stride])
  B = Mul_fix(12785, data[i+stride]) - Mul_fix(64277, data[i+7*stride])
  C = Mul_fix(54491, data[i+3*stride]) + Mul_fix(36410, data[i+5*stride])
  D = Mul_fix(54491, data[i+5*stride]) - Mul_fix(36410, data[i+3*stride])
  Ad = Mul_fix(46341, A - C)
  Bd = Mul_fix(46341, B - D)
  Cd = A + C
  Dd = B + D
  E = Mul_fix(46341, data[i] + data[i+4*stride])
  F = Mul_fix(46341, data[i] - data[i+4*stride])
  G = Mul_fix(60547, data[i+2*stride]) + Mul_fix(25080, data[i+6*stride])
  H = Mul_fix(25080, data[i+2*stride]) - Mul_fix(60547, data[i+6*stride])
  Ed = E - G
  Gd = E + G
  Add = F + Ad
  Bdd = Bd - H
  Fd = F - Ad
  Hd = Bd + H

  data[i] = Gd + Cd
  data[i+stride] = Add + Hd
  data[i+2*stride] = Add - Hd
  data[i+3*stride] = Ed + Dd
  data[i+4*stride] = Ed - Dd
  data[i+5*stride] = Fd + Bdd
  data[i+6*stride] = Fd - Bdd
  data[i+7*stride] = Gd - Cd

#2D iDCT is performed first on rows, then columns (note order can affect lower bits!)

#Note that we dequantized all coefficients to 4 times their real value. Since each coefficient is run through the
#iDCT twice (horizontal & vertical), the final values must be divided by 16.

def idct(data):

  for y in range(8):
    idct_1D(data, y*8, 1);

  for x in range(8):
    idct_1D(data, x, 8);

  for x in range(64):
    data[x] = (data[x] + 8) >> 4                            #add for rounding; /16 (remember dequant was *4!)

#this routine converts intra-coded blocks of data into unsigned chars

def pixelize(data, w, h, dx, dy):
  pix = []

  for y in range(h):
    for x in range(w):
      xx = x+dx
      yy = y+dy
      bx = xx >> 3
      by = yy >> 3
      ix = xx % 8
      iy = yy % 8
      #print "pixelize: bx, by, ix, iy",bx,by,ix,iy
      p = data[bx][by][ix + iy*8]
      p += 128
      if p < 0:
        p = 0
      if p > 255:
        p = 255
      pix.append(p)

#return a linear list of suitably clamped & range-checked values from a map of blocks
  return pix

# *** DECODING THE FRAME ***

#ok, let's do it!

def decode_frame():
  print
  print "DECODING FRAME"

#First, we decode the frame header:

  packet_type = readbit()
  print "packet_type:", packet_type
  if packet_type != 0:
    print "illegal packet type"
    abort()
    
  is_predicted = readbit()
  print "is_predicted:", is_predicted
  quality_index = readbits(6)
  print "quality_index =", quality_index
  scalefactor_AC = scale_table_AC[quality_index]            #(ThisFrameQualityValue in C)
  print "scalefactor_AC =", scalefactor_AC
  scalefactor_DC = scale_table_DC[quality_index]
  print "scalefactor_DC =", scalefactor_DC

  if is_predicted == 0:                                     #0 = keyframe, 1 = interframe

#OK, this is a keyframe.  That means we just have 'intra' coded blocks.
    print "decoding keyframe"
    keyframe_type = readbit()                               #keyframe type always == 1 (DCT) (for now)
    readbits(2)                                             #2 unused bits

#compute some values based on width & height:
    blocks_Y = int( (encoded_height*encoded_width)/64 )     #Y blocks coded
    blocks_UV = int(blocks_Y / 2)
    blocks_U = int(blocks_UV / 2)
    blocks_V = blocks_U
    blocks_tot = int(blocks_Y * 1.5)                        #Y and UV blocks coded

#initialize a map of coefficients.  For each coded block, we will eventually have 64:
    global coeffs
    coeffs = [ [] for x in range(blocks_tot) ]

#Theora encodes each coefficient for every block in sequence. IE first we decode all the DC coefficients; then
#coefficient #1, then #2, and so on.
#Also, as we go higher into the coefficient index, we will use different huffman tables:

    for i in range(64):                                     #For each coefficient index,

      if i == 0:                                            #get DC huffman tables for coeff=0
        huff_Y = readbits(4)
        huff_UV = readbits(4)
      elif i == 1:
        huff_Y = readbits(4)+16                             #get AC huff tables at 1, 6, 15, and 28
        huff_UV = readbits(4)+16
      elif i == 6:
        huff_Y += 16
        huff_UV += 16
      elif i == 15:
        huff_Y += 16
        huff_UV += 16
      elif i == 28:
        huff_Y += 16
        huff_UV += 16

#now, for every block, decode our coefficient:

      for x in range(blocks_tot):
        if x < blocks_Y:                                    #if we're in the Y plane,
          huff = huff_Y
        else:
          huff = huff_UV

#first check whether this coefficient was already decoded (Because of an end-of-block or other run):

        if len(coeffs[x]) <= i:                             #if this coeff has not been set

#if not, get a token:

          run, val = parsetoken(huffs[huff])

#if this is an end-of-block token, that means we have a run of blocks to mark as fully decoded:

          if val == 'eob':                                  #eob = End Of Block run
            xx = x
            for r in range(run):                            #clear <run> blocks (and wrap around!!)
              done = len(coeffs[xx])                        #this many coeffs are set in this block
              remain = 64 - done                            #this many remain
              for j in range (remain):
                coeffs[xx].append(0)                        #zero out the rest of the block
              skipped=0                                     #keep a count of skipped blocks
              while len(coeffs[xx]) >i:                     #skip blocks that are coded up to this point or further
                xx = (xx+1) % blocks_tot                    #next block (and wrap around)
                skipped += 1
                if skipped >= blocks_tot:                   #quit if we're done (or we can loop forever)
                  break
#otherwise the token represents a run of zeros followed by a value:

          else:                                             #zero run + value
            for r in range (run):                           #a run of zeros
              coeffs[x].append(0)
            coeffs[x].append(val)                           #followed by a val

#now 'de-hilbertize' coefficient blocks:

    Yheight = int(encoded_height/8)
    Ywidth = int(encoded_width/8)
    UVheight = int(Yheight/2)
    UVwidth = int(Ywidth/2)
    global comapY, comapU, comapV
    comapY = de_hilbert(Ywidth, Yheight, coeffs)
    comapU = de_hilbert(UVwidth, UVheight, coeffs[blocks_Y:])
    comapV = de_hilbert(UVwidth, UVheight, coeffs[(blocks_Y+blocks_U):])

#next, we need to reverse the DC prediction-based delta coding:

    DCpredict(comapY, Ywidth, Yheight)
    DCpredict(comapV, UVwidth, UVheight)
    DCpredict(comapU, UVwidth, UVheight)

#finally reorder, dequantize, and iDCT all the coefficients, for each color plane:

    temp = [0 for x in range(64)]                           #temporary array

    for y in range(Yheight):
      for x in range(Ywidth):
        unzig (comapY[x][y], temp)
        dequant(temp, scalefactor_DC, scalefactor_AC, Y_quantizer)
        idct(temp)
        comapY[x][y] = temp + []                            #Python quirk -- force assignment by copy (not reference)

    for y in range(UVheight):
      for x in range(UVwidth):
        unzig (comapU[x][y], temp)
        dequant(temp, scalefactor_DC, scalefactor_AC, UV_quantizer)
        idct(temp)
        comapU[x][y] = temp + []

    for y in range(UVheight):
      for x in range(UVwidth):
        unzig (comapV[x][y], temp)
        dequant(temp, scalefactor_DC, scalefactor_AC, UV_quantizer)
        idct(temp)
        comapV[x][y] = temp + []

#convert the image into a raw byte array of planar Y, V, U:
#[note: this format differs from Mau's dumpvid, which lays out U & V interleaved rather than strict planar]

    pixY = pixelize(comapY, decode_width, decode_height, offset_x, offset_y)
    pixU = pixelize(comapU, decode_width>>1, decode_height>>1, offset_x>>1, offset_y>>1)
    pixV = pixelize(comapV, decode_width>>1, decode_height>>1, offset_x>>1, offset_y>>1)

#return the three color planes:

    return pixY, pixU, pixV

#Decode Predicted Frame:                THIS SECTION UNFINISHED
  else: 
    print "decoding interframe (NOT!)"
    coding_scheme = readbits(3)
    if coding_scheme == 0:
      mode_alphabet = []                                    #define a list (think of it as an array)
      for x in range(8):
        mode_alphabet.append(readbits(3))                   #add another mode to the list

#end of definition for decode_frame()


#MAIN TEST SEQUENCE

#let's test our routines by parsing the stream headers and the first frame.

print
print "DECODING STREAM HEADERS"

#Read and Verify Stream Headers:

read_page_header()
read_info_header()
read_page_header()                                          #I know there's one around here
read_comment_header()                                       #but not here! (this is encoder-specific -- will be fixed)
read_table_header()

#print some useful information:

print "vendor string:", vendor_string
print "comment length:", comment_string_len
print "version:", version_major, version_minor, version_subminor 
print "encoded width:", encoded_width
print "encoded height:", encoded_height
print "decode width:", decode_width
print "decode height:", decode_height
print "X offset:", offset_x
print "Y offset:", offset_y
print "fps:", fps_numerator, "/", fps_denominator
print "aspect:", aspect_numerator, "/", aspect_denominator
print "colorspace:",
if colorspace == 0:
  print "not specified"
elif colorspace == 1:
  print "ITU 601"
elif colorspace == 2:
  print "CIE 709"
else:
  print "colorspace type not recognized, assuming unspecified"
  colorspace = 0
print "target bitrate:", bitrate
print "target quality:", quality

# read the first frame...

read_page_header()                                          #see prev. note about this function -- becomes read_packet()
Y, U, V = decode_frame()

#and write it to disk.

buf = array('B', Y+U+V)
outfile = file("out.raw","wb")
outfile.write(buf)

#that's all for now
print "done"

 