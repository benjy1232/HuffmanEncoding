#include "huffman_encoding.h"

#include <stdio.h>

bool treeNode_comparator(void *tn0, void *tn1)
{
    return ((TreeNode *)tn0)->weight > ((TreeNode *)tn1)->weight;
}

bool buildHuffmanTree(LinkedList *priorityQueue, TreeNode **rootPtr)
{
    if (!priorityQueue || !priorityQueue->head)
    {
        fprintf(stderr, "Unable to go through priority queue\n");
        return false;
    }

    if (!rootPtr)
    {
        fprintf(stderr, "Unable to use rootPtr\n");
        return false;
    }

    while (priorityQueue->head && priorityQueue->head->next)
    {
        struct TreeNode *leftNode = (struct TreeNode *)llist_popfront(priorityQueue);
        struct TreeNode *rightNode = (struct TreeNode *)llist_popfront(priorityQueue);
        struct TreeNode *newNode = (struct TreeNode *)malloc(sizeof(TreeNode));
        if (!newNode)
        {
            fprintf(stderr, "Unable to allocate new tree node\n");
            return false;
        }
        newNode->character = '\0';
        newNode->weight = leftNode->weight + rightNode->weight;
        newNode->left = leftNode;
        newNode->right = rightNode;
        if (!llist_insertUsingCompare(priorityQueue, newNode, treeNode_comparator))
            return false;
    }
    *rootPtr = (TreeNode *)llist_popfront(priorityQueue);

    return true;
}

bool generateHuffmanEncodings(TreeNode *root, HuffmanEncoding *curEncoding, HuffmanEncoding **const begin,
                              HuffmanEncoding *const end)
{
    if (!root || !curEncoding || !begin || !*begin || !end)
        return false;

    bool isSuccessful = true;
    if (root->left)
    {
        curEncoding->bitStr <<= 1;
        curEncoding->length++;
        isSuccessful = generateHuffmanEncodings(root->left, curEncoding, begin, end);
        curEncoding->bitStr >>= 1;
        curEncoding->length--;
    }

    if (root->right && isSuccessful)
    {
        curEncoding->bitStr <<= 1;
        curEncoding->bitStr |= 1;
        curEncoding->length++;
        isSuccessful = generateHuffmanEncodings(root->right, curEncoding, begin, end);
        curEncoding->bitStr >>= 1;
        curEncoding->length--;
    }

    if (!root->left && !root->right)
    {
        HuffmanEncoding *encoding = *begin;
        if (encoding == end)
        {
            fprintf(stderr, "Reached end of array\n");
            return false;
        }

        encoding->bitStr = curEncoding->bitStr;
        encoding->length = curEncoding->length;
        encoding->character = root->character;
        (*begin)++;
    }
    return isSuccessful;
}

void freeHuffmanTree(TreeNode *root)
{
    if (!root)
        return;

    if (root->left)
        freeHuffmanTree(root->left);
    if (root->right)
        freeHuffmanTree(root->right);
    free(root);
}

void freeHuffmanTreeCb(void *root)
{
    freeHuffmanTree((TreeNode *)root);
}

void printHuffmanEncodings(TreeNode *root, HuffmanEncoding *curEncoding)
{
    if (!root)
        return;

    if (root->left)
    {
        curEncoding->bitStr <<= 1;
        curEncoding->length++;
        printHuffmanEncodings((TreeNode *)root->left, curEncoding);
        curEncoding->bitStr >>= 1;
        curEncoding->length--;
    }
    if (root->right)
    {
        curEncoding->bitStr <<= 1;
        curEncoding->bitStr |= 1;
        curEncoding->length++;
        printHuffmanEncodings((TreeNode *)root->right, curEncoding);
        curEncoding->bitStr >>= 1;
        curEncoding->length--;
    }

    if (!root->left && !root->right)
        printf("%c: 0x%016lx, %d\n", root->character, curEncoding->bitStr, curEncoding->length);
}
