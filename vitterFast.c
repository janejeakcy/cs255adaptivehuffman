/*************************************************************************
 *
 *	File:	VITTERFAST.c
 *	Author:  Jing Huang & Liang Wu
 *
 *	Description: adaptive Huffman coding -- VITTERFAST
 *
 *	Update:		Version 1.0		04/04/2014	Initial version.  
 *
 *
 ************************************************************************/

#include "VITTERFAST.h"

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

static int GetBit(VITTERFASTDECODER *decoder)
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


static void PutBit(VITTERFASTENCODER *encoder, int bit)
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

void VITTERFASTEncoderFlush(VITTERFASTENCODER *encoder)
{
	if (encoder->mask != 0x80)
	{
		PutByte(encoder->stream, encoder->rack, &(encoder->CurrentBytes), encoder->IsFile);
	}
}

/* pre-order print out every tree node */
static void PrintVITTERFASTTree(VITTERFASTTREENODE *localRoot)
{
	printf("number = %d, weight = %d, symbol = %d\n", localRoot->number, localRoot->weight, localRoot->symbol);
	
	if (localRoot->left != NULL)
	{
		PrintVITTERFASTTree(localRoot->left);
	}
	if (localRoot->right != NULL)
	{
		PrintVITTERFASTTree(localRoot->right);
	}
	
}

