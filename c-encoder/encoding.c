#include "huffman_encoding.h"
#include "list.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/uio.h>

#define BUFFER_LEN 4096
#define BITS_PER_BYTE 8
#define ASCII_CHAR_MAP_LEN INT8_MAX + 1
#define TEXT_DATA_BUFFER_LEN 1024

struct ASCIICharMap {
	size_t map[ASCII_CHAR_MAP_LEN];
};

void freeIov(void* arg) {
	struct iovec* iov = (struct iovec*) arg;
	free(iov->iov_base);
	free(iov);
}

bool readOriginalFile(struct LinkedList* outputList, FILE* inputFile, size_t* bytesRead) {
	if (!outputList || !inputFile)
		return false;

	size_t bytesAddedToBuf = 0;
	do {
		char* textBuffer = (char*) malloc(TEXT_DATA_BUFFER_LEN);
		if (!textBuffer)
			return false;
		memset(textBuffer, 0, TEXT_DATA_BUFFER_LEN);
		struct iovec* iov = (struct iovec*) malloc(sizeof(struct iovec));
		if (!iov)
			return false;

		bytesAddedToBuf = fread(textBuffer, sizeof(char), TEXT_DATA_BUFFER_LEN, inputFile);
		iov->iov_base = textBuffer;
		iov->iov_len = bytesAddedToBuf;
		*bytesRead += bytesAddedToBuf;
		llist_pushback(outputList, iov);
	} while (bytesAddedToBuf == TEXT_DATA_BUFFER_LEN);
	return true;
}

bool getCharacterFrequenciesHelper(struct iovec* iov, struct ASCIICharMap* outputMap) {
	const char* rawIter = (char*) iov->iov_base;
	const char* const rawTextEnd = (char*) iov->iov_base + iov->iov_len;
	while (rawIter != rawTextEnd) {
		if (*rawIter > sizeof(outputMap->map) / sizeof(outputMap->map[0])) {
			fprintf(stderr, "Character is unable to be stored in frequency map\r\n");
			return false;
		}
		outputMap->map[*rawIter]++;
		rawIter++;
	}
	return true;
}

bool getCharacterFrequencies(struct LinkedList* inputData, struct ASCIICharMap* outputMap) {
	if (!inputData || !outputMap)
		return false;

	struct Node* inputDataIter = inputData->head;
	while (inputDataIter) {
		if (!getCharacterFrequenciesHelper(inputDataIter->data, outputMap)) {
			return false;
		}
		inputDataIter = inputDataIter->next;
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
	memcpy(buf + sizeof(originalFileLen), &dictLen, sizeof(dictLen));
	return sizeof(originalFileLen) + sizeof(dictLen);
}

bool writeEncodedFile(FILE* encodedFile, size_t originalFileSize, size_t dictSize,
		      struct LinkedList* inputData, struct ASCIICharMap* asciiMap) {
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
	struct LinkedList originalFileData = {
		.head = NULL,
		.tail = NULL
	};

	size_t originalFileSize = 0;
	bool success = readOriginalFile(&originalFileData, originalFile, &originalFileSize);
	fclose(originalFile);

	if (!success) {
		fprintf(stderr, "Failed to read file");
		llist_free(&originalFileData, freeIov);
		return 1;
	}

	struct ASCIICharMap asciiCharMap;
	memset(asciiCharMap.map, 0, sizeof(asciiCharMap.map));
	success = getCharacterFrequencies(&originalFileData, &asciiCharMap);
	if (!success) {
		fprintf(stderr, "Unable to get frequency map\n");
		return 1;
	}

	// Create priority queue
	struct LinkedList priorityQueue = {
		.head = NULL,
		.tail = NULL
	};
	success = createPriorityQueue(&asciiCharMap, &priorityQueue);
	if (!success) {
		fprintf(stderr, "Unable to create priority queue\n");
		llist_free(&originalFileData, freeIov);
		llist_free(&priorityQueue, freeHuffmanTreeCb);
		return 1;
	}

	struct TreeNode* treeRoot = NULL;
	success = buildHuffmanTree(&priorityQueue, &treeRoot);
	if (!success) {
		fprintf(stderr, "No tree created\n");
		llist_free(&originalFileData, freeIov);
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
		llist_free(&originalFileData, freeIov);
		return 1;
	}

	struct HuffmanEncoding* huffIter = huffArray;
	success = getHuffmanEncodings(treeRoot, &initialEncoding, &huffIter,
				      huffArray + huffArrayLen);
	freeHuffmanTree(treeRoot);
	if (!success) {
		fprintf(stderr, "Unable to work properly\n");
		llist_free(&originalFileData, freeIov);
		return 1;
	}

	intptr_t diff = (intptr_t) huffIter - (intptr_t) huffArray;
	size_t dictSize = diff;
	printf("dictSize: %lu\n", dictSize);

	uint8_t* byteArray = (uint8_t*) malloc(BUFFER_LEN);
	if (!byteArray) {
		fprintf(stderr, "Unable to allocate buffer for byte array\n");
		llist_free(&originalFileData, freeIov);
		return 1;
	}

	int32_t bytesWritten = 0;
	FILE* encodedFile = fopen(encodedFileName, "wb");
	if (!encodedFile) {
		fprintf(stderr, "argc: %d\n", argc);
		fprintf(stderr, "Original File: %s\n", argv[1]);
		fprintf(stderr, "Failed to create file %s\n", encodedFileName);
		free(huffArray);
		llist_free(&originalFileData, freeIov);
		return 1;
	}

	bytesWritten += populateEncodingHdr(byteArray, originalFileSize, dictSize);

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

	// Write Encoded File
	uint8_t* byteIter = byteArray;
	const uint8_t* endIter = byteArray + BUFFER_LEN;
	uint32_t bitsWritten = 0;
	while (originalFileData.head) {
		struct iovec* iov = llist_popfront(&originalFileData);
		const char* const iovEnd = (char*) iov->iov_base + iov->iov_len;
		for (char* c = iov->iov_base; c != iovEnd; c++) {
			struct HuffmanEncoding* he = NULL;
			for (size_t heIdx = 0; heIdx < huffArrayLen; heIdx++) {
				if (huffArray[heIdx].character == *c) {
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
		free(iov->iov_base);
		free(iov);
	}

	fwrite(byteArray, sizeof(uint8_t), bytesWritten + 1, encodedFile);
	fclose(encodedFile);
	free(huffArray);
	free(byteArray);
}
