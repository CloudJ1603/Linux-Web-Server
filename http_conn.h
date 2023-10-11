#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.h"
#include <sys/uio.h>

class http_conn {
public:
    // all events on the sockets are registered under the same epoll object
    static int m_epollfd;  
    static int m_user_count;   // number of users

    http_conn() {};
    ~http_conn() {};

    void process();  // process request from client end
    
    void init(int sockfd, const sockaddr_int& addr);  // initialize new connection
    void close_conn();

private:
    int m_sockfd;            // the socket connected with this HTTP
    sockaddr_in m_address;  // the address of the socket


};


#endif