#include "checker/checker_server.hpp"
#include "common/config.hpp"
#include <thread>
using namespace boost::asio;

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
    ssl::context ctx(ssl::context::tlsv1_server);
    ctx.set_options(ssl::context::default_workarounds
        | ssl::context::no_sslv2);
    ctx.set_verify_mode(ssl::verify_none);
    ctx.use_private_key_file(privkeyfile, ssl::context::pem);
    ctx.use_certificate_chain_file(certfile);
    while(true){
      try
      {
        auto stream = 
            new ssl::stream<ip::tcp::socket>(ios, ctx);
        acceptor.accept(stream->lowest_layer());
        ssl_socket *socket = new ssl_socket(stream, ssl::stream<ip::tcp::socket>::server);
        std::string command;
        socket->read(command, '\n');
        if(command == "QSIZE"){
            int qsize;
            {
                std::unique_lock<std::mutex> lock(_mutex);
                qsize = _queue.size();
            }
            std::ostringstream ss;
            ss << qsize;
            socket->write(ss.str(), '\n');
            delete socket;
        } else if(command == "TEST"){
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _queue.push(socket);
            }
            _condition.notify_one();
        } else {
            delete socket;
        }
      } catch(...) {} //TODO delete socket properly on exception thrown
    }
}
void checker_server::checker_thread(){
    while(true){
      try
      {
        ssl_socket *stream;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _condition.wait(lock, [=]{ return !_queue.empty(); });
            stream = _queue.back();
            _queue.pop();
        }
        std::string submissionid;
        stream->read(submissionid, '\n');
        submission_data submission = 
            _resource_manager.get_submission_data(submissionid);
        switch(submission.result){
        case submission_data::NO_SUCH_SUBMISSION:
            stream->write("END NO SUCH SUBMISSION", '\n');
            break;
        case submission_data::NO_SUCH_EXTENSION:
            _resource_manager.add_test_result(test_result(submissionid, "EXT", 
            submission.testid));
            stream->write("END EXT", '\n');
            break;
        case submission_data::NO_TEST:
            stream->write("END NO TEST", '\n');
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
                    "CME", submission.testid));
                stream->write("END CME", '\n');
                break;
            }
            auto testgroups = _resource_manager.get_tests(submission.variantid);
            std::string end_status = "OK";
            for(auto testgroup : testgroups){
                stream->write("TESTGROUP") 
                       .write(testgroup.testgroupname, '\n'); //TODO replace ' 's in
                                                              //testgroupname to '_'s
                for(auto test : testgroup.tests){
                    stream->write("\tTEST")
                           .write(test.testname, '\n');       //TODO same as above
                    auto result = _checker.test_solution(
                        submissionid,
                        compiled_filename,
                        submission.run_script_filename,
                        test
                    );
                    stream->write("\tRESULT")
                           .write(result.status)
                           .write(result.time, '\n')
                           .write("", '\n');
                _resource_manager.add_test_result(test_result(submissionid, 
                    result.status, result.testid));
                    if(result.status != "OK"){
                        end_status = result.status;
                        goto outside;
                    }
                }
            }
            outside:
            stream->write("END RESULT")
                   .write(end_status, '\n');
            break;
        }
        delete stream;
      } catch(...) {} //TODO: delete stream properly on exception thrown
    }
}
