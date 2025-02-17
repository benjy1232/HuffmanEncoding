cmake_minimum_required(VERSION 3.30)
project(HuffmanEncoding C)

set(CMAKE_C_STANDARD 17)

add_executable(encoding encoding.c
        list.h
        list.c
        huffman_encoding.c
        huffman_encoding.h)
