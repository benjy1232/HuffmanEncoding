//
// Created by bserrano on 2/15/2025.
//

#ifndef HUFFMAN_ENCODING_H
#define HUFFMAN_ENCODING_H

#include "list.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct TreeNode
{
	char   character;
	size_t weight;
	struct TreeNode* left;
	struct TreeNode* right;
};

struct HuffmanEncoding
{
	uint64_t bitStr;
	int32_t  length;
	char	 character;
};

bool treeNode_comparator(void* tn0, void* tn1);
bool buildHuffmanTree(struct LinkedList* priorityQueue, struct TreeNode** rootPtr);
bool getHuffmanEncodings(struct TreeNode* root,
			   struct HuffmanEncoding* curEncoding,
			   struct HuffmanEncoding** const begin,
			   struct HuffmanEncoding* const end);

void printHuffmanEncodings(struct TreeNode* root, struct HuffmanEncoding* curEncoding);
void freeHuffmanTree(struct TreeNode* root);
void freeHuffmanTreeCb(void* root);

#endif //HUFFMAN_ENCODING_H
