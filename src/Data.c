#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../include/Structs.h"

// Use NULL to check for errors in when using this function
char *GenerateDataStream(char *path, size_t *totalsize,
                         size_t *byteStreamSizeOffset) {
  struct dirent **directoryEntries;
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

    size_t pathLength = strlen(fullPath) + 1;
    FileHeader *header = malloc(sizeof(FileHeader) + pathLength);
    if (header == NULL) {
      free(fullPath);
      perror("Unable to allocate memory for header");
      free(directoryEntries);
      return NULL;
    }

    if (S_ISDIR(fileattr.st_mode)) {
      size_t subStreamSize = 0;
      size_t offset = byteStreamSize + *byteStreamSizeOffset;
      char *subStream = GenerateDataStream(fullPath, &subStreamSize, &offset);

      if (subStream) {
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

        memmove(byteStream + byteStreamSize, subStream, subStreamSize);
        byteStreamSize += subStreamSize;
        free(subStream);
      }
    } else {
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

      header->mode = fileattr.st_mode & 0x0FFF;
      strcpy(header->filename, filename);
      strcpy(header->filepath, fullPath);
      header->headerSize = sizeof(FileHeader) + pathLength;
      header->fileOffset = byteStreamSize + header->headerSize + fileSize +
                           *byteStreamSizeOffset;

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
      memmove(byteStream + byteStreamSize, header, header->headerSize);
      printf("Writing header to %zu\n", byteStreamSize);
      byteStreamSize += header->headerSize;

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

      memmove(byteStream + byteStreamSize, fileContent, fileSize);
      byteStreamSize += fileSize;

      free(fileContent);
    }
    free(fullPath);
    free(directoryEntries[i]);
    *totalsize = byteStreamSize;
  }

  free(directoryEntries);
  return byteStream;
}

// Should probably return a sha256 of the entire directory so
void RecreateFromDataStream(char *byteStream, char *destPath) {}
