/*************************************************************************
 *
 *	File:	fgk.c
 *	Author:  Jing Huang & Liang Wu
 *
 *	Description: adaptive Huffman coding -- FGK
 *
 *	Update:		Version 1.0		04/04/2014	Initial version.  
 *
 *
 ************************************************************************/

#include "fgk.h"

static int GetByte(void *stream, int *CurrentBytes, int IsFile)
{
	int value;
	
	if (IsFile)
	{
		value = getc((FILE *)stream);
	}
	else
	{
		value = ((unsigned char *)stream)[*CurrentBytes];
	}
	
	*CurrentBytes += 1;
	
	return value;
}


static void PutByte(void *stream, int value, int *CurrentBytes, int IsFile)
{
	if (IsFile)
	{
		putc(value, (FILE *)stream);
	}
	else
	{
		((unsigned char *)stream)[*CurrentBytes] = (unsigned char)value;
	}

	*CurrentBytes += 1;
}

static int GetBit(FGKDECODER *decoder)
{
	int value;
	
	if (decoder->mask == 0x80)
	{
        decoder->rack = GetByte(decoder->stream, &(decoder->CurrentBytes), decoder->IsFile);
	}
	
	value = decoder->rack & decoder->mask;
	decoder->mask >>= 1;
   
	if (decoder->mask == 0)
	{
		decoder->mask = 0x80;
	}
	
	return (value ? 1 : 0);
}


static void PutBit(FGKENCODER *encoder, int bit)
{
	if (bit)
	{
		encoder->rack |= encoder->mask;
	}
   
	encoder->mask >>= 1;
	
	if (encoder->mask == 0) 
	{
		PutByte(encoder->stream, encoder->rack, &(encoder->CurrentBytes), encoder->IsFile);
		encoder->mask = 0x80;
		encoder->rack = 0;
	}
}

void FGKEncoderFlush(FGKENCODER *encoder)
{
	if (encoder->mask != 0x80)
	{
		PutByte(encoder->stream, encoder->rack, &(encoder->CurrentBytes), encoder->IsFile);
	}
}

/* pre-order print out every tree node */
static void PrintFGKTree(FGKTREENODE *localRoot)
{
	printf("number = %d, weight = %d, symbol = %d\n", localRoot->number, localRoot->weight, localRoot->symbol);
	
	if (localRoot->left != NULL)
	{
		PrintFGKTree(localRoot->left);
	}
	if (localRoot->right != NULL)
	{
		PrintFGKTree(localRoot->right);
	}
	
}

static FGKTREENODE *FGKTreeNodeInit(FGKTREE *tree)
{	
	FGKTREENODE *node;
	if ((node = (FGKTREENODE *) malloc (sizeof(FGKTREENODE))) == NULL)
	{
		printf("FGKTreeNodeInit(): fail to allocate new node!");
		return NULL;
	}	
	
	node->symbol = -1;
	node->weight = 0;
	node->number = tree->maxNumber + 1;
	node->isLeft = false;
	node->parent = NULL;
	node->left = NULL;
	node->right = NULL;
		
	tree->maxNumber++;
	
	return node;
}

static int FGKTreeInit(FGKTREE *tree)
{
	tree->maxNumber = 0;
	if ((tree->root = FGKTreeNodeInit(tree)) == NULL)
	{
		printf("FGKTreeInit(): fail to initiate root node!");
		return -1;
	}
	
	return 0;
}

static int FGKEncoderInit(FGKENCODER *encoder, void *stream, int IsFile)
{
	int i;
	encoder->IsFile = IsFile;	
	encoder->OutBits = 0;
	encoder->CurrentBytes = 0;
	encoder->rack = 0;
	encoder->mask = 0x80;
	encoder->stream = stream;
	
	for (i = 0; i < 8; i++)
	{
		encoder->symbolRecord[i] = 0;
	}
	
	if ((encoder->tree = (FGKTREE *) malloc (sizeof(FGKTREE))) == NULL)
	{
		printf("FGKEncoderInit(): fail to allocate fgk tree!\n");
		return -1;
	}
	
	if ((FGKTreeInit(encoder->tree)) == -1)
	{
		printf("FGKEncoderInit(): fail to initiate fgk tree!\n");
		return -1;
	} ;
	
	return 0;
}

