	#include "common/config.hpp"
#include "common/ssl_socket.hpp"
#include "webserver/commands.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <pqxx/pqxx> 
#include <cstdlib>
#include <ctime>
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
		ip::tcp::endpoint endpoint(ip::tcp::v4(), checker_ports[checker]);
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
		ip::tcp::endpoint endpoint(ip::tcp::v4(), checker_ports[minqchecker]);
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
    if(contests.empty() ){
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
    socket.read(memlimit_).readtext(infile).readtext(outfile);
    conn.prepare("insert test", 
        "  INSERT INTO tests(problemid, testname, description, timelimit, "
        "			   memlimit, infile, outfile) "
        "       VALUES ($1, $2, $3, $4, $5, $6, $7)");
    float timelimit = atof(timelimit_.c_str());
    int memlimit = atoi(memlimit_.c_str());
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

void removeproblem(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    conn.prepare("moderated contests", 
        "      SELECT contestid           " 
        "        FROM participations      "
        "NATURAL JOIN variants            "
        "       WHERE userid = $1 "
        "         AND is_moderator = true ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("moderated contests")(userid).exec();
    if(contests.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    std::string problemid;
    socket.read(problemid);
    conn.prepare("delete problem", 
        "  DELETE FROM problems      "
        "        WHERE problemid = $1");
    txn.prepared("delete problem")(problemid).exec();
    socket.write("OK", '\n');
    txn.commit();
    return;
}


void removetestgroup(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    conn.prepare("moderated contests", 
        "      SELECT contestid           " 
        "        FROM participations      "
        "NATURAL JOIN variants            "
        "       WHERE userid = $1 "
        "         AND is_moderator = true ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("moderated contests")(userid).exec();
    if(contests.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    std::string testgroupid;
    socket.read(testgroupid);
    conn.prepare("delete testgroup", 
        "  DELETE FROM testgroups      "
        "        WHERE testgroupid = $1");
    txn.prepared("delete testgroup")(testgroupid).exec();
    socket.write("OK", '\n');
    txn.commit();
    return;
}

void removetest(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    conn.prepare("moderated contests", 
        "      SELECT contestid           " 
        "        FROM participations      "
        "NATURAL JOIN variants            "
        "       WHERE userid = $1 "
        "         AND is_moderator = true ");
    int userid = user["userid"].as<int>();
    pqxx::result contests = txn.prepared("moderated contests")(userid).exec();
    if(contests.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    std::string testid;
    socket.read(testid);
    conn.prepare("delete test", 
        "  DELETE FROM tests      "
        "        WHERE testid = $1");
    txn.prepared("delete test")(testid).exec();
    socket.write("OK", '\n');
    txn.commit();
    return;
}


void removetestfromgroup(
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
    conn.prepare("delete t_t", 
        "  DELETE FROM testgroups_tests "
        "        WHERE testgroupid = $2 AND testid = $1");
    txn.prepared("delete t_t")(testid)(testgroupid).exec();
    socket.write("OK", '\n');
    txn.commit();
    return;
}



void removegroupfromvariant(
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
    conn.prepare("delete v_t", 
        "  DELETE FROM variants_tests "
        "        WHERE testgroupid = $2 AND variantid = $1");
    txn.prepared("insert v_t")(variantid)(testgroupid).exec();
    socket.write("OK", '\n');
    txn.commit();
    return;
}



void createuser(
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
    const int pass_len = 10;
    const int sigma = 26;
    char pass[pass_len], chars[sigma + 1];
    srand(time(NULL));
    std::iota(chars, chars + sigma, 'a');
    std::generate_n(pass, pass_len, [chars](){return chars[rand() % sigma];});
    pass[pass_len] = '\0';
    std::string login, username, email;
    socket.read(login).read(username).read(email);
    conn.prepare("insert user",
        "INSERT INTO users(login, username, authtoken, email)  "
        "     VALUES ($1, $2, $3, $4)                          "
    );
    txn.prepared("insert user")(login)(username)(pass)(login + "@mail.com").exec();
    socket.write("PASSWORD").write(pass, '\n').write("OK", '\n');
    txn.commit();
    return;
}

void addusertocontest(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    std::string login, contestname, is_moderator;
    socket.read(login).read(contestname).read(is_moderator);
    conn.prepare("select contestid",
    	"SELECT contestid        "
    	"  FROM contests         "
    	" WHERE contestname = $1 ");
    auto contestids = txn.prepared("select contestid")(contestname).exec();
    if(contestids.empty()){
        socket.write("ERROR NOSUCHCONTEST", '\n');
        return;
    }
    int contestid = contestids.begin()["contestid"].as<int>();
    conn.prepare("participation info",
    	"SELECT *                  " 
    	"  FROM participations     "
    	" WHERE userid = $1        "
    	"   AND contestid = $2     "
    	"   AND is_moderator = true");
    auto infos = txn.prepared("participation info")(user["userid"].as<int>())(contestid).exec();
    if(infos.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    conn.prepare("select userid",
    	"SELECT userid    "
    	"  FROM users     "
    	" WHERE login = $1");
    auto userids = txn.prepared("select userid")(login).exec();
    if(userids.empty()){
        socket.write("ERROR NOUSER", '\n');
        return;
    }
    int userid = userids.begin()["userid"].as<int>();
    conn.prepare("add participant",
    	"INSERT INTO participations"
    	"     VALUES ($1, $2, $3)  ");
    txn.prepared("add participant")(userid)(contestid)(is_moderator == "true").exec();
    socket.write("OK", '\n');
    txn.commit();
    return;
}

void addlanguagetovariant(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    std::string contestname, shortname, roname, extension;
    socket.read(contestname).read(shortname).read(roname).read(extension);
    
    conn.prepare("select contestid",
    	"SELECT contestid        "
    	"  FROM contests         "
    	" WHERE contestname = $1 ");
    auto contestids = txn.prepared("select contestid")(contestname).exec();
    if(contestids.empty()){
        socket.write("ERROR NOSUCHCONTEST", '\n');
        return;
    }
    int contestid = contestids.begin()["contestid"].as<int>();
    conn.prepare("select variantid",
    	"SELECT variantid        "
    	"  FROM variants         "
    	" WHERE contestid = $1   "
    	"   AND shortname = $2   ");
    auto variantids = txn.prepared("select variantid")(contestid)(shortname).exec();
    if(variantids.empty()){
        socket.write("ERROR NOVARIANT", '\n');
        return;
    }
    int variantid = variantids.begin()["variantid"].as<int>();
    
    conn.prepare("participation info",
    	"SELECT *                  " 
    	"  FROM participations     "
    	" WHERE userid = $1        "
    	"   AND contestid = $2     "
    	"   AND is_moderator = true");
    auto infos = txn.prepared("participation info")(user["userid"].as<int>())(contestid).exec();
    if(infos.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    
    conn.prepare("select optionid",
    	"SELECT running_optionid    "
    	"  FROM running_options     "
    	" WHERE running_optionname= $1");
    auto optionids = txn.prepared("select optionid")(roname).exec();
    if(optionids.empty()){
        socket.write("ERROR NOOPTION", '\n');
        return;
    }
    int optionid = optionids.begin()["running_optionid"].as<int>();
    conn.prepare("add option to variant",
    	"INSERT INTO variants_running_options"
    	"     VALUES ($1, $2, $3)  ");
    txn.prepared("add option to variant")(optionid)(extension)(variantid).exec();
    socket.write("OK", '\n');
    txn.commit();
    return;
}

void viewlanguages(
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
    auto options = txn.exec("SELECT running_optionname FROM running_options");
    for(auto option : options){
    	socket.write("OPTION")
    		.write(option["running_optionname"].as<std::string>(), '\n');
    }
    socket.write("OK", '\n');
    return;
}
void viewresults(
        pqxx::result::tuple &user, 
        ssl_socket& socket, 
        pqxx::connection &conn){
    pqxx::work txn(conn);
    std::string contestname, shortname;
    socket.read(contestname).read(shortname);
    conn.prepare("variantid is_moderator", 
        "      SELECT variantid, is_moderator   " 
        "        FROM participations p          "
        "NATURAL JOIN contests c                "
        "        JOIN variants v                "
        "          ON c.contestid = v.contestid "
        "       WHERE userid = $1               "
        "         AND c.contestname = $2        "
        "         AND v.shortname = $3          ");
    int userid = user["userid"].as<int>();
    pqxx::result vis = txn.prepared("variantid is_moderator")(user["userid"].as<int>())(contestname)(shortname).exec();
    if(vis.empty() && !user["is_administrator"].as<bool>()){
        socket.write("ERROR NOPERMISSION", '\n');
        return;
    }
    bool is_moderator = vis.begin()["is_moderator"].as<bool>() || user["is_administrator"].as<bool>();
    int variantid = vis.begin()["variantid"].as<int>();
    pqxx::result results;
    conn.prepare("n tests",
        "SELECT COUNT(*) AS n_tests    "
        "FROM variants_testgroups      "
        "NATURAL JOIN testgroups_tests "
        "WHERE variantid = $1          "
    );
    int n_tests = txn.prepared("n tests")(variantid).exec().begin()["n_tests"].as<int>();
    if(is_moderator){
        conn.prepare("results", 
        "SELECT submissionid, username, status "
        "FROM results                          "
        "NATURAL JOIN submissions              "
        "NATURAL JOIN statuses                 "
        "NATURAL JOIN users                    "
        "WHERE variantid = $1                  "
        "ORDER BY submissionid DESC            ");
        results = txn.prepared("results")(variantid).exec();
    } else {
        conn.prepare("results", 
        "SELECT submissionid, username, status "
        "FROM results                          "
        "NATURAL JOIN submissions              "
        "NATURAL JOIN statuses                 "
        "NATURAL JOIN users                    "
        "WHERE variantid = $1                  "
        "AND userid = $2                       "
        "ORDER BY submissionid DESC            ");
        results = txn.prepared("results")(variantid)(user["userid"].as<int>()).exec();
    }
    int submissionid = -1, counter = 0;
    std::string username = "", status = "";
    socket.write("submit#", '\t').write("user", '\t').write("\tstatus", '\n');
    for(auto result : results){
    	int submissionid2 = result["submissionid"].as<int>();
    	if(submissionid != submissionid2){
    	    if(counter < n_tests)
    	        status = "QUE";
    	    if(submissionid != -1){
    	        socket.write(std::to_string(submissionid), '\t').write(username, '\t').write(status, '\n');
    	    }
    	    counter = 0;
    	    status = "OK";
    	    username = result["username"].as<std::string>();
    	}
    	submissionid = submissionid2;
    	if(result["status"].as<std::string>() != "OK")
    	    status = result["status"].as<std::string>();
    	counter++;
    }
    	    if(counter < n_tests)
    	        status = "QUE";
    	    if(submissionid != -1){
    	        socket.write(std::to_string(submissionid), '\t').write(username, '\t').write(status, '\n');
    	    }
    socket.write("OK", '\n');
    return;
}



void createcontest(
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
    std::string contestname, description;
    socket.read(contestname).readtext(description);
    conn.prepare("insert contest", 
        "  INSERT INTO contests(contestname, description) VALUES($1, $2)");
    txn.prepared("insert contest")(contestname)(description).exec();
    int contestid = txn.exec("SELECT currval('contests_contestid_seq') "
                             "    AS contestid                         "
                            ).begin()["contestid"].as<int>();
    conn.prepare("insert mod", 
        "  INSERT INTO participations VALUES($1, $2, 't')");
    txn.prepared("insert mod")(user["userid"].as<int>())(contestid).exec();
    
    socket.write("OK", '\n');
    txn.commit();
}
#define ADD_COMMAND(map, command) map[#command] = command
std::map<std::string, command_handler> command_handlers(){
    std::map<std::string, command_handler> result;
    ADD_COMMAND(result, submit);
    ADD_COMMAND(result, createuser);
    ADD_COMMAND(result, viewcontests);
    ADD_COMMAND(result, viewvariants);
    ADD_COMMAND(result, viewvariant);
    ADD_COMMAND(result, viewproblems);
    ADD_COMMAND(result, viewproblem);
    ADD_COMMAND(result, viewtestgroup);
    ADD_COMMAND(result, viewtest);
    ADD_COMMAND(result, addproblem);
    ADD_COMMAND(result, addtestgroup);
    ADD_COMMAND(result, addtest);
    ADD_COMMAND(result, addtesttogroup);
    ADD_COMMAND(result, addvariant);
    ADD_COMMAND(result, addgrouptovariant);
    ADD_COMMAND(result, removeproblem);
    ADD_COMMAND(result, removetestgroup);
    ADD_COMMAND(result, removetest);
    ADD_COMMAND(result, removetestfromgroup);
    ADD_COMMAND(result, removegroupfromvariant);
    ADD_COMMAND(result, addusertocontest);
    ADD_COMMAND(result, addlanguagetovariant);
    ADD_COMMAND(result, viewlanguages);
    ADD_COMMAND(result, viewresults);
    ADD_COMMAND(result, createcontest);
    return result;
}

