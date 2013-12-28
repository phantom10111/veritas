#include "resource_manager.hpp"
#include "config.hpp"
#include <pqxx/pqxx> 
#include <pqxx/binarystring>
#include <string>
#include <iostream>
#include <fstream>

void resource_manager::write_to_file(
    std::string filename, 
    pqxx::binarystring str
){
    std::ofstream filestream;
    filestream.open(filename.c_str(), std::ios::binary);
    filestream.write(str.str().c_str(), str.length());
    filestream.close();
}

submission_data resource_manager::get_submission_data(std::string submissionid){
    pqxx::connection conn(DB_CONN_INFO);
    pqxx::work txn(conn);
    std::string query;
    query += "SELECT testgroupid, \
                     testgroup_name, \
                     testid, \
                     test_name, \
                     infile, \
                     outfile, \
                     timelimit, \
                     memlimit \
                FROM submissions \
        NATURAL JOIN variants_testgroups \
        NATURAL JOIN testgroups \
        NATURAL JOIN testgroups_tests \
        NATURAL JOIN tests \
               WHERE submissionid = " + submissionid +
          " ORDER BY testgroupid";
    pqxx::result testgroups = txn.exec(query.c_str());
    
    std::vector<testgroup_data> testgroup_list;
    for(
        pqxx::result::const_iterator row = testgroups.begin();
        row != testgroups.end();
        ++row){
        std::string testgroupid      = row["testgroupid"].as<std::string>(),
                    testgroup_name   = row["testgroup_name"].as<std::string>(),
                    testid           = row["testid"].as<std::string>(),
                    test_name        = row["test_name"].as<std::string>(),
                    timelimit        = row["timelimit"].as<std::string>(),
                    memlimit         = row["memlimit"].as<std::string>(),
                    input_file_name  = testid + ".in",
                    output_file_name = testid + ".out";
        pqxx::binarystring infile (row["infile"]),
                           outfile(row["outfile"]);
        write_to_file(input_file_name, infile);
        write_to_file(output_file_name, outfile);
        std::cout << testgroupid      << " "
                  << testid           << " "
                  << test_name        << " " 
                  << input_file_name  << " " 
                  << output_file_name << " " 
                  << timelimit        << " "
                  << memlimit         << std::endl;
                  
        if(testgroup_list.empty() || 
            testgroup_list.back().testgroup_name != testgroup_name){
            testgroup_data testgroup(testgroup_name);
            testgroup_list.push_back(testgroup);
        }
        test_data test(
            testid, 
            test_name, 
            timelimit, 
            memlimit, 
            input_file_name, 
            output_file_name
       );
        testgroup_list.back().tests.push_back(test);
    }
    std::cout << std::endl;
    return submission_data("", "", std::vector<testgroup_data>());
}
