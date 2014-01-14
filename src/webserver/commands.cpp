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
        socket.write("ERROR NOSUCHPARTICIPATION", '\n');
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
        socket.write("ERROR NOSUCHVARIANT", '\n');
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
        socket.write("ERROR FILETOLARGE", '\n');
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
    delete[] submit_bytes;
    
    std::string submissionid = txn.exec(
        "SELECT currval('submissions_submissionid_seq') "
        "    AS submissionid                            "
    ).begin()["submissionid"].as<std::string>();
    txn.commit();
    
    boost::asio::io_service ios;

    int minqsize, minqchecker = -1;
    
	
	for(int checker = 0; checker < n_checkers; checker++){
	    std::string host = checker_hosts[checker];
	    std::string port = std::to_string(checker_ports[checker]);
	    boost::asio::ip::tcp::iostream connection(host, port);
        boost::asio::ip::tcp::no_delay opt(true);
        connection.rdbuf()->set_option(opt);
        if(!connection.good()) {
            socket.write("ERROR BADCONNECTION", '\n');
            return;
        }
        std::string input;
        int qsize;
        connection << "QSIZE" << std::endl;
        connection >> input >> qsize;
        if(minqsize == -1 || qsize < minqsize){
            minqsize = qsize;
            minqchecker = checker;
        }
	}
    if(minqchecker == -1){
        socket.write("ERROR NOCHECKER", '\n');
    }
    std::string host = checker_hosts[minqchecker];
	std::string port = std::to_string(checker_ports[minqchecker]);
	boost::asio::ip::tcp::iostream connection(host, port);
    boost::asio::ip::tcp::no_delay opt(true);
    connection.rdbuf()->set_option(opt);
    if(!connection.good()){
        socket.write("ERROR BADCONNECTION", '\n');
        return;
    }
    connection << "TEST " << submissionid << std::endl;
    std::string line;
    do {
        std::getline(connection, line);
        socket.write(line, '\n');
    } while(line.size() < 3 || line.substr(0, 3) != "END");
    socket.write("OK", '\n');
}

void viewcontests(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    pqxx::result contests;
    if(user["is_administrator"].as<bool>()){
        conn.prepare("all contests",
            "SELECT contestname, description"
            "  FROM contests                ");
        contests = txn.prepared("all contests").exec();
    } else {
        conn.prepare("contests",
                "      SELECT contestname, description "
                "        FROM contests                 "
                "NATURAL JOIN participations           "
                "       WHERE userid = $1              ");
        std::string userid = user["userid"].as<std::string>();
        contests = txn.prepared("contests")(userid).exec();
    }
    
    for(auto row : contests){
        socket.write(row["contestname"].as<std::string>(), '\t')
              .write(row["description"].as<std::string>(), '\n');
    }
    socket.write("OK", '\n');
    return;   
}

void viewvariants(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    std::string contestname;
    socket.read(contestname);
    pqxx::work txn(conn);
    conn.prepare("user contests", 
        "      SELECT contestid        " 
        "        FROM participations   "
        "NATURAL JOIN contests         "
        "       WHERE userid = $1      "
        "         AND contestname = $2 ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("user contests")(userid)
                                                    (contestname).exec();
    if(contests.empty()){
        socket.write("ERROR NOSUCHPARTICIPATION", '\n');
        return;
    }
    int contestid = contests.begin()["contestid"].as<int>();
    
    conn.prepare("variants", 
        "SELECT shortname, variantname, submissible_to " 
        "  FROM variants                               "
        " WHERE contestid = $1                         ");
    
    pqxx::result variants = txn.prepared("variants")(contestid).exec();
    for(auto row : variants){
        auto const &submissible_to = row["submissible_to"];
        std::string deadline = submissible_to.is_null() ? "none" :
                               submissible_to.as<std::string>();
        socket
            .write("CONTEST")
            .write(row["shortname"].as<std::string>())
            .write(row["variantname"].as<std::string>(), '\n')
            .write("DEADLINE")
            .write(deadline, '\n');
    }
    socket.write("OK", '\n');
    return;   
}

void viewvariant(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    std::string contestname, shortname;
    socket.read(contestname).read(shortname);
    pqxx::work txn(conn);
    conn.prepare("user contests", 
        "      SELECT contestid        " 
        "        FROM participations   "
        "NATURAL JOIN contests         "
        "       WHERE userid = $1      "
        "         AND contestname = $2 ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("user contests")(userid)
                                                    (contestname).exec();
    if(contests.empty()){
        socket.write("ERROR NOSUCHPARTICIPATION", '\n');
        return;
    }
    int contestid = contests.begin()["contestid"].as<int>();
    
    conn.prepare("variant", 
        "SELECT variantname, description " 
        "  FROM variants                 "
        " WHERE contestid = $1           "
        "   AND shortname = $2           ");
    
    pqxx::result variants = txn.prepared("variant")(contestid)
                                                  (shortname).exec();
                                                  
    if(variants.empty()){
        socket.write("ERROR NOSUCHVARIANT", '\n');
        return;
    }
    for(auto row : variants){
        socket
            .write("VARIANT")
            .write(shortname)
            .write(row["variantname"].as<std::string>(), '\n')
            .writetext(row["description"].as<std::string>());
    }
    socket.write("\nOK", '\n');
    return;   
}

std::map<std::string, command_handler> command_handlers(){
    std::map<std::string, command_handler> result;
    result["submit"] = submit;
    result["viewcontests"] = viewcontests;
    result["viewvariants"] = viewvariants;
    result["viewvariant"] = viewvariant;
    return result;
}


