
#include "common/ssl_socket.hpp"
#include "webserver/commands.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <pqxx/pqxx> 
#include <map>


void submit(pqxx::result::tuple &user, 
            ssl_socket& stream, 
            pqxx::connection &conn){
    stream.write("OK");
}

std::map<std::string, command_handler> command_handlers(){
    std::map<std::string, command_handler> result;
    result["submit"] = submit;
    return result;
}


