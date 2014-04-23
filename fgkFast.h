/*************************************************************************
 *
 *	File:	FGKFAST.h
 *	Author:  Jing Huang & Liang Wu
 *
 *
 ************************************************************************/

#ifndef __FGKFAST_H_
#define __FGKFAST_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NUM_BITS_IN_INT      32

typedef struct FGKFASTNode
{
	int symbol;
	int weight;
	int number;
	bool isLeft;
	struct FGKFASTNode *parent, *left, *right;
}FGKFASTTREENODE;

typedef struct
{
	FGKFASTTREENODE *root;
	FGKFASTTREENODE *zeroNode;
	int maxNumber;
}FGKFASTTREE;

typedef struct 
{
	int IsFile;	
    int rack, value, CurrentBytes;
	int InBits, OutBits;
	int symbolRecord[8];
	int bit;
	bool hasBit;
	unsigned char mask;
	void *stream;
	FGKFASTTREE *tree;
	FGKFASTTREENODE *nodeList[513];
} FGKFASTENCODER, FGKFASTDECODER, FGKFASTCODER;




void FGKFASTEncoderFlush(FGKFASTENCODER *encoder);
FGKFASTENCODER *FGKFASTEncoderAlloc(void *stream, int IsFile);
void FGKFASTEncoderEncode(FGKFASTENCODER *encoder, int symbol);
void FGKFASTEncoderDealloc(FGKFASTENCODER *encoder);
int FGKFASTEncoderBytesWrite(FGKFASTDECODER *encoder);
FGKFASTDECODER *FGKFASTDEcoderAlloc(void *stream, int IsFile);
int FGKFASTDecoderDecode(FGKFASTDECODER *decoder);
void FGKFASTDecoderDealloc(FGKFASTDECODER *decoder);
int FGKFASTDecoderBytesRead(FGKFASTDECODER *decoder);

#endif