#ifndef DATA_H
#define DATA_H

#include <stdio.h>
char *GenerateDataStream(char *path, size_t *totalSize, size_t *i);
void RecreateFromDataStream(char *byteStream, char *destPath);

#endif
