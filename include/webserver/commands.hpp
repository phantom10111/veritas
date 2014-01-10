
#include "common/ssl_socket.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <pqxx/pqxx> 
#include <map>

typedef void (*command_handler)(pqxx::result::tuple&, ssl_socket&, pqxx::connection&);

void submit(pqxx::result::tuple&, ssl_socket&, pqxx::connection&);

std::map<std::string, command_handler> command_handlers(); 

