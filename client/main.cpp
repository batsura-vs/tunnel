#include "io.h"
#include "tun.h"
#include "utils.h"
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
#include <string>
#include <unistd.h>

struct ClientParams {
  std::string device{"tun2"};
  std::string ip_network{"10.0.0.2/24"};
  std::string interface{"wlo1"};
  std::string gateway{};
  std::string server_ip{"127.0.0.1"};
  int port{5555};

  ClientParams(int argc, char *argv[]) { from_args(argc, argv); }

private:
  void from_args(int argc, char *argv[]) {
    for (int i{1}; i < argc; i++) {
      std::string arg{argv[i]};
      if (arg == "--tun-name") {
        device = argv[++i];
      } else if (arg == "--net") {
        ip_network = argv[++i];
      } else if (arg == "--interface") {
        interface = argv[++i];
      } else if (arg == "--gateway") {
        gateway = argv[++i];
      } else if (arg == "--server") {
        server_ip = argv[++i];
      } else if (arg == "--port") {
        port = std::stoi(argv[++i]);
      } else {
        std::cerr << "Unknown arg: " << arg << std::endl;
      }
    }
  }
};

int main(int argc, char *argv[]) {
  ClientParams config{argc, argv};

  std::string device = config.device;
  int tun_fd = tun_alloc(device.data());
  if (tun_fd < 0) {
    std::cerr << "tun_alloc failed\n";
    return 1;
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

  try {
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
  } catch (const std::exception &e) {
    std::cerr << "client error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
