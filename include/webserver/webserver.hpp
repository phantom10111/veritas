#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP
#include "webserver/commands.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>

class webserver {
public:
    void run(int port);
    
private:
    void server_thread(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> *stream);
    std::map<std::string, command_handler> handlers = command_handlers();
};

#endif
