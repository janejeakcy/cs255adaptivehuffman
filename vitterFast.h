/*************************************************************************
 *
 *	File:	VitterFast.h
 *	Author:  Jing Huang & Liang Wu
 *
 *
 ************************************************************************/

#ifndef __VITTERFAST_H_
#define __VITTERFAST_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NUM_BITS_IN_INT      32

typedef struct VITTERFASTNode
{
	int symbol;
	int weight;
	int number;
	bool isLeft;
	struct VITTERFASTNode *parent, *left, *right;
}VITTERFASTTREENODE;

typedef struct
{
	VITTERFASTTREENODE *root;
	VITTERFASTTREENODE *zeroNode;
	int maxNumber;
}VITTERFASTTREE;

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
	VITTERFASTTREE *tree;
	VITTERFASTTREENODE *nodeList[513];
} VITTERFASTENCODER, VITTERFASTDECODER, VITTERFASTCODER;




void VITTERFASTEncoderFlush(VITTERFASTENCODER *encoder);
VITTERFASTENCODER *VITTERFASTEncoderAlloc(void *stream, int IsFile);
void VITTERFASTEncoderEncode(VITTERFASTENCODER *encoder, int symbol);
void VITTERFASTEncoderDealloc(VITTERFASTENCODER *encoder);
int VITTERFASTEncoderBytesWrite(VITTERFASTDECODER *encoder);
VITTERFASTDECODER *VITTERFASTDEcoderAlloc(void *stream, int IsFile);
int VITTERFASTDecoderDecode(VITTERFASTDECODER *decoder);
void VITTERFASTDecoderDealloc(VITTERFASTDECODER *decoder);
int VITTERFASTDecoderBytesRead(VITTERFASTDECODER *decoder);

#endif