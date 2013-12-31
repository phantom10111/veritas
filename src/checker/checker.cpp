
#include "checker.hpp"

int checker::compile(
    std::string const &compile_script_filename, 
    std::string const &solution_filename, 
    std::string const &compiled_filename
){
    return 0;
}
test_result checker::test_solution(
    std::string const &submissionid, 
    std::string const &solution_filename,
    std::string const &run_script_filename, 
    test_data const &test){
    return test_result(submissionid, test.testid, "OK", "0.00");
}
