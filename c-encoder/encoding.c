#include "huffman_encoding.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_LEN 1024
#define BITS_PER_BYTE 8

bool compareInts(void* intA, void* intB) {
    return *((int*)intA) > *((int*)intB);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Need to specify a file to encode");
        fprintf(stderr, "Need to specify file to write data into");
        return 1;
    }
    char* originalFileName = argv[1];
    char* encodedFileName = argv[2];
    FILE* beeMovie = fopen(originalFileName, "r");
    if (!beeMovie) {
        fprintf(stderr, "%s: Unable to open file: %s (errno: %d)\r\n",
		__func__, originalFileName, errno);
        return 2;
    }

    size_t asciiCharMap[INT8_MAX] = {0};
    for (int c = fgetc(beeMovie); c != EOF; c = fgetc(beeMovie)) {
        asciiCharMap[c]++;
    }
    fclose(beeMovie);

    struct LinkedList llist = {
        .head = NULL,
        .tail = NULL
    };

    for (int i = 0; i < INT8_MAX; i++) {
        if (asciiCharMap[i] == 0)
            continue;

        struct TreeNode* node = (struct TreeNode*) malloc(sizeof(struct TreeNode));
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
    if (!treeRoot) {
        fprintf(stderr, "No tree created\n");
        return 1;
    }
    struct HuffmanEncoding encoding = {
        .bitStr = 0,
        .length = 0,
        .character = 0
    };

    getHuffmanEncodings(treeRoot, &encoding, &llist);
    freeHuffmanTree(treeRoot);
    struct HuffmanEncoding huffArray[INT8_MAX] = {};
    size_t dictSize = 0;
    while (llist.head) {
        struct HuffmanEncoding* encoding = (struct HuffmanEncoding*) llist_popfront(&llist);
        huffArray[(size_t)encoding->character] = *encoding;
        dictSize += sizeof(*encoding);
        free(encoding);
    };

    uint8_t byteArray[BUFFER_LEN] = {};
    int32_t bytesWritten = 0;
    FILE* encodedFile = fopen(encodedFileName, "wb");
    if (!encodedFile) {
        fprintf(stderr, "argc: %d\n", argc);
        fprintf(stderr, "Original File: %s\n", argv[1]);
        fprintf(stderr, "Failed to create file %s\n", encodedFileName);
        return 1;
    }
    // First 8 bytes are file length
    memcpy(byteArray, &treeRoot->weight, sizeof(treeRoot->weight));
    bytesWritten += sizeof(treeRoot->weight);
    // Next 8 bytes are the dict length
    printf("dictSize: %lu\n", dictSize);
    memcpy(byteArray + bytesWritten, &dictSize, sizeof(dictSize));
    bytesWritten += sizeof(dictSize);
    // Start writing the encoding into the array
    for (size_t i = 0; i < INT8_MAX; i++) {
        if (huffArray[i].length == 0)
            continue;
        if (bytesWritten + sizeof(huffArray[i]) >= BUFFER_LEN) {
            fwrite(byteArray, sizeof(uint8_t), bytesWritten, encodedFile);
            memset(byteArray, 0, sizeof(byteArray));
            bytesWritten = 0;
        }
        memcpy(byteArray + bytesWritten, huffArray + i, sizeof(huffArray[i]));
        bytesWritten += sizeof(huffArray[i]);
    }
    fwrite(byteArray, sizeof(uint8_t), bytesWritten, encodedFile);
    bytesWritten = 0;
    memset(byteArray, 0, sizeof(byteArray));

    uint8_t* iter = byteArray;
    const uint8_t* endIter = byteArray + BUFFER_LEN;
    uint32_t bitsWritten = 0;
    beeMovie = fopen(argv[1], "r");
    for (int c = fgetc(beeMovie); c != EOF; c = fgetc(beeMovie)) {
        struct HuffmanEncoding he = huffArray[c];
        uint64_t bitStr = he.bitStr;
        int32_t length = he.length;
        // v: 45 (0b101101) l: 8 , v: 19 (0b10011) l: 6
        // MSB                     LSB
        // 0b00101101 | 0100_11 | 01_0011 | 0000
        // Bytes:
        // 0x2D       | 0x4_D 0x_3 | 0
        // Start from MSB go towards LSB
        for (int i = 0; i < length; i++) {
            // Get the highest bit in bitStr
            uint64_t bitToSet = bitStr >> (length - i - 1) & 1;
            uint32_t bitIdx = BITS_PER_BYTE - 1 - (bitsWritten % BITS_PER_BYTE);
            *iter |= bitToSet << bitIdx;
            bitsWritten++;
            if (bitsWritten % BITS_PER_BYTE == 0) {
                iter++;
                bitsWritten = 0;
                bytesWritten++;
                if (iter == endIter) {
                    fwrite(byteArray, sizeof(uint8_t), bytesWritten, encodedFile);
                    bytesWritten = 0;
                    iter = byteArray;
                    memset(byteArray, 0, sizeof(byteArray));
                }
            }
        }
    }
    fclose(beeMovie);

    fwrite(byteArray, sizeof(uint8_t), bytesWritten + 1, encodedFile);
    fclose(encodedFile);
}
