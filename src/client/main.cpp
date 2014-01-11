#include "common/config.hpp"
#include "common/ssl_socket.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>
#include <cstring>
#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>

using namespace boost::asio;

int main(int argc, char *argv[]){
	io_service ios;
	ip::tcp::endpoint endpoint(ip::tcp::v4(), webserver_port);
	ssl::context ctx(ssl::context::tlsv1_client);
	ctx.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2);
	ctx.set_verify_mode(ssl::verify_peer);
	ctx.load_verify_file(certfile);

	auto stream = new ssl::stream<ip::tcp::socket>(ios, ctx);
	stream->lowest_layer().connect(endpoint);
	ssl_socket socket(stream, ssl::stream<ip::tcp::socket>::client);
	
	std::string login, password;
	std::cout << "login: " << std::flush;
	std::cin >> login;
	
	std::cout << "password: " << std::flush;
	
	termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    std::cin >> password;
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
    std::cout << std::endl;
    socket.write(login);
    socket.write(password);
    
    for(int i = 1; i < argc; i++)
        if(strchr(argv[i], '.'))
            socket.writefile(argv[i], 1000);
        else
            socket.write(argv[i]);

	std::string response;
	while(response != "OK" && 
	        response.substr(0, std::min(5, (int)response.size())) != "ERROR"){
	    socket.read(response, '\n');
        std::cout << response << std::endl;
    }
	return 0;
}
