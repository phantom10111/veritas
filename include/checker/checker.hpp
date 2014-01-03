#ifndef CHECKER_HPP
#define CHECKER_HPP
#include "checker/submission_data.hpp"
#include "checker/test_result.hpp"
class checker {
public:
    int compile(
        std::string const &compile_script_filename, 
        std::string const &solution_filename, 
        std::string const &compiled_filename
    );
    test_result test_solution(
        std::string const &submissionid, 
        std::string const &solution_filename,
        std::string const &run_script_filename, 
        test_data const &test
    );
};
#endif
