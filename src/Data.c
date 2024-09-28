#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../include/Structs.h"

// Use NULL to check for errors in when using this function
char *GenerateDataStream(char *path, size_t *totalsize) {
  struct dirent **directoryEntries;
  printf("%s\n", path);
  int directoryEntriesAmount =
      scandir(path, &directoryEntries, NULL, alphasort);
  if (directoryEntriesAmount < 0) {
    perror("scandir");
    return NULL;
  }

  char *byteStream = NULL;
  size_t byteStreamSize = *totalsize;

  for (int i = 0; i < directoryEntriesAmount; i++) {
    char *filename = directoryEntries[i]->d_name;

    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
      free(directoryEntries[i]);
      continue;
    }

    struct stat fileattr;

    size_t fullPathSize = strlen(path) + strlen(filename) + 2;
    char *fullPath = malloc(fullPathSize);
    if (fullPath == NULL) {
      perror("Unable to allocate memory for fullPath");
      free(directoryEntries);
      return NULL;
    }
    snprintf(fullPath, fullPathSize, "%s/%s", path, filename);

    if (stat(fullPath, &fileattr) < 0) {
      perror("stat");
      free(fullPath);
      free(directoryEntries);
      return NULL;
    }

    printf("--------------------------------\n");
    printf("Processing: %s\n", fullPath);

    size_t pathLength = strlen(fullPath) + 1;
    FileHeader *header = malloc(sizeof(FileHeader) + pathLength);
    if (header == NULL) {
      free(fullPath);
      perror("Unable to allocate memory for header");
      free(directoryEntries);
      return NULL;
    }

    if (S_ISDIR(fileattr.st_mode)) {
      size_t subStreamSize = byteStreamSize;
      printf("Recursion call for: %s\nCurrent ByteStreamSize: %zu\n", fullPath,
             byteStreamSize);
      char *subStream = GenerateDataStream(fullPath, &subStreamSize);

      if (subStream) {
        printf("===Bytes detected. Writing into buffer===\n");
        char *newByteStream =
            realloc(byteStream, byteStreamSize + subStreamSize);
        if (newByteStream == NULL) {
          perror("Reallocation failed");
          free(byteStream);
          free(directoryEntries);
          free(fullPath);
          return NULL;
        }
        byteStream = newByteStream;

        memcpy(byteStream + byteStreamSize, subStream, subStreamSize);
        printf("%zu byteStreamSize %zu subStreamSize\n", byteStreamSize,
               subStreamSize);
        byteStreamSize = subStreamSize;
        printf("ByteStream update to %zu\n", byteStreamSize);
        free(subStream);
      }
    } else {
      printf("File call for: %s\n", fullPath);
      FILE *file = fopen(fullPath, "rb");
      if (!file) {
        perror("fopen");
        free(directoryEntries);
        free(fullPath);
        return NULL;
      }

      fseek(file, 0, SEEK_END);
      size_t fileSize = ftell(file);
      fseek(file, 0, SEEK_SET);

      size_t pathLength = strlen(fullPath) + 1;
      FileHeader *header = malloc(sizeof(FileHeader) + pathLength);
      if (header == NULL) {
        perror("Unable to allocate memory for header");
        fclose(file);
        free(fullPath);
        free(directoryEntries);
        return NULL;
      }

      header->mode = fileattr.st_mode;
      strcpy(header->filename, filename);
      strcpy(header->filepath, fullPath);
      header->headerSize = sizeof(FileHeader) + pathLength;
      header->fileOffset = byteStreamSize + header->headerSize + fileSize;

      printf("Mode: %o, Filename: %s, Filepath: %s, Header Size: %i, File "
             "Offset: %i\n",
             header->mode, header->filename, header->filepath,
             header->headerSize, header->fileOffset);

      printf("byteStreamSize: %zu Header Size: %i\n", byteStreamSize,
             header->headerSize);
      char *newByteStream =
          realloc(byteStream, byteStreamSize + header->headerSize);
      if (newByteStream == NULL) {
        perror("Reallocation failed");
        free(byteStream);
        free(header);
        free(fullPath);
        fclose(file);
        free(directoryEntries);
        return NULL;
      }
      byteStream = newByteStream;

      memcpy(byteStream + byteStreamSize, header, header->headerSize);
      byteStreamSize += header->headerSize;
      printf("ByteStream update to %zu\n", byteStreamSize);

      free(header);

      char *fileContent = malloc(fileSize);
      if (!fileContent) {
        perror("Unable to allocate memory for file content");
        fclose(file);
        free(fullPath);
        free(directoryEntries);
        return NULL;
      }

      fread(fileContent, 1, fileSize, file);
      fclose(file);

      char *newByteFileStream = realloc(byteStream, byteStreamSize + fileSize);
      if (newByteFileStream == NULL) {
        perror("Reallocation failed");
        free(byteStream);
        free(fileContent);
        free(fullPath);
        free(directoryEntries);
        return NULL;
      }
      byteStream = newByteFileStream;

      memcpy(byteStream + byteStreamSize, fileContent, fileSize);
      byteStreamSize += fileSize;
      printf("ByteStream update to %zu\n", byteStreamSize);

      free(fileContent);
    }
    free(fullPath);
    free(directoryEntries[i]);
    *totalsize = byteStreamSize;
    printf("Total size was expanded to: %zu\n", *totalsize);
  }

  free(directoryEntries);
  return byteStream;
}
