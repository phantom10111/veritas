#include "common/config.hpp"
#include "common/ssl_socket.hpp"
#include "webserver/webserver.hpp"
#include "webserver/commands.hpp"
#include <thread>
#include <pqxx/pqxx> 
#include <pqxx/binarystring>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>
#include <iostream>

//Sorry, convention
using namespace boost::asio;

void webserver::run(int port){
    io_service ios;
    ip::tcp::endpoint endpoint(ip::tcp::v4(), port);
    ip::tcp::acceptor acceptor(ios, endpoint);
    ssl::context ctx(ssl::context::tlsv1_server);
    ctx.set_options(ssl::context::default_workarounds
        | ssl::context::no_sslv2);
    ctx.set_verify_mode(ssl::verify_none);
    ctx.use_private_key_file(privkeyfile, ssl::context::pem);
    ctx.use_certificate_chain_file(certfile);
    while(true){
        ssl::stream<ip::tcp::socket> *stream = 
            new ssl::stream<ip::tcp::socket>(ios, ctx);
        acceptor.accept(stream->lowest_layer());
        std::thread(&webserver::server_thread, this, stream).detach();
    }
}
    
void webserver::server_thread(ssl::stream<ip::tcp::socket> *stream){
  try
  {
    
    ssl_socket socket(stream, ssl::stream<ip::tcp::socket>::server);
    std::string login, pass;
    pqxx::connection conn(DB_CONN_INFO);
    std::cout << "..." << std::endl;
    socket.read(login).read(pass);
    std::cout << "..." << std::endl;
    pqxx::result users = select_users(conn, login);
    std::cout << "..." << std::endl;
    if(users.empty()){
        socket.write("ERROR NOSUCHUSER", '\n');
        return;
    }
    auto user = *users.begin();
    std::string userid = user["userid"].as<std::string>();
    std::string authtoken = user["authtoken"].as<std::string>();
    if(pass != authtoken){
        socket.write("ERROR WRONGPASSWORD", '\n');
        return;
    }
    std::string command;
    socket.read(command);
    std::cout << "ws" << std::endl;
    if(handlers.count(command))
        handlers[command](user, socket, conn);
    else
        socket.write("ERROR NOSUCHCOMMAND", '\n');
  } catch(...) {std::cout << "sigh" << std::endl;}
}

pqxx::result webserver::select_users(
    pqxx::connection &conn, 
    std::string const &login) {
    conn.prepare("login", 
        "SELECT *          "
        "  FROM users      "
        " WHERE login = $1;");
    pqxx::work txn(conn);
    return txn.prepared("login")(login).exec();
}
