#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

int CreateDirectories(const char *path) {
  char tmp[256];
  char *p = NULL;
  size_t len;
  mode_t mode = 0755;

  snprintf(tmp, sizeof(tmp), "%s", path);
  len = strlen(tmp);

  p = strrchr(tmp, '/');
  if (p != NULL) {
    *p = 0;
  } else {
    return 0;
  }

  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      mkdir(tmp, mode);
      *p = '/';
    }
  }
  return mkdir(tmp, mode);
}

int CleanUpIfExtractionFails(char *destPath) {
  struct dirent **directoryEntries;
  int directoryEntriesAmount =
      scandir(destPath, &directoryEntries, NULL, alphasort);
  if (directoryEntriesAmount < 0) {
    printf("%s\n", destPath);
    perror("scandir");
    return 0;
  }

  for (int i = 0; i < directoryEntriesAmount; i++) {
    char *filename = directoryEntries[i]->d_name;

    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
      free(directoryEntries[i]);
      continue;
    }

    size_t fullPathSize = strlen(destPath) + strlen(filename) + 2;
    char *fullPath = malloc(fullPathSize);
    if (fullPath == NULL) {
      perror("Unable to allocate memory for fullPath");
      free(directoryEntries);
      return 0;
    }
    snprintf(fullPath, fullPathSize, "%s/%s", destPath, filename);

    struct stat fileattr;

    if (stat(fullPath, &fileattr) < 0) {
      perror("stat");
      free(fullPath);
      free(directoryEntries);
      return 0;
    }

    if (S_ISDIR(fileattr.st_mode)) {
      CleanUpIfExtractionFails(fullPath);
      rmdir(fullPath);
    } else {
      remove(fullPath);
    }
  }
  return 1;
}

// Should probably return a sha256 of the entire directory so
void RecreateFromDataStream(char *byteStream, char *destPath,
                            size_t byteStreamSize) {
  // loop over bytestreamsize
  // serialize header
  // create file with metadata retrieved from header
  // offset += headersize
  // write from offset to fileoffset into file? Idk if I need to loop there
  // offset = fileoffset

  size_t current_byte = 0;

  while (current_byte < byteStreamSize) {
    FileHeader *header = (FileHeader *)(byteStreamSize + byteStreamSize);
    if (CreateDirectories(header->filepath) == 0) {
      printf("Error creating directory for %s\n", header->filename);
      continue;
    }
  }
}
