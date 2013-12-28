#ifndef TESTGROUP_DATA_HPP
#define TESTGROUP_DATA_HPP
#include <vector>
#include <string>

#include "test_data.hpp"

class testgroup_data {
public:
    testgroup_data(
        std::string testgroup_name,
        std::vector<test_data> tests = std::vector<test_data>()
    ):
        testgroup_name(testgroup_name), 
        tests(tests){};
    std::string testgroup_name;
    std::vector<test_data> tests;
};
#endif
