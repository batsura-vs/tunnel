#include <cstdlib>
#include <iostream>

int run(const char *cmd) {
  int rc = system(cmd);
  if (rc != 0) {
    std::cerr << "failed: " << cmd << "\n";
  }
  return rc;
}

