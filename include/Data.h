#ifndef DATA_H
#define DATA_H

#include "./Structs.h"
#include <stdio.h>
char *GenerateDataStream(char *path, size_t *totalSize, size_t *i);
void RecreateFromDataStream(char *byteStream, char *destPath,
                            size_t byteStreamSize);
int CreateDirectories(const char *path);
int CleanUpIfExtractionFails(char *destPath);
int CreateFileAndWriteContent(FileHeader *header, char *byteStream,
                              size_t offset, size_t byteStreamSize,
                              char *fullPath);
char *normalizePath(const char *basePath, const char *relativePath);

#endif
