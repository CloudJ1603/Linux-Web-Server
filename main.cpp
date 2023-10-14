#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>

#include "locker.h"
#include "threadpool.h"
#include "http_conn.h"


#define MAX_FD 65535            // the max number of file descriptor
#define MAX_EVENT_NUMBER 10000  // the max number of events 

void addsig(int sig, void(handler)(int))
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

// add file descriptor to epoll
extern void addfd(int epollfd, int fd, bool one_shot);
// delete file descriptor from epoll
extern void removefd(int epollfd, int fd);
// modify file descriptor in epoll
extern void modfd(int epollfd, int fd, int ev);


int main(int argc, char* argv[]) {
    if(argc <= 1) {
        printf("User Input should adhere to the following format: %s port_number\n", basename(argv[0]));
        exit(-1);
    }

    // Get the port number
    int port = atoi(argv[1]);

    // Process SIGPIPE signal
    /*
        When writing data to a socket, but the other end of the connection is closed unexpectedly, 
        writing to the socket may generate a 'SIGPIPE'. By setting 'SIGPIPE' to 'SIG_IGN', we can 
        handle the error gracefully rather than having the program terminate abruptly.

        SIGPIE: A signal generated when a process tried to write to a pipe or socket that has been closes.
                By default, when a process reeiveds a 'SIGPIPE' signal and does not handle it, the process is terminated.
        
        SIG_IGN: ignore
    */
    addsig(SIGPIPE, SIG_IGN);

    // create and initialize threadpool 
    threadpool<http_conn>* pool = NULL;
    try {
        pool = new threadpool<http_conn>;
    } catch (...) {
        exit(-1);
    }

    // create an array for 
    http_conn* users = new http_conn[MAX_FD];

    // create the socket for listening
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);

    // set multiplexing
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // bind
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    bind(listenfd, (struct sockaddr*)&address. sizeof(address));

    // listen
    listen(listenfd, 5);

    // create epoll object,
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);

    // add the listening file descriptor to the epoll object
    addfd(epollfd, listenfd, false);
    http_conn::m_epollfd = epollfd;

    while(true) {   
        // the number of events
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

        if(num < 0 &&  errno != EINTR) {
            printf("epoll failure\n");
            break;
        }

        // traverse event array 
        for(int i = 0; i < num; i++) {
            int sockfd = events[i].data.fd;
           
            if(sockfd == listenfd) {
            // there is client end connected 
                struct sockaddr_in client_address;
                socklen_t client_addrlen = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlen);
                
                // current connections reach the upper bound
                if(http_conn::m_user_count >= MAX_FD) {

                    // need to give client a message saying the server is busy now
                    close(connfd);
                    continue;
                }

                // initialize the new client's data, put into user array
                users[connfd].init(connfd, client_address);

            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                // 
                users[sockfd].close_conn();
            } else if(events[i].events & EPOLLIN) {
                // read all the user data at once
                if(users[sockfd].read()) {
                    pool->append(users + sockfd);
                } else {
                    users[sockfd].close_conn();
                }
            } else if(events[i].events & EPOLLOUT) {
                // write all the user data at once
                if( !users[sockfd].write() ) {   // if fail to write, close connection
                    users[sockfd].close_conn();
                }
            }
        }
    }

    close(epollfd);
    close(listenfd);
    delete [] users;
    delete pool;
    






    return 0;
}