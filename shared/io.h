#include <boost/asio.hpp>

void bind_o(boost::asio::ip::tcp::socket &socket,
            boost::asio::posix::stream_descriptor &tun_stream);

void bind_i(boost::asio::ip::tcp::socket &socket,
            boost::asio::posix::stream_descriptor &tun_stream);
