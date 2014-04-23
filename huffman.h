/*************************************************************************
 *
 *	File:	huffman.h
 *	Author:	Jing Huang & Liang Wu
 *
 ************************************************************************/

#ifndef __HUFFMAN_H_
#define __HUFFMAN_H_

#include "fgk.h"
#include "vitter.h"
#include "fgkFast.h"
#include "vitterFast.h"


//#define __USE_FGK__ // FGK
//#define __USE_VITTER__ // VITTER
#define __USE_FGK_FAST__ // FGKFAST
//#define __USE_VITTER_FAST__ // VITTERFAST

 
#ifdef __USE_FGK__ // FGK
typedef	FGKENCODER	HUFFMANENCODER;
typedef	FGKDECODER	HUFFMANDECODER;
#define HuffmanEncoderFlush(encoder) FGKEncoderFlush(encoder)
#define HuffmanEncoderEncode(encoder, symbol) FGKEncoderEncode(encoder, symbol)
#define HuffmanEncoderAlloc(stream, IsFile) FGKEncoderAlloc(stream, IsFile)
#define HuffmanEncoderDealloc(encoder) FGKEncoderDealloc(encoder)
#define HuffmanEncoderBytesWrite(encoder) FGKEncoderBytesWrite(encoder)
#define HuffmanDecoderDecode(decoder) FGKDecoderDecode(decoder)
#define HuffmanDecoderAlloc(stream, IsFile) FGKDecoderAlloc(stream, IsFile)
#define HuffmanDecoderDealloc(decoder) FGKDecoderDealloc(decoder)
#define HuffmanDecoderBytesRead(decoder) FGKDecoderBytesRead(decoder)
#endif         
    
#ifdef __USE_FGK_FAST__ // FGKFAST
typedef	FGKFASTENCODER	HUFFMANENCODER;
typedef	FGKFASTDECODER	HUFFMANDECODER;
#define HuffmanEncoderFlush(encoder) FGKFASTEncoderFlush(encoder)
#define HuffmanEncoderEncode(encoder, symbol) FGKFASTEncoderEncode(encoder, symbol)
#define HuffmanEncoderAlloc(stream, IsFile) FGKFASTEncoderAlloc(stream, IsFile)
#define HuffmanEncoderDealloc(encoder) FGKFASTEncoderDealloc(encoder)
#define HuffmanEncoderBytesWrite(encoder) FGKFASTEncoderBytesWrite(encoder)
#define HuffmanDecoderDecode(decoder) FGKFASTDecoderDecode(decoder)
#define HuffmanDecoderAlloc(stream, IsFile) FGKFASTDecoderAlloc(stream, IsFile)
#define HuffmanDecoderDealloc(decoder) FGKFASTDecoderDealloc(decoder)
#define HuffmanDecoderBytesRead(decoder) FGKDecoderBytesRead(decoder)
#endif   

#ifdef __USE_VITTER__ // VITTER
typedef	VITTERENCODER	HUFFMANENCODER;
typedef	VITTERDECODER	HUFFMANDECODER;
#define HuffmanEncoderFlush(encoder) VITTEREncoderFlush(encoder)
#define HuffmanEncoderEncode(encoder, symbol) VITTEREncoderEncode(encoder, symbol)
#define HuffmanEncoderAlloc(stream, IsFile) VITTEREncoderAlloc(stream, IsFile)
#define HuffmanEncoderDealloc(encoder) VITTEREncoderDealloc(encoder)
#define HuffmanEncoderBytesWrite(encoder) VITTEREncoderBytesWrite(encoder)
#define HuffmanDecoderDecode(decoder) VITTERDecoderDecode(decoder)
#define HuffmanDecoderAlloc(stream, IsFile) VITTERDecoderAlloc(stream, IsFile)
#define HuffmanDecoderDealloc(decoder) VITTERDecoderDealloc(decoder)
#define HuffmanDecoderBytesRead(decoder) VITTERDecoderBytesRead(decoder)
#endif         


#ifdef __USE_VITTER_FAST__ // VITTERFAST
typedef	VITTERFASTENCODER	HUFFMANENCODER;
typedef	VITTERFASTDECODER	HUFFMANDECODER;
#define HuffmanEncoderFlush(encoder) VITTERFASTEncoderFlush(encoder)
#define HuffmanEncoderEncode(encoder, symbol) VITTERFASTEncoderEncode(encoder, symbol)
#define HuffmanEncoderAlloc(stream, IsFile) VITTERFASTEncoderAlloc(stream, IsFile)
#define HuffmanEncoderDealloc(encoder) VITTERFASTEncoderDealloc(encoder)
#define HuffmanEncoderBytesWrite(encoder) VITTERFASTEncoderBytesWrite(encoder)
#define HuffmanDecoderDecode(decoder) VITTERFASTDecoderDecode(decoder)
#define HuffmanDecoderAlloc(stream, IsFile) VITTERFASTDecoderAlloc(stream, IsFile)
#define HuffmanDecoderDealloc(decoder) VITTERFASTDecoderDealloc(decoder)
#define HuffmanDecoderBytesRead(decoder) VITTERFASTDecoderBytesRead(decoder)
#endif 

#endif