FGKENCODER *FGKEncoderAlloc(void *stream, int IsFile)
{
	FGKENCODER *encoder;

	if ((encoder = (FGKENCODER *) malloc (sizeof(FGKENCODER))) == NULL)
	{
		printf("FGKEncoderAlloc(): fail to allocate fgk encoder.\n");
		return NULL;
	}

	if (FGKEncoderInit(encoder, stream, IsFile) == -1)
	{
		printf("FGKEncoderAlloc(): fail to initiate fgk encoder.\n");
		return NULL;
	}
	
	return encoder;
}


/* Recursive call to find Zero Node */
static FGKTREENODE *findZeroNode(FGKTREENODE *localRoot)
{
	FGKTREENODE *node, *node1, *node2;
	node = NULL;
	node1 = NULL;
	node2 = NULL;
	
	if (localRoot != NULL && localRoot->weight == 0)
	{
		node = localRoot;
		return node;		
	}
	else if (localRoot != NULL && localRoot->weight != 0)
	{
		node1 = findZeroNode(localRoot->left);
		node2 = findZeroNode(localRoot->right);
		if (node1 != NULL)
		{
			return node1;
		}
		else if (node2 != NULL)
		{
			return node2;
		}
	}
	return node;	
}

static void OutputNodeCode(FGKENCODER *encoder, FGKTREENODE *node)
{
	FGKTREENODE *iter = node;
	int reversedOutputBits[8];
	int i, j, bit, currentDepth = 0;
	
	if (iter == encoder->tree->root)
	{
		 return;
	}
	
	for (i = 0; i < 8; i++)
	{
		reversedOutputBits[i] = 0;
	}
	
	/* start from the node to be encoded, find all of its ancestors all the way to the root except root,
       and record each bit along the way in reversedOutputBits arrays */
	while (iter != encoder->tree->root)
	{
		if (iter->isLeft)
		{
			reversedOutputBits[currentDepth / NUM_BITS_IN_INT] <<= 1;	
		}
		else
		{
			reversedOutputBits[currentDepth / NUM_BITS_IN_INT] = (reversedOutputBits[currentDepth / NUM_BITS_IN_INT] << 1) + 1;
		}
		currentDepth++;
		iter = iter->parent;
	}
	
	//PutBit(encoder, 0); // output root node bit
	
	/* output the succesive bits along the tree until the node, which have been stored in reversedOutputBits arrays */
	for (i = currentDepth / NUM_BITS_IN_INT; i >= 0; i--)
	{
		if (i == currentDepth / NUM_BITS_IN_INT)
		{
			for (j = 0; j < currentDepth % NUM_BITS_IN_INT; j++)
			{
				bit = (reversedOutputBits[i] >> j) & 1;
				PutBit(encoder, bit);
			}
		}
		else
		{
			for (j = 0; j < NUM_BITS_IN_INT; j++)
			{
				bit = (reversedOutputBits[i] >> j) & 1;
				PutBit(encoder, bit);
			}
		}
	}
}

static void OutputZeroNodeCode(FGKENCODER *encoder, FGKTREENODE *zeroNode, int symbol)
{
	int i, bit;
	
	OutputNodeCode(encoder, zeroNode);
	
	/* specify which symbol it is */	
	for (i = 7; i >= 0; i--)
	{
		bit = (symbol >> i) & 1;
		PutBit(encoder, bit);		

	}		
}

static FGKTREENODE *FGKEncoderOutputZeroNodeCode(FGKENCODER *encoder, int symbol)
{
	FGKTREENODE *zeroNode, *root;
	
	root = encoder->tree->root;
	
	zeroNode = findZeroNode(root);
	
	OutputZeroNodeCode(encoder, zeroNode, symbol);
			
	return zeroNode;
}

/* Recursive call to find the correct Node */
static FGKTREENODE *findNode(FGKTREENODE *localRoot, int symbol)
{
	FGKTREENODE *node, *node1, *node2;
	node = NULL;
	node1 = NULL;
	node2 = NULL;
	
	if (localRoot != NULL && localRoot->symbol == symbol)
	{
		node = localRoot;
		return node;		
	}
	else if (localRoot != NULL && localRoot->symbol != symbol)
	{
		node1 = findNode(localRoot->left, symbol);
		node2 = findNode(localRoot->right, symbol);
		if (node1 != NULL)
		{
			return node1;
		}
		else if (node2 != NULL)
		{
			return node2;
		}
	}
	return node;	
}

