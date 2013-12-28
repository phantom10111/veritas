#ifndef TEST_DATA_HPP
#define TEST_DATA_HPP
#include <string>
class test_data {
public:
    test_data(
        std::string testid,
        std::string test_name,
        std::string input_file_name,
        std::string output_file_name,
        std::string timelimit,
        std::string memlimit
    ):
        testid(testid), 
        test_name(test_name), 
        input_file_name(input_file_name),
        output_file_name(output_file_name),
        timelimit(timelimit),
        memlimit(memlimit){};
    std::string testid;
    std::string test_name;
    std::string input_file_name;
    std::string output_file_name;
    std::string timelimit;
    std::string memlimit;
};
#endif
