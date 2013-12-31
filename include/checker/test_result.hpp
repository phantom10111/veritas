#ifndef TEST_RESULT_HPP
#define TEST_RESULT_HPP
class test_result {
public:
    test_result(
        std::string submissionid, 
        std::string status, 
        std::string testid, 
        std::string time = "0"
    ):
        submissionid(submissionid), 
        testid(testid), 
        status(status), 
        time(time){};
    std::string submissionid;
    std::string testid;
    std::string status;
    std::string time;
};
#endif
