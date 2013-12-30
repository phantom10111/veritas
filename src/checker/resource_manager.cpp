#include "resource_manager.hpp"
#include "config.hpp"
#include <pqxx/pqxx> 
#include <pqxx/binarystring>
#include <string>
#include <iostream>
#include <fstream>

submission_data resource_manager::get_submission_data(
    std::string submissionid
){
    auto submission = select_submission(submissionid);
    
    if(submission.empty())
        return submission_data(submission_data::NO_SUCH_SUBMISSION);
    
    auto row = submission.begin();
    std::string variantid = row["variantid"].as<std::string>(),
                extension = row["extension"].as<std::string>();
    
    
    pqxx::result option = select_option(variantid, extension);
    
    if(option.empty())
        return submission_data(submission_data::NO_SUCH_EXTENSION);
    
    row = option.begin();
    std::string running_optionid = row["running_optionid"].as<std::string>();
    pqxx::binarystring compile_script(row["compile_script"]),
                       run_script    (row["run_script"]);
                       
    std::string compile_script_name = std::string("compile") + running_optionid;
    std::string run_script_name = std::string("run") + running_optionid;
    
    write_to_file(compile_script_name, compile_script);
    write_to_file(run_script_name, run_script);
    
    
    
    pqxx::result tests = select_tests(variantid);
    std::vector<testgroup_data> testgroup_vector;
    
    for(row = tests.begin(); row != tests.end(); row++){
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
        
    return submission_data(submission_data::OK, compile_script_name, 
                run_script_name, testgroup_vector);
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
    std::string submissionid
){
    pqxx::connection conn(DB_CONN_INFO);
    pqxx::work txn(conn);
    std::string query = 
        std::string() + "SELECT variantid, extension \
                           FROM submissions \
                          WHERE submissionid = "+ submissionid +";";
    return txn.exec(query);
}
pqxx::result resource_manager::select_option(
    std::string variantid, 
    std::string extension
){
    pqxx::connection conn(DB_CONN_INFO);
    pqxx::work txn(conn);
    std::string query = 
        std::string() + "SELECT running_optionid, compile_script, run_script \
                           FROM running_options \
                   NATURAL JOIN variants_running_options \
                          WHERE variantid = "+ variantid +" \
                            AND extension = '"+ extension +"';";
    return txn.exec(query);
    
}
pqxx::result resource_manager::select_tests(std::string variantid){
    pqxx::connection conn(DB_CONN_INFO);
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
