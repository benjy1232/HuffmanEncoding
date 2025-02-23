cmake_minimum_required(VERSION 3.30)
project(HuffmanEncoding C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

add_executable(encoding c-encoder/encoding.c
        c-encoder/list.h
        c-encoder/list.c
        c-encoder/huffman_encoding.c
        c-encoder/huffman_encoding.h)

add_executable(decoding cpp-decoder/decoding.cc)
