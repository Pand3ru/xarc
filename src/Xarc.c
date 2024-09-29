#include "../include/DataTests.h"

int main() {
  TestGenerateDataStream();
  // TestRecreateFromDataStream("~/testdir/");
  TestCleanUpIfExtractionFails("~/testdir/");
  return 0;
}
