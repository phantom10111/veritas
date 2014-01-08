#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP
#include "webserver/commands.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>
#include <pqxx/pqxx>

class webserver {
public:
    void run(int port);
    
private:
    void server_thread(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> *stream);
    pqxx::result select_users(pqxx::connection &conn, std::string const &login);
    std::map<std::string, command_handler> handlers = command_handlers();
};

#endif
