#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
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
  mode_t mode = 0755;

  snprintf(tmp, sizeof(tmp), "%s", path);

  p = strrchr(tmp, '/');
  if (p != NULL) {
    *p = 0;
  } else {
    return -1;
  }

  for (p = tmp + 1; *p; p++) {
    printf("%s\n", p);
    if (*p == '/') {
      *p = 0;
      printf("if: %s\n", tmp);
      mkdir(tmp, mode);
      *p = '/';
    }
  }
  printf("%s\n", tmp);
  mkdir(tmp, mode);
  DIR *dir = opendir(tmp);
  if (dir) {
    closedir(dir);
    return 0;
  } else {
    return -1;
  }
}

int CleanUpIfExtractionFails(char *destPath) {
  struct dirent **directoryEntries;
  int directoryEntriesAmount =
      scandir(destPath, &directoryEntries, NULL, alphasort);
  if (directoryEntriesAmount < 0) {
    printf("error on cleanup: %s\n", destPath);
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
  struct stat fileattr;
  if (stat(destPath, &fileattr) < 0) {
    return 0;
  }
  if (S_ISDIR(fileattr.st_mode)) {
    rmdir(destPath);
  } else {
    remove(destPath);
  }
  return 1;
}

int CreateFileAndWriteContent(FileHeader *header, char *byteStream,
                              size_t offset, size_t byteStreamSize,
                              char *fullPath) {
  int file = open(fullPath, O_WRONLY | O_CREAT | O_TRUNC, header->mode);
  if (file == -1) {
    perror("File creation");
    return -1;
  }
  ssize_t bytesWritten =
      write(file, byteStream + offset + header->headerSize,
            byteStreamSize - (byteStreamSize - header->fileOffset) - offset -
                header->headerSize); /* ;.................;
                                           o hS    fO     bSs */
  printf("created %s\n", fullPath);
  if (bytesWritten == -1) {
    perror("Error writing to file");
    return -1;
  }
  printf("Wrote content for: %s\n", fullPath);
  return 1;
}

char *normalizePath(char *basePath, char *relativePath) {
  char *normalizedPath = malloc(strlen(basePath) + strlen(relativePath) + 2);
  if (normalizedPath == NULL) {
    perror("malloc");
    return NULL;
  }
  char *relativePath_mutable = strdup(relativePath);
  char *basePath_mutable = strdup(basePath);

  strcpy(normalizedPath, basePath_mutable);
  printf("BasePath: %s\nRelativePath: %s\n", basePath_mutable,
         relativePath_mutable);

  char *token = strtok(relativePath_mutable, "/");
  while (token) {
    if (strcmp(token, ".") == 0) {
      token = strtok(NULL, "/");
      continue;
    } else if (strcmp(token, "..") == 0) {
      char *lastSlash = strrchr(normalizedPath, '/');
      if (lastSlash) {
        *lastSlash = '\0';
      }
      token = strtok(NULL, "/");
      continue;
    }

    strcat(normalizedPath, "/");
    strcat(normalizedPath, token);
    token = strtok(NULL, "/");
  }

  printf("normalizedPath: %s\n", normalizedPath);
  return normalizedPath;
}

// DestPath should end on a trailing /
int RecreateFromDataStream(char *byteStream, char *destPath,
                           size_t byteStreamSize) {
  struct stat fileattr;
  if (stat(destPath, &fileattr) < 0) {
    return -1;
  }
  // loop over bytestreamsize
  // serialize header
  // create file with metadata retrieved from header
  // offset += headersize
  // write from offset to fileoffset into file? Idk if I need to loop there
  // offset = fileoffset
  //
  // /home/panderu/folder
  // ../../xarc/sdaf/asdf/sadf

  size_t current_byte = 0;

  while (current_byte < byteStreamSize) {
    printf("-------------------------\n");
    FileHeader *header = (FileHeader *)(byteStream + current_byte);
    char *fullPath = normalizePath(destPath, header->filepath);
    if (fullPath == NULL) {
      return -1;
    }
    printf("Creating directory for %s\n", fullPath);
    assert(CreateDirectories(fullPath) != -1);
    assert(CreateFileAndWriteContent(header, byteStream, current_byte,
                                     byteStreamSize, fullPath) != -1);
    current_byte = header->fileOffset;
  }
  return 1;
}
