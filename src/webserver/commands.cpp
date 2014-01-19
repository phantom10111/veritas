#include "common/config.hpp"
#include "common/ssl_socket.hpp"
#include "webserver/commands.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <pqxx/pqxx> 
#include <cstdlib>
#include <fstream>
#include <map>
using namespace boost::asio;


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
      try {
	std::string host = checker_hosts[checker];
	ip::tcp::resolver resolv(ios);
	auto it = resolv.resolve(host);
        ip::tcp::endpoint endpoint = *it;
	endpoint.port(checker_ports[checker]);
        ssl::context ctx(ssl::context::tlsv1_client);
        ctx.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2);
        ctx.set_verify_mode(ssl::verify_peer);
        ctx.load_verify_file(certfile);
        auto stream = new ssl::stream<ip::tcp::socket>(ios, ctx);
        stream->lowest_layer().connect(endpoint);
        ssl_socket connection(stream, ssl::stream<ip::tcp::socket>::client);
        connection.write("QSIZE", '\n');
        std::string input;
        std::istringstream ss;
        connection.read(input, '\n');
        ss.str(input);
        int qsize;
        ss >> qsize;
        if(minqchecker == -1 || qsize < minqsize){
            minqsize = qsize;
            minqchecker = checker;
        }
      } catch(...) {
      }
	}
    if(minqchecker == -1){
        socket.write("ERROR NOCHECKER", '\n');
        return;
    }
  try {
    std::string host = checker_hosts[minqchecker];
    ip::tcp::resolver resolv(ios);
    auto it = resolv.resolve(host);
    ip::tcp::endpoint endpoint = *it;
    endpoint.port(checker_ports[minqchecker]);
    ssl::context ctx(ssl::context::tlsv1_client);
    ctx.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2);
    ctx.set_verify_mode(ssl::verify_peer);
    ctx.load_verify_file(certfile);
    auto stream = new ssl::stream<ip::tcp::socket>(ios, ctx);
	stream->lowest_layer().connect(endpoint);
    ssl_socket connection(stream, ssl::stream<ip::tcp::socket>::client);
    connection.write("TEST", '\n').write(submissionid, '\n');
    std::string line;
    do {
        connection.read(line, '\n');
        socket.write(line, '\n');
    } while(line.size() < 3 || line.substr(0, 3) != "END");
  } catch(...) {
    socket.write("ERROR BADCONNECTION", '\n');
    return;
  }
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
            .write("VARIANT")
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

