#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

static const size_t DICT_ENTRY_LEN = 16;
static const size_t BYTE_ARRAY_LEN = 1024;
static const size_t BITS_PER_BYTE = 8;

struct BitString {
    uint64_t bitStr;
    int32_t  len;
};

struct BitStringMapEntry {
    uint64_t bitStr;
    int32_t  len;
    char     character;
};

using BitStringMap = std::unordered_map<uint64_t, std::vector<BitStringMapEntry>>;

class HuffmanDecoder {
public:
    static const size_t BITS_PER_BYTE = 8;
    HuffmanDecoder() = delete;
    HuffmanDecoder(const HuffmanDecoder&) = delete;
    HuffmanDecoder(uint64_t uncompressedFileLen, const BitStringMap& m_BitStringMap);

    void decodeByteArray(const std::byte* byteArray, size_t byteArrayLen);
    bool finished() const { return m_BytesDecoded == m_UncompressedFileLen; }
    std::string_view getDecodedString() const { return std::string_view(m_DecodedString.get(), m_UncompressedFileLen); }
private:
    uint64_t     m_UncompressedFileLen;
    uint64_t     m_BytesDecoded;
    BitString    m_CurrentBitString;
    BitStringMap m_BitStringMap;
    std::unique_ptr<char[]>  m_DecodedString;
};

HuffmanDecoder::HuffmanDecoder(uint64_t uncompressedFileLen, const BitStringMap& bitStringMap)
    : m_UncompressedFileLen(uncompressedFileLen),
    m_BytesDecoded(0),
    m_CurrentBitString(),
    m_BitStringMap(bitStringMap),
    m_DecodedString(new char[uncompressedFileLen]) {
}

void HuffmanDecoder::decodeByteArray(const std::byte* byteArray, size_t byteArrayLen) {
    if (m_BytesDecoded == m_UncompressedFileLen) {
        return;
    }
    char* decodeStringIter = m_DecodedString.get() + m_BytesDecoded;
    char* const decodedStringEnd = m_DecodedString.get() + m_UncompressedFileLen;
    const std::byte* const byteArrayEnd = byteArray + byteArrayLen;
    for (const std::byte* byteIter = byteArray; byteIter != byteArrayEnd; byteIter++) {
        // Read from MSB to LSB
        for (size_t i = 0; i < BITS_PER_BYTE; i++) {
            m_CurrentBitString.bitStr <<= 1;
            m_CurrentBitString.bitStr |= (static_cast<uint64_t>(*byteIter) >> (BITS_PER_BYTE - 1 - i)) & 1;
            m_CurrentBitString.len++;
            if (m_BitStringMap.find(m_CurrentBitString.bitStr) == m_BitStringMap.end()) {
                continue;
            }
            for (const auto& bitStringEntry : m_BitStringMap.at(m_CurrentBitString.bitStr)) {
                if (bitStringEntry.len == m_CurrentBitString.len) {
                    *decodeStringIter = bitStringEntry.character;
                    m_CurrentBitString.bitStr = 0;
                    m_CurrentBitString.len = 0;
                    decodeStringIter++;
                    m_BytesDecoded++;
                    if (decodeStringIter == decodedStringEnd)
                        return;
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Need file to read" << std::endl;
        return 1;
    }
    std::ifstream encodedFile(argv[1], std::ios::binary);
    uint64_t uncompressedFileLen = 0;
    size_t dictLen = 0;
    if (!encodedFile.read(reinterpret_cast<char*>(&uncompressedFileLen), sizeof(uncompressedFileLen))) {
        std::cerr << "Unable to read file data" << std::endl;
        return 1;
    }
    // Subtract because fileLen is NULL padded
    uncompressedFileLen--;

    if (!encodedFile.read(reinterpret_cast<char*>(&dictLen), sizeof(dictLen))) {
        std::cerr << "Unable to read file data" << std::endl;
        return 1;
    }

    std::vector<char> dictBuf(dictLen);
    if (!encodedFile.read(dictBuf.data(), dictBuf.size())) {
        std::cerr << "Unable to read dictionary of file" << std::endl;
        return 1;
    }

    BitStringMap huffmanMap(dictLen / DICT_ENTRY_LEN);
    size_t offset = 0;
    while (offset < dictLen) {
        BitStringMapEntry* tmp = reinterpret_cast<BitStringMapEntry*>(dictBuf.data() + offset);
        huffmanMap[tmp->bitStr].push_back(*tmp);
        offset += DICT_ENTRY_LEN;
    }

    std::array<std::byte, BYTE_ARRAY_LEN> byteArray;
    byteArray.fill(std::byte{0});
    HuffmanDecoder huffmanDecoder(uncompressedFileLen, huffmanMap);
    while (encodedFile.read(reinterpret_cast<char*>(byteArray.data()), byteArray.size()) && !huffmanDecoder.finished()) {
        huffmanDecoder.decodeByteArray(byteArray.data(), byteArray.size());
        byteArray.fill(std::byte{0});
    }

    if (encodedFile.eof() && !huffmanDecoder.finished()) {
        huffmanDecoder.decodeByteArray(byteArray.data(), encodedFile.gcount());
    }

    std::cout << huffmanDecoder.getDecodedString();
}
