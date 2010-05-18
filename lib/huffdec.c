/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2009                *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function:
    last mod: $Id$

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "huffdec.h"
#include "decint.h"



/*Instead of storing every branching in the tree, subtrees can be collapsed
   into one node, with a table of size 1<<nbits pointing directly to its
   descedents nbits levels down.
  This allows more than one bit to be read at a time, and avoids following all
   the intermediate branches with next to no increased code complexity once
   the collapsed tree has been built.
  We do _not_ require that a subtree be complete to be collapsed, but instead
   store duplicate pointers in the table, and record the actual depth of the
   node below its parent.
  This tells us the number of bits to advance the stream after reaching it.

  This turns out to be equivalent to the method described in \cite{Hash95},
   without the requirement that codewords be sorted by length.
  If the codewords were sorted by length (so-called ``canonical-codes''), they
   could be decoded much faster via either Lindell and Moffat's approach or
   Hashemian's Condensed Huffman Code approach, the latter of which has an
   extremely small memory footprint.
  We can't use Choueka et al.'s finite state machine approach, which is
   extremely fast, because we can't allow multiple symbols to be output at a
   time; the codebook can and does change between symbols.
  It also has very large memory requirements, which impairs cache coherency.

  We store the tree packed in an array of 16-bit integers (words).
  These come in two flavors.
  The first is a "bitree", or binary tree, where each node always has exactly
   two children, stored in consecutive words.
  If the child is negative or zero, then it is a leaf node.
  These are stored directly in the child pointer to save space, since they only
   require a single word.
  The token value is -leaf.
  If the child is positive, then it is the index of another internal node in
   the table.

  The second flavor is a general "tree", where each node consists of a single
   word, followed consecutively by two or more indices of its children.
  Let n be the value of this first word.
  This is the number of bits that need to be read to traverse the node, and
   must be positive.
  1<<n entries follow in the array, each an index to a child node.
  If a leaf node would have been before reading n bits, then it is duplicated
   the necessary number of times in this table.
  Leaf nodes pack both a token value and their actual depth in the tree.
  The token in the leaf node is (-leaf&255).
  The number of bits that need to be consumed to reach the leaf, starting from
   the current node, is (-leaf>>8).

  @ARTICLE{Hash95,
    author="Reza Hashemian",
    title="Memory Efficient and High-Speed Search {Huffman} Coding",
    journal="{IEEE} Transactions on Communications",
    volume=43,
    number=10,
    pages="2576--2581",
    month=Oct,
    year=1995
  }*/



/*The number of internal tokens associated with each of the spec tokens.*/
static const unsigned char OC_DCT_TOKEN_MAP_ENTRIES[TH_NDCT_TOKENS]={
  1,1,1,4,8,1,1,8,1,1,1,1,1,2,2,2,2,4,8,2,2,2,4,2,2,2,2,2,8,2,4,8
};

/*The map from external spec-defined tokens to internal tokens.
  This is constructed so that any extra bits read with the original token value
   can be masked off the least significant bits of its internal token index.
  In addition, all of the tokens which require additional extra bits are placed
   at the start of the list, and grouped by type.
  OC_DCT_REPEAT_RUN3_TOKEN is placed first, as it is an extra-special case, so
   giving it index 0 may simplify comparisons on some architectures.
  These requirements require some substantial reordering.*/
