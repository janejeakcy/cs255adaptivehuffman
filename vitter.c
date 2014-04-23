/*************************************************************************
 *
 *	File:	vitter.c
 *	Author:  Jing Huang & Liang Wu
 *
 *	Description: adaptive Huffman coding -- Vitter
 *
 *	Update:		Version 1.0		04/04/2014	Initial version.  
 *
 *
 ************************************************************************/

#include "vitter.h"

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

static int GetBit(VITTERDECODER *decoder)
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


static void PutBit(VITTERENCODER *encoder, int bit)
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

void VITTEREncoderFlush(VITTERENCODER *encoder)
{
	if (encoder->mask != 0x80)
	{
		PutByte(encoder->stream, encoder->rack, &(encoder->CurrentBytes), encoder->IsFile);
	}
}

/* pre-order print out every tree node */
static void PrintVITTERTree(VITTERTREENODE *localRoot)
{
	printf("number = %d, weight = %d, symbol = %d\n", localRoot->number, localRoot->weight, localRoot->symbol);
	
	if (localRoot->left != NULL)
	{
		PrintVITTERTree(localRoot->left);
	}
	if (localRoot->right != NULL)
	{
		PrintVITTERTree(localRoot->right);
	}
	
}

static VITTERTREENODE *VITTERTreeNodeInit(VITTERTREE *tree)
{	
	VITTERTREENODE *node;
	if ((node = (VITTERTREENODE *) malloc (sizeof(VITTERTREENODE))) == NULL)
	{
		printf("VITTERTreeNodeInit(): fail to allocate new node!");
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

static int VITTERTreeInit(VITTERTREE *tree)
{
	tree->maxNumber = 0;
	if ((tree->root = VITTERTreeNodeInit(tree)) == NULL)
	{
		printf("VITTERTreeInit(): fail to initiate root node!");
		return -1;
	}
	
	return 0;
}

static int VITTEREncoderInit(VITTERENCODER *encoder, void *stream, int IsFile)
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
	
	if ((encoder->tree = (VITTERTREE *) malloc (sizeof(VITTERTREE))) == NULL)
	{
		printf("VITTEREncoderInit(): fail to allocate VITTER tree!\n");
		return -1;
	}
	
	if ((VITTERTreeInit(encoder->tree)) == -1)
	{
		printf("VITTEREncoderInit(): fail to initiate VITTER tree!\n");
		return -1;
	} ;
	
	return 0;
}

VITTERENCODER *VITTEREncoderAlloc(void *stream, int IsFile)
{
	VITTERENCODER *encoder;

	if ((encoder = (VITTERENCODER *) malloc (sizeof(VITTERENCODER))) == NULL)
	{
		printf("VITTEREncoderAlloc(): fail to allocate VITTER encoder.\n");
		return NULL;
	}

	if (VITTEREncoderInit(encoder, stream, IsFile) == -1)
	{
		printf("VITTEREncoderAlloc(): fail to initiate VITTER encoder.\n");
		return NULL;
	}
	
	return encoder;
}


/* Recursive call to find Zero Node */
static VITTERTREENODE *findZeroNode(VITTERTREENODE *localRoot)
{
	VITTERTREENODE *node, *node1, *node2;
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

static void OutputNodeCode(VITTERENCODER *encoder, VITTERTREENODE *node)
{
	VITTERTREENODE *iter = node;
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

static void OutputZeroNodeCode(VITTERENCODER *encoder, VITTERTREENODE *zeroNode, int symbol)
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

static VITTERTREENODE *VITTEREncoderOutputZeroNodeCode(VITTERENCODER *encoder, int symbol)
{
	VITTERTREENODE *zeroNode, *root;
	
	root = encoder->tree->root;
	
	zeroNode = findZeroNode(root);
	
	OutputZeroNodeCode(encoder, zeroNode, symbol);
			
	return zeroNode;
}

/* Recursive call to find the correct Node */
static VITTERTREENODE *findNode(VITTERTREENODE *localRoot, int symbol)
{
	VITTERTREENODE *node, *node1, *node2;
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

static VITTERTREENODE *VITTEREncoderOutputNonZeroNodeCode(VITTERENCODER *encoder, int symbol)
{
	VITTERTREENODE *node, *root;
	
	root = encoder->tree->root;
	
	node = findNode(root, symbol);
	
	OutputNodeCode(encoder, node);
	
	return node;
}

static bool isExisted(VITTERCODER *coder, int symbol)
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

static VITTERTREENODE *VITTEREncoderOutputCode(VITTERENCODER *encoder, int symbol)
{
	VITTERTREENODE *node;
	
	if (isExisted(encoder, symbol))
	{
		node = VITTEREncoderOutputNonZeroNodeCode(encoder, symbol);
	}
	else
	{
		node = VITTEREncoderOutputZeroNodeCode(encoder, symbol);
	}
	
	return node;
}

static bool isZeroNodeSibling(VITTERTREENODE *node)
{
	VITTERTREENODE *sibling;
	
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

static bool isLeafNode(VITTERTREENODE *node)
{
	if (node->left == NULL)
	{
		return true;
	}
	return false;
}

/* post-order traverse to find the internal nodes with the same weight */
static void findSameWeightInternalNodes(VITTERTREENODE *localRoot, VITTERTREENODE *sameWeightNodes[256], int weight, int *count)
{
	if (localRoot->left != NULL)
	{
		findSameWeightInternalNodes(localRoot->left, sameWeightNodes, weight, count);
	}
	if (localRoot->right != NULL)
	{
		findSameWeightInternalNodes(localRoot->right, sameWeightNodes, weight, count);
	}
	if (localRoot->weight == weight && isLeafNode(localRoot) == false)
	{
		sameWeightNodes[*count] = localRoot;
		*count += 1;
	}
}

/* post-order traverse to find the leaf nodes with the same weight */
static void findSameWeightLeafNodes(VITTERTREENODE *localRoot, VITTERTREENODE *sameWeightNodes[256], int weight, int *count)
{
	if (localRoot->left != NULL)
	{
		findSameWeightLeafNodes(localRoot->left, sameWeightNodes, weight, count);
	}
	if (localRoot->right != NULL)
	{
		findSameWeightLeafNodes(localRoot->right, sameWeightNodes, weight, count);
	}
	if (localRoot->weight == weight && isLeafNode(localRoot) == true)
	{
		sameWeightNodes[*count] = localRoot;
		*count += 1;
	}
}

static void printSameWeightNodes(VITTERTREENODE *sameWeightNodes[256], int count)
{
	int i;
	VITTERTREENODE *iter;
	
	for (i = 0; i < count; i++)
	{
		iter = sameWeightNodes[i];
		printf("i = %d, node->number = %d, node->weight = %d, node->symbol = %d\n", i, iter->number, iter->weight, iter->symbol);
	}
}

static VITTERTREENODE *findLeaderInLeafBlock(VITTERTREE *tree, VITTERTREENODE *node)
{
	VITTERTREENODE *iter = NULL;
	VITTERTREENODE *sameWeightNodes[256];
	int weight = node->weight;
	int i, count, number;
	
	count = 0;
	findSameWeightLeafNodes(tree->root, sameWeightNodes, weight, &count);
	
	//printSameWeightNodes(sameWeightNodes, count);
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

static int partition(VITTERTREENODE *sameWeightNodes[256], int p, int r)
{
	int i, j;
	VITTERTREENODE *pivotNode, *tempNode;
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

static void quickSort(VITTERTREENODE *sameWeightNodes[256], int p, int r)
{
	int q;
	if (p < r)
	{
		q = partition(sameWeightNodes, p, r);
		quickSort(sameWeightNodes, p, q - 1);
		quickSort(sameWeightNodes, q + 1, r);
	}
}

static void sortByIncrementNumber(VITTERTREENODE *sameWeightNodes[256], int count)
{
	quickSort(sameWeightNodes, 0, count -1);
}

static void slideNodes(VITTERTREENODE *sameWeightNodes[256], VITTERTREENODE *node, int count)
{
	VITTERTREENODE *iter, *tempNode;
	int i, tempNumber;
	bool tempIsLeft;
	
	/*if (node == sameWeightNodes[0])
	{
		return;
	}*/
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


static VITTERTREENODE *slideAndIncrement(VITTERTREE *tree, VITTERTREENODE *node)
{
	VITTERTREENODE *iter = NULL;
	VITTERTREENODE *sameWeightNodes[256];
	VITTERTREENODE *tempNode;
	int weight = node->weight;
	int i, count, number;
	bool tempIsLeft;
	iter = node;	
	count = 0;
	
	if (isLeafNode(node))
	{
		findSameWeightInternalNodes(tree->root, sameWeightNodes, weight, &count);
		//printSameWeightNodes(sameWeightNodes, count);
		if (count > 0)
		{
			sortByIncrementNumber(sameWeightNodes, count);
			slideNodes(sameWeightNodes, iter, count);
		}
		iter->weight++;
		iter = iter->parent;		
	}
	else
	{
		tempNode = iter->parent;
		findSameWeightLeafNodes(tree->root, sameWeightNodes, weight + 1, &count);
		//printSameWeightNodes(sameWeightNodes, count);
		if (count > 0)
		{
			sortByIncrementNumber(sameWeightNodes, count);
			slideNodes(sameWeightNodes, iter, count);
		}
		iter->weight++;
		iter = tempNode;		
	}
	return iter;
}
		

static void VITTERTreeUpdate(VITTERCODER *coder, VITTERTREENODE *node, int symbol)
{
	VITTERTREENODE *parentOfZeroNode, *newZeroNode, *iter, *tempNode, *leader, *parentOfIter;
	VITTERTREENODE *leafToIncrement = NULL;	
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
		 
		parentOfZeroNode = VITTERTreeNodeInit(coder->tree);
		newZeroNode = VITTERTreeNodeInit(coder->tree);
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
		leader = findLeaderInLeafBlock(coder->tree, iter);

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
		parentOfIter = slideAndIncrement(coder->tree, iter);
		iter = parentOfIter;
	}
	if (iter == coder->tree->root)
	{
		iter->weight++;
	}
	if (leafToIncrement != NULL)
	{
		parentOfIter = slideAndIncrement(coder->tree, leafToIncrement);
	}
}
		

void VITTEREncoderEncode(VITTERENCODER *encoder, int symbol)
{ 
	//printf("\n");
	//printf("symbol = %d\n", symbol);
	VITTERTREENODE *node = VITTEREncoderOutputCode(encoder, symbol);
	VITTERTreeUpdate(encoder, node, symbol);	
	//PrintVITTERTree(encoder->tree->root);
}

/* post-order traverse and delete tree nodes */
static void VITTERTreeNodesDealloc(VITTERTREENODE *localRoot)  
{
	if (localRoot == NULL) return;
	VITTERTreeNodesDealloc(localRoot->left);
	VITTERTreeNodesDealloc(localRoot->right);
	free(localRoot);
}

static void VITTERTreeDealloc(VITTERTREE *tree)
{
	if (tree == NULL) return;
    VITTERTreeNodesDealloc(tree->root);
	free(tree);
}

void VITTEREncoderDealloc(VITTERENCODER *encoder)
{
	if (encoder == NULL) return;
    if (encoder->tree) VITTERTreeDealloc(encoder->tree);
	free(encoder);
}

int VITTEREncoderBytesWrite(VITTERENCODER *encoder)
{
	return encoder->CurrentBytes;
}

static int VITTERDecoderInit(VITTERDECODER *decoder, void *stream, int IsFile)
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
	
	if ((decoder->tree = (VITTERTREE *) malloc (sizeof(VITTERTREE))) == NULL)
	{
		printf("VITTERDecoderInit(): fail to allocate VITTER tree!\n");
		return -1;
	}
	
	if ((VITTERTreeInit(decoder->tree)) == -1)
	{
		printf("VITTERDecoderInit(): fail to initiate VITTER tree!\n");
		return -1;
	} ;
	
	return 0;
}


VITTERDECODER *VITTERDecoderAlloc(void *stream, int IsFile)
{
	VITTERDECODER *decoder;

	if ((decoder = (VITTERDECODER *) malloc (sizeof(VITTERDECODER))) == NULL)
	{
		printf("VITTERDecoderAlloc(): fail to allocate VITTER decoder.\n");
		return NULL;
	}

	if (VITTERDecoderInit(decoder, stream, IsFile) == -1)
	{
		printf("VITTERDecoderAlloc(): fail to initiate VITTER decoder.\n");
		return NULL;
	}
	
	return decoder;
}

static VITTERTREENODE *isFeasible(int bit, VITTERTREENODE *node)
{
	VITTERTREENODE *iter = NULL;
	
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

static VITTERTREENODE *VITTERDecoderOutputSymbol(VITTERDECODER *decoder, int *symbol)
{
	int bit, i;
	VITTERTREENODE *node, *iter;
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

int VITTERDecoderDecode(VITTERDECODER *decoder)
{
	int symbol = 0;
	
	VITTERTREENODE *node = VITTERDecoderOutputSymbol(decoder, &symbol);
	//printf("\n");
	//printf("symbol = %d\n", symbol);
	
	VITTERTreeUpdate(decoder, node, symbol);	
	//PrintVITTERTree(decoder->tree->root);
	
	return symbol;
}

void VITTERDecoderDealloc(VITTERDECODER *decoder)
{
	if (decoder == NULL) return;
    if (decoder->tree) VITTERTreeDealloc(decoder->tree);
	free(decoder);
}

int VITTERDecoderBytesRead(VITTERDECODER *decoder)
{
	return decoder->CurrentBytes;
}