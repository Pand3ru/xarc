#include "../include/Data.h"
#include <stdio.h>
#include <stdlib.h>

// Declare your GenerateDataStream function and necessary includes here
char *GenerateDataStream(char *path, size_t *totalsize);

int main() {
  size_t totalSize; // Declare a variable to hold the total size
  char *dataStream = GenerateDataStream(".", &totalSize); // Call the function

  if (dataStream != NULL) {
    // Successfully received a byte stream; print the total size
    printf("Total size of the data stream: %zu bytes\n", totalSize);

    // Print the entire data stream in hexadecimal format
    for (size_t i = 0; i < totalSize; i++) {
      // printf("%02X ", (unsigned char)dataStream[i]);
    }
    printf("\n");

    free(dataStream); // Free the allocated byte stream
  } else {
    // Handle error if the function returns NULL
    printf("Failed to generate data stream.\n");
  }

  return 0;
}
