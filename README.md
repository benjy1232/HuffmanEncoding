# Huffman Encoding/Decoding

This will encode a text file using huffman encoding.
The encoding implementation is written in C while the original decoding implementation
was written in C++.

This is primarily serving as a way to teach myself the basics and make sure I understand what
I'm doing in all these different languages.

The file format is essentially this:
```
--------------------------------------------------------------
| Uncompressed File Len | Dictionary Len | Dictionary | Data |
--------------------------------------------------------------
| 8 Bytes               | 8 bytes        | dict_len   | data |
--------------------------------------------------------------
```
The `dict_len` is always guaranteed to be divisible by 16 bytes, because
each dictionary entry is 16 bytes
```c
struct HuffmanEncoding
{
    uint64_t bitStr;
    int32_t  length;
    char     character;
};

```

Each byte should be read from MSB to LSB to ensure the right character encoding is read.
The actual data is written in Little Endian
