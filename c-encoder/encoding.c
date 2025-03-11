#include "huffman_encoding.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_LEN 1024
#define BITS_PER_BYTE 8

struct ASCIICharMap {
	size_t map[INT8_MAX];
};

bool getCharacterFrequencies(FILE* inputFile, struct ASCIICharMap* outputMap) {
	if (!inputFile)
		return false;

	for (int c = fgetc(inputFile); c != EOF; c = fgetc(inputFile)) {
		if (c >= INT8_MAX) {
			fprintf(stderr, "Character is unable to be stored in frequency map\r\n");
			return false;
		}
		outputMap->map[c]++;
	}
	return true;
}

bool compareInts(void* intA, void* intB) {
	return *((int*)intA) > *((int*)intB);
}

bool createPriorityQueue(struct ASCIICharMap* inputMap,
			 struct LinkedList* outputPriorityQueue) {
	bool isSuccess = true;
	for (size_t i = 0; i < sizeof(inputMap->map); i++) {
		if (inputMap->map[i] == 0)
			continue;

		struct TreeNode* node =
			(struct TreeNode*) malloc(sizeof(struct TreeNode));
		if (!node) {
			fprintf(stderr, "Unable to enough memory for new node");
			return false;
		}
		node->character = (char)i;
		node->weight = inputMap->map[i];
		node->left = NULL;
		node->right = NULL;
		isSuccess = llist_insertUsingCompare(outputPriorityQueue,
						     node,
						     treeNode_comparator);
	}
	return isSuccess;
}

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "Need to specify a file to encode\n");
		fprintf(stderr, "Need to specify file to write data into\n");
		return 1;
	}
	char* originalFileName = argv[1];
	char* encodedFileName = argv[2];
	FILE* originalFile = fopen(originalFileName, "r");
	if (!originalFile) {
		fprintf(stderr, "%s: Unable to open file: %s (errno: %d)\n",
			__func__, originalFileName, errno);
		return 1;
	}

	struct ASCIICharMap asciiCharMap;
	memset(asciiCharMap.map, 0, sizeof(asciiCharMap.map));
	bool success = getCharacterFrequencies(originalFile, &asciiCharMap);
	fclose(originalFile);

	if (!success) {
		fprintf(stderr, "Unable to get frequency map\n");
		return 1;
	}

	struct LinkedList llist = {
		.head = NULL,
		.tail = NULL
	};
	success = createPriorityQueue(&asciiCharMap, &llist);
	if (!success) {
		fprintf(stderr, "Unable to create priority queue\n");
		llist_free(&llist, freeHuffmanTreeCb);
		return 1;
	}

	struct TreeNode* treeRoot = NULL;
	success = buildHuffmanTree(&llist, &treeRoot);
	if (!success) {
		fprintf(stderr, "No tree created\n");
		llist_free(&llist, freeHuffmanTreeCb);
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
	originalFile = fopen(argv[1], "r");
	for (int c = fgetc(originalFile); c != EOF; c = fgetc(originalFile)) {
		struct HuffmanEncoding he = huffArray[c];
		uint64_t bitStr = he.bitStr;
		int32_t length = he.length;
		for (int i = length - 1; i >= 0; i--) {
			// Get the highest bit in bitStr
			uint64_t bitToSet = (bitStr >> i) & 1;
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
	fclose(originalFile);

	fwrite(byteArray, sizeof(uint8_t), bytesWritten + 1, encodedFile);
	fclose(encodedFile);
}
