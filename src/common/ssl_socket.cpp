#include "common/ssl_socket.hpp"
#include <sstream>

ssl_socket::ssl_socket(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> *socket, boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::handshake_type type): socket(socket), buf(), pos(0)
{
	socket->lowest_layer().set_option(boost::asio::socket_base::keep_alive(true));
	socket->lowest_layer().set_option(boost::asio::ip::tcp::no_delay(true));
	socket->handshake(type);
}

ssl_socket::~ssl_socket()
{
	delete socket;
}

ssl_socket& ssl_socket::operator>>(std::string& what)
{
	char c[1024];
	int len = socket->read_some(boost::asio::buffer(c));
	what = std::string(c, len);
	return *this;
}

ssl_socket& ssl_socket::getline(std::string& what)
{
	std::stringstream str;
	while(true)
	{
		if(pos >= buf.size())
		{
			*this >> buf;
			pos = 0;
		}
		str << buf[pos++];
		if(buf[pos++] == '\n')
			break;
	}
	what = str.str();
	return *this;
}
