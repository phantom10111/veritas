#include "common/ssl_socket.hpp"
#include <sstream>
#include <fstream>
#include <cctype>

ssl_socket::ssl_socket(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> *socket, boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::handshake_type type): socket(socket), buf(), pos(0)
{
	socket->lowest_layer().set_option(boost::asio::socket_base::keep_alive(true));
	socket->lowest_layer().set_option(boost::asio::ip::tcp::no_delay(true));
	socket->handshake(type);
}

ssl_socket::~ssl_socket()
{
	delete socket;
}

ssl_socket& ssl_socket::write(std::string const &what, char delim)
{
    char delim_str[1];
    delim_str[0] = delim;
    boost::asio::write(*socket, boost::asio::buffer(what.c_str(), what.size()));
    boost::asio::write(*socket, boost::asio::buffer(delim_str));
    return *this;
}

ssl_socket& ssl_socket::read(std::string& what, char delim)
{
	char c[1024];
	std::stringstream str;
	
	while(true)
	{
		if(pos >= buf.size())
		{
			int len = socket->read_some(boost::asio::buffer(c));
        	buf = std::string(c, len);
        	
			pos = 0;
		}
		char c = buf[pos++];
		if(c == delim)
			break;
		str << c;
	}
	what = str.str();
	return *this;
}

ssl_socket &ssl_socket::writetext(std::string const &what){
    write(std::to_string(what.size()));
    boost::asio::write(*socket, boost::asio::buffer(what.c_str(), what.size()));
    return *this;
}

ssl_socket &ssl_socket::readtext(std::string &what){
    std::string sizestr;
    read(sizestr);
    
	char c[1024];
	std::stringstream str;
	
	int size = atoi(sizestr.c_str());
	
	for(int i = 0; i < size; i++)
	{
		if(pos >= buf.size())
		{
			int len = socket->read_some(boost::asio::buffer(c));
        	buf = std::string(c, len);
        	
			pos = 0;
		}
		str << buf[pos++];
	}
	what = str.str();
	return *this;
}
	

int ssl_socket::writefile(std::string const &filename, int max_size){
    char c[BUFFER_SIZE + 1];
	std::ifstream filestream(filename.c_str(), std::ios::binary | std::ios::ate);
    int filesize = filestream.tellg();
    if(filesize > max_size)
        return 1;
    this->write(std::to_string(filesize));
    filestream.clear();
    filestream.seekg(0, std::ios::beg);
    while(filesize > 0){
        int maxlen = std::min(filesize, BUFFER_SIZE);
        filestream.read(c, maxlen);
        int len = filestream.gcount();
        c[len] = '\0';
        this->write(std::string(c, len));
		filesize -= len;
    }
    return 0;
}

int ssl_socket::readfile(std::string const &filename, int max_size){
    char c[BUFFER_SIZE + 1];
	std::ofstream filestream;
    filestream.open(filename.c_str(), std::ios::binary);
    std::string lengthstr;
    this->read(lengthstr);
    int filesize = atoi(lengthstr.c_str());
    if(filesize > max_size)
        return 1;
    while(filesize > 0){
		if(pos >= buf.size())
		{
			int len = socket->read_some(boost::asio::buffer(c));
        	buf = std::string(c, len);
			pos = 0;
		}
		int len = buf.size() - pos;
		filestream.write(buf.c_str() + pos, len);
		pos = buf.size();
		filesize -= len;
    }
    return 0;

}
