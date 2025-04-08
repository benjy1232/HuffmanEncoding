//
// Created by bserrano on 2/15/2025.
//

#ifndef HUFFMAN_ENCODING_H
#define HUFFMAN_ENCODING_H

#include "list.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct TreeNode
{
    char character;
    size_t weight;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

typedef struct
{
    uint64_t bitStr;
    int32_t length;
    char character;
} HuffmanEncoding;

bool treeNode_comparator(void *tn0, void *tn1);
bool buildHuffmanTree(LinkedList *priorityQueue, TreeNode **rootPtr);
bool generateHuffmanEncodings(TreeNode *root, HuffmanEncoding *curEncoding, HuffmanEncoding **const begin,
                              HuffmanEncoding *const end);

void printHuffmanEncodings(TreeNode *root, HuffmanEncoding *curEncoding);
void freeHuffmanTree(TreeNode *root);
void freeHuffmanTreeCb(void *root);

#endif // HUFFMAN_ENCODING_H
