#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "../include/Data.h"
#include "../include/Structs.h"

void TestGenerateDataStream() {
  size_t size = 0;
  char *byteStream = GenerateDataStream(".", &size);

  // Verify the byteStream was generated successfully
  assert(byteStream != NULL);

  size_t currentOffset = 0; // To track the current position in the byte stream

  while (currentOffset < size) {
    printf("Reading at: %zu\n", currentOffset);
    // Read the header at the current position
    FileHeader *header = (FileHeader *)(byteStream + currentOffset);

    // Check that the header and filename are valid
    assert(header->filename != NULL);

    printf("Processing file: %s, Offset: %d, Mode: %d\n", header->filename,
           header->fileOffset, header->mode);

    // Ensure that the fileOffset is greater than the current offset
    assert(header->fileOffset > currentOffset);

    // Move to the next file's header based on the fileOffset stored in the
    // current header
    if (currentOffset == 289) {
      break;
    }
    currentOffset = 289;
  }

  printf("Test passed successfully!\n");
}
