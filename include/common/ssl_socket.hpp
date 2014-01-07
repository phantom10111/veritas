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

	ssl_socket& write(std::string const &);
	ssl_socket& read(std::string&);
	ssl_socket& writefile(std::string const &);
	ssl_socket& readfile(std::string const &);

private:
    static const int BUFFER_SIZE = 128;
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> *socket;
	std::string buf;
	int pos;
};


#endif
