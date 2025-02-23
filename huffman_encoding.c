//
// Created by bserrano on 2/15/2025.
//

#include <stdio.h>
#include "huffman_encoding.h"

bool treeNode_comparator(void* tn0, void* tn1) {
    return ((struct TreeNode*)tn0)->weight > ((struct TreeNode*)tn1)->weight;
}

struct TreeNode* buildHuffmanTree(struct LinkedList* llist) {
    if (!llist || !llist->head)
        return NULL;

    while (llist->head && llist->head->next) {
        struct TreeNode* leftNode = (struct TreeNode*) llist_popfront(llist);
        struct TreeNode* rightNode = (struct TreeNode*) llist_popfront(llist);
        struct TreeNode* newNode = (struct TreeNode*) malloc(sizeof(struct TreeNode));
        if (!newNode)
            return NULL;
        newNode->character = '\0';
        newNode->weight = leftNode->weight + rightNode->weight;
        newNode->left = leftNode;
        newNode->right = rightNode;
        llist_insertUsingCompare(llist, newNode, treeNode_comparator);
    }

    return (struct TreeNode*) llist_popfront(llist);
}

void getHuffmanEncodings(struct TreeNode* root, struct HuffmanEncoding* curEncoding, struct LinkedList* llist) {
    if (!root || !llist)
        return;

    if (root->left) {
        curEncoding->bitStr <<= 1;
        curEncoding->length++;
        getHuffmanEncodings(root->left, curEncoding, llist);
        curEncoding->bitStr >>= 1;
        curEncoding->length--;
    }

    if (root->right) {
        curEncoding->bitStr <<= 1;
        curEncoding->bitStr |= 1;
        curEncoding->length++;
        getHuffmanEncodings(root->right, curEncoding, llist);
        curEncoding->bitStr >>= 1;
        curEncoding->length--;
    }

    if (!root->left && !root->right) {
        struct HuffmanEncoding* encoding = (struct HuffmanEncoding*) malloc(sizeof(struct HuffmanEncoding));
        if (!encoding) {
            fprintf(stderr, "Unable to allocate memory for encoding node\n");
            return;
        }

        encoding->bitStr = curEncoding->bitStr;
        encoding->length = curEncoding->length;
        encoding->character = root->character;
        llist_pushback(llist, encoding);
    }
}

void freeHuffmanTree(struct TreeNode* root) {
    if (!root)
        return;

    if (root->left)
        freeHuffmanTree(root->left);
    if (root->right)
        freeHuffmanTree(root->right);
    free(root);
}

void printHuffmanEncodings(struct TreeNode* root, struct HuffmanEncoding* curEncoding) {
    if (!root)
        return;

    if (root->left) {
        curEncoding->bitStr <<= 1;
        curEncoding->length++;
        printHuffmanEncodings(root->left, curEncoding);
        curEncoding->bitStr >>= 1;
        curEncoding->length--;
    }
    if (root->right) {
        curEncoding->bitStr <<= 1;
        curEncoding->bitStr |= 1;
        curEncoding->length++;
        printHuffmanEncodings(root->right, curEncoding);
        curEncoding->bitStr >>= 1;
        curEncoding->length--;
    }
    if (!root->left && !root->right)
        printf("%c: 0x%016lx, %d\n", root->character, curEncoding->bitStr, curEncoding->length);
}

