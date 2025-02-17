#include "huffman_encoding.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bool compareInts(void* intA, void* intB) {
    return *((int*)intA) > *((int*)intB);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Need to specify a file to encode");
        return 1;
    }
    FILE* beeMovie = fopen(argv[1], "r");
    if (!beeMovie) {
        fprintf(stderr, "%s: Unable to open file: %s (errno: %d)\r\n",
		__func__, argv[1], errno);
        return 2;
    }

    size_t asciiCharMap[INT8_MAX] = {0};
    int max = -1;
    for (int c = fgetc(beeMovie); c != EOF; c = fgetc(beeMovie)) {
        asciiCharMap[c]++;
    }
    int closeSuccess = fclose(beeMovie);

    struct LinkedList llist = {
        .head = NULL,
        .tail = NULL
    };

    for (int i = 0; i < INT8_MAX; i++) {
        if (asciiCharMap[i] == 0)
            continue;

        struct TreeNode* node = malloc(sizeof(struct TreeNode));
        if (!node) {
            fprintf(stderr, "Unable to enough memory for new node");
            break;
        }
        node->character = (char)i;
        node->weight = asciiCharMap[i];
        node->left = NULL;
        node->right = NULL;
        llist_insertUsingCompare(&llist, node, treeNode_comparator);
    }

    struct TreeNode* treeRoot = buildHuffmanTree(&llist);
    if (!treeRoot)
        return 1;
    struct HuffmanEncoding encoding = {
        .bitStr = 0,
        .length = 0
    };
    printHuffmanEncodings(treeRoot, &encoding);
}
