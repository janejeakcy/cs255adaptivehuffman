/*************************************************************************
 *
 *	File:    main.c
 *	Author:  Jing Huang & Liang Wu
 *
 *
 *	Description: example compressors trying to compress files with
 *  adaptive Huffman coding.
 *
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "huffman.h"
#include "timer.h"

int encodedFileLength;

/* create an input buffer for faster I/O */
#define IN_BUFSIZE 16384
unsigned char input_buf[ IN_BUFSIZE ];
unsigned int nread = 0, in_i = 0;

int GetFileLength(char *FileName)
{
	struct stat statistics;

	if (stat(FileName, &statistics) == -1) return 0;

	return (int)statistics.st_size;
}


void Enc(void)
{
	double WriteBytes, ReadBytes = 0;
	int symbol;
	double duration;
	char InFileName[50] = "D:\\SJSU\\c++program\\tt\\AdaptiveHuffmanCoding\\061";
	char OutFileName[50] = "D:\\SJSU\\c++program\\tt\\AdaptiveHuffmanCoding\\a";
	FILE *InFile, *OutFile;
	HUFFMANENCODER *HuffmanCoder;

	encodedFileLength = GetFileLength(InFileName);

	printf("Huffman Encoder 1.0 \n");

	if ((InFile = fopen(InFileName, "rb")) == NULL)
	{
		printf("fail to open file %s.\n", InFileName);
		exit(1);
	}

	if ((OutFile = fopen(OutFileName, "wb")) == NULL)
	{
		printf("fail to open file %s.\n", OutFileName);
		exit(1);
	}


	HuffmanCoder = HuffmanEncoderAlloc(OutFile, 1);

	StartTimer();
	/*for ( ; ; )
	{
		symbol = getc(InFile);

		if (symbol != EOF)
		{
			HuffmanEncoderEncode(HuffmanCoder, symbol);
		}
		else
		{
			//HuffmanEncoderEncode(HuffmanCoder, EndSymbol);
			break;
		}

		ReadBytes += 1;
	}*/


	while( true )
	{
		/* load the input buffer. */
		nread = fread( input_buf, 1, IN_BUFSIZE, InFile);
		if ( nread == 0 ) break;
		in_i = 0;
	
		/* get bytes from the buffer and compress them. */
		while( in_i < nread )
		{
			symbol = (unsigned char) *(input_buf + in_i);
			++in_i;
			if (symbol != EOF)
			{
				HuffmanEncoderEncode(HuffmanCoder, symbol);
			}
			else
			{
				//HuffmanEncoderEncode(HuffmanCoder, EndSymbol);
				break;
			}
	
			ReadBytes += 1;
		}
	}
	HuffmanEncoderFlush(HuffmanCoder);

	StopTimer();
	duration = ElapsedTime();
	printf("Encode time: %lf\n", duration);


	printf("ReadBytes : %d (%.3fk)\n", (int)ReadBytes, ReadBytes / 1024);
	WriteBytes = HuffmanEncoderBytesWrite(HuffmanCoder);
	printf("WriteBytes: %d (%.3fk)\n", (int)WriteBytes, WriteBytes / 1024);
	printf("compression ratio: %.2f%%\n", (double) WriteBytes / ReadBytes * 100);
	//printf("compression ratio: %.2f%%\n", (1 - WriteBytes / ReadBytes) * 100);

	HuffmanEncoderDealloc(HuffmanCoder);

	fclose(InFile);
	fclose(OutFile);

	printf("done.\n\n");
}


void Dec(void)
{
	int symbol;
	int length, count;
	double duration;
	char InFileName[50] = "D:\\SJSU\\c++program\\tt\\AdaptiveHuffmanCoding\\a";
	char OutFileName[50] = "D:\\SJSU\\c++program\\tt\\AdaptiveHuffmanCoding\\062";
	FILE *InFile, *OutFile;
	HUFFMANDECODER *HuffmanDecoder;

	printf("Huffman Decoder 1.0 \n");

	length = GetFileLength(InFileName);
	if ((InFile = fopen(InFileName, "rb")) == NULL)
	{
		printf("fail to open file %s.\n", InFileName);
		exit(1);
	}

	if ((OutFile = fopen(OutFileName, "wb")) == NULL)
	{
		printf("fail to open file %s.\n", OutFileName);
		exit(1);
	}


	count = 0;

	HuffmanDecoder = HuffmanDecoderAlloc(InFile, 1);

	StartTimer();
	for ( ; ; )
	{
		symbol = HuffmanDecoderDecode(HuffmanDecoder);
		count++;
		putc(symbol, OutFile);

		/*if (FGKDecoderBytesRead(HuffmanDecoder) == length )
		{
			break;
		}*/
		if(count == encodedFileLength)
		{
			break;
		}
	}

	StopTimer();
	duration = ElapsedTime();
	printf("Decode time: %lf\n", duration);


	HuffmanDecoderDealloc(HuffmanDecoder);

	fclose(InFile);
	fclose(OutFile);

	printf("done.\n\n");
}


int main(int argc, char **argv)
{
	Enc();
	Dec();

	return 0;
}