static FGKTREENODE *FGKEncoderOutputNonZeroNodeCode(FGKENCODER *encoder, int symbol)
{
	FGKTREENODE *node, *root;
	
	root = encoder->tree->root;
	
	node = findNode(root, symbol);
	
	OutputNodeCode(encoder, node);
	
	return node;
}

static bool isExisted(FGKCODER *coder, int symbol)
{
	/* if symbolBit is 1, then this symbol has existed */
	int i = symbol / NUM_BITS_IN_INT;
	int j = symbol % NUM_BITS_IN_INT;
	int symbolBit = (coder->symbolRecord[i] >> j) & 1;
	
	if (symbolBit)
	{
		return true;
	}
	return false;
}

static FGKTREENODE *FGKEncoderOutputCode(FGKENCODER *encoder, int symbol)
{
	FGKTREENODE *node;
	
	if (isExisted(encoder, symbol))
	{
		node = FGKEncoderOutputNonZeroNodeCode(encoder, symbol);
	}
	else
	{
		node = FGKEncoderOutputZeroNodeCode(encoder, symbol);
	}
	
	return node;
}

static bool isZeroNodeSibling(FGKTREENODE *node)
{
	FGKTREENODE *sibling;
	if (node->parent != NULL)
	{
		if (node->isLeft == true)
		{
			sibling = node->parent->right;
		}
		else
		{
			sibling = node->parent->left;
		}
		if (sibling->weight == 0)
		{
			return true;
		}	
	}
	return false;
}

static bool isLeafNode(FGKTREENODE *node)
{
	if (node->left == NULL)
	{
		return true;
	}
	return false;
}

/* pre-order traverse to find the nodes with the same weight */
static void findSameWeightNodes(FGKTREENODE *localRoot, FGKTREENODE *sameWeightNodes[256], int weight, int *count)
{
	if (localRoot->weight == weight)
	{
		sameWeightNodes[*count] = localRoot;
		*count += 1;
	}
	if (localRoot->left != NULL)
	{
		findSameWeightNodes(localRoot->left, sameWeightNodes, weight, count);
	}
	if (localRoot->right != NULL)
	{
		findSameWeightNodes(localRoot->right, sameWeightNodes, weight, count);
	}
}

static FGKTREENODE *findLowestNumberedLeaf(FGKTREE *tree, FGKTREENODE *node)
{
	FGKTREENODE *iter = NULL;
	FGKTREENODE *sameWeightNodes[256];
	int weight = node->weight;
	int i, count, number;
	
	count = 0;
	findSameWeightNodes(tree->root, sameWeightNodes, weight, &count);
	
	number = 99999;
	for (i = 0; i < count; i++)
	{
		if (sameWeightNodes[i]->number < number && isLeafNode(sameWeightNodes[i]))
		{
			iter = sameWeightNodes[i];
			number = sameWeightNodes[i]->number;
		}
	}
	
	return iter;
}

static FGKTREENODE *findLowestNumberedNode(FGKTREE *tree, FGKTREENODE *node)
{
	FGKTREENODE *iter;
	FGKTREENODE *sameWeightNodes[256];
	int weight = node->weight;
	int i, count, number;
	
	count = 0;
	findSameWeightNodes(tree->root, sameWeightNodes, weight, &count);
	number = sameWeightNodes[0]->number;
	number = 99999;
	for (i = 0; i < count; i++)
	{
		if (sameWeightNodes[i]->number < number)
		{
			iter = sameWeightNodes[i];
			number = sameWeightNodes[i]->number;
		}
	}
	
	return iter;
}