void viewproblems(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    conn.prepare("moderated contests", 
        "      SELECT contestid           " 
        "        FROM participations      "
        "       WHERE userid = $1         "
        "         AND is_moderator = true ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("moderated contests")(userid).exec();
    if(contests.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    conn.prepare("problems", 
        "SELECT problemid, problemname " 
        "  FROM problems               ");
    
    pqxx::result problems = txn.prepared("problems").exec();
                                                  
    if(problems.empty()){
        socket.write("ERROR NOPROBLEMS", '\n');
        return;
    }
    for(auto row : problems){
        socket
            .write("PROBLEM")
            .write(row["problemid"].as<std::string>())
            .write(row["problemname"].as<std::string>(), '\n');
    }
    socket.write("\nOK", '\n');
    return;   
}

void viewproblem(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    conn.prepare("moderated contests", 
        "      SELECT contestid           " 
        "        FROM participations      "
        "       WHERE userid = $1         "
        "         AND is_moderator = true ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("moderated contests")(userid).exec();
    if(contests.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    conn.prepare("problems", 
        "SELECT *             " 
        "  FROM problems      "
        " WHERE problemid = $1");
    
    std::string problemid_str;
    socket.read(problemid_str);
    int problemid = atoi(problemid_str.c_str());
    pqxx::result problems = txn.prepared("problems")(problemid).exec();
                                                  
    if(problems.empty()){
        socket.write("ERROR NOSUCHPROBLEM", '\n');
        return;
    }
    for(auto row : problems){
        socket
            .write("PROBLEM")
            .write(row["problemid"].as<std::string>())
            .write(row["problemname"].as<std::string>(), '\n')
            .writetext(row["description"].as<std::string>()).write("", '\n');
    }
    conn.prepare("testgroups",
        "SELECT * FROM testgroups WHERE problemid = $1");
    pqxx::result testgroups = txn.prepared("testgroups")(problemid).exec();
    for(auto row : testgroups){
        socket
            .write("TESTGROUP")
            .write(row["testgroupid"].as<std::string>())
            .write(row["testgroupname"].as<std::string>(), '\n');
    }
    socket.write("\nOK", '\n');
    return;   
}

void viewtestgroup(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    conn.prepare("moderated contests", 
        "      SELECT contestid           " 
        "        FROM participations      "
        "       WHERE userid = $1         "
        "         AND is_moderator = true ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("moderated contests")(userid).exec();
    if(contests.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    conn.prepare("tests", 
        "SELECT testid, testname      " 
        "  FROM testgroups_tests      "
        "NATURAL JOIN tests"
        " WHERE testgroupid = $1");
    
    std::string testgroupid_str;
    socket.read(testgroupid_str);
    int testgroupid = atoi(testgroupid_str.c_str());
    pqxx::result tests = txn.prepared("tests")(testgroupid).exec();
                                                  
    if(tests.empty()){
        socket.write("ERROR EMPTYTESTGROUP", '\n');
        return;
    }
    for(auto row : tests){
        socket
            .write("TEST")
            .write(row["testid"].as<std::string>())
            .write(row["testname"].as<std::string>(), '\n');
    }
    socket.write("\nOK", '\n');
    return;   
}

void viewtest(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    conn.prepare("moderated contests", 
        "      SELECT contestid           " 
        "        FROM participations      "
        "       WHERE userid = $1         "
        "         AND is_moderator = true ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("moderated contests")(userid).exec();
    if(contests.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    conn.prepare("tests", 
        "SELECT *      " 
        "  FROM tests      "
        " WHERE testid = $1");
    
    std::string testid_str;
    socket.read(testid_str);
    int testid = atoi(testid_str.c_str());
    pqxx::result tests = txn.prepared("tests")(testid).exec();
                                                  
    if(tests.empty()){
        socket.write("ERROR NOSUCHTEST", '\n');
        return;
    }
    for(auto row : tests){
        socket
            .write("TEST")
            .write(row["testid"].as<std::string>())
            .write(row["problemid"].as<std::string>())
            .write(row["testname"].as<std::string>())
            .write(row["timelimit"].as<std::string>())
            .write(row["memlimit"].as<std::string>())
            .write(row["testname"].as<std::string>(), '\n');
        socket.writetext(pqxx::binarystring(row["infile"]).str());
        socket.writetext(pqxx::binarystring(row["outfile"]).str());
    }
    socket.write("\nOK", '\n');
    return;   
}

void addproblem(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    conn.prepare("moderated contests", 
        "      SELECT contestid           " 
        "        FROM participations      "
        "       WHERE userid = $1         "
        "         AND is_moderator = true ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("moderated contests")(userid).exec();
    if(contests.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    std::string problemname, description;
    socket.read(problemname).readtext(description);
    conn.prepare("insert problem", 
        "  INSERT INTO problems(problemname, description) VALUES($1, $2)");
    txn.prepared("insert problem")(problemname)(description).exec();
    pqxx::result problemids = 
    	txn.exec("SELECT currval('problems_problemid_seq') AS problemid");
    std::string problemid = problemids.begin()["problemid"].as<std::string>();
    socket.write("PROBLEMID").write(problemid, '\n').write("OK", '\n');
    txn.commit();
    return;
}

void addtestgroup(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    conn.prepare("moderated contests", 
        "      SELECT contestid           " 
        "        FROM participations      "
        "       WHERE userid = $1         "
        "         AND is_moderator = true ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("moderated contests")(userid).exec();
    if(contests.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    std::string problemid, testgroupname;
    socket.read(problemid).read(testgroupname);
    conn.prepare("insert testgroup", 
        "  INSERT INTO testgroups(problemid, testgroupname) VALUES($1, $2)");
    txn.prepared("insert testgroup")(problemid)(testgroupname).exec();
    pqxx::result testgroupids = 
    	txn.exec("SELECT currval('testgroups_testgroupid_seq') AS testgroupid");
    std::string testgroupid = testgroupids.begin()["testgroupid"].as<std::string>();
    socket.write("TESTGROUPID").write(testgroupid, '\n').write("OK", '\n');
    txn.commit();
    return;
}

void addtest(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    conn.prepare("moderated contests", 
        "      SELECT contestid           " 
        "        FROM participations      "
        "       WHERE userid = $1         "
        "         AND is_moderator = true ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("moderated contests")(userid).exec();
    if(contests.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    std::string problemid, testname, description, timelimit_, memlimit_, infile, 
    	outfile;
    socket.read(problemid).read(testname).readtext(description).read(timelimit_);
    //std::cout << problemid << " " << testname << " " << description << " | " << timelimit_ << " " << std::endl;
    socket.read(memlimit_).readtext(infile).readtext(outfile);
    conn.prepare("insert test", 
        "  INSERT INTO tests(problemid, testname, description, timelimit, "
        "			   memlimit, infile, outfile) "
        "       VALUES ($1, $2, $3, $4, $5, $6, $7)");
    float timelimit = atof(timelimit_.c_str());
    int memlimit = atoi(memlimit_.c_str());
    //std::cout << timelimit << " " << memlimit << std::endl;
    txn.prepared("insert test")(problemid)(testname)(description)
    	(timelimit)(memlimit)(infile)(outfile).exec();
    pqxx::result testids = 
    	txn.exec("SELECT currval('tests_testid_seq') AS testid");
    std::string testid = testids.begin()["testid"].as<std::string>();
    socket.write("TESTID").write(testid, '\n').write("OK", '\n');
    txn.commit();
    return;
}

void addtesttogroup(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    conn.prepare("moderated contests", 
        "      SELECT contestid           " 
        "        FROM participations      "
        "       WHERE userid = $1         "
        "         AND is_moderator = true ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("moderated contests")(userid).exec();
    if(contests.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    std::string testid, testgroupid;
    socket.read(testid).read(testgroupid);
    conn.prepare("insert t_t", 
        "  INSERT INTO testgroups_tests "
        "       VALUES ($2, $1)");
    txn.prepared("insert t_t")(testid)(testgroupid).exec();
    socket.write("OK", '\n');
    txn.commit();
    return;
}


void addvariant(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    std::string contestid, problemid, from, to, shortname, name, description;
    socket.read(contestid);
    conn.prepare("moderated contests", 
        "      SELECT contestid           " 
        "        FROM participations      "
        "       WHERE userid = $1         "
        "         AND contestid = $2 "
        "         AND is_moderator = true ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("moderated contests")(userid)(contestid).exec();
    if(contests.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    socket.read(problemid).read(from).read(to).read(shortname).
    	readtext(name).readtext(description);
    conn.prepare("insert variant", 
        "  INSERT INTO variants(contestid, problemid, submissible_from, "
        "			   submissible_to, shortname, variantname, description) "
        "       VALUES ($1, $2, $3, $4, $5, $6, $7)");
    txn.prepared("insert variant")(contestid)(problemid)(from)(to)(shortname)
    	(name)(description).exec();
    auto id = txn.exec("SELECT currval('variants_variantid_seq') AS variantid")
    	.begin()["variantid"].as<std::string>();
    socket.write("VARIANTID").write(id, '\n');	
    socket.write("OK", '\n');
    txn.commit();
    return;
}

void addgrouptovariant(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    std::string testgroupid, variantid;
    socket.read(testgroupid).read(variantid);
    conn.prepare("moderated contests", 
        "      SELECT contestid           " 
        "        FROM participations      "
        "NATURAL JOIN variants            "
        "       WHERE variantid = $1      "
        "         AND userid = $2 "
        "         AND is_moderator = true ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("moderated contests")(variantid)(userid).exec();
    if(contests.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    conn.prepare("insert v_t", 
        "  INSERT INTO variants_testgroups "
        "       VALUES ($1, $2)");
    txn.prepared("insert v_t")(variantid)(testgroupid).exec();
    socket.write("OK", '\n');
    txn.commit();
    return;
}

std::map<std::string, command_handler> command_handlers(){
    std::map<std::string, command_handler> result;
    result["submit"] = submit;
    result["viewcontests"] = viewcontests;
    result["viewvariants"] = viewvariants;
    result["viewvariant"] = viewvariant;
    result["viewproblems"] = viewproblems;
    result["viewproblem"] = viewproblem;
    result["viewtestgroup"] = viewtestgroup;
    result["viewtest"] = viewtest;
    result["addproblem"] = addproblem;
    result["addtestgroup"] = addtestgroup;
    result["addtest"] = addtest;
    result["addtesttogroup"] = addtesttogroup;
    result["addvariant"] = addvariant;
    result["addgrouptovariant"] = addgrouptovariant;
    return result;
}





