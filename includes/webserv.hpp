#pragma once

#include <iostream>
#include <ctime>
#include <exception>
#include <string>
#include <sstream>
#include <unistd.h>      // execve, dup, dup2, pipe, fork, chdir, access, read, write, close
#include <cstring>      // strerror, gai_strerror
#include <cerrno>       // errno
#include <sys/types.h>  // socket, accept, listen, send, recv, bind, connect, setsockopt, getsockname, getprotobyname, fcntl, stat, opendir, readdir, closedir
#include <sys/socket.h> // socket, accept, listen, send, recv, bind, connect, setsockopt, getsockname, socketpair
#include <netinet/in.h> // htons, htonl, ntohs, ntohl
#include <netdb.h>      // getaddrinfo, freeaddrinfo, getprotobyname
#include <sys/select.h> // select
#include <poll.h>       // poll
#include <signal.h>     // signal, kill
#include <sys/wait.h>   // waitpid
#include <fcntl.h>      // fcntl
#include <dirent.h>     // opendir, readdir, closedir
#include <sys/stat.h>   // stat
#include <fstream>      // ifstream
#include <sstream>      // istringstream
#include <cstdlib>      // atoi
#include <map>
#include <vector>
#include <deque>
#include <list>
#include <iomanip>
#include <sys/epoll.h>  
#include <algorithm>
#include <arpa/inet.h>
#include <limits>
#include <ctime>

#include "request.hpp"
#include "Response.hpp"
#include "config.hpp"
#include "_Get.hpp"
#include "cgi.hpp"
#include "client.hpp"
#include "delete.hpp"

#define SEND_BUFFER_SIZE 2048
#define TIMEOUT 30
#define ERROR_FILE "logs/error.log"



using namespace std;



extern vector <Serv> servers;


void _Create_Servers();
void _Run_Server();
std::vector<std::string> split_string_with_multiple_delemetres(std::string &str, std::string delimiters);
std::string get_extention(Request &req);
std::string generate_random_name();
Response *createResponse(int code, Request *req);
location get_location(std::string file_name, Serv &config);
string get_CGI_script(std::string file_name, size_t index, size_t start_pos);
bool is_CGI(std::string file_name, size_t index, size_t start_pos);
Response *create_redirection(location &loc, Request &req);
int get_server_index_(string &host, int fd);
std::string trim(std::string& str);
