#include "io.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/impl/write.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/exception/to_string.hpp>
#include <boost/system/detail/error_code.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

namespace Constants {
inline constexpr uint32_t buffer_size = 2048;
}

std::array<unsigned char, 4> header_to_array(uint32_t number) {
  std::array<unsigned char, 4> header = {0, 0, 0, 0};
  header[0] = number >> 24;
  header[1] = number >> 16;
  header[2] = number >> 8;
  header[3] = number;
  return header;
}
std::uint32_t header_from_array(std::array<unsigned char, 4> buf) {
  return (static_cast<std::uint32_t>(buf[0]) << 24) |
         (static_cast<std::uint32_t>(buf[1]) << 16) |
         (static_cast<std::uint32_t>(buf[2]) << 8) |
         static_cast<std::uint32_t>(buf[3]);
}

/**
 * @brief Сетевой пакет с заголовком размера.
 *
 * Используется при обмене данными между TUN-интерфейсом и TCP-сокетом.
 */
struct Packet {
  /** @brief Четырёхбайтовый заголовок с размером payload */
  std::array<unsigned char, 4> header{};

  /** @brief Буфер с полезной нагрузкой пакета (IP-пакет из TUN). */
  std::array<unsigned char, Constants::buffer_size> payload{};

  /** @brief Фактический размер полезной нагрузки в байтах. */
  std::size_t payload_size{};

  /**
   * @brief Создаёт пустой пакет.
   */
  Packet() = default;

  /**
   * @brief Создаёт пакет из заданной полезной нагрузки и формирует заголовок
   * размера.
   * @param payload Буфер с полезной нагрузкой пакета.
   * @param payload_size Размер полезной нагрузки в байтах.
   */
  Packet(std::array<unsigned char, Constants::buffer_size> payload,
         std::size_t payload_size)
      : payload{payload}, payload_size{payload_size} {
    header = header_to_array(payload_size);
  }
};

/**
 * @brief Преобразует четыре байта в строку IPv4-адреса.
 * @param start Указатель на первый из четырёх байтов адреса.
 * @return Строковое представление адреса в виде "a.b.c.d".
 */
std::string parse_ip_v4(unsigned char* start) {
  return std::to_string(start[0]) + '.' + std::to_string(start[1]) + '.' +
         std::to_string(start[2]) + '.' + std::to_string(start[3]);
}

/**
 * @brief Преобразует шестнадцать байтов в строку IPv6-адреса.
 * @param start Указатель на первый из шестнадцати байтов адреса.
 * @return Строковое представление адреса либо "<invalid ipv6>" при ошибке.
 */
std::string parse_ip_v6(unsigned char* start) {
  char buf[INET6_ADDRSTRLEN];

  if (inet_ntop(AF_INET6, start, buf, sizeof(buf)) == nullptr) {
    return "<invalid ipv6>";
  }

  return std::string(buf);
}

/**
 * @brief Выводит в стандартный поток информацию о маршруте пакета.
 * @param packet Указатель на пакет, для которого выводится IP-версия и маршрут.
 */
void log_packet(std::shared_ptr<Packet> packet) {
  std::uint32_t ip_version = packet->payload[0] >> 4;
  std::cout << "Packet ip:" << ip_version << "; ";
  std::string src_ip;
  std::string dst_ip;
  if (ip_version == 4) {
    src_ip = parse_ip_v4(&packet->payload[12]);
    dst_ip = parse_ip_v4(&packet->payload[16]);
  } else {
    src_ip = parse_ip_v6(&packet->payload[8]);
    dst_ip = parse_ip_v6(&packet->payload[24]);
  }
  std::cout << "route " << src_ip << " -> " << dst_ip << "; ";
  std::cout << std::endl;
}

void bind_o(boost::asio::ip::tcp::socket& socket,
            boost::asio::posix::stream_descriptor& tun_stream) {
  auto outgoing_packet = std::make_shared<Packet>();
  tun_stream.async_read_some(
      boost::asio::buffer(outgoing_packet->payload),
      [outgoing_packet, &socket, &tun_stream](
          boost::system::error_code error_code,
          std::size_t outgoing_bytes_size) {
        if (error_code) {
          throw IOException("tun read failed: " + error_code.message());
        }
        outgoing_packet->payload_size = outgoing_bytes_size;
        outgoing_packet->header = header_to_array(outgoing_bytes_size);
        log_packet(outgoing_packet);
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
void bind_i(boost::asio::ip::tcp::socket& socket,
            boost::asio::posix::stream_descriptor& tun_stream) {
  auto incoming_packet = std::make_shared<Packet>();
  boost::asio::async_read(
      socket, boost::asio::buffer(incoming_packet->header),
      [incoming_packet, &tun_stream, &socket](
          boost::system::error_code error_code,
          std::size_t incoming_bytes_size) {
        if (error_code) {
          throw IOException("socket header read failed: " +
                            error_code.message());
        }
        incoming_packet->payload_size =
            header_from_array(incoming_packet->header);
        if (incoming_packet->payload_size > Constants::buffer_size) {
          throw IOException("packet is too large: " +
                            std::to_string(incoming_packet->payload_size));
        }
        boost::asio::async_read(
            socket,
            boost::asio::buffer(incoming_packet->payload,
                                incoming_packet->payload_size),
            [incoming_packet, &tun_stream, &socket](
                boost::system::error_code error_code,
                std::size_t incoming_bytes_size) {
              if (error_code) {
                throw IOException("socket payload read failed: " +
                                  error_code.message());
              }
              log_packet(incoming_packet);
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
