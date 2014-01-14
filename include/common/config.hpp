#ifndef CONFIG_HPP
#define CONFIG_HPP
#include<cstdlib>
#include<string>
#include<vector>

extern const char* DB_CONN_INFO;
extern const char* privkeyfile;
extern const char* certfile;
extern const char* dhfile;

typedef const char*        Host;
typedef unsigned short int Port;

extern const int n_checkers;
extern const Host checker_hosts[];
extern const Port checker_ports[];

const Port webserver_port = 4714;

const int MAX_SUBMIT_SIZE = 102400; //100kB
#endif
