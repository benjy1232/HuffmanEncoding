#include "huffman_encoding.h"
#include "list.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

#define BUFFER_LEN 1024
#define BITS_PER_BYTE 8
#define ASCII_CHAR_MAP_LEN INT8_MAX + 1
#define HUFF_ARRAY_LEN INT8_MAX + 1
#define TEXT_DATA_BUFFER_LEN 1024

/**
 * @brief ASCIICharMap is a wrapper for a size_t array with a defined size of
 * `ASCII_CHAR_MAP_LEN`
 */
typedef struct
{
    size_t map[ASCII_CHAR_MAP_LEN];
} ASCIICharMap;

void freeIov(void *arg)
{
    struct iovec *iov = (struct iovec *)arg;
    free(iov->iov_base);
    free(iov);
}

/**
 * @brief Reads all the contents of a text file and store the data into a
 *        LinkedList of iovs
 *
 * @param[in] inputFileName - The path/name to the input file
 * @param[out] outputList - The list in which all the iovs will be stored
 * @param[out] bytesRead - A pointer to the number of bytes that has been read
 */
bool readFileToLinkedList(char *inputFilePath, LinkedList *outputList, size_t *bytesRead)
{
    if (!outputList || !inputFilePath || !bytesRead)
        return false;

    FILE *inputFile = fopen(inputFilePath, "r");
    if (!inputFile)
    {
        fprintf(stderr, "%s: Unable to open file: %s (errno: %d)\n", __func__, inputFilePath, errno);
        return false;
    }

    size_t bytesAddedToBuf = 0;
    do
    {
        char *textBuffer = (char *)malloc(TEXT_DATA_BUFFER_LEN);
        if (!textBuffer)
        {
            fprintf(stderr, "Unable to allocate textBuffer\n");
            fclose(inputFile);
            return false;
        }
        memset(textBuffer, 0, TEXT_DATA_BUFFER_LEN);

        struct iovec *iov = (struct iovec *)malloc(sizeof(struct iovec));
        if (!iov)
        {
            fprintf(stderr, "Unable to allocate struct iovec to store textbuffer");
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
 * @param[in] iov - Input IOV to read data from
 * @param[out] outputMap - Map to store read data into
 * @returns true on success, false for any failure
 */
bool getCharFrequenciesHelper(struct iovec *iov, ASCIICharMap *outputMap)
{
    const char *rawIter = (char *)iov->iov_base;
    const char *const rawTextEnd = (char *)iov->iov_base + iov->iov_len;
    while (rawIter != rawTextEnd)
    {
        if (*rawIter > sizeof(outputMap->map) / sizeof(outputMap->map[0]))
        {
            fprintf(stderr, "Character is unable to be stored in frequency map\n");
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
bool getCharacterFrequencies(LinkedList *inputData, ASCIICharMap *outputMap)
{
    if (!inputData || !outputMap)
        return false;

    struct Node *inputDataIter = inputData->head;
    while (inputDataIter)
    {
        if (!getCharFrequenciesHelper(inputDataIter->data, outputMap))
        {
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
bool createPriorityQueue(ASCIICharMap *inputMap, LinkedList *outputPriQ)
{
    bool isSuccess = true;
    size_t mapSize = sizeof(inputMap->map) / sizeof(inputMap->map[0]);
    for (size_t i = 0; i < mapSize; i++)
    {
        if (inputMap->map[i] == 0)
            continue;

        TreeNode *node = (TreeNode *)malloc(sizeof(TreeNode));
        if (!node)
        {
            fprintf(stderr, "Unable to enough memory for new node\n");
            return false;
        }
        node->character = (char)i;
        node->weight = inputMap->map[i];
        node->left = NULL;
        node->right = NULL;
        isSuccess = llist_insertUsingCompare(outputPriQ, node, treeNode_comparator);
    }
    return isSuccess;
}

bool getHuffmanEncoding(LinkedList *inputFileList, HuffmanEncoding *huffDict, const size_t huffArrayLen,
                        uint64_t *dictSize)
{
    if (!inputFileList || !huffDict || !dictSize)
        return false;

    ASCIICharMap asciiCharMap;
    memset(asciiCharMap.map, 0, sizeof(asciiCharMap.map));
    bool success = getCharacterFrequencies(inputFileList, &asciiCharMap);
    if (!success)
    {
        fprintf(stderr, "Unable to get frequency map\n");
        return false;
    }

    // Create priority queue
    LinkedList priorityQueue = {.head = NULL, .tail = NULL};
    success = createPriorityQueue(&asciiCharMap, &priorityQueue);
    if (!success)
    {
        fprintf(stderr, "Unable to create priority queue\n");
        llist_free(&priorityQueue, freeHuffmanTreeCb);
        return false;
    }

    TreeNode *treeRoot = NULL;
    success = buildHuffmanTree(&priorityQueue, &treeRoot);
    if (!success)
    {
        fprintf(stderr, "No tree created\n");
        llist_free(&priorityQueue, freeHuffmanTreeCb);
        return false;
    }
    HuffmanEncoding initialEncoding = {.bitStr = 0, .length = 0, .character = 0};

    HuffmanEncoding *huffIter = huffDict;
    success = generateHuffmanEncodings(treeRoot, &initialEncoding, &huffIter, huffDict + huffArrayLen);
    freeHuffmanTree(treeRoot);
    if (!success)
    {
        fprintf(stderr, "Unable to work properly\n");
        return false;
    }

    if ((uintptr_t)huffIter > (uintptr_t)huffDict)
        *dictSize = (uint64_t)((uintptr_t)huffIter - (uintptr_t)huffDict);
    else
        *dictSize = (uint64_t)((uintptr_t)huffDict - (uintptr_t)huffIter);

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
size_t populateEncodingHdr(uint8_t *buf, uint64_t originalFileSize, uint64_t dictSize)
{
    memcpy(buf, &originalFileSize, sizeof(originalFileSize));
    memcpy(buf + sizeof(originalFileSize), &dictSize, sizeof(dictSize));
    return sizeof(originalFileSize) + sizeof(dictSize);
}

bool writeDictToFile(FILE *encodedFile, struct iovec *bufIov, size_t *bufOffset, HuffmanEncoding *huffEncodings)
{
    if (!encodedFile || !bufIov || !bufOffset || !huffEncodings || !bufIov->iov_base)
    {
        fprintf(stderr, "Unable to write dict to file\n");
        return false;
    }
    uint8_t *buf = bufIov->iov_base;
    size_t bufLen = bufIov->iov_len;
    // Start writing the encoding into the array
    for (size_t i = 0; i < INT8_MAX; i++)
    {
        if (huffEncodings[i].length == 0)
            continue;
        if (*bufOffset + sizeof(huffEncodings[i]) >= bufLen)
        {
            fwrite(buf, sizeof(uint8_t), *bufOffset, encodedFile);
            memset(buf, 0, bufLen);
            *bufOffset = 0;
        }
        memcpy(buf + *bufOffset, huffEncodings + i, sizeof(huffEncodings[i]));
        *bufOffset += sizeof(huffEncodings[i]);
    }
    fwrite(buf, sizeof(uint8_t), *bufOffset, encodedFile);
    *bufOffset = 0;
    memset(buf, 0, bufLen);
    return true;
}

HuffmanEncoding *huffDictGetChar(HuffmanEncoding *huffDict, char c)
{
    for (size_t hdIdx = 0; hdIdx < HUFF_ARRAY_LEN; hdIdx++)
    {
        if (huffDict[hdIdx].character == c)
            return &huffDict[hdIdx];
    }
    return NULL;
}

bool writeBitString(FILE *encodedFile, struct iovec *bufIov, uint64_t *bufIdxPtr, int32_t *bitIdxPtr,
                    HuffmanEncoding *encoding)
{
    uint8_t *buf = bufIov->iov_base;
    uint64_t bitStr = encoding->bitStr;
    int32_t length = encoding->length;
    for (int i = length - 1; i >= 0; i--)
    {
        // Get the highest bit in bitStr
        uint64_t bitToSet = (bitStr >> i) & 1;
        uint32_t bitIdx = BITS_PER_BYTE - 1 - (*bitIdxPtr);
        buf[*bufIdxPtr] |= bitToSet << bitIdx;
        (*bitIdxPtr)++;
        if (*bitIdxPtr % BITS_PER_BYTE == 0)
        {
            (*bitIdxPtr) = 0;
            (*bufIdxPtr)++;
            if (*bufIdxPtr >= bufIov->iov_len)
            {
                fwrite(buf, sizeof(uint8_t), *bufIdxPtr, encodedFile);
                (*bufIdxPtr) = 0;
                memset(buf, 0, BUFFER_LEN);
            }
        }
    }
    return true;
}

/**
 * @brief Do Something
 */
bool writeEncodedData(FILE *encodedFile, struct iovec *bufIov, HuffmanEncoding *encodingDict,
                      LinkedList *originalFileData, uint64_t originalFileSize)
{
    if (!encodedFile || !bufIov || !encodingDict || !originalFileData)
    {
        return false;
    }

    uint64_t bytesWritten = 0;
    int32_t bitsWritten = 0;
    int32_t numIterations = 0;
    size_t bytesEncoded = 0;
    while (originalFileData->head)
    {
        struct iovec *iov = (struct iovec *)llist_popfront(originalFileData);
        const char *fileData = (char *)iov->iov_base;
        const size_t iovLen = iov->iov_len;
        bytesEncoded += iovLen;
        for (size_t i = 0; i < iovLen; i++)
        {
            HuffmanEncoding *he = huffDictGetChar(encodingDict, fileData[i]);
            if (!he)
                return false;

            if (!writeBitString(encodedFile, bufIov, &bytesWritten, &bitsWritten, he))
                return false;
        }
        numIterations++;
        free(iov->iov_base);
        free(iov);
    }

    printf("Num Iterations: %d\n", numIterations);
    printf("Bytes Encoded : %lu\n", bytesEncoded);

    if (bytesWritten != 0)
        fwrite(bufIov->iov_base, sizeof(uint8_t), bytesWritten + 1, encodedFile);

    return true;
}

/**
 * @brief Write the encoded file that will be used
 */
bool writeEncodedFile(FILE *encodedFile, HuffmanEncoding *dict, uint64_t dictLen, LinkedList *originalFileData,
                      uint64_t originalFileSize)
{
    uint8_t buf[BUFFER_LEN] = {};
    memset(buf, 0, sizeof(uint8_t) * BUFFER_LEN);
    struct iovec bufIov = {.iov_base = buf, .iov_len = BUFFER_LEN};

    size_t bufOffset = populateEncodingHdr(buf, originalFileSize, dictLen);
    bool success = writeDictToFile(encodedFile, &bufIov, &bufOffset, dict);
    success = writeEncodedData(encodedFile, &bufIov, dict, originalFileData, originalFileSize);
    return success;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "Need to specify a file to encode\n");
        fprintf(stderr, "Need to specify file to write data into\n");
        return 1;
    }
    char *inputFilePath = argv[1];
    char *outputFilePath = argv[2];
    LinkedList inputFileList = {.head = NULL, .tail = NULL};

    uint64_t originalFileSize = 0;
    bool success = readFileToLinkedList(inputFilePath, &inputFileList, &originalFileSize);
    if (!success)
    {
        fprintf(stderr, "Failed to read file");
        llist_free(&inputFileList, freeIov);
        return 1;
    }
    printf("Original File Size: %lu\n", originalFileSize);

    const size_t huffArraySize = sizeof(HuffmanEncoding) * HUFF_ARRAY_LEN;
    HuffmanEncoding *huffEncodings = (HuffmanEncoding *)malloc(huffArraySize);
    if (!huffEncodings)
    {
        fprintf(stderr, "Unable to allocate memory for huffArray\n");
        llist_free(&inputFileList, freeIov);
        return 1;
    }
    memset(huffEncodings, 0, huffArraySize);

    uint64_t dictSize = 0;
    success = getHuffmanEncoding(&inputFileList, huffEncodings, HUFF_ARRAY_LEN, &dictSize);
    if (!success)
    {
        fprintf(stderr, "Failed to get huffman encoding\n");
        return 1;
    }
    printf("dictSize: %lu\n", dictSize);

    FILE *encodedFile = fopen(outputFilePath, "wb");
    success = writeEncodedFile(encodedFile, huffEncodings, dictSize, &inputFileList, originalFileSize);
    fclose(encodedFile);
    free(huffEncodings);
    if (!success)
    {
        fprintf(stderr, "Failed to write encoded file: %s", outputFilePath);
        return 1;
    }
    return 0;
}
