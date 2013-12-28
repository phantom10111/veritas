#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP
#include "submission_data.hpp"
#include "test_result.hpp"
#include <pqxx/pqxx>
class resource_manager {
public:
    submission_data get_submission_data(std::string submissionid);
    void add_test_result(test_result result);
    void update_test(std::string testid);
private:
    void write_to_file(std::string filename, pqxx::binarystring str);
};
#endif