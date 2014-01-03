#include "server/webserver.hpp"
#include <thread>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
void webserver::run(int port){
    boost::asio::io_service ios;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
    boost::asio::ip::tcp::acceptor acceptor(ios, endpoint);
    while(true){
        boost::asio::ip::tcp::iostream *stream = 
            new boost::asio::ip::tcp::iostream();
        acceptor.accept(*stream->rdbuf());
        std::thread(&webserver::server_thread, this, stream).detach();
        std::cout << " out\n";
    }
}
    
void webserver::server_thread(boost::asio::ip::tcp::iostream *stream){
    (*stream) << "wat?" << std::endl;
    std::string str;
    (*stream) >> str;
    delete stream;
}
