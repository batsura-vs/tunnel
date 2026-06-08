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
#include <sys/types.h>
#include <unistd.h>

int run(const char *cmd) {
  int rc = system(cmd);
  if (rc != 0) {
    std::cerr << "failed: " << cmd << "\n";
  }
  return rc;
}

int main() {
  char device[] = "tun1";
  int tun_fd = tun_alloc(device);

  if (tun_fd < 0) {
    std::cerr << "tun_alloc failed\n";
    return 1;
  }

  run("iptables -t nat -A POSTROUTING -s 10.8.0.2/32 -o wlo1 -j MASQUERADE");
  run("iptables -A FORWARD -i tun1 -o wlo1 -j ACCEPT");
  run("iptables -A FORWARD -i wlo1 -o tun1 -m conntrack --ctstate "
      "RELATED,ESTABLISHED -j ACCEPT");

  try {
    boost::asio::io_context io_context;

    using boost::asio::ip::tcp;

    tcp::endpoint endpoint(tcp::v4(), 5555);
    tcp::acceptor acceptor(io_context, endpoint);

    tcp::socket socket(io_context);

    acceptor.accept(socket);
    std::cout << "client connected\n";
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
