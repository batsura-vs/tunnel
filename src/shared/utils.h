#pragma once
#include <stdexcept>
#include <string>

/**
 * @file utils.h
 * @brief Вспомогательные функции проекта.
 */

/**
 * @brief Выполняет команду в системной оболочке.
 * @param cmd Команда в виде строки с нулевым символом в конце.
 * @return Код завершения команды, возвращенный системной оболочкой.
 * @throws IOException Если команда завершается с ненулевым кодом.
 */
int run(const char *cmd);

/**
 * @brief Возвращает следующий аргумент командной строки.
 * @param argc Количество аргументов командной строки.
 * @param argv Массив аргументов командной строки.
 * @param index Индекс текущего аргумента; при успехе увеличивается на один.
 * @param arg Имя аргумента, для которого читается значение.
 * @return Значение, следующее за текущим аргументом.
 * @throws IOException Если значение отсутствует.
 */
std::string next_arg(int argc, char *argv[], int &index,
                     const std::string &arg);

/**
 * @brief Преобразует строку с номером TCP-порта в целое число.
 * @param value Строковое представление порта.
 * @return Номер порта в диапазоне от 1 до 65535.
 * @throws IOException Если строка не является корректным номером порта.
 */
int parse_port(const std::string &value);

/**
 * @brief Тип исключения для ошибок парсинга.
 */
class ParseException : public std::runtime_error {
public:
  /**
   * @brief Создает исключение с текстом ошибки.
   * @param message Описание ошибки.
   */
  explicit ParseException(const std::string &message)
      : std::runtime_error(message) {};
};

/**
 * @brief Параметры запуска серверной части туннеля.
 *
 * Хранит значения, необходимые серверу для создания TUN-интерфейса,
 * настройки маршрутизации и запуска TCP-сервера.
 */
struct ServerParams {
  /** @brief Имя создаваемого TUN-интерфейса. */
  std::string device{"tun1"};

  /** @brief IP-адрес и маска сети, назначаемые TUN-интерфейсу. */
  std::string ip_network{"10.0.0.1/24"};

  /** @brief Имя физического сетевого интерфейса для выхода в сеть. */
  std::string interface{"wlo1"};

  /** @brief IP-адрес, на котором сервер принимает входящие подключения. */
  std::string listen_on{"0.0.0.0"};

  /** @brief TCP-порт, на котором сервер принимает подключения. */
  int port{5555};

  /**
   * @brief Создает параметры сервера из аргументов командной строки.
   * @param argc Количество аргументов командной строки.
   * @param argv Массив аргументов командной строки.
   * @throws IOException Если передан неизвестный аргумент, отсутствует значение
   *         аргумента или указан некорректный порт.
   */
  ServerParams(int argc, char *argv[]) { from_args(argc, argv); }

private:
  void from_args(int argc, char *argv[]) {
    for (int i{1}; i < argc; i++) {
      std::string arg{argv[i]};
      if (arg == "--tun-name") {
        device = next_arg(argc, argv, i, arg);
      } else if (arg == "--net") {
        ip_network = next_arg(argc, argv, i, arg);
      } else if (arg == "--interface") {
        interface = next_arg(argc, argv, i, arg);
      } else if (arg == "--listen-on") {
        listen_on = next_arg(argc, argv, i, arg);
      } else if (arg == "--port") {
        port = parse_port(next_arg(argc, argv, i, arg));
      } else {
        throw ParseException("Unknown arg: " + arg);
      }
    }
  }
};

/**
 * @brief Параметры запуска клиентской части туннеля.
 *
 * Хранит значения, необходимые клиенту для создания TUN-интерфейса,
 * настройки маршрутов и подключения к TCP-серверу.
 */
struct ClientParams {
  /** @brief Имя создаваемого TUN-интерфейса. */
  std::string device{"tun2"};

  /** @brief IP-адрес и маска сети, назначаемые TUN-интерфейсу. */
  std::string ip_network{"10.0.0.2/24"};

  /** @brief Имя физического сетевого интерфейса для маршрута к серверу. */
  std::string interface{"wlo1"};

  /** @brief IP-адрес шлюза для маршрута к серверу; пустая строка означает
   * маршрут без шлюза. */
  std::string gateway{};

  /** @brief IP-адрес сервера, к которому подключается клиент. */
  std::string server_ip{"127.0.0.1"};

  /** @brief TCP-порт сервера. */
  int port{5555};

  /**
   * @brief Создает параметры клиента из аргументов командной строки.
   * @param argc Количество аргументов командной строки.
   * @param argv Массив аргументов командной строки.
   * @throws IOException Если передан неизвестный аргумент, отсутствует значение
   *         аргумента или указан некорректный порт.
   */
  ClientParams(int argc, char *argv[]) { from_args(argc, argv); }

private:
  void from_args(int argc, char *argv[]) {
    for (int i{1}; i < argc; i++) {
      std::string arg{argv[i]};
      if (arg == "--tun-name") {
        device = next_arg(argc, argv, i, arg);
      } else if (arg == "--net") {
        ip_network = next_arg(argc, argv, i, arg);
      } else if (arg == "--interface") {
        interface = next_arg(argc, argv, i, arg);
      } else if (arg == "--gateway") {
        gateway = next_arg(argc, argv, i, arg);
      } else if (arg == "--server") {
        server_ip = next_arg(argc, argv, i, arg);
      } else if (arg == "--port") {
        port = parse_port(next_arg(argc, argv, i, arg));
      } else {
        throw ParseException("Unknown arg: " + arg);
      }
    }
  }
};
