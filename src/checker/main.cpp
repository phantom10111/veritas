#include "common/config.hpp"
#include "checker/checker_server.hpp"
#include <cstdlib>
#include <iostream>
int main(int argc, char *argv[]){
    if(argc != 2){
        std::cerr << "Usage: checker <checker_no>\n";
        return 1;
    }
    checker_server srv;
    int checker_no = std::atoi(argv[1]);
    srv.run(checker_ports[checker_no]);
    pause();
}
