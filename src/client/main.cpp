#include "common/config.hpp"
#include "common/ssl_socket.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>


int main(){
	boost::asio::io_service ios;
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), webserver_port);
	boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv1_client);
	ctx.set_options(boost::asio::ssl::context::default_workarounds
		| boost::asio::ssl::context::no_sslv2);
	ctx.set_verify_mode(boost::asio::ssl::verify_peer);
	ctx.load_verify_file(certfile);

	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> *stream = new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(ios, ctx);
	stream->lowest_layer().connect(endpoint);
	ssl_socket socket(stream, boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::client);

	socket.write("wlis");
	socket.write("54ri857");
	socket.write("submit");
	std::string response;
	socket.read(response);
	std::cout << response;
	
	return 0;
}
