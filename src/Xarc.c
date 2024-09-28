#include "../include/Data.h"
#include "../include/TestGenerateDataStream.h"
#include <stdio.h>

int main() {
  TestGenerateDataStream();
  size_t size = 0;
  char *stream = GenerateDataStream(".", &size, 0);
  printBytesAsHex(stream, size);
  return 0;
}
