#include "common/config.hpp"
#include "webserver/webserver.hpp"
int main(){
    webserver ws;
    ws.run(webserver_port);
}
