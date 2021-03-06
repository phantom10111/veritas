#ifndef SUBMISSION_DATA_HPP
#define SUBMISSION_DATA_HPP
#include <vector>
#include <string>
#include "checker/testgroup_data.hpp"
class submission_data{
public:
    enum get_data_result{
        OK,
        NO_SUCH_SUBMISSION,
        NO_SUCH_EXTENSION,
        NO_TEST
    } result;
    submission_data(
        get_data_result result,
        std::string testid = "",
        std::string solution_filename = "",
        std::string compile_script_filename = "",
        std::string run_script_filename = "",
        std::string variantid = ""
    ):
        solution_filename(solution_filename),
        compile_script_filename(compile_script_filename),
        run_script_filename(run_script_filename),
        result(result),
        variantid(variantid),
        testid(testid){};
    std::string solution_filename;
    std::string compile_script_filename;
    std::string run_script_filename;
    std::string variantid;
    std::string testid;
};
#endif
