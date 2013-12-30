#ifndef SUBMISSION_DATA_HPP
#define SUBMISSION_DATA_HPP
#include <vector>
#include <string>
#include "testgroup_data.hpp"
class submission_data{
public:
    enum get_data_result{
        OK,
        NO_SUCH_SUBMISSION,
        NO_SUCH_EXTENSION
    } result;
    submission_data(
        get_data_result result,
        std::string compile_script_file_name = "",
        std::string run_script_file_name = "",
        std::vector<testgroup_data> testgroups = std::vector<testgroup_data>()
    ):
        compile_script_file_name(compile_script_file_name),
        run_script_file_name(run_script_file_name),
        testgroups(testgroups),
        result(result){};
    std::string compile_script_file_name;
    std::string run_script_file_name;
    std::vector<testgroup_data> testgroups;
};
#endif
