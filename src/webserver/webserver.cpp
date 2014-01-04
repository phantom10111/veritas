#include "common/config.hpp"
#include "webserver/webserver.hpp"
#include <thread>
#include <pqxx/pqxx> 
#include <pqxx/binarystring>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <iostream>
void webserver::run(int port){
    boost::asio::io_service ios;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
    boost::asio::ip::tcp::acceptor acceptor(ios, endpoint);
    while(true){
        boost::asio::ip::tcp::iostream *stream = 
            new boost::asio::ip::tcp::iostream();
        acceptor.accept(*stream->rdbuf());
        std::thread(&webserver::server_thread, this, stream).detach();
    }
}
    
void webserver::server_thread(std::iostream *stream){
    std::string login, pass;
    pqxx::connection conn(DB_CONN_INFO);
    getline(getline(*stream, login), pass);
    
    std::string query = 
        std::string() + "SELECT * \
                           FROM users \
                          WHERE login = '"+ login +"';";
    pqxx::work txn(conn);
    pqxx::result user = txn.exec(query);
    if(user.empty()){
        *stream << "ERROR NOSUCHUSER\n";
        delete stream;
        return;
    }
    std::string userid = user.begin()["userid"].as<std::string>();
    delete stream;
}
