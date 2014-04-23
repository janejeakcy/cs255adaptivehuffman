/*************************************************************************
 *
 *	File:	fgk.h
 *	Author:  Jing Huang & Liang Wu
 *
 *
 ************************************************************************/

#ifndef __FGK_H_
#define __FGK_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NUM_BITS_IN_INT      32

typedef struct FGKNode
{
	int symbol;
	int weight;
	int number;
	bool isLeft;
	struct FGKNode *parent, *left, *right;
}FGKTREENODE;

typedef struct
{
	FGKTREENODE *root;
	int maxNumber;
}FGKTREE;

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
	FGKTREE *tree;
} FGKENCODER, FGKDECODER, FGKCODER;




void FGKEncoderFlush(FGKENCODER *encoder);
FGKENCODER *FGKEncoderAlloc(void *stream, int IsFile);
void FGKEncoderEncode(FGKENCODER *encoder, int symbol);
void FGKEncoderDealloc(FGKENCODER *encoder);
int FGKEncoderBytesWrite(FGKDECODER *encoder);
FGKDECODER *FGKDEcoderAlloc(void *stream, int IsFile);
int FGKDecoderDecode(FGKDECODER *decoder);
void FGKDecoderDealloc(FGKDECODER *decoder);
int FGKDecoderBytesRead(FGKDECODER *decoder);

#endif