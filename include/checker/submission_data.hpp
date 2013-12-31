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
        std::string solution_filename = "" ,
        std::string compile_script_filename = "",
        std::string run_script_filename = "",
        std::vector<testgroup_data> testgroups = std::vector<testgroup_data>()
    ):
        solution_filename(solution_filename),
        compile_script_filename(compile_script_filename),
        run_script_filename(run_script_filename),
        testgroups(testgroups),
        result(result){};
    std::string solution_filename;
    std::string compile_script_filename;
    std::string run_script_filename;
    std::vector<testgroup_data> testgroups;
};
#endif
