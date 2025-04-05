#include "huffman_encoding.h"
#include "list.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>

#define BUFFER_LEN           4096
#define BITS_PER_BYTE        8
#define ASCII_CHAR_MAP_LEN   INT8_MAX + 1
#define HUFF_ARRAY_LEN       INT8_MAX + 1
#define TEXT_DATA_BUFFER_LEN 1024

/**
 * @brief ASCIICharMap is a wrapper for a size_t array with a defined size of `ASCII_CHAR_MAP_LEN`
 */
typedef struct {
	size_t map[ASCII_CHAR_MAP_LEN];
} ASCIICharMap;

void freeIov(void* arg) {
	struct iovec* iov = (struct iovec*) arg;
	free(iov->iov_base);
	free(iov);
}

/**
 * @brief Reads all the contents of a text file and store the data into a LinkedList of iovs
 *
 * @param[in] inputFileName - The path/name to the input file
 * @param[out] outputList - The list in which all the iovs will be stored
 * @param[out] bytesRead - A pointer to the number of bytes that has been read
 */
bool readFileToLinkedList(char* inputFilePath, LinkedList* outputList, size_t* bytesRead) {
	if (!outputList || !inputFilePath || !bytesRead)
		return false;

	FILE* inputFile = fopen(inputFilePath, "r");
	if (!inputFile) {
		fprintf(stderr, "%s: Unable to open file: %s (errno: %d)\n",
			__func__, inputFilePath, errno);
		return false;
	}

	size_t bytesAddedToBuf = 0;
	do {
		char* textBuffer = (char*) malloc(TEXT_DATA_BUFFER_LEN);
		if (!textBuffer) {
			fprintf(stderr, "Unable to allocate textBuffer\n");
			fclose(inputFile);
			return false;
		}
		memset(textBuffer, 0, TEXT_DATA_BUFFER_LEN);

		struct iovec* iov = (struct iovec*) malloc(sizeof(struct iovec));
		if (!iov) {
			fprintf(stderr, "Unable to allocate iovec to store textbuffer");
			free(textBuffer);
			fclose(inputFile);
			return false;
		}

		bytesAddedToBuf = fread(textBuffer, sizeof(char), TEXT_DATA_BUFFER_LEN, inputFile);
		iov->iov_base = textBuffer;
		iov->iov_len = bytesAddedToBuf;
		*bytesRead += bytesAddedToBuf;
		llist_pushback(outputList, iov);
	} while (bytesAddedToBuf == TEXT_DATA_BUFFER_LEN);

	fclose(inputFile);
	return true;
}

/**
 * @brief Get the character frequencies from an iov
 *
 * @param[in]  iov - Input IOV to read data from
 * @param[out] outputMap - Map to store read data into
 * @returns true on success, false for any failure
 */
