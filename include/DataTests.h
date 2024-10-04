#ifndef DATATESTS_H
#define DATATESTS_H
#include <stdio.h>
void TestGenerateDataStream();
void printBytesAsHex(const char *byteStream, size_t size);
void printStructBytes(void *ptr, size_t size);
void TestRecreateFromDataStream(char *destPath);
void TestCleanUpIfExtractionFails(char *destPath);
void TestCreateDirectories(char *filePath);
#endif
