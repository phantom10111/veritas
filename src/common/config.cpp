#include "common/config.hpp"

const char* DB_CONN_INFO = "dbname=phantom";
const char* privkeyfile = "privkey.pem";
const char* certfile = "server.pem";
const char* dhfile = "dh512.pem";

const int n_checkers = 3;
const Host checker_hosts[] = {"localhost", "localhost", "localhost"};
const Port checker_ports[] = {4711, 4712, 4713};
