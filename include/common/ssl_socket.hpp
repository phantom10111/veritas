#ifndef SSL_SOCKET_HPP
#define SSL_SOCKET_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>

class ssl_socket
{
public:
	ssl_socket(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> *, boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::handshake_type);
	~ssl_socket();

	ssl_socket& operator>>(std::string&);

	template<class T>
	ssl_socket& operator<<(T& what)
	{
		write(*socket, boost::asio::buffer(what, sizeof(what)));
		return *this;
	}

	ssl_socket& getline(std::string&);

private:
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> *socket;
	std::string buf;
	int pos;
};


#endif
