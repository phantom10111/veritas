#include "checker/checker_server.hpp"
#include <thread>

void checker_server::run(int port){
    std::thread acceptor(&checker_server::acceptor_thread, this, port);
    std::thread checker(&checker_server::checker_thread, this);
    acceptor.join();
    checker.join();
}
    
void checker_server::acceptor_thread(int port){
    boost::asio::io_service ios;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
    boost::asio::ip::tcp::acceptor acceptor(ios, endpoint);
    while(true){
        boost::asio::ip::tcp::iostream *stream = 
            new boost::asio::ip::tcp::iostream();
        acceptor.accept(*(*stream).rdbuf());
        std::string command;
        (*stream) >> command;
        if(command == "QSIZE"){
            int qsize;
            {
                std::unique_lock<std::mutex> lock(_mutex);
                qsize = _queue.size();
            }
            (*stream) << qsize << std::endl;
            delete stream;
        } else if(command == "TEST"){
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _queue.push(stream);
            }
            _condition.notify_one();
        } else {
            delete stream;
        }
    }
}
void checker_server::checker_thread(){
    while(true){
        boost::asio::ip::tcp::iostream *stream;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _condition.wait(lock, [=]{ return !_queue.empty(); });
            stream = _queue.back();
            _queue.pop();
        }
        std::string submissionid;
        (*stream) >> submissionid;
        submission_data submission = 
            _resource_manager.get_submission_data(submissionid);
        switch(submission.result){
        case submission_data::NO_SUCH_SUBMISSION:
            (*stream) << "NOSUCHSUBMISSION" << std::endl;
            break;
        case submission_data::NO_SUCH_EXTENSION:
            _resource_manager.add_test_result(test_result(submissionid, "EXT", 
            "-1"));//TODO change -1 to actual testid (any testid)
            (*stream) << "RESULT EXT" << std::endl;
            break;
        case submission_data::OK:
            auto compiled_filename = submissionid;
            int compilation_status = _checker.compile(
                submission.compile_script_filename, 
                submission.solution_filename,
                compiled_filename
            );
            if(compilation_status){
                _resource_manager.add_test_result(test_result(submissionid, 
                    "CME", "-1"));//TODO change -1 to actual testid (any testid)
                (*stream) << "RESULT CME" 
                          << std::endl;
            }
            auto testgroups = _resource_manager.get_tests(submission.variantid);
            std::string end_status = "OK";
            for(auto testgroup : testgroups){
                (*stream) << "TESTGROUP " 
                          << testgroup.testgroupname //TODO replace ' 's in
                                                     //testgroupname to '_'s
                          << std::endl;
                for(auto test : testgroup.tests){
                    (*stream) << "\tTEST "
                              << test.testname       //TODO same as above
                              << std::endl;
                    auto result = _checker.test_solution(
                        submissionid,
                        compiled_filename,
                        submission.run_script_filename,
                        test
                    );
                    (*stream) << "\tRESULT "
                              << result.status
                              << " "
                              << result.time
                              << std::endl
                              << std::endl;
                _resource_manager.add_test_result(test_result(submissionid, 
                    result.status, result.testid));
                    if(result.status != "OK"){
                        end_status = result.status;
                        goto outside;
                    }
                }
            }
            outside:
            (*stream) << "END RESULT "
                      << end_status
                      << std::endl;
            break;
        }
        delete stream;
    }
}
