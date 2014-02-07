#include "checker/resource_manager.hpp"
#include "common/config.hpp"
#include <pqxx/pqxx> 
#include <pqxx/binarystring>
#include <string>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <cstdio>

submission_data resource_manager::get_submission_data(
    std::string submissionid
){
    pqxx::connection conn(DB_CONN_INFO);
    
    auto submission = select_submission(conn, submissionid);
    
    if(submission.empty())
        return submission_data(submission_data::NO_SUCH_SUBMISSION);
    
    auto solution = select_solution(conn, submissionid);
    
    if(solution.empty())
        return submission_data(submission_data::NO_SUCH_SUBMISSION);
    
    auto row = submission.begin();
    std::string variantid = row["variantid"].as<std::string>(),
                extension = row["extension"].as<std::string>();
    
    pqxx::result testids = select_any_test(conn, variantid);
    if(testids.empty()){
        return submission_data(submission_data::NO_TEST);
    }
    std::string testid = testids.begin()["testid"].as<std::string>();
    pqxx::result option = select_option(conn, variantid, extension);
    
    if(option.empty())
        return submission_data(submission_data::NO_SUCH_EXTENSION, testid);
    
    row = option.begin();
    std::string running_optionid = row["running_optionid"].as<std::string>();
    pqxx::binarystring compile_script(row["compile_script"]),
                       run_script    (row["run_script"]);
                       
    std::string compile_script_filename = std::string("./compile") + running_optionid;
    std::string run_script_filename = std::string("./run") + running_optionid;
    
    write_to_file(compile_script_filename, compile_script);
    write_to_file(run_script_filename, run_script);

    chmod(compile_script_filename.c_str(), S_IRWXU);
    chmod(run_script_filename.c_str(), S_IRWXU);
    
    row = solution.begin(); 
    pqxx::binarystring solution_file(row["file"]);
    std::string solution_filename = submissionid + "." + extension;
    
    write_to_file(solution_filename, solution_file);
        
    return submission_data(submission_data::OK, testid, solution_filename, 
                compile_script_filename, run_script_filename, variantid);
}

void resource_manager::cleanup_submission_data(const submission_data &submission)
{
	remove(submission.compile_script_filename.c_str());
	remove(submission.run_script_filename.c_str());
	remove(submission.solution_filename.c_str());
}

std::vector<testgroup_data> resource_manager::get_tests(std::string variantid){
    
    pqxx::connection conn(DB_CONN_INFO);
    
    pqxx::result tests = select_tests(conn, variantid);
    std::vector<testgroup_data> testgroup_vector;
    
    for(auto row = tests.begin(); row != tests.end(); row++){
        std::string testgroupname = row["testgroupname"].as<std::string>(),
                    testid        = row["testid"].as<std::string>(),
                    testname      = row["testname"].as<std::string>(),
                    timelimit     = row["timelimit"].as<std::string>(),
                    memlimit      = row["memlimit"].as<std::string>(),
                    infilename    = testid + ".in",
                    outfilename   = testid + ".out";
        
        pqxx::binarystring infile(row["infile"]),
                           outfile(row["outfile"]);
        if(testgroup_vector.empty() 
            || testgroup_vector.back().testgroupname != testgroupname)
            testgroup_vector.emplace_back(testgroupname);
        
        write_to_file(infilename, infile);
        write_to_file(outfilename, outfile);
        testgroup_vector.back().tests.emplace_back(testid, testname, 
            infilename, outfilename, timelimit, memlimit);
    }
    return testgroup_vector;
}

void resource_manager::cleanup_tests(const std::vector<testgroup_data> &vec)
{
	for(auto testgroup : vec)
		for(auto test : testgroup.tests)
		{
			remove(test.infilename.c_str());
			remove(test.outfilename.c_str());
		}
}

void resource_manager::add_test_result(test_result result){
    pqxx::connection conn(DB_CONN_INFO);
    pqxx::work txn(conn);
    conn.prepare("statusids", 
        "SELECT statusid    "
        "  FROM statuses    "
        " WHERE status = $1 "
    );
    pqxx::result statusids = txn.prepared("statusids")(result.status).exec();
    if(statusids.empty()){
        statusids = txn.prepared("statusids")("UNK").exec();
    }
    int statusid = statusids.begin()["statusid"].as<int>();
    conn.prepare("insert result",
        "INSERT INTO results(submissionid, testid, statusid, time)"
        "     VALUES ($1, $2, $3, $4)                             "
    );
    txn.prepared("insert result")(result.submissionid)(result.testid)
        (statusid)(result.time).exec();
    txn.commit();
}

void resource_manager::write_to_file(
    std::string filename, 
    pqxx::binarystring str
){
    std::ofstream filestream;
    filestream.open(filename.c_str(), std::ios::binary);
    filestream.write(str.str().c_str(), str.length());
    filestream.close();
}


pqxx::result resource_manager::select_submission(
    pqxx::connection &conn, 
    std::string submissionid
){
    pqxx::work txn(conn);
    std::string query = 
        std::string() + "SELECT variantid, extension \
                           FROM submissions \
                          WHERE submissionid = "+ submissionid +";";
    return txn.exec(query);
}
pqxx::result resource_manager::select_option(
    pqxx::connection &conn, 
    std::string variantid, 
    std::string extension
){
    pqxx::work txn(conn);
    std::string query = 
        std::string() + "SELECT running_optionid, compile_script, run_script \
                           FROM running_options \
                   NATURAL JOIN variants_running_options \
                          WHERE variantid = "+ variantid +" \
                            AND extension = '"+ extension +"';";
    return txn.exec(query);
    
}
pqxx::result resource_manager::select_tests(
    pqxx::connection &conn,     
    std::string variantid){
    pqxx::work txn(conn);
    std::string query = 
        std::string() + "SELECT testgroupname, testid, testname, \
                                    timelimit, memlimit, infile, outfile\
                           FROM variants_testgroups \
                   NATURAL JOIN testgroups \
                   NATURAL JOIN testgroups_tests \
                   NATURAL JOIN tests \
                          WHERE variantid = "+ variantid +" \
                       ORDER BY testgroupname;";
    return txn.exec(query);

}
pqxx::result resource_manager::select_solution(pqxx::connection &conn, 
                                std::string submissionid){
    
    pqxx::work txn(conn);
    std::string query = 
        std::string() + "SELECT file\
                           FROM submissions \
                          WHERE submissionid = "+ submissionid +";";
    return txn.exec(query);
}
pqxx::result resource_manager::select_any_test(pqxx::connection &conn,
                                    std::string variantid){
    pqxx::work txn(conn);
    conn.prepare("any testid",
        "      SELECT testid              "
        "        FROM variants_testgroups "
        "NATURAL JOIN testgroups          "
        "NATURAL JOIN testgroups_tests    "
        "NATURAL JOIN tests               "
        "       WHERE variantid = $1      "
        "       LIMIT 1                   "
    );
    return txn.prepared("any testid")(variantid).exec();
}
