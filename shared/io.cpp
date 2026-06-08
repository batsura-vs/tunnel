#include "io.h"
#include <array>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/impl/write.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/detail/error_code.hpp>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <sys/types.h>
#include <unistd.h>

namespace Constants {
inline constexpr uint32_t buffer_size = 2048;
}

std::array<unsigned char, 4> to_array(uint32_t number) {
  std::array<unsigned char, 4> header = {0, 0, 0, 0};
  header[0] = number >> 24;
  header[1] = number >> 16;
  header[2] = number >> 8;
  header[3] = number;
  return header;
}
uint32_t from_array(std::array<unsigned char, 4> buf) {
  return (static_cast<std::uint32_t>(buf[0]) << 24) |
         (static_cast<std::uint32_t>(buf[1]) << 16) |
         (static_cast<std::uint32_t>(buf[2]) << 8) |
         static_cast<std::uint32_t>(buf[3]);
}

struct Packet {
  std::array<unsigned char, 4> header{};
  std::array<char, Constants::buffer_size> payload{};
  std::size_t payload_size{};

  Packet() = default;
  Packet(std::array<char, Constants::buffer_size> payload,
         std::size_t payload_size)
      : payload{payload}, payload_size{payload_size} {
    header = to_array(payload_size);
  }
};

void bind_o(boost::asio::ip::tcp::socket &socket,
            boost::asio::posix::stream_descriptor &tun_stream) {
  auto outgoing_packet = std::make_shared<Packet>();
  tun_stream.async_read_some(
      boost::asio::buffer(outgoing_packet->payload),
      [outgoing_packet, &socket,
       &tun_stream](boost::system::error_code error_code,
                    std::size_t outgoing_bytes_size) {
        if (error_code) {
          throw IOException("tun read failed: " + error_code.message());
        }
        outgoing_packet->payload_size = outgoing_bytes_size;
        outgoing_packet->header = to_array(outgoing_bytes_size);
        std::cout << "Sending: " << outgoing_packet->payload_size << std::endl;
        boost::asio::async_write(
            socket,
            std::array{boost::asio::buffer(outgoing_packet->header),
                       boost::asio::buffer(outgoing_packet->payload.data(),
                                           outgoing_bytes_size)},
            [outgoing_packet, &socket, &tun_stream](auto error_code,
                                                    auto written) {
              if (error_code) {
                throw IOException("socket write failed: " +
                                  error_code.message());
              }
              bind_o(socket, tun_stream);
            });
      });
}
void bind_i(boost::asio::ip::tcp::socket &socket,
            boost::asio::posix::stream_descriptor &tun_stream) {
  auto incoming_packet = std::make_shared<Packet>();
  boost::asio::async_read(
      socket, boost::asio::buffer(incoming_packet->header),
      [incoming_packet, &tun_stream,
       &socket](boost::system::error_code error_code,
                std::size_t incoming_bytes_size) {
        if (error_code) {
          throw IOException("socket header read failed: " +
                            error_code.message());
        }
        incoming_packet->payload_size = from_array(incoming_packet->header);
        if (incoming_packet->payload_size > Constants::buffer_size) {
          throw IOException("packet is too large: " +
                            std::to_string(incoming_packet->payload_size));
        }
        std::cout << "Receiving: " << incoming_packet->payload_size
                  << std::endl;
        boost::asio::async_read(
            socket,
            boost::asio::buffer(incoming_packet->payload,
                                incoming_packet->payload_size),
            [incoming_packet, &tun_stream,
             &socket](boost::system::error_code error_code,
                      std::size_t incoming_bytes_size) {
              if (error_code) {
                throw IOException("socket payload read failed: " +
                                  error_code.message());
              }
              boost::asio::async_write(
                  tun_stream,
                  boost::asio::buffer(incoming_packet->payload,
                                      incoming_packet->payload_size),
                  [incoming_packet, &socket, &tun_stream](auto error_code,
                                                          auto written) {
                    if (error_code) {
                      throw IOException("tun write failed: " +
                                        error_code.message());
                    }
                    bind_i(socket, tun_stream);
                  });
            });
      });
}
