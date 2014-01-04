#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>

class webserver {
public:
    void run(int port);
    
private:
    void server_thread(std::iostream *stream);
};

#endif
