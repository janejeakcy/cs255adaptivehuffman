/*************************************************************************
 *
 *	File:	FGKFAST.c
 *	Author:  Jing Huang & Liang Wu
 *
 *	Description: adaptive Huffman coding -- FGKFAST
 *
 *	Update:		Version 1.0		04/04/2014	Initial version.  
 *
 *
 ************************************************************************/

#include "FGKFAST.h"

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

static int GetBit(FGKFASTDECODER *decoder)
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


static void PutBit(FGKFASTENCODER *encoder, int bit)
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

void FGKFASTEncoderFlush(FGKFASTENCODER *encoder)
{
	if (encoder->mask != 0x80)
	{
		PutByte(encoder->stream, encoder->rack, &(encoder->CurrentBytes), encoder->IsFile);
	}
}

/* pre-order print out every tree node */
static void PrintFGKFASTTree(FGKFASTTREENODE *localRoot)
{
	printf("number = %d, weight = %d, symbol = %d\n", localRoot->number, localRoot->weight, localRoot->symbol);
	
	if (localRoot->left != NULL)
	{
		PrintFGKFASTTree(localRoot->left);
	}
	if (localRoot->right != NULL)
	{
		PrintFGKFASTTree(localRoot->right);
	}
	
}

static FGKFASTTREENODE *FGKFASTTreeNodeInit(FGKFASTCODER *coder)
{	
	FGKFASTTREENODE *node;
	if ((node = (FGKFASTTREENODE *) malloc (sizeof(FGKFASTTREENODE))) == NULL)
	{
		printf("FGKFASTTreeNodeInit(): fail to allocate new node!");
		return NULL;
	}	
	
	node->symbol = -1;
	node->weight = 0;
	node->number = coder->tree->maxNumber + 1;
	node->isLeft = false;
	node->parent = NULL;
	node->left = NULL;
	node->right = NULL;
	
	coder->nodeList[coder->tree->maxNumber] = node;
	coder->tree->zeroNode = node;
	coder->tree->maxNumber++;
	
	return node;
}

static int FGKFASTTreeInit(FGKFASTCODER *coder)
{
	coder->tree->maxNumber = 0;
	if ((coder->tree->root = FGKFASTTreeNodeInit(coder)) == NULL)
	{
		printf("FGKFASTTreeInit(): fail to initiate root node!");
		return -1;
	}	
	
	return 0;
}

static int FGKFASTEncoderInit(FGKFASTENCODER *encoder, void *stream, int IsFile)
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
	
	if ((encoder->tree = (FGKFASTTREE *) malloc (sizeof(FGKFASTTREE))) == NULL)
	{
		printf("FGKFASTEncoderInit(): fail to allocate FGKFAST tree!\n");
		return -1;
	}
	
	if ((FGKFASTTreeInit(encoder)) == -1)
	{
		printf("FGKFASTEncoderInit(): fail to initiate FGKFAST tree!\n");
		return -1;
	} ;
	
	return 0;
}

FGKFASTENCODER *FGKFASTEncoderAlloc(void *stream, int IsFile)
{
	FGKFASTENCODER *encoder;

	if ((encoder = (FGKFASTENCODER *) malloc (sizeof(FGKFASTENCODER))) == NULL)
	{
		printf("FGKFASTEncoderAlloc(): fail to allocate FGKFAST encoder.\n");
		return NULL;
	}

	if (FGKFASTEncoderInit(encoder, stream, IsFile) == -1)
	{
		printf("FGKFASTEncoderAlloc(): fail to initiate FGKFAST encoder.\n");
		return NULL;
	}
	
	return encoder;
}


