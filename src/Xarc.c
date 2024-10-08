#include "../include/DataTests.h"

int main() {
  // TestNormalizePath();
  TestGenerateDataStream();
  //  TestCleanUpIfExtractionFails("/home/panderu/tests/testcleanup");
  // TestCreateDirectories("/home/panderu/test/test2/test3/sdf");
  TestRecreateFromDataStream("/home/panderu/testdir/");
  return 0;
}
