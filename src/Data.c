#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../include/Structs.h"

// Use NULL to check for errors in when using this function
char *GenerateDataStream(char *path, size_t *totalsize) {
  struct dirent **directoryEntries;
  int directoryEntriesAmount =
      scandir(path, &directoryEntries, NULL, alphasort);
  if (directoryEntriesAmount < 0) {
    perror("scandir");
    return NULL;
  }

  char *byteStream = NULL;
  size_t byteStreamSize = 0;

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
      size_t subStreamSize = 0;
      char *subStream = GenerateDataStream(fullPath, &subStreamSize);

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

        memcpy(byteStream + byteStreamSize, subStream, subStreamSize);
        byteStreamSize += subStreamSize;
        free(subStream);
      }
    } else {
      header->mode = fileattr.st_mode;
      strcpy(header->filename, filename);
      strcpy(header->filepath, fullPath);
      header->headerSize = sizeof(FileHeader) + pathLength;

      char *newByteStream =
          realloc(byteStream, byteStreamSize + header->headerSize);
      if (newByteStream == NULL) {
        perror("Reallocation failed");
        free(byteStream);
        free(directoryEntries);
        free(header);
        free(fullPath);
        return NULL;
      }
      byteStream = newByteStream;

      memcpy(byteStream + byteStreamSize, header, header->headerSize);
      byteStreamSize += header->headerSize;
      free(header);

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
      char *fileContent = malloc(fileSize);
      if (!fileContent) {
        perror("Unable to allocate memory for file content");
        fclose(file);
        free(directoryEntries);
        free(fullPath);
        return NULL;
      }
      fread(fileContent, 1, fileSize, file);
      fclose(file);

      char *newByteFileStream = realloc(byteStream, byteStreamSize + fileSize);
      if (newByteFileStream == NULL) {
        perror("Reallocation failed");
        free(byteStream);
        free(directoryEntries);
        free(fileContent);
        free(fullPath);
        return NULL;
      }
      byteStream = newByteFileStream;
      memcpy(byteStream + byteStreamSize, fileContent, fileSize);
      byteStreamSize += fileSize;
      free(fileContent);
    }
    free(fullPath);
    free(directoryEntries[i]);
  }

  free(directoryEntries);
  *totalsize = byteStreamSize;
  return byteStream;
}