/* Recursive call to find Zero Node */
static FGKFASTTREENODE *findZeroNode(FGKFASTTREENODE *localRoot)
{
	FGKFASTTREENODE *node, *node1, *node2;
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

static void OutputNodeCode(FGKFASTENCODER *encoder, FGKFASTTREENODE *node)
{
	FGKFASTTREENODE *iter = node;
	char reversedOutputBits[256];
	int i, bit, depth;
	
	depth = 0;
	
	if (iter == encoder->tree->root)
	{
		 return;
	}
	
		
	/* start from the node to be encoded, find all of its ancestors all the way to the root except root,
       and record each bit along the way in reversedOutputBits arrays */
	while (iter != encoder->tree->root)
	{
		if (iter->isLeft)
		{
			reversedOutputBits[depth++] = 0;	
		}
		else
		{
			reversedOutputBits[depth++] = 1;
		}
		iter = iter->parent;
	}
	
	//PutBit(encoder, 0); // output root node bit
	
	/* output the succesive bits along the tree until the node, which have been stored in reversedOutputBits arrays */
	for (i = depth - 1; i >= 0; i--)
	{
		bit = reversedOutputBits[i];
		PutBit(encoder, bit);
	}
}

static void OutputZeroNodeCode(FGKFASTENCODER *encoder, FGKFASTTREENODE *zeroNode, int symbol)
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

static FGKFASTTREENODE *FGKFASTEncoderOutputZeroNodeCode(FGKFASTENCODER *encoder, int symbol)
{
	FGKFASTTREENODE *zeroNode;
	
	zeroNode = encoder->tree->zeroNode;
	
	OutputZeroNodeCode(encoder, zeroNode, symbol);
			
	return zeroNode;
}

/* Recursive call to find the correct Node */
static FGKFASTTREENODE *findNode(FGKFASTENCODER *encoder, int symbol)
{
	FGKFASTTREENODE *node = NULL;
	int i;
	
	for (i = 0; i < encoder->tree->maxNumber; i++)
	{
		if (encoder->nodeList[i]->symbol == symbol)
		{
			node = encoder->nodeList[i];	
			break;		
		}
	}
	return node;
}
	

static FGKFASTTREENODE *FGKFASTEncoderOutputNonZeroNodeCode(FGKFASTENCODER *encoder, int symbol)
{
	FGKFASTTREENODE *node;
	
	node = findNode(encoder, symbol);
	
	OutputNodeCode(encoder, node);
	
	return node;
}

static bool isExisted(FGKFASTCODER *coder, int symbol)
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

static FGKFASTTREENODE *FGKFASTEncoderOutputCode(FGKFASTENCODER *encoder, int symbol)
{
	FGKFASTTREENODE *node;
	
	if (isExisted(encoder, symbol))
	{
		node = FGKFASTEncoderOutputNonZeroNodeCode(encoder, symbol);
	}
	else
	{
		node = FGKFASTEncoderOutputZeroNodeCode(encoder, symbol);
	}
	
	return node;
}

static bool isZeroNodeSibling(FGKFASTTREENODE *node)
{
	FGKFASTTREENODE *sibling;
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

static bool isLeafNode(FGKFASTTREENODE *node)
{
	if (node->left == NULL)
	{
		return true;
	}
	return false;
}

/* pre-order traverse to find the nodes with the same weight */
static void findSameWeightNodes(FGKFASTTREENODE *localRoot, FGKFASTTREENODE *sameWeightNodes[256], int weight, int *count)
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

static FGKFASTTREENODE *findLowestNumberedLeaf(FGKFASTCODER *coder, FGKFASTTREENODE *node)
{
	FGKFASTTREENODE *iter = node;
	int i;
	
	i = node->number - 2;
	while (coder->nodeList[i]->weight == node->weight)
	{
		if (isLeafNode(coder->nodeList[i]))
		{
			iter = coder->nodeList[i];
		}
		i--;
	}
	
	return iter;
}
	

static FGKFASTTREENODE *findLowestNumberedNode(FGKFASTCODER *coder, FGKFASTTREENODE *node)
{
	FGKFASTTREENODE *iter = node;
	int i;
	
	i = node->number - 2;
	while (coder->nodeList[i]->weight == node->weight)
	{
		iter = coder->nodeList[i];
		i--;
	}
	
	return iter;
}

static void FGKFASTTreeUpdate(FGKFASTCODER *coder, FGKFASTTREENODE *node, int symbol)
{
	FGKFASTTREENODE *parentOfZeroNode, *newZeroNode, *iter, *lowestNumberLeaf, *lowestNumberNode, *tempNode;
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
		 
		parentOfZeroNode = FGKFASTTreeNodeInit(coder);
		newZeroNode = FGKFASTTreeNodeInit(coder);
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
		
		tempNode = coder->nodeList[parentOfZeroNode->number - 1];
		coder->nodeList[parentOfZeroNode->number - 1] = coder->nodeList[iter->number - 1];
		coder->nodeList[iter->number - 1] = tempNode;
		
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
		lowestNumberLeaf = findLowestNumberedLeaf(coder, iter);
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
			
			tempNode = coder->nodeList[iter->number - 1];
			coder->nodeList[iter->number - 1] = coder->nodeList[lowestNumberLeaf->number - 1];
			coder->nodeList[lowestNumberLeaf->number - 1] = tempNode;
					
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
		lowestNumberNode = findLowestNumberedNode(coder, iter);
		
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
			
			tempNode = coder->nodeList[iter->number - 1];
			coder->nodeList[iter->number - 1] = coder->nodeList[lowestNumberNode->number - 1];
			coder->nodeList[lowestNumberNode->number - 1] = tempNode;
					
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

void FGKFASTEncoderEncode(FGKFASTENCODER *encoder, int symbol)
{ 
	//printf("\n");
	//printf("symbol = %d\n", symbol);
	FGKFASTTREENODE *node = FGKFASTEncoderOutputCode(encoder, symbol);
	FGKFASTTreeUpdate(encoder, node, symbol);	
	//PrintFGKFASTTree(encoder->tree->root);
}

/* post-order traverse and delete tree nodes */
static void FGKFASTTreeNodesDealloc(FGKFASTTREENODE *localRoot)  
{
	if (localRoot == NULL) return;
	FGKFASTTreeNodesDealloc(localRoot->left);
	FGKFASTTreeNodesDealloc(localRoot->right);
	free(localRoot);
}

static void FGKFASTTreeDealloc(FGKFASTTREE *tree)
{
	if (tree == NULL) return;
    FGKFASTTreeNodesDealloc(tree->root);
	free(tree);
}

void FGKFASTEncoderDealloc(FGKFASTENCODER *encoder)
{
	if (encoder == NULL) return;
    if (encoder->tree) FGKFASTTreeDealloc(encoder->tree);
	free(encoder);
}

int FGKFASTEncoderBytesWrite(FGKFASTENCODER *encoder)
{
	return encoder->CurrentBytes;
}

static int FGKFASTDecoderInit(FGKFASTDECODER *decoder, void *stream, int IsFile)
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
	
	if ((decoder->tree = (FGKFASTTREE *) malloc (sizeof(FGKFASTTREE))) == NULL)
	{
		printf("FGKFASTDecoderInit(): fail to allocate FGKFAST tree!\n");
		return -1;
	}
	
	if ((FGKFASTTreeInit(decoder)) == -1)
	{
		printf("FGKFASTDecoderInit(): fail to initiate FGKFAST tree!\n");
		return -1;
	} ;
	
	return 0;
}


FGKFASTDECODER *FGKFASTDecoderAlloc(void *stream, int IsFile)
{
	FGKFASTDECODER *decoder;

	if ((decoder = (FGKFASTENCODER *) malloc (sizeof(FGKFASTENCODER))) == NULL)
	{
		printf("FGKFASTDecoderAlloc(): fail to allocate FGKFAST decoder.\n");
		return NULL;
	}

	if (FGKFASTDecoderInit(decoder, stream, IsFile) == -1)
	{
		printf("FGKFASTDecoderAlloc(): fail to initiate FGKFAST decoder.\n");
		return NULL;
	}
	
	return decoder;
}

static FGKFASTTREENODE *isFeasible(int bit, FGKFASTTREENODE *node)
{
	FGKFASTTREENODE *iter = NULL;
	
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

static FGKFASTTREENODE *FGKFASTDecoderOutputSymbol(FGKFASTDECODER *decoder, int *symbol)
{
	int bit, i;
	FGKFASTTREENODE *node, *iter;
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

int FGKFASTDecoderDecode(FGKFASTDECODER *decoder)
{
	int symbol = 0;
	
	FGKFASTTREENODE *node = FGKFASTDecoderOutputSymbol(decoder, &symbol);
	//printf("\n");
	//printf("symbol = %d\n", symbol);
	
	FGKFASTTreeUpdate(decoder, node, symbol);	
	//PrintFGKFASTTree(decoder->tree->root);
	
	return symbol;
}

void FGKFASTDecoderDealloc(FGKFASTDECODER *decoder)
{
	if (decoder == NULL) return;
    if (decoder->tree) FGKFASTTreeDealloc(decoder->tree);
	free(decoder);
}

int FGKFASTDecoderBytesRead(FGKFASTDECODER *decoder)
{
	return decoder->CurrentBytes;
}