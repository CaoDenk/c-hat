#include <iostream>
#include "ITestRunner.h"

int main() {
  std::cout << "[Runner] Starting immutable_method tests runner" << std::endl;
  int code = run_immutable_method_tests();
  std::cout << "[Runner] ImmutableMethod tests exit code: " << code << std::endl;
  return code;
}