static void FGKTreeUpdate(FGKCODER *coder, FGKTREENODE *node, int symbol)
{
	FGKTREENODE *parentOfZeroNode, *newZeroNode, *iter, *lowestNumberLeaf, *lowestNumberNode, *tempNode;
	int tempNumber, i, j;
	bool tempIsLeft;
	iter = node;
	
	/* if iter is zero node */
	if (!isExisted(coder, symbol))
	{
		/* update the record of which symbol has existed */
		i = symbol / NUM_BITS_IN_INT;
		j = symbol % NUM_BITS_IN_INT;		
		coder->symbolRecord[i] |=  (1 << j);		
				
		/* replace iter by a parent 0-node with two leaf 0-node children, 
		 * and numbered in the order parent, left child, and right child (different from algorithm description),
		 * iter = left child just created */
		 
		parentOfZeroNode = FGKTreeNodeInit(coder->tree);
		newZeroNode = FGKTreeNodeInit(coder->tree);
		parentOfZeroNode->parent = iter->parent;
		
		if (iter->parent != NULL)
		{
			if (iter->isLeft) iter->parent->left = parentOfZeroNode;
			else iter->parent->right = parentOfZeroNode;
		}		
		parentOfZeroNode->left = iter;
		parentOfZeroNode->right = newZeroNode;
		iter->parent = parentOfZeroNode;
		newZeroNode->parent = parentOfZeroNode;
		tempNumber = parentOfZeroNode->number;
		parentOfZeroNode->number = iter->number;
		iter->number = tempNumber; 
		iter->isLeft = true;
		iter->symbol = symbol;
		
		/* if first symbol, then set root node as the parentOfZeroNode */
		if (coder->tree->maxNumber == 3)
		{
			coder->tree->root = parentOfZeroNode;
		}		
	}
	
	/* if iter is sibling of zero node */
	if (isZeroNodeSibling(iter))
	{
		/* find the lowest numbered leaf of the same weight */
		lowestNumberLeaf = findLowestNumberedLeaf(coder->tree, iter);
		if (iter != lowestNumberLeaf)
		{
			/* replace this leaf with iter */
			if (lowestNumberLeaf->isLeft)
			{
				lowestNumberLeaf->parent->left = iter;
			}
			else
			{
				lowestNumberLeaf->parent->right = iter;
			}
			if (iter->isLeft)
			{
				iter->parent->left = lowestNumberLeaf;
			}
			else
			{
				iter->parent->right = lowestNumberLeaf;
			}
			tempIsLeft = iter->isLeft;
			iter->isLeft = lowestNumberLeaf->isLeft;
			lowestNumberLeaf->isLeft = tempIsLeft;
			
			tempNode = iter->parent;
			iter->parent = lowestNumberLeaf->parent;
			lowestNumberLeaf->parent = tempNode;
			
					
			tempNumber = iter->number;
			iter->number = lowestNumberLeaf->number;
			lowestNumberLeaf->number = tempNumber;
		
		}
		
		/* increment iter's weight by 1 */
		iter->weight++;
		
		/* iter = iter's parent */
		iter = iter->parent;				
	}
	
	
	/* while iter is not the root */
	while (iter != coder->tree->root)
	{
		/* find the lowest numbered node of the same weight */
		lowestNumberNode = findLowestNumberedNode(coder->tree, iter);
		
		/* replace this node with iter */
		if (lowestNumberNode->isLeft)
			{
				lowestNumberNode->parent->left = iter;
			}
			else
			{
				lowestNumberNode->parent->right = iter;
			}
			if (iter->isLeft)
			{
				iter->parent->left = lowestNumberNode;
			}
			else
			{
				iter->parent->right = lowestNumberNode;
			}
			tempIsLeft = iter->isLeft;
			iter->isLeft = lowestNumberNode->isLeft;
			lowestNumberNode->isLeft = tempIsLeft;
			
			tempNode = iter->parent;
			iter->parent = lowestNumberNode->parent;
			lowestNumberNode->parent = tempNode;
			
					
			tempNumber = iter->number;
			iter->number = lowestNumberNode->number;
			lowestNumberNode->number = tempNumber;
		
		/* increment iter's weight by 1 */
		iter->weight++;
				
		/* iter = iter's parent */
		iter = iter->parent;
	}
	
	if (iter == coder->tree->root)
	{
		iter->weight++;
	}
	
}

void FGKEncoderEncode(FGKENCODER *encoder, int symbol)
{ 
	//printf("\n");
	//printf("symbol = %d\n", symbol);
	FGKTREENODE *node = FGKEncoderOutputCode(encoder, symbol);
	FGKTreeUpdate(encoder, node, symbol);	
	//PrintFGKTree(encoder->tree->root);
}

/* post-order traverse and delete tree nodes */
static void FGKTreeNodesDealloc(FGKTREENODE *localRoot)  
{
	if (localRoot == NULL) return;
	FGKTreeNodesDealloc(localRoot->left);
	FGKTreeNodesDealloc(localRoot->right);
	free(localRoot);
}

