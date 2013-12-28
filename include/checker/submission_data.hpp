#ifndef SUBMISSION_DATA_HPP
#define SUBMISSION_DATA_HPP
#include <vector>
#include <string>
#include "testgroup_data.hpp"
class submission_data{
public:
    submission_data(
        std::string compile_script_file_name,
        std::string run_script_file_name,
        std::vector<testgroup_data> testgroups
    ):
        compile_script_file_name(compile_script_file_name),
        run_script_file_name(run_script_file_name),
        testgroups(testgroups){};
    std::string compile_script_file_name;
    std::string run_script_file_name;
    std::vector<testgroup_data> testgroups;
};
#endif
