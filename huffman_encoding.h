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
};

bool treeNode_comparator(void* tn0, void* tn1);
struct TreeNode* buildHuffmanTree(struct LinkedList* llist);
void printHuffmanEncodings(struct TreeNode* root, struct HuffmanEncoding* curEncoding);

#endif //HUFFMAN_ENCODING_H
