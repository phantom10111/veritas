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
    ctx.use_tmp_dh_file(dhfile);
    while(true){
        ssl::stream<ip::tcp::socket> *stream = 
            new ssl::stream<ip::tcp::socket>(ios, ctx);
        acceptor.accept(stream->lowest_layer());
        std::thread(&webserver::server_thread, this, stream).detach();
    }
}
    
void webserver::server_thread(ssl::stream<ip::tcp::socket> *stream){
    ssl_socket socket(stream, ssl::stream<ip::tcp::socket>::server);
    std::string login, pass;
    pqxx::connection conn(DB_CONN_INFO);
    socket.read(login).read(pass);
    //dropping CRs and LFs
    login.erase(login.find_last_not_of("\n\r\t")+1);
    pass.erase(pass.find_last_not_of("\n\r\t")+1);
    std::string query = 
        std::string() + "SELECT *     \n"
                        "  FROM users \n"
                        " WHERE login = '"+ login + "';\n";
    pqxx::work txn(conn);
    pqxx::result user = txn.exec(query);
    if(user.empty()){
        socket.write("ERROR NO SUCH USER");
        return;
    }
    std::string userid = user.begin()["userid"].as<std::string>();
    std::string authtoken = user.begin()["authtoken"].as<std::string>();
    if(pass !=authtoken){
        socket.write("ERROR WRONGPASS");
        return;
    }
    socket.write("OK");
}