static const unsigned char OC_DCT_TOKEN_MAP[TH_NDCT_TOKENS]={
  /*OC_DCT_EOB1_TOKEN (0 extra bits)*/
  15,
  /*OC_DCT_EOB2_TOKEN (0 extra bits)*/
  16,
  /*OC_DCT_EOB3_TOKEN (0 extra bits)*/
  17,
  /*OC_DCT_REPEAT_RUN0_TOKEN (2 extra bits)*/
  88,
  /*OC_DCT_REPEAT_RUN1_TOKEN (3 extra bits)*/
  80,
  /*OC_DCT_REPEAT_RUN2_TOKEN (4 extra bits)*/
   1,
  /*OC_DCT_REPEAT_RUN3_TOKEN (12 extra bits)*/
   0,
  /*OC_DCT_SHORT_ZRL_TOKEN (3 extra bits)*/
  48,
  /*OC_DCT_ZRL_TOKEN (6 extra bits)*/
  14,
  /*OC_ONE_TOKEN (0 extra bits)*/
  56,
  /*OC_MINUS_ONE_TOKEN (0 extra bits)*/
  57,
  /*OC_TWO_TOKEN (0 extra bits)*/
  58,
  /*OC_MINUS_TWO_TOKEN (0 extra bits)*/
  59,
  /*OC_DCT_VAL_CAT2 (1 extra bit)*/
  60,
  62,
  64,
  66,
  /*OC_DCT_VAL_CAT3 (2 extra bits)*/
  68,
  /*OC_DCT_VAL_CAT4 (3 extra bits)*/
  72,
  /*OC_DCT_VAL_CAT5 (4 extra bits)*/
   2,
  /*OC_DCT_VAL_CAT6 (5 extra bits)*/
   4,
  /*OC_DCT_VAL_CAT7 (6 extra bits)*/
   6,
  /*OC_DCT_VAL_CAT8 (10 extra bits)*/
   8,
  /*OC_DCT_RUN_CAT1A (1 extra bit)*/
  18,
  20,
  22,
  24,
  26,
  /*OC_DCT_RUN_CAT1B (3 extra bits)*/
  32,
  /*OC_DCT_RUN_CAT1C (4 extra bits)*/
  12,
  /*OC_DCT_RUN_CAT2A (2 extra bits)*/
  28,
  /*OC_DCT_RUN_CAT2B (3 extra bits)*/
  40
};


/*The log_2 of the size of a lookup table is allowed to grow to relative to
   the number of unique nodes it contains.
  E.g., if OC_HUFF_SLUSH is 2, then at most 75% of the space in the tree is
   wasted (each node will have an amortized cost of at most 20 bytes when using
   4-byte pointers).
  Larger numbers can decode tokens with fewer read operations, while smaller
   numbers may save more space (requiring as little as 8 bytes amortized per
   node, though there will be more nodes).
  With a sample file:
  32233473 read calls are required when no tree collapsing is done (100.0%).
  19269269 read calls are required when OC_HUFF_SLUSH is 0 (59.8%).
  11144969 read calls are required when OC_HUFF_SLUSH is 1 (34.6%).
  10538563 read calls are required when OC_HUFF_SLUSH is 2 (32.7%).
  10192578 read calls are required when OC_HUFF_SLUSH is 3 (31.6%).
  Since a value of 1 gets us the vast majority of the speed-up with only a
   small amount of wasted memory, this is what we use.
  This value must be less than 7, or you could create a tree with more than
   32767 entries, which would overflow the 16-bit words used to index it.*/
#define OC_HUFF_SLUSH (1)
/*The root of the tree is the fast path, and a larger value here is more
   beneficial than elsewhere in the tree.
  3 appears to give the best performance, trading off between increased use of
   the single-read fast path and cache footprint for the tables.
  using 3 here The VP3 tables are about 2.6x larger using 3 here than using 1.*/
#define OC_ROOT_HUFF_SLUSH (3)



/*Unpacks a sub-tree from the given buffer.
  _opb:      The buffer to unpack from.
  _bitree:   The tree to store the sub-tree in.
  _nbinodes: The number of nodes available for the tree.
  _binode:   The index of the tree to store it in.
             This is updated to point to the first available slot past the
              sub-tree on return.
             If an error occurs, this is set larger than _nbinodes.
  Return: The node index of the sub-tree, or leaf node value if this was a leaf
   node and this wasn't the root of the tree.*/