static void FGKTreeDealloc(FGKTREE *tree)
{
	if (tree == NULL) return;
    FGKTreeNodesDealloc(tree->root);
	free(tree);
}

void FGKEncoderDealloc(FGKENCODER *encoder)
{
	if (encoder == NULL) return;
    if (encoder->tree) FGKTreeDealloc(encoder->tree);
	free(encoder);
}

int FGKEncoderBytesWrite(FGKENCODER *encoder)
{
	return encoder->CurrentBytes;
}

static int FGKDecoderInit(FGKDECODER *decoder, void *stream, int IsFile)
{
	int i;
	decoder->IsFile = IsFile;	
	decoder->OutBits = 0;
	decoder->CurrentBytes = 0;
	decoder->rack = 0;
	decoder->mask = 0x80;
	decoder->stream = stream;
	decoder->hasBit = false;
	
	for (i = 0; i < 8; i++)
	{
		decoder->symbolRecord[i] = 0;
	}
	
	if ((decoder->tree = (FGKTREE *) malloc (sizeof(FGKTREE))) == NULL)
	{
		printf("FGKDecoderInit(): fail to allocate fgk tree!\n");
		return -1;
	}
	
	if ((FGKTreeInit(decoder->tree)) == -1)
	{
		printf("FGKDecoderInit(): fail to initiate fgk tree!\n");
		return -1;
	} ;
	
	return 0;
}


FGKDECODER *FGKDecoderAlloc(void *stream, int IsFile)
{
	FGKDECODER *decoder;

	if ((decoder = (FGKDECODER *) malloc (sizeof(FGKDECODER))) == NULL)
	{
		printf("FGKDecoderAlloc(): fail to allocate fgk decoder.\n");
		return NULL;
	}

	if (FGKDecoderInit(decoder, stream, IsFile) == -1)
	{
		printf("FGKDecoderAlloc(): fail to initiate fgk decoder.\n");
		return NULL;
	}
	
	return decoder;
}

static FGKTREENODE *isFeasible(int bit, FGKTREENODE *node)
{
	FGKTREENODE *iter = NULL;
	
	if (bit == 0)
	{
		if (node->left != NULL)
		{
			iter = node->left;			
		}
	}
	else
	{
		if (node->right != NULL)
		{
			iter = node->right;
		}		
	}
	return iter;
}

static FGKTREENODE *FGKDecoderOutputSymbol(FGKDECODER *decoder, int *symbol)
{
	int bit, i;
	FGKTREENODE *node, *iter;
	iter = decoder->tree->root;
	
	/* read in first bit of child of root*/
	if (decoder->hasBit)
	{
		bit = decoder->bit;
	}
	else
	{
		bit = GetBit(decoder);	
	}
 
	while ((node = isFeasible(bit, iter)) != NULL)
	{
		bit = GetBit(decoder);
		iter = node;
	}
	
	/* need to store the last bit since this is used to decode the next symbol or read in new symbol*/
	decoder->bit = bit;
	decoder->hasBit = true;
	
	node = iter;
	
	if (node->weight == 0)  // zero node
	{
		/* read the new symbol */
		bit = decoder->bit;
		decoder->hasBit = false;
		*symbol |= bit;		
		for (i = 0; i < 7; i++)
		{
			*symbol <<= 1;
			bit = GetBit(decoder);
			*symbol |= bit;
		}
	}
	else
	{
		*symbol = node->symbol;
	}
	
	return node;
}

int FGKDecoderDecode(FGKDECODER *decoder)
{
	int symbol = 0;
	
	FGKTREENODE *node = FGKDecoderOutputSymbol(decoder, &symbol);
	//printf("\n");
	//printf("symbol = %d\n", symbol);
	
	FGKTreeUpdate(decoder, node, symbol);	
	//PrintFGKTree(decoder->tree->root);
	
	return symbol;
}

void FGKDecoderDealloc(FGKDECODER *decoder)
{
	if (decoder == NULL) return;
    if (decoder->tree) FGKTreeDealloc(decoder->tree);
	free(decoder);
}

int FGKDecoderBytesRead(FGKDECODER *decoder)
{
	return decoder->CurrentBytes;
}