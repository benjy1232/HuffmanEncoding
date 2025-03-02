#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

static const size_t DICT_ENTRY_LEN = 16;
static const size_t BYTE_ARRAY_LEN = 1024;

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
    static const int BITS_PER_BYTE = 8;
    HuffmanDecoder() = delete;
    HuffmanDecoder(const HuffmanDecoder&) = delete;
    HuffmanDecoder(uint64_t fileLen, const BitStringMap& bitStringMap);

    void decodeByteArray(const std::byte* byteArray, size_t byteArrayLen);
    bool isFinished() const { return m_BytesDecoded == m_UncompressedFileLen; }
    std::string_view getDecodedString() const
        { return std::string_view(m_DecodedString.get(), m_UncompressedFileLen); }
private:
    bool decodeByte(const std::byte byte);

    uint64_t     m_UncompressedFileLen;
    uint64_t     m_BytesDecoded;
    BitString    m_CurrentBitString;
    BitStringMap m_BitStringMap;
    std::unique_ptr<char[]>  m_DecodedString;
};

HuffmanDecoder::HuffmanDecoder(uint64_t fileLen, const BitStringMap& bitStringMap)
    : m_UncompressedFileLen(fileLen),
    m_BytesDecoded(0),
    m_CurrentBitString(),
    m_BitStringMap(bitStringMap),
    m_DecodedString(new char[fileLen]) {
}

bool HuffmanDecoder::decodeByte(const std::byte currentByte) {
    if (isFinished()) {
        return false;
    }
    char* decodeStringIter = m_DecodedString.get() + m_BytesDecoded;
    // Read from MSB to LSB
    for (int bitIdx = BITS_PER_BYTE - 1; bitIdx >= 0; bitIdx--) {
        m_CurrentBitString.bitStr <<= 1;
        uint64_t currentBit = static_cast<uint64_t>(currentByte) >> bitIdx;
        m_CurrentBitString.bitStr |= currentBit & 1;
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
                if (isFinished())
                    return true;
            }
        }
    }
    return true;
}

void HuffmanDecoder::decodeByteArray(const std::byte* byteArray, size_t byteArrayLen) {
    if (m_BytesDecoded == m_UncompressedFileLen) {
        return;
    }
    const std::byte* const byteArrayEnd = byteArray + byteArrayLen;
    for (const std::byte* byteIter = byteArray; byteIter != byteArrayEnd; byteIter++) {
        if (!decodeByte(*byteIter) || isFinished())
            return;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Need file to read" << std::endl;
        return 1;
    }
    char* encodedFilePath = argv[1];
    std::ifstream encodedFile(encodedFilePath, std::ios::binary);
    uint64_t uncompressedFileLen = 0;
    size_t dictLen = 0;
    if (!encodedFile.read(reinterpret_cast<char*>(&uncompressedFileLen), sizeof(uncompressedFileLen))) {
        std::cerr << "Unable to read file data" << std::endl;
        return 1;
    }

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

    std::array<char, BYTE_ARRAY_LEN> byteArray;
    byteArray.fill(0);
    HuffmanDecoder huffmanDecoder(uncompressedFileLen, huffmanMap);
    while (encodedFile.read(reinterpret_cast<char*>(byteArray.data()), byteArray.size()) && !huffmanDecoder.isFinished()) {
        huffmanDecoder.decodeByteArray(reinterpret_cast<std::byte*>(byteArray.data()), byteArray.size());
        byteArray.fill(0);
    }

    if (encodedFile.eof() && !huffmanDecoder.isFinished()) {
        huffmanDecoder.decodeByteArray(reinterpret_cast<std::byte*>(byteArray.data()), byteArray.size());
    }

    std::cout << huffmanDecoder.getDecodedString();
}