static int oc_huff_tree_unpack(oc_pack_buf *_opb,
 ogg_int16_t *_bitree,int _nbinodes,int *_binode){
  long bits;
  int  binode;
  binode=*_binode;
  if(binode>_nbinodes)return 0;
  bits=oc_pack_read1(_opb);
  if(oc_pack_bytes_left(_opb)<0){
    *_binode=_nbinodes+1;
    return 0;
  }
  /*Read an internal node:*/
  if(!bits){
    *_binode=binode+2;
    if(binode+2>_nbinodes)return 0;
    _bitree[binode+0]=
     (ogg_int16_t)oc_huff_tree_unpack(_opb,_bitree,_nbinodes,_binode);
    _bitree[binode+1]=
     (ogg_int16_t)oc_huff_tree_unpack(_opb,_bitree,_nbinodes,_binode);
  }
  /*Read a leaf node:*/
  else{
    int ntokens;
    int token;
    bits=oc_pack_read(_opb,OC_NDCT_TOKEN_BITS);
    if(oc_pack_bytes_left(_opb)<0){
      *_binode=_nbinodes+1;
      return 0;
    }
    /*Find out how many internal tokens we translate this external token into.
      This must be a power of two.*/
    ntokens=OC_DCT_TOKEN_MAP_ENTRIES[bits];
    token=OC_DCT_TOKEN_MAP[bits];
    if(ntokens<=1){
      /*If we only have the one leaf, return it directly.*/
      return -token;
    }
    else{
      int last_row_sz;
      int i;
      int j;
      /*Fill in a complete binary tree pointing to the internal nodes.*/
      *_binode=binode+2*(ntokens-1);
      if(binode+2*(ntokens-1)>_nbinodes)return 0;
      last_row_sz=ntokens>>1;
      for(i=1;i<last_row_sz;i<<=1){
        int child;
        child=binode+2*(2*i-1);
        for(j=0;j<i;j++){
          _bitree[binode+2*(i-1+j)+0]=(ogg_int16_t)child;
          child+=2;
          _bitree[binode+2*(i-1+j)+1]=(ogg_int16_t)child;
          child+=2;
        }
      }
      /*And now the last row pointing to the leaf nodes.*/
      for(i=j=0;j<last_row_sz;j++){
        _bitree[binode+2*(last_row_sz-1+j)+0]=(ogg_int16_t)-(token+i++);
        _bitree[binode+2*(last_row_sz-1+j)+1]=(ogg_int16_t)-(token+i++);
      }
    }
  }
  return binode>0?binode:1;
}

/*Finds the depth of shortest branch of the given sub-tree.
  _bitree: A packed binary Huffman tree.
  _binode: The root of the given sub-tree.
  Return: The smallest depth of a leaf node in this sub-tree.
          0 indicates this sub-tree is a leaf node.*/
static int oc_huff_bitree_mindepth(const ogg_int16_t *_bitree,int _binode){
  int child;
  int depth0;
  int depth1;
  child=_bitree[_binode+0];
  depth0=child<=0?0:oc_huff_bitree_mindepth(_bitree,child);
  child=_bitree[_binode+1];
  depth1=child<=0?0:oc_huff_bitree_mindepth(_bitree,child);
  return OC_MINI(depth0,depth1)+1;
}

/*Finds the number of internal nodes at a given depth, plus the number of
   leaves at that depth or shallower.
  _bitree: A packed binary Huffman tree.
  _binode: The root of the given sub-tree.
  Return: The number of unique entries that would be contained in a jump table
           of the given depth.*/
static int oc_huff_bitree_occupancy(const ogg_int16_t *_bitree,int _binode,
 int _depth){
  int child1;
  int child2;
  if(_depth<=0)return 1;
  child1=_bitree[_binode+0];
  child2=_bitree[_binode+1];
  return (child1<=0?1:oc_huff_bitree_occupancy(_bitree,child1,_depth-1))+
   (child2<=0?1:oc_huff_bitree_occupancy(_bitree,child2,_depth-1));
}

