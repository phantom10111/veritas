#ifndef TESTGROUP_DATA_HPP
#define TESTGROUP_DATA_HPP
#include <vector>
#include <string>

#include "test_data.hpp"

class testgroup_data {
public:
    testgroup_data(
        std::string testgroupname,
        std::vector<test_data> tests = std::vector<test_data>()
    ):
        testgroupname(testgroupname), 
        tests(tests){};
    std::string testgroupname;
    std::vector<test_data> tests;
};
#endif
