
#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>

typedef struct {
  uint32_t headerSize;
  uint32_t mode;
  uint32_t fileOffset;
  char filename[255 * sizeof(char)];
  char filepath[];
} FileHeader;

#endif
