#include "io.h"
#include "tun.h"
#include "utils.h"
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/impl/write.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/detail/error_code.hpp>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  try {
    ServerParams config{argc, argv};
    std::string device = config.device;
    int tun_fd = tun_alloc(device.data());

    if (tun_fd < 0) {
      throw IOException("tun_alloc failed");
    }

    run(("ip addr replace " + config.ip_network + " dev " + config.device)
            .c_str());

    run(("ip link set " + config.device + " up").c_str());

    run(("iptables -t nat -A POSTROUTING -s " + config.ip_network + " -o " +
         config.interface + " -j MASQUERADE")
            .c_str());

    run(("iptables -A FORWARD -i " + config.device + " -o " + config.interface +
         " -j ACCEPT")
            .c_str());

    run(("iptables -A FORWARD -i " + config.interface + " -o " + config.device +
         " -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT")
            .c_str());

    boost::asio::io_context io_context;

    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::make_address(config.listen_on), config.port);
    boost::asio::ip::tcp::acceptor acceptor(io_context, endpoint);

    boost::asio::ip::tcp::socket socket(io_context);

    acceptor.accept(socket);
    std::cout << "client connected\n";
    boost::asio::posix::stream_descriptor tun_stream{io_context, tun_fd};
    bind_i(socket, tun_stream);
    bind_o(socket, tun_stream);
    io_context.run();
  } catch (const std::exception &e) {
    std::cerr << "server error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
