
#include "common/config.hpp"
#include "common/ssl_socket.hpp"
#include "webserver/commands.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <pqxx/pqxx> 
#include <cstdlib>
#include <fstream>
#include <map>


void submit(pqxx::result::tuple &user, 
            ssl_socket& socket, 
            pqxx::connection &conn){
    std::string contestname, shortname, extension;
    socket.read(contestname).read(shortname);
    conn.prepare("user contests", 
        "      SELECT contestid        " 
        "        FROM participations   "
        "NATURAL JOIN contests         "
        "       WHERE userid = $1      "
        "         AND contestname = $2 ");
    int userid = user["userid"].as<int>();
    pqxx::work txn(conn);
    pqxx::result contests = txn.prepared("user contests")(userid)
                                                    (contestname).exec();
    if(contests.empty()){
        socket.write("ERROR NO SUCH PARTICIPATION");
        return;
    }
    std::string contestid = contests.begin()["contestid"].as<std::string>();
    conn.prepare("variantid", 
        " SELECT variantid      "
        "   FROM variants       "
        "  WHERE contestid = $1 "
        "    AND shortname = $2 ");
    pqxx::result variants = txn.prepared("variantid")(contestid)
                                                    (shortname).exec();
    if(contests.empty()){
        socket.write("ERROR NO SUCH VARIANT");
        return;
    }
    std::string variantid = variants.begin()["variantid"].as<std::string>();
    socket.read(extension);
    std::string filename = 
        user["login"].as<std::string>() + 
        shortname + 
        "." + 
        extension;
    if(socket.readfile(filename, MAX_SUBMIT_SIZE)){
        socket.write("ERROR FILE TO LARGE");
        return;
    }
    std::ifstream filestream(filename.c_str(), std::ios::binary | std::ios::ate);
    int filesize = filestream.tellg();
    filestream.clear();
    filestream.seekg(0, std::ios::beg);
    
    char *submit_bytes = new char[filesize];
    int bytes_read = 0;
    while(bytes_read < filesize){
        filestream.read(submit_bytes + bytes_read, filesize - bytes_read);
        bytes_read += filestream.gcount();
    }
    pqxx::binarystring submit_binary((const void*) submit_bytes, (size_t)filesize);
    conn.prepare("submit", 
        "INSERT INTO submissions(userid, variantid, extension, file) " 
        "     VALUES($1, $2, $3, $4)                                 ");
        
    txn.prepared("submit")(userid)(variantid)(extension)(submit_binary).exec();
    txn.commit();
        
    delete[] submit_bytes;
    socket.write("OK");
}

std::map<std::string, command_handler> command_handlers(){
    std::map<std::string, command_handler> result;
    result["submit"] = submit;
    return result;
}


