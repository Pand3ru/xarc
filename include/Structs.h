#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>

typedef struct {
  uint32_t header_size;
  uint32_t mode;
  uint64_t file_offset;
  char file_name[255 * sizeof(char)];
  char file_path[];
} FileHeader;

#endif
