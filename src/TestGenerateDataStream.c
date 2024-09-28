#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "../include/Data.h"
#include "../include/Structs.h"

void TestGenerateDataStream() {
  size_t size = 0;
  size_t offset = 0;
  char *byteStream = GenerateDataStream(".", &size, &offset);

  assert(byteStream != NULL);

  size_t currentOffset = 0;

  while (currentOffset < size) {
    printf("Reading at: %zu\n", currentOffset);
    FileHeader *header = (FileHeader *)(byteStream + currentOffset);

    assert(header->filename != NULL);

    printf(
        "Processing file: %s, Offset: %d, Mode: %d iteration: %zu Path: %s\n",
        header->filename, header->fileOffset, header->mode, currentOffset,
        header->filepath);

    assert(header->fileOffset > currentOffset);
    currentOffset = header->fileOffset;
  }
  printf("Test passed successfully!\n");
}

void printBytesAsHex(const char *byteStream, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (i % 16 == 0) {
      printf("\nOffset %04zu: ", i);
    }
    printf("%c ", (unsigned char)byteStream[i]);
  }
  printf("\n");
}
void printStructBytes(void *ptr, size_t size) {
  unsigned char *bytePtr = (unsigned char *)ptr;

  printf("==========\nPrinting Struct Bytes: \n");
  for (size_t i = 0; i < size; i++) {
    printf("%02X ", bytePtr[i]);
  }
  printf("\n==========\n");
}