/*Determines the size in words of a Huffman tree node that represents a
   subtree of depth _nbits.
  _nbits: The depth of the subtree.
          This must be greater than zero.
  Return: The number of words required to store the node.*/
static size_t oc_huff_node_size(int _nbits){
  return 1+(1<<_nbits);
}

/*Determines the size in words of a Huffman subtree.
  _tree: The complete Huffman tree.
  _node: The index of the root of the desired subtree.
  Return: The number of words required to store the tree.*/
static size_t oc_huff_tree_size(const ogg_int16_t *_tree,int _node){
  size_t size;
  int    nchildren;
  int    n;
  int    i;
  n=_tree[_node];
  size=oc_huff_node_size(n);
  nchildren=1<<n;
  i=0;
  do{
    int child;
    child=_tree[_node+i+1];
    if(child<=0)i+=1<<n-(-child>>8);
    else{
      size+=oc_huff_tree_size(_tree,child);
      i++;
    }
  }
  while(i<nchildren);
  return size;
}

/*Makes a copy of the given Huffman sub-tree.
  _dst: The destination tree.
  _src: The destination tree.
  _node: The index Huffman sub-tree to copy.
  Return: The index of the first available slot after the sub-tree copy.*/
static int oc_huff_tree_copy(ogg_int16_t *_dst,
 const ogg_int16_t *_src,int _node){
  int nchildren;
  int ret;
  int i;
  int n;
  n=_src[_node];
  _dst[_node++]=(ogg_int16_t)n;
  nchildren=1<<n;
  ret=_node+nchildren;
  for(i=0;i<nchildren;i++){
    int child;
    child=_src[_node+i];
    _dst[_node+i]=(ogg_int16_t)child;
    if(child>0){
      /*Note: The assignment to ret here assumes that storage for children is
         in order, and consecutive (e.g., child==ret before the call).*/
      ret=oc_huff_tree_copy(_dst,_src,child);
    }
  }
  return ret;
}

/*Computes the number of words of storage that will be required by
   oc_huff_tree_collapse() for the given sub-tree.
  _bitree: The binary tree to collapse.
  _binode: The index of the root of the sub-tree to fill it with.
           _bitree[_binode] must ==1.
  _depth:  The depth of the nodes that our parent in the collapsed sub-tree
            will fill its table with.
           Pass 0 for the root of the tree.
  Return: The number of words required to store the collapsed tree.*/
static size_t oc_huff_bitree_collapse_size(ogg_int16_t *_bitree,int _binode,
 int _depth){
  size_t size;
  int    child1;
  int    child2;
  if(_depth>0)size=0;
  else{
    int mindepth;
    int slush;
    int loccupancy;
    int occupancy;
    _depth=mindepth=oc_huff_bitree_mindepth(_bitree,_binode);
    occupancy=1<<mindepth;
    slush=_binode>0?OC_HUFF_SLUSH:OC_ROOT_HUFF_SLUSH;
    do{
      loccupancy=occupancy;
      occupancy=oc_huff_bitree_occupancy(_bitree,_binode,++_depth);
    }
    while(occupancy>loccupancy&&occupancy>=1<<OC_MAXI(_depth-slush,0));
    _depth--;
    size=oc_huff_node_size(_depth);
  }
  child1=_bitree[_binode+0];
  child2=_bitree[_binode+1];
  if(child1>0)size+=oc_huff_bitree_collapse_size(_bitree,child1,_depth-1);
  if(child2>0)size+=oc_huff_bitree_collapse_size(_bitree,child2,_depth-1);
  return size;
}

static int oc_huff_tree_collapse(ogg_int16_t *_tree,int *_node,
 const ogg_int16_t *_bitree,int _binode);

