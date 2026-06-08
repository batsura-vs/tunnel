#pragma once

/**
 * @file io.h
 * @brief Функции для обмена пакетами между TCP-сокетом и TUN-интерфейсом.
 */

#include <boost/asio.hpp>
#include <stdexcept>
#include <string>

/**
 * @brief Запускает асинхронную передачу пакетов из TUN-интерфейса в TCP-сокет.
 * @param socket TCP-сокет, в который отправляются данные пакетов.
 * @param tun_stream Потоковый дескриптор, связанный с TUN-интерфейсом.
 * @throws IOException Если чтение из TUN или запись в сокет завершается ошибкой.
 */
void bind_o(boost::asio::ip::tcp::socket &socket,
            boost::asio::posix::stream_descriptor &tun_stream);

/**
 * @brief Запускает асинхронную передачу пакетов из TCP-сокета в TUN-интерфейс.
 * @param socket TCP-сокет, из которого принимаются данные пакетов.
 * @param tun_stream Потоковый дескриптор, связанный с TUN-интерфейсом.
 * @throws IOException Если чтение из сокета, проверка размера пакета или запись в TUN завершается ошибкой.
 */
void bind_i(boost::asio::ip::tcp::socket &socket,
            boost::asio::posix::stream_descriptor &tun_stream);

/**
 * @brief Тип исключения для ошибок ввода-вывода в туннеле.
 */
class IOException : public std::runtime_error {
public:
  /**
   * @brief Создает исключение с текстом ошибки.
   * @param message Описание ошибки.
   */
  explicit IOException(const std::string &message) : std::runtime_error(message) {};
};
