#ifndef TEST_DATA_HPP
#define TEST_DATA_HPP
#include <string>
class test_data {
public:
    test_data(
        std::string testid,
        std::string testname,
        std::string infilename,
        std::string outfilename,
        std::string timelimit,
        std::string memlimit
    ):
        testid(testid), 
        testname(testname), 
        infilename(infilename),
        outfilename(outfilename),
        timelimit(timelimit),
        memlimit(memlimit){};
    std::string testid;
    std::string testname;
    std::string infilename;
    std::string outfilename;
    std::string timelimit;
    std::string memlimit;
};
#endif
