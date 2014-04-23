/*************************************************************************
 *
 *	File:	vitter.h
 *	Author:  Jing Huang & Liang Wu
 *
 *
 ************************************************************************/

#ifndef __VITTER_H_
#define __VITTER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NUM_BITS_IN_INT      32

typedef struct VITTERNode
{
	int symbol;
	int weight;
	int number;
	bool isLeft;
	struct VITTERNode *parent, *left, *right;
}VITTERTREENODE;

typedef struct
{
	VITTERTREENODE *root;
	int maxNumber;
}VITTERTREE;

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
	VITTERTREE *tree;
} VITTERENCODER, VITTERDECODER, VITTERCODER;




void VITTEREncoderFlush(VITTERENCODER *encoder);
VITTERENCODER *VITTEREncoderAlloc(void *stream, int IsFile);
void VITTEREncoderEncode(VITTERENCODER *encoder, int symbol);
void VITTEREncoderDealloc(VITTERENCODER *encoder);
int VITTEREncoderBytesWrite(VITTERDECODER *encoder);
VITTERDECODER *VITTERDEcoderAlloc(void *stream, int IsFile);
int VITTERDecoderDecode(VITTERDECODER *decoder);
void VITTERDecoderDealloc(VITTERDECODER *decoder);
int VITTERDecoderBytesRead(VITTERDECODER *decoder);

#endif