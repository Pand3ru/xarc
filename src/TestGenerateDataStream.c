#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "../include/Data.h"
#include "../include/Structs.h"

void TestGenerateDataStream() {
  size_t size = 0;
  char *byteStream = GenerateDataStream(".", &size, 0);

  // Verify the byteStream was generated successfully
  assert(byteStream != NULL);

  size_t currentOffset = 0; // To track the current position in the byte stream

  while (currentOffset < size) {
    printf("Reading at: %zu\n", currentOffset);
    // Read the header at the current position
    FileHeader *header = (FileHeader *)(byteStream + currentOffset);

    // Check that the header and filename are valid
    assert(header->filename != NULL);

    printf(
        "Processing file: %s, Offset: %d, Mode: %d iteration: %zu Path: %s\n",
        header->filename, header->fileOffset, header->mode, currentOffset,
        header->filepath);

    // Ensure that the fileOffset is greater than the current offset
    assert(header->fileOffset > currentOffset);
    currentOffset = header->fileOffset;
  }
  printf("Test passed successfully!\n");
}

void printBytesAsHex(const char *byteStream, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (i % 16 == 0) {
      // Print the current offset before the next 16 bytes
      printf("\nOffset %04zu: ", i);
    }
    printf("%c ", (unsigned char)byteStream[i]);
  }
  printf("\n");
}
void printStructBytes(void *ptr, size_t size) {
  unsigned char *bytePtr =
      (unsigned char *)ptr; // Treat the struct as a byte array

  printf("==========\nPrinting Struct Bytes: \n");
  for (size_t i = 0; i < size; i++) {
    printf("%02X ", bytePtr[i]); // Print each byte as hexadecimal
  }
  printf("\n==========\n");
}
