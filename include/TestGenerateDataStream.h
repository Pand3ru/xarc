#ifndef TEST_BYTESTREAM_H
#define TEST_BYTESTREAM_H
#include <stdio.h>
void TestGenerateDataStream();
void printBytesAsHex(const char *byteStream, size_t size);
void printStructBytes(void *ptr, size_t size);
#endif
