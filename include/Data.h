#ifndef DATA_H
#define DATA_H

#include <stdio.h>
char *GenerateDataStream(char *path, size_t *totalSize, size_t *i);
void RecreateFromDataStream(char *byteStream, char *destPath,
                            size_t byteStreamSize);
int CreateDirectories(const char *path);
int CleanUpIfExtractionFails(char *destPath);

#endif
