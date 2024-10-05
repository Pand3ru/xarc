#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../include/Data.h"
#include "../include/DataTests.h"
#include "../include/Structs.h"

void TestGenerateDataStream() {
  size_t size = 0;
  size_t offset = 0;
  char *byteStream = GenerateDataStream(".", &size, &offset);

  assert(byteStream != NULL);

  size_t currentOffset = 0;

  while (currentOffset < size) {
    FileHeader *header = (FileHeader *)(byteStream + currentOffset);

    assert(header->filename != NULL);
    assert(header->fileOffset > currentOffset);
    currentOffset = header->fileOffset;
  }
  printf("TestGenerateDataStream: passed\n");
}

void printBytesAsHex(const char *byteStream, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (i % 16 == 0) {
    }
    printf("%02b", (unsigned char)byteStream[i]);
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

void TestCleanUpIfExtractionFails(char *destPath) {
  mkdir(destPath, 0755);
  int err = CleanUpIfExtractionFails(destPath);
  if (err != 0) {
    struct stat dest_fileattr;
    assert(stat(destPath, &dest_fileattr) >= 0);
  } else {
    printf("Cleanup failed due to different reasons. Test invalid\n");
  }
  printf("TestCleanUp: Passed\n");
}
void TestRecreateFromDataStream(char *destPath) {
  // Retrieve DataStream
  // Call function
  // Iterate through datasteam
  // Append destPath and check if every file exists, mode and size is
  // equal/maybe skip path all together.
  size_t size = 0;
  size_t offset = 0;
  char *byteStream = GenerateDataStream("..", &size, &offset); // edge case

  char *modDestPath = strdup(destPath);
  if (modDestPath == NULL) {
    perror("Unable to allocate memory for modDestPath");
    return;
  }

  size_t destPathLen = strlen(modDestPath);
  if (modDestPath[destPathLen - 1] == '/') {
    modDestPath[destPathLen - 1] = '\0';
  }

  RecreateFromDataStream(byteStream, destPath, size);

  size_t currentOffset = 0;

  while (currentOffset < size) {
    printf("Reading at: %zu\n", currentOffset);
    FileHeader *header = (FileHeader *)(byteStream + currentOffset);

    assert(header->filename != NULL);

    char *filename = header->filename;
    char *orig_filepath = header->filepath + 2; // should delete the leading
                                                // './'
    size_t fullPathSize =
        strlen(orig_filepath) + strlen(filename) + strlen(destPath);
    char *fullPath = malloc(fullPathSize);
    if (fullPath == NULL) {
      perror("Unable to allocate memory for fullPath");
      return;
    }

    snprintf(fullPath, fullPathSize, "%s%s", modDestPath, orig_filepath);
    printf("Fullpath in test%s\n", fullPath);

    struct stat orig_fileattr;
    assert(stat(header->filepath, &orig_fileattr) >= 0);

    struct stat dest_fileattr;
    assert(stat(header->filepath, &dest_fileattr) >= 0);

    assert(orig_fileattr.st_mode == dest_fileattr.st_mode);
    assert(orig_fileattr.st_size == dest_fileattr.st_size);

    assert(header->fileOffset > currentOffset);
    currentOffset = header->fileOffset;
  }
  printf("TestRecreateFromDataStream: passed\n");
}

void TestCreateDirectories(char *filePath) {
  char *filePath_mutable = strdup(filePath);
  char *res = strrchr(filePath_mutable, '/');
  if (res != NULL && res != filePath_mutable) {
    int loc = res - filePath_mutable;
    filePath_mutable[loc] = '\0';
  }
  if (CreateDirectories(filePath) != -1) {
    struct stat fileattr;
    assert(stat(filePath_mutable, &fileattr) >= 0);
  } else {
    printf("CreateDirectories Failed\n");
    return;
  }
  printf("CreateDirectories: Passed\n");
}

void TestNormalizePath() {
  char *new = normalizePath("/home/panderu/Projects",
                            "../../../../../../../home/panderu/Desktop");
  assert(strcmp(new, "/home/panderu/Projects/Desktop"));
}
