#include <boost/asio.hpp>
#include <stdexcept>
#include <string>

void bind_o(boost::asio::ip::tcp::socket &socket,
            boost::asio::posix::stream_descriptor &tun_stream);

void bind_i(boost::asio::ip::tcp::socket &socket,
            boost::asio::posix::stream_descriptor &tun_stream);

class IOException : public std::runtime_error {
public:
  explicit IOException(const std::string &message) : std::runtime_error(message) {};
};
