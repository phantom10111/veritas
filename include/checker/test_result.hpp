#ifndef TEST_RESULT_HPP
#define TEST_RESULT_HPP
class test_result {
public:
    test_result(
        std::string submissionid, 
        std::string testid, 
        std::string statusid, 
        std::string time
    ):
        submissionid(submissionid), 
        testid(testid), 
        statusid(statusid), 
        time(time){};
    std::string submissionid;
    std::string testid;
    std::string statusid;
    std::string time;
};
#endif