/*Fills the given nodes table with all the children in the sub-tree at the
   given depth.
  The nodes in the sub-tree with a depth less than that stored in the table
   are freed.
  The sub-tree must be binary and complete up until the given depth.
  _tree:   The storage for the collapsed Huffman tree.
  _node:   The index of the next available storage in the tree.
  _offs:   The index of the nodes to fill.
  _bitree: The binary tree to collapse.
  _binode: The index of the root of the sub-tree to fill it with.
           _bitree[_binode] must be <=1 .
  _level:  The current level in the table.
           0 indicates that the current node should be stored, regardless of
            whether it is a leaf node or an internal node.
  _depth:  The depth of the nodes to fill the table with, relative to their
            parent.*/
static void oc_huff_node_fill(ogg_int16_t *_tree,int *_node,int _offs,
 const ogg_int16_t *_bitree,int _binode,int _level,int _depth){
  if(_level<=0)_tree[_offs]=oc_huff_tree_collapse(_tree,_node,_bitree,_binode);
  else{
    int child1;
    int child2;
    int leaf;
    int i;
    _level--;
    child1=_bitree[_binode+0];
    if(child1<=0){
      leaf=-(_depth-_level<<8|-child1);
      for(i=0;i<1<<_level;i++)_tree[_offs+i]=(ogg_int16_t)leaf;
    }
    else oc_huff_node_fill(_tree,_node,_offs,_bitree,child1,_level,_depth);
    _offs+=1<<_level;
    child2=_bitree[_binode+1];
    if(child2<=0){
      leaf=-(_depth-_level<<8|-child2);
      for(i=0;i<1<<_level;i++)_tree[_offs+i]=(ogg_int16_t)leaf;
    }
    else oc_huff_node_fill(_tree,_node,_offs,_bitree,child2,_level,_depth);
  }
}

/*Finds the largest complete sub-tree rooted at the current node and collapses
   it into a single node.
  This procedure is then applied recursively to all the children of that node.
  _tree:   The storage for the collapsed Huffman tree.
  _node:   The index of the next available storage in the tree.
  _offs:   The index of the nodes to fill.
  _bitree: The binary tree to collapse.
  _binode: The index of the root of the sub-tree to fill it with.
           _bitree[_binode] must be <=1 .
  Return: The new root of the collapsed sub-tree.*/
static int oc_huff_tree_collapse(ogg_int16_t *_tree,int *_node,
 const ogg_int16_t *_bitree,int _binode){
  int mindepth;
  int depth;
  int loccupancy;
  int occupancy;
  int slush;
  int node;
  depth=mindepth=oc_huff_bitree_mindepth(_bitree,_binode);
  occupancy=1<<mindepth;
  slush=_binode>0?OC_HUFF_SLUSH:OC_ROOT_HUFF_SLUSH;
  do{
    loccupancy=occupancy;
    occupancy=oc_huff_bitree_occupancy(_bitree,_binode,++depth);
  }
  while(occupancy>loccupancy&&occupancy>=1<<OC_MAXI(depth-slush,0));
  depth--;
  node=*_node;
  _tree[node]=depth;
  *_node=node+1+(1<<depth);
  oc_huff_node_fill(_tree,_node,node+1,_bitree,_binode,depth,depth);
  return node;
}

/*Unpacks a set of Huffman trees, and reduces them to a collapsed
   representation.
  _opb:   The buffer to unpack the trees from.
  _nodes: The table to fill with the Huffman trees.
  Return: 0 on success, or a negative value on error.*/
