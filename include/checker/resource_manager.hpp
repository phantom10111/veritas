#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP
#include "submission_data.hpp"
#include "test_result.hpp"
#include <pqxx/pqxx>
#include <vector>
class resource_manager {
public:
    submission_data get_submission_data(std::string submissionid);
    std::vector<testgroup_data> get_tests(std::string submissionid);
    void add_test_result(test_result result);
    void update_test(std::string testid);
private:
    void write_to_file(std::string filename, pqxx::binarystring str);
    pqxx::result select_submission(pqxx::connection &conn, 
                                    std::string submissionid);
    pqxx::result select_option(pqxx::connection &conn, 
                        std::string variantid, std::string extension);
    pqxx::result select_tests(pqxx::connection &conn, std::string variantid);
    pqxx::result select_solution(pqxx::connection &conn, 
                                    std::string submissionid);
};
#endif
