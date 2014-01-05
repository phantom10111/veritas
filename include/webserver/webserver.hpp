#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>

class webserver {
public:
    void run(int port);
    
private:
    void server_thread(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> *stream);
};

#endif