int oc_huff_trees_unpack(oc_pack_buf *_opb,
 ogg_int16_t *_nodes[TH_NHUFFMAN_TABLES]){
  int i;
  for(i=0;i<TH_NHUFFMAN_TABLES;i++){
    ogg_int16_t   nodes[255*2];
    ogg_int16_t  *tree;
    size_t        size;
    int           node;
    int           root;
    /*Unpack the full tree into a temporary buffer.*/
    node=0;
    root=oc_huff_tree_unpack(_opb,nodes,sizeof(nodes)/sizeof(*nodes),&node);
    if(node>sizeof(nodes)/sizeof(*nodes))return TH_EBADHEADER;
    /*Figure out how big the collapsed tree will be.*/
    size=root<=0?3:oc_huff_bitree_collapse_size(nodes,0,0);
    tree=(ogg_int16_t *)_ogg_malloc(size*sizeof(*tree));
    if(tree==NULL)return TH_EFAULT;
    /*It's legal to have a tree with just a single node, which requires no bits
       to decode and always returns the same token.
      However, no encoder actually does this (yet).
      To avoid a special case in oc_huff_token_decode(), we construct a tree
       that looks ahead one bit, and then advances the stream 0 bits.*/
    if(root<=0){
      tree[0]=1;
      tree[1]=tree[2]=-(0<<8|-root);
    }
    /*Otherwise, collapse the tree.*/
    else{
      node=0;
      oc_huff_tree_collapse(tree,&node,nodes,0);
    }
    _nodes[i]=tree;
  }
  return 0;
}

/*Makes a copy of the given set of Huffman trees.
  _dst: The array to store the copy in.
  _src: The array of trees to copy.*/
int oc_huff_trees_copy(ogg_int16_t *_dst[TH_NHUFFMAN_TABLES],
 const ogg_int16_t *const _src[TH_NHUFFMAN_TABLES]){
  int total;
  int i;
  total=0;
  for(i=0;i<TH_NHUFFMAN_TABLES;i++){
    size_t       size;
    ogg_int16_t *tree;
    size=oc_huff_tree_size(_src[i],0);
    total+=size;
    tree=(ogg_int16_t *)_ogg_malloc(size*sizeof(*tree));
    if(tree==NULL){
      while(i-->0)_ogg_free(_dst[i]);
      return TH_EFAULT;
    }
    oc_huff_tree_copy(tree,_src[i],0);
    _dst[i]=tree;
  }
  return 0;
}

/*Frees the memory used by a set of Huffman trees.
  _nodes: The array of trees to free.*/
void oc_huff_trees_clear(ogg_int16_t *_nodes[TH_NHUFFMAN_TABLES]){
  int i;
  for(i=0;i<TH_NHUFFMAN_TABLES;i++)_ogg_free(_nodes[i]);
}


/*Unpacks a single token using the given Huffman tree.
  _opb:  The buffer to unpack the token from.
  _node: The tree to unpack the token with.
  Return: The token value.*/
int oc_huff_token_decode(oc_pack_buf *_opb,const ogg_int16_t *_tree){
  const unsigned char *ptr;
  const unsigned char *stop;
  oc_pb_window         window;
  int                  available;
  long                 bits;
  int                  node;
  int                  n;
  ptr=_opb->ptr;
  window=_opb->window;
  stop=_opb->stop;
  available=_opb->bits;
  node=0;
  for(;;){
    n=_tree[node];
    if(n>available){
      for(;;){
        /*We don't bother setting eof because we won't check for it after we've
           started decoding DCT tokens.*/
        if(ptr>=stop){
          available=OC_LOTS_OF_BITS;
          break;
        }
        if(available>OC_PB_WINDOW_SIZE-8)break;
        available+=8;
        window|=(oc_pb_window)*ptr++<<OC_PB_WINDOW_SIZE-available;
      }
      /*Note: We never request more than 24 bits, so there's no need to fill in
         the last partial byte here.*/
    }
    bits=window>>OC_PB_WINDOW_SIZE-n;
    node=_tree[node+1+bits];
    if(node<=0)break;
    window<<=n;
    available-=n;
  }
  node=-node;
  n=node>>8;
  window<<=n;
  available-=n;
  _opb->ptr=ptr;
  _opb->window=window;
  _opb->bits=available;
  return node&255;
}
