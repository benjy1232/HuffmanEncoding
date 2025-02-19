#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

static const size_t DICT_ENTRY_LEN = 16;
static const size_t BYTE_ARRAY_LEN = 1024;
static const size_t BITS_PER_BYTE = 8;

struct BitString {
    uint64_t bitStr;
    int32_t  len;
    char     character;
};

void decodeArray(const std::array<std::byte, BYTE_ARRAY_LEN>& encodedArray, std::vector<char>& decodedArray, const std::unordered_map<uint64_t, std::vector<BitString>>& decodeMap) {
    size_t currentBitStrLen = 0;
    uint64_t currentBitStr = 0;
    for (const auto& b : encodedArray) {
        // Read from MSB to LSB
        for (int i = 0; i < BITS_PER_BYTE; i++) {
            currentBitStr <<= 1;
            currentBitStr |= (static_cast<uint64_t>(b) >> BITS_PER_BYTE - 1 - i) & 1;
            currentBitStrLen++;
            if (decodeMap.find(currentBitStr) == decodeMap.end()) {
                continue;
            }
            for (const auto& bitString : decodeMap.at(currentBitStr)) {
                if (currentBitStrLen == bitString.len) {
                    currentBitStr = 0;
                    currentBitStrLen = 0;
                    decodedArray.push_back(bitString.character);
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Need a file to decode" << std::endl;
        return 1;
    }
    std::ifstream encodedFile(argv[1], std::ios::binary);
    uint64_t uncompressedFileLen = 0;
    size_t dictLen = 0;
    if (encodedFile.read(reinterpret_cast<char*>(&uncompressedFileLen), sizeof(uncompressedFileLen))) {
        const std::streamsize bytesRead = encodedFile.gcount();
    } else {
        std::cerr << "Unable to read file data" << std::endl;
        return 1;
    }

    if (encodedFile.read(reinterpret_cast<char*>(&dictLen), sizeof(dictLen))) {
        const std::streamsize bytesRead = encodedFile.gcount();
    } else {
        std::cerr << "Unable to read file data" << std::endl;
        return 1;
    }

    std::vector<char> dictBuf(dictLen);
    if (!encodedFile.read(dictBuf.data(), dictBuf.size())) {
        std::cerr << "Unable to read dictionary of file" << std::endl;
        return 1;
    }

    std::unordered_map<uint64_t, std::vector<BitString>> huffmanMap(dictLen / DICT_ENTRY_LEN);
    size_t offset = 0;
    while (offset < dictLen) {
        BitString* tmp = reinterpret_cast<BitString*>(dictBuf.data() + offset);
        huffmanMap[tmp->bitStr].push_back(*tmp);
        offset += DICT_ENTRY_LEN;
    }

    std::array<std::byte, BYTE_ARRAY_LEN> byteArray;
    byteArray.fill(std::byte{0});
    std::vector<char> output;
    while (encodedFile.read(reinterpret_cast<char*>(byteArray.data()), byteArray.size())) {
        decodeArray(byteArray, output, huffmanMap);
        byteArray.fill(std::byte{0});

    }
    std::cout << encodedFile.good() << std::endl;
    std::cout << output.data();
}