static VITTERFASTTREENODE *VITTERFASTTreeNodeInit(VITTERFASTCODER *coder)
{	
	VITTERFASTTREENODE *node;
	if ((node = (VITTERFASTTREENODE *) malloc (sizeof(VITTERFASTTREENODE))) == NULL)
	{
		printf("VITTERFASTTreeNodeInit(): fail to allocate new node!");
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

static int VITTERFASTTreeInit(VITTERFASTCODER *coder)
{
	coder->tree->maxNumber = 0;
	if ((coder->tree->root = VITTERFASTTreeNodeInit(coder)) == NULL)
	{
		printf("VITTERFASTTreeInit(): fail to initiate root node!");
		return -1;
	}	
	
	return 0;
}

static int VITTERFASTEncoderInit(VITTERFASTENCODER *encoder, void *stream, int IsFile)
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
	
	if ((encoder->tree = (VITTERFASTTREE *) malloc (sizeof(VITTERFASTTREE))) == NULL)
	{
		printf("VITTERFASTEncoderInit(): fail to allocate VITTERFAST tree!\n");
		return -1;
	}
	
	if ((VITTERFASTTreeInit(encoder)) == -1)
	{
		printf("VITTERFASTEncoderInit(): fail to initiate VITTERFAST tree!\n");
		return -1;
	} ;
	
	return 0;
}

VITTERFASTENCODER *VITTERFASTEncoderAlloc(void *stream, int IsFile)
{
	VITTERFASTENCODER *encoder;

	if ((encoder = (VITTERFASTENCODER *) malloc (sizeof(VITTERFASTENCODER))) == NULL)
	{
		printf("VITTERFASTEncoderAlloc(): fail to allocate VITTERFAST encoder.\n");
		return NULL;
	}

	if (VITTERFASTEncoderInit(encoder, stream, IsFile) == -1)
	{
		printf("VITTERFASTEncoderAlloc(): fail to initiate VITTERFAST encoder.\n");
		return NULL;
	}
	
	return encoder;
}


/* Recursive call to find Zero Node */
static VITTERFASTTREENODE *findZeroNode(VITTERFASTTREENODE *localRoot)
{
	VITTERFASTTREENODE *node, *node1, *node2;
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

static void OutputNodeCode(VITTERFASTENCODER *encoder, VITTERFASTTREENODE *node)
{
	VITTERFASTTREENODE *iter = node;
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

static void OutputZeroNodeCode(VITTERFASTENCODER *encoder, VITTERFASTTREENODE *zeroNode, int symbol)
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

static VITTERFASTTREENODE *VITTERFASTEncoderOutputZeroNodeCode(VITTERFASTENCODER *encoder, int symbol)
{
	VITTERFASTTREENODE *zeroNode;
	
	zeroNode = encoder->tree->zeroNode;
	
	OutputZeroNodeCode(encoder, zeroNode, symbol);
			
	return zeroNode;
}

/* Recursive call to find the correct Node */
static VITTERFASTTREENODE *findNode(VITTERFASTENCODER *encoder, int symbol)
{
	VITTERFASTTREENODE *node = NULL;
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
	

static VITTERFASTTREENODE *VITTERFASTEncoderOutputNonZeroNodeCode(VITTERFASTENCODER *encoder, int symbol)
{
	VITTERFASTTREENODE *node;
	
	node = findNode(encoder, symbol);
	
	OutputNodeCode(encoder, node);
	
	return node;
}

static bool isExisted(VITTERFASTCODER *coder, int symbol)
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

static VITTERFASTTREENODE *VITTERFASTEncoderOutputCode(VITTERFASTENCODER *encoder, int symbol)
{
	VITTERFASTTREENODE *node;
	
	if (isExisted(encoder, symbol))
	{
		node = VITTERFASTEncoderOutputNonZeroNodeCode(encoder, symbol);
	}
	else
	{
		node = VITTERFASTEncoderOutputZeroNodeCode(encoder, symbol);
	}
	
	return node;
}

static bool isZeroNodeSibling(VITTERFASTTREENODE *node)
{
	VITTERFASTTREENODE *sibling;
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

static bool isLeafNode(VITTERFASTTREENODE *node)
{
	if (node->left == NULL)
	{
		return true;
	}
	return false;
}


static void findSameWeightInternalNodes(VITTERFASTCODER *coder, VITTERFASTTREENODE *sameWeightNodes[256], VITTERFASTTREENODE *node, int *count)
{
	int i =  node->number - 2;
	int j;
	while (coder->nodeList[i]->weight == node->weight)
	{		
		i--;
	}
	for (j = i + 1; j < node->number - 1; j++)
	{
		sameWeightNodes[*count] = coder->nodeList[j];
		*count += 1;
	}
}

	
static void findSameWeightLeafNodes(VITTERFASTCODER *coder, VITTERFASTTREENODE *sameWeightNodes[256], VITTERFASTTREENODE *node, int *count)
{
	int i =  node->number - 2;
	int j;
	while (coder->nodeList[i]->weight == node->weight + 1 && isLeafNode(coder->nodeList[i]))
	{		
		i--;
	}
	for (j = i + 1; j < node->number - 1; j++)
	{
		sameWeightNodes[*count] = coder->nodeList[j];
		*count += 1;
	}
}

VITTERFASTTREENODE *findLeaderInLeafBlock(VITTERFASTCODER *coder, VITTERFASTTREENODE *node)
{
	VITTERFASTTREENODE *iter = node;
	int i;
	
	i = node->number - 2;
	while (coder->nodeList[i]->weight == node->weight && isLeafNode(coder->nodeList[i]))
	{
		iter = coder->nodeList[i];
		i--;
	}
	
	return iter;
}

int partition(VITTERFASTTREENODE *sameWeightNodes[256], int p, int r)
{
	int i, j;
	VITTERFASTTREENODE *pivotNode, *tempNode;
	pivotNode= sameWeightNodes[r];
	i = p - 1;
		
	for (j = p; j < r; j++)
	{
		if (sameWeightNodes[j]->number < pivotNode->number)
		{
			tempNode = sameWeightNodes[j];
			sameWeightNodes[j] = sameWeightNodes[i + 1];
			sameWeightNodes[i + 1] = tempNode;
			i++;
		}
	}
	tempNode = sameWeightNodes[i + 1];
	sameWeightNodes[i + 1] = sameWeightNodes[r];
	sameWeightNodes[r] = tempNode;
	
	return (i + 1);
}

void quickSort(VITTERFASTTREENODE *sameWeightNodes[256], int p, int r)
{
	int q;
	if (p < r)
	{
		q = partition(sameWeightNodes, p, r);
		quickSort(sameWeightNodes, p, q - 1);
		quickSort(sameWeightNodes, q + 1, r);
	}
}

void sortByIncrementNumber(VITTERFASTTREENODE *sameWeightNodes[256], int count)
{
	quickSort(sameWeightNodes, 0, count -1);
}

void slideNodes(VITTERFASTTREENODE *sameWeightNodes[256], VITTERFASTTREENODE *node, VITTERFASTCODER *coder, int count)
{
	VITTERFASTTREENODE *iter, *tempNode, *tempNode2;
	int i, tempNumber;
	bool tempIsLeft;
	
	tempNode2 = coder->nodeList[node->number - 1];
	coder->nodeList[node->number - 1] = coder->nodeList[sameWeightNodes[count - 1]->number - 1];
	for (i = count - 1; i >= 1; i--)
	{
		coder->nodeList[sameWeightNodes[i]->number - 1] = coder->nodeList[sameWeightNodes[i - 1]->number - 1];
	}
	coder->nodeList[sameWeightNodes[0]->number - 1] = tempNode2;
	
	
	tempNode = sameWeightNodes[0]->parent;
	tempIsLeft = sameWeightNodes[0]->isLeft;
	tempNumber = sameWeightNodes[0]->number;	
	
	for (i = 0; i < count - 1; i++)
	{
		iter = sameWeightNodes[i];			
		iter->parent = sameWeightNodes[i + 1]->parent;
		iter->isLeft = sameWeightNodes[i + 1]->isLeft;
		iter->number = sameWeightNodes[i + 1]->number;
		if (sameWeightNodes[i + 1]->parent != NULL)
		{
			if (sameWeightNodes[i + 1]->isLeft)
				sameWeightNodes[i + 1]->parent->left = iter;
			else
				sameWeightNodes[i + 1]->parent->right = iter;
		}
	}
	iter = sameWeightNodes[count - 1];
	iter->parent = node->parent;
	iter->isLeft = node->isLeft;
	iter->number = node->number;
	if (node->parent != NULL)
	{
		if (node->isLeft)
			node->parent->left = iter;
		else
			node->parent->right = iter;
	}
	
	iter = node;
	iter->parent = tempNode;
	iter->isLeft = tempIsLeft;
	iter->number = tempNumber;
	if (tempNode != NULL)
	{
		if (tempIsLeft)
			tempNode->left = iter;
		else
			tempNode->right = iter;
	}
	
	
}


VITTERFASTTREENODE *slideAndIncrement(VITTERFASTCODER *coder, VITTERFASTTREENODE *node)
{
	VITTERFASTTREENODE *iter = NULL;
	VITTERFASTTREENODE *sameWeightNodes[256];
	VITTERFASTTREENODE *tempNode;
	int weight = node->weight;
	int i, count, number;
	bool tempIsLeft;
	iter = node;	
	count = 0;
	
	if (isLeafNode(node))
	{
		findSameWeightInternalNodes(coder, sameWeightNodes, node, &count);
		//printSameWeightNodes(sameWeightNodes, count);
		if (count > 0)
		{
			//sortByIncrementNumber(sameWeightNodes, count);
			slideNodes(sameWeightNodes, iter, coder, count);
		}
		iter->weight++;
		iter = iter->parent;		
	}
	else
	{
		tempNode = iter->parent;
		findSameWeightLeafNodes(coder, sameWeightNodes, node, &count);
		//printSameWeightNodes(sameWeightNodes, count);
		if (count > 0)
		{
			//sortByIncrementNumber(sameWeightNodes, count);
			slideNodes(sameWeightNodes, iter, coder, count);
		}
		iter->weight++;
		iter = tempNode;		
	}
	return iter;
}
		

static void VITTERFASTTreeUpdate(VITTERFASTCODER *coder, VITTERFASTTREENODE *node, int symbol)
{
	VITTERFASTTREENODE *parentOfZeroNode, *newZeroNode, *iter, *tempNode, *leader, *parentOfIter;
	VITTERFASTTREENODE *leafToIncrement = NULL;	
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
		 
		parentOfZeroNode = VITTERFASTTreeNodeInit(coder);
		newZeroNode = VITTERFASTTreeNodeInit(coder);
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
		leafToIncrement = iter;
		iter = parentOfZeroNode;
		
		/* if first symbol, then set root node as the parentOfZeroNode */
		if (coder->tree->maxNumber == 3)
		{
			coder->tree->root = parentOfZeroNode;
		}		
	}
	/* if symbol has already existed */
	else
	{
		/* find the leader of the same block */
		leader = findLeaderInLeafBlock(coder, iter);

		if (iter != leader)
		{
			/* replace this leaf with iter */
			if (leader->isLeft)
			{
				leader->parent->left = iter;
			}
			else
			{
				leader->parent->right = iter;
			}
			if (iter->isLeft)
			{
				iter->parent->left = leader;
			}
			else
			{
				iter->parent->right = leader;
			}
			tempIsLeft = iter->isLeft;
			iter->isLeft = leader->isLeft;
			leader->isLeft = tempIsLeft;
			
			tempNode = iter->parent;
			iter->parent = leader->parent;
			leader->parent = tempNode;
			
			tempNode = coder->nodeList[iter->number - 1];
			coder->nodeList[iter->number - 1] = coder->nodeList[leader->number - 1];
			coder->nodeList[leader->number - 1] = tempNode;
					
			tempNumber = iter->number;
			iter->number = leader->number;
			leader->number = tempNumber;
		}
	
		
		/* if iter is sibling of zero node */
		if (isZeroNodeSibling(iter))
		{			
			/* leader = iter */
			leafToIncrement = iter;
		
			/* iter = iter's parent */
			iter = iter->parent;				
		}
		
	}
	
	/* while iter is not the root */
	while (iter != coder->tree->root)
	{
		parentOfIter = slideAndIncrement(coder, iter);
		iter = parentOfIter;
	}
	if (iter == coder->tree->root)
	{
		iter->weight++;
	}
	if (leafToIncrement != NULL)
	{
		parentOfIter = slideAndIncrement(coder, leafToIncrement);
	}
}



void VITTERFASTEncoderEncode(VITTERFASTENCODER *encoder, int symbol)
{ 
	//printf("\n");
	//printf("symbol = %d\n", symbol);
	VITTERFASTTREENODE *node = VITTERFASTEncoderOutputCode(encoder, symbol);
	VITTERFASTTreeUpdate(encoder, node, symbol);	
	//PrintVITTERFASTTree(encoder->tree->root);
}

/* post-order traverse and delete tree nodes */
static void VITTERFASTTreeNodesDealloc(VITTERFASTTREENODE *localRoot)  
{
	if (localRoot == NULL) return;
	VITTERFASTTreeNodesDealloc(localRoot->left);
	VITTERFASTTreeNodesDealloc(localRoot->right);
	free(localRoot);
}

static void VITTERFASTTreeDealloc(VITTERFASTTREE *tree)
{
	if (tree == NULL) return;
    VITTERFASTTreeNodesDealloc(tree->root);
	free(tree);
}

void VITTERFASTEncoderDealloc(VITTERFASTENCODER *encoder)
{
	if (encoder == NULL) return;
    if (encoder->tree) VITTERFASTTreeDealloc(encoder->tree);
	free(encoder);
}

int VITTERFASTEncoderBytesWrite(VITTERFASTENCODER *encoder)
{
	return encoder->CurrentBytes;
}

static int VITTERFASTDecoderInit(VITTERFASTDECODER *decoder, void *stream, int IsFile)
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
	
	if ((decoder->tree = (VITTERFASTTREE *) malloc (sizeof(VITTERFASTTREE))) == NULL)
	{
		printf("VITTERFASTDecoderInit(): fail to allocate VITTERFAST tree!\n");
		return -1;
	}
	
	if ((VITTERFASTTreeInit(decoder)) == -1)
	{
		printf("VITTERFASTDecoderInit(): fail to initiate VITTERFAST tree!\n");
		return -1;
	} ;
	
	return 0;
}


VITTERFASTDECODER *VITTERFASTDecoderAlloc(void *stream, int IsFile)
{
	VITTERFASTDECODER *decoder;

	if ((decoder = (VITTERFASTENCODER *) malloc (sizeof(VITTERFASTENCODER))) == NULL)
	{
		printf("VITTERFASTDecoderAlloc(): fail to allocate VITTERFAST decoder.\n");
		return NULL;
	}

	if (VITTERFASTDecoderInit(decoder, stream, IsFile) == -1)
	{
		printf("VITTERFASTDecoderAlloc(): fail to initiate VITTERFAST decoder.\n");
		return NULL;
	}
	
	return decoder;
}

static VITTERFASTTREENODE *isFeasible(int bit, VITTERFASTTREENODE *node)
{
	VITTERFASTTREENODE *iter = NULL;
	
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

static VITTERFASTTREENODE *VITTERFASTDecoderOutputSymbol(VITTERFASTDECODER *decoder, int *symbol)
{
	int bit, i;
	VITTERFASTTREENODE *node, *iter;
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

int VITTERFASTDecoderDecode(VITTERFASTDECODER *decoder)
{
	int symbol = 0;
	
	VITTERFASTTREENODE *node = VITTERFASTDecoderOutputSymbol(decoder, &symbol);
	//printf("\n");
	//printf("symbol = %d\n", symbol);
	
	VITTERFASTTreeUpdate(decoder, node, symbol);	
	//PrintVITTERFASTTree(decoder->tree->root);
	
	return symbol;
}

void VITTERFASTDecoderDealloc(VITTERFASTDECODER *decoder)
{
	if (decoder == NULL) return;
    if (decoder->tree) VITTERFASTTreeDealloc(decoder->tree);
	free(decoder);
}

int VITTERFASTDecoderBytesRead(VITTERFASTDECODER *decoder)
{
	return decoder->CurrentBytes;
}