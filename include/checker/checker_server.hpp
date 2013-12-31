#ifndef CHECKER_SERVER_HPP
#define CHECKER_SERVER_HPP
#include "checker.hpp"
#include "resource_manager.hpp"
#include <queue>
#include <boost/asio/io_service.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <mutex>
#include <condition_variable>
class checker_server {
public:
    void run(int port);
    
private:
    void acceptor_thread(int port);
    void checker_thread();
    checker                                    _checker;
    resource_manager                           _resource_manager;
    std::queue<boost::asio::ip::tcp::iostream*> _queue;
    std::mutex                                 _mutex;
    std::condition_variable                    _condition;
};

#endif
