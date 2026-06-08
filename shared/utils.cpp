#include "io.h"

#include <cstdlib>
#include <string>

int run(const char *cmd) {
  int rc = system(cmd);
  if (rc != 0) {
    throw IOException("failed: " + std::string(cmd));
  }
  return rc;
}
