#include <fcntl.h>
#include <unistd.h>

#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/impl/write.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/detail/error_code.hpp>
#include <iostream>
#include <string>

#include "io.h"
#include "tun.h"
#include "utils.h"

int main(int argc, char* argv[]) {
  try {
    ClientParams config{argc, argv};

    std::string device = config.device;
    int tun_fd = tun_alloc(device.data(), config.tun_device.c_str());
    if (tun_fd < 0) {
      throw IOException("tun_alloc failed: " + config.tun_device);
    }
    std::cout << "tun allocated" << std::endl;

    run(("ip addr replace " + config.ip_network + " dev " + config.device)
            .c_str());
    if (config.gateway.empty()) {
      run(("ip route replace " + config.server_ip + "/32 dev " +
           config.interface)
              .c_str());
    } else {
      run(("ip route replace " + config.server_ip + "/32 via " +
           config.gateway + " dev " + config.interface)
              .c_str());
    }
    run(("ip link set " + config.device + " up").c_str());
    run(("ip route replace 0.0.0.0/1 dev " + config.device).c_str());
    run(("ip route replace 128.0.0.0/1 dev " + config.device).c_str());

    boost::asio::io_context io_context;

    using boost::asio::ip::tcp;

    tcp::resolver resolver(io_context);
    auto endpoints =
        resolver.resolve(config.server_ip, std::to_string(config.port));

    tcp::socket socket(io_context);
    boost::asio::connect(socket, endpoints);

    std::cout << "connected to server\n";

    boost::asio::posix::stream_descriptor tun_stream{io_context, tun_fd};
    bind_i(socket, tun_stream);
    bind_o(socket, tun_stream);
    io_context.run();
  } catch (const std::exception& e) {
    std::cerr << "client error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
