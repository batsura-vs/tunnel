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

std::string next_arg(int argc, char *argv[], int &index,
                     const std::string &arg) {
  if (index + 1 >= argc) {
    throw IOException("Missing value for " + arg);
  }
  return argv[++index];
}

int parse_port(const std::string &value) {
  std::size_t parsed_chars{};
  int port{};
  try {
    port = std::stoi(value, &parsed_chars);
  } catch (const std::exception &) {
    throw IOException("Invalid port: " + value);
  }
  if (parsed_chars != value.size() || port < 1 || port > 65535) {
    throw IOException("Invalid port: " + value);
  }
  return port;
}
