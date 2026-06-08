#include "io.h"
#include "tun.h"
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/impl/write.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/detail/error_code.hpp>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

int run(const char *cmd) {
  int rc = system(cmd);
  if (rc != 0) {
    std::cerr << "failed: " << cmd << "\n";
  }
  return rc;
}

int main(int argc, char *argv[]) {
  char device[] = "tun2";
  int tun_fd = tun_alloc(device);
  if (tun_fd < 0) {
    std::cerr << "tun_alloc failed\n";
    return 1;
  }
  std::cout << "tun allocated" << std::endl;

  run("ip addr replace 10.8.0.2 peer 10.8.0.1 dev tun2");
  run("ip link set tun2 up");
  run("ip route replace 0.0.0.0/1 dev tun2");
  run("ip route replace 128.0.0.0/1 dev tun2");

  try {
    boost::asio::io_context io_context;

    using boost::asio::ip::tcp;

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);

    tcp::socket socket(io_context);
    boost::asio::connect(socket, endpoints);

    std::cout << "connected to server\n";

    boost::asio::posix::stream_descriptor tun_stream{io_context, tun_fd};
    bind_i(socket, tun_stream);
    bind_o(socket, tun_stream);
    io_context.run();
  } catch (const std::exception &e) {
    std::cerr << "client error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
