#ifndef CONFIG_HPP
#define CONFIG_HPP
#include<cstdlib>
#include<string>
#include<vector>

extern const char* DB_CONN_INFO;

typedef const char*        Host;
typedef unsigned short int Port;

extern const int n_checkers;
extern const Host checker_hosts[];
extern const Port checker_ports[];


#endif
