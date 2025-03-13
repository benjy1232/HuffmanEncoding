#include "huffman_encoding.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_LEN 4096
#define BITS_PER_BYTE 8

struct ASCIICharMap {
	size_t map[INT8_MAX + 1];
};

bool getCharacterFrequencies(FILE* inputFile, struct ASCIICharMap* outputMap) {
	if (!inputFile)
		return false;

	for (int c = fgetc(inputFile); c != EOF; c = fgetc(inputFile)) {
		if (c > sizeof(outputMap->map) / sizeof(outputMap->map[0])) {
			fprintf(stderr, "Character is unable to be stored in frequency map\r\n");
			return false;
		}
		outputMap->map[c]++;
	}
	return true;
}

static bool createPriorityQueue(struct ASCIICharMap* inputMap,
				struct LinkedList* outputPriorityQueue) {
	bool isSuccess = true;
	size_t mapSize = sizeof(inputMap->map) / sizeof(inputMap->map[0]);
	for (size_t i = 0; i < mapSize; i++) {
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

size_t populateEncodingHdr(uint8_t* buf, size_t originalFileLen, size_t dictLen) {
	memcpy(buf, &originalFileLen, sizeof(originalFileLen));
	memcpy(buf, &dictLen, sizeof(dictLen));
	return sizeof(originalFileLen) + sizeof(dictLen);
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

	struct LinkedList priorityQueue = {
		.head = NULL,
		.tail = NULL
	};
	success = createPriorityQueue(&asciiCharMap, &priorityQueue);
	if (!success) {
		fprintf(stderr, "Unable to create priority queue\n");
		llist_free(&priorityQueue, freeHuffmanTreeCb);
		return 1;
	}

	struct TreeNode* treeRoot = NULL;
	success = buildHuffmanTree(&priorityQueue, &treeRoot);
	if (!success) {
		fprintf(stderr, "No tree created\n");
		llist_free(&priorityQueue, freeHuffmanTreeCb);
		return 1;
	}
	struct HuffmanEncoding initialEncoding = {
		.bitStr = 0,
		.length = 0,
		.character = 0
	};

	size_t huffArrayLen = INT8_MAX + 1;
	size_t huffArraySize = sizeof(struct HuffmanEncoding) * huffArrayLen;
	struct HuffmanEncoding* huffArray =
		(struct HuffmanEncoding*) malloc(huffArraySize);
	if (!huffArray) {
		fprintf(stderr, "Unable to allocate memory for huffArray\n");
		return 1;
	}

	struct HuffmanEncoding* huffIter = huffArray;
	success = getHuffmanEncodingsV2(treeRoot, &initialEncoding, &huffIter,
					huffArray + huffArrayLen);
	freeHuffmanTree(treeRoot);
	if (!success) {
		fprintf(stderr, "Unable to work properly\n");
		return 1;
	}

	intptr_t diff = (intptr_t) huffIter - (intptr_t) huffArray;
	size_t dictSize = diff;
	printf("dictSize: %lu\n", dictSize);

	uint8_t* byteArray = (uint8_t*) malloc(BUFFER_LEN);
	if (!byteArray) {
		fprintf(stderr, "Unable to allocate buffer for byte array\n");
		return 1;
	}

	int32_t bytesWritten = 0;
	FILE* encodedFile = fopen(encodedFileName, "wb");
	if (!encodedFile) {
		fprintf(stderr, "argc: %d\n", argc);
		fprintf(stderr, "Original File: %s\n", argv[1]);
		fprintf(stderr, "Failed to create file %s\n", encodedFileName);
		free(huffArray);
		return 1;
	}

	bytesWritten += populateEncodingHdr(byteArray, treeRoot->weight, dictSize);

	// Start writing the encoding into the array
	for (size_t i = 0; i < INT8_MAX; i++) {
		if (huffArray[i].length == 0)
			continue;
		if (bytesWritten + sizeof(huffArray[i]) >= BUFFER_LEN) {
			fwrite(byteArray, sizeof(uint8_t), bytesWritten, encodedFile);
			memset(byteArray, 0, BUFFER_LEN);
			bytesWritten = 0;
		}
		memcpy(byteArray + bytesWritten, huffArray + i, sizeof(huffArray[i]));
		bytesWritten += sizeof(huffArray[i]);
	}
	fwrite(byteArray, sizeof(uint8_t), bytesWritten, encodedFile);
	bytesWritten = 0;
	memset(byteArray, 0, BUFFER_LEN);

	uint8_t* byteIter = byteArray;
	const uint8_t* endIter = byteArray + BUFFER_LEN;
	uint32_t bitsWritten = 0;
	originalFile = fopen(argv[1], "r");
	for (int c = fgetc(originalFile); c != EOF; c = fgetc(originalFile)) {
		struct HuffmanEncoding* he = NULL;
		for (size_t heIdx = 0; heIdx < huffArrayLen; heIdx++) {
			if ((int)huffArray[heIdx].character == c) {
				he = huffArray + heIdx;
				break;
			}
		}
		if (!he)
			return 1;
		uint64_t bitStr = he->bitStr;
		int32_t length = he->length;
		for (int i = length - 1; i >= 0; i--) {
			// Get the highest bit in bitStr
			uint64_t bitToSet = (bitStr >> i) & 1;
			uint32_t bitIdx = BITS_PER_BYTE - 1 - (bitsWritten % BITS_PER_BYTE);
			*byteIter |= bitToSet << bitIdx;
			bitsWritten++;
			if (bitsWritten % BITS_PER_BYTE == 0) {
				byteIter++;
				bitsWritten = 0;
				bytesWritten++;
				if (byteIter == endIter) {
					fwrite(byteArray, sizeof(uint8_t), bytesWritten, encodedFile);
					bytesWritten = 0;
					byteIter = byteArray;
					memset(byteArray, 0, BUFFER_LEN);
				}
			}
		}
	}
	fclose(originalFile);

	fwrite(byteArray, sizeof(uint8_t), bytesWritten + 1, encodedFile);
	fclose(encodedFile);
	free(huffArray);
	free(byteArray);
}
