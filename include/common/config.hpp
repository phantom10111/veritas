#ifndef CONFIG_HPP
#define CONFIG_HPP
#include<cstdlib>
#include<string>
#include<vector>

const char* DB_CONN_INFO = "dbname=veritas";

typedef const char*        Host;
typedef unsigned short int Port;

const int checkers_no = 3;
Host checker_hosts[] = {"localhost", "localhost", "localhost"};
Port checker_ports[] = { 4711,       4712,        4713};

const int webservers_no = 3;
Host webserver_hosts[] = {"localhost", "localhost", "localhost"};
Port webserver_ports[] = { 4714,       4715,        4716};


#endif
