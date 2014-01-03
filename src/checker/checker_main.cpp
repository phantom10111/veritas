#include "config.hpp"
#include "checker_server.hpp"
int main(){
    checker_server srv;
    srv.run(checker_port);
    pause();
}