bool getCharacterFrequenciesHelper(struct iovec* iov, ASCIICharMap* outputMap) {
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

/**
 * @brief Get the Character Frequencies for a Linked List
 *
 * @param inputData
 * @param outputMap
 * @return true
 * @return false
 */
bool getCharacterFrequencies(LinkedList* inputData, ASCIICharMap* outputMap) {
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

/**
 * @brief Create a PriorityQueue from an ASCIICharMap
 *
 * @param[in] inputMap
 * @param[out] outputPriorityQueue
 * @return true
 */
static bool createPriorityQueue(ASCIICharMap* inputMap,
				LinkedList* outputPriorityQueue) {
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

bool getHuffmanEncoding(LinkedList* inputFileList, struct HuffmanEncoding* huffEncodings, const size_t huffArrayLen, size_t* dictSize) {
	if (!inputFileList || !huffEncodings || !dictSize)
		return false;

	ASCIICharMap asciiCharMap;
	memset(asciiCharMap.map, 0, sizeof(asciiCharMap.map));
	bool success = getCharacterFrequencies(inputFileList, &asciiCharMap);
	if (!success) {
		fprintf(stderr, "Unable to get frequency map\n");
		return false;
	}

	// Create priority queue
	LinkedList priorityQueue = {
		.head = NULL,
		.tail = NULL
	};
	success = createPriorityQueue(&asciiCharMap, &priorityQueue);
	if (!success) {
		fprintf(stderr, "Unable to create priority queue\n");
		llist_free(&priorityQueue, freeHuffmanTreeCb);
		return false;
	}

	struct TreeNode* treeRoot = NULL;
	success = buildHuffmanTree(&priorityQueue, &treeRoot);
	if (!success) {
		fprintf(stderr, "No tree created\n");
		llist_free(&priorityQueue, freeHuffmanTreeCb);
		return false;
	}
	struct HuffmanEncoding initialEncoding = {
		.bitStr = 0,
		.length = 0,
		.character = 0
	};

	struct HuffmanEncoding* huffIter = huffEncodings;
	success = generateHuffmanEncodings(treeRoot, &initialEncoding, &huffIter,
				      	   huffEncodings + huffArrayLen);
	freeHuffmanTree(treeRoot);
	if (!success) {
		fprintf(stderr, "Unable to work properly\n");
		return false;
	}

	if ((uintptr_t) huffIter > (uintptr_t) huffEncodings)
		*dictSize = (uintptr_t) huffIter - (uintptr_t) huffEncodings;
	else
		*dictSize = (uintptr_t) huffEncodings - (uintptr_t) huffIter;

	return true;
}

/**
 * @brief
 *
 * @param buf
 * @param originalFileSize
 * @param dictLen
 * @return size_t
 */
size_t populateEncodingHdr(uint8_t* buf, size_t originalFileSize, size_t dictSize) {
	memcpy(buf, &originalFileSize, sizeof(originalFileSize));
	memcpy(buf + sizeof(originalFileSize), &dictSize, sizeof(dictSize));
	return sizeof(originalFileSize) + sizeof(dictSize);
}

/**
 * @brief
 *
 * @param encodedFile
 * @param originalFileSize
 * @param dictSize
 * @param inputData
 * @param asciiMap
 * @return true
 * @return false
 */
bool writeEncodedFile(char* encodedFile, size_t originalFileSize, size_t dictSize,
		      LinkedList* inputData, ASCIICharMap* asciiMap) {
	return true;
}

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "Need to specify a file to encode\n");
		fprintf(stderr, "Need to specify file to write data into\n");
		return 1;
	}
	char* inputFilePath  = argv[1];
	char* outputFilePath = argv[2];
	LinkedList inputFileList = {
		.head = NULL,
		.tail = NULL
	};

	size_t originalFileSize = 0;
	bool success = readFileToLinkedList(inputFilePath, &inputFileList, &originalFileSize);
	if (!success) {
		fprintf(stderr, "Failed to read file");
		llist_free(&inputFileList, freeIov);
		return 1;
	}

	const size_t huffArraySize = sizeof(struct HuffmanEncoding) * HUFF_ARRAY_LEN;
	struct HuffmanEncoding* huffEncodings =
		(struct HuffmanEncoding*) malloc(huffArraySize);
	if (!huffEncodings) {
		fprintf(stderr, "Unable to allocate memory for huffArray\n");
		llist_free(&inputFileList, freeIov);
		return 1;
	}
	memset(huffEncodings, 0, huffArraySize);

	size_t dictSize = 0;
	success = getHuffmanEncoding(&inputFileList, huffEncodings, HUFF_ARRAY_LEN, &dictSize);
	printf("dictSize: %lu\n", dictSize);

	uint8_t* byteArray = (uint8_t*) malloc(BUFFER_LEN);
	if (!byteArray) {
		fprintf(stderr, "Unable to allocate buffer for byte array\n");
		llist_free(&inputFileList, freeIov);
		return 1;
	}

	int32_t bytesWritten = 0;
	FILE* encodedFile = fopen(outputFilePath, "wb");
	if (!encodedFile) {
		fprintf(stderr, "argc: %d\n", argc);
		fprintf(stderr, "Original File: %s\n", argv[1]);
		fprintf(stderr, "Failed to create file %s\n", outputFilePath);
		free(huffEncodings);
		llist_free(&inputFileList, freeIov);
		return 1;
	}

	bytesWritten += populateEncodingHdr(byteArray, originalFileSize, dictSize);

	// Start writing the encoding into the array
	for (size_t i = 0; i < INT8_MAX; i++) {
		if (huffEncodings[i].length == 0)
			continue;
		if (bytesWritten + sizeof(huffEncodings[i]) >= BUFFER_LEN) {
			fwrite(byteArray, sizeof(uint8_t), bytesWritten, encodedFile);
			memset(byteArray, 0, BUFFER_LEN);
			bytesWritten = 0;
		}
		memcpy(byteArray + bytesWritten, huffEncodings + i, sizeof(huffEncodings[i]));
		bytesWritten += sizeof(huffEncodings[i]);
	}
	fwrite(byteArray, sizeof(uint8_t), bytesWritten, encodedFile);
	bytesWritten = 0;
	memset(byteArray, 0, BUFFER_LEN);

	// Write Encoded File
	uint8_t* byteIter = byteArray;
	const uint8_t* endIter = byteArray + BUFFER_LEN;
	uint32_t bitsWritten = 0;
	while (inputFileList.head) {
		struct iovec* iov = llist_popfront(&inputFileList);
		const char* const iovEnd = (char*) iov->iov_base + iov->iov_len;
		for (char* c = iov->iov_base; c != iovEnd; c++) {
			struct HuffmanEncoding* he = NULL;
			for (size_t heIdx = 0; heIdx < HUFF_ARRAY_LEN; heIdx++) {
				if (huffEncodings[heIdx].character == *c) {
					he = huffEncodings + heIdx;
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
				uint32_t bitIdx = BITS_PER_BYTE - 1 - bitsWritten;
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

	if (bytesWritten != 0)
		fwrite(byteArray, sizeof(uint8_t), bytesWritten + 1, encodedFile);
	fclose(encodedFile);
	free(huffEncodings);
	free(byteArray);
}
