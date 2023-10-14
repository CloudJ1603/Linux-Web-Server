#include "http_conn.h"

static int http_conn::m_epollfd = -1;      // events from all sockets are registered to the same epoll object
static int http_conn::m_user_count = 0;    // number of users

// set FD as non-blocking
int setnonblocking(int fd) 
{
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
}


// add file descriptor which require listening to epoll
void addfd(int epollfd, int fd, bool one_shot)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP;

    /*

    */
    if(one_shot) {
        event.events | EPOLLONESHOT;
    }

    epoll_ctl(epollfd, EPOLL_CTR_ADD, fd, &event);
    // set up the fd as non-blocking
    setnonblocking(fd);

}

// remove file descriptor which require listening from epoll
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTR_DEL, fd, 0);
    clsoe(fd);
}

// modify file descriptor in epoll
// reset EPOLLONESHOT to ensure EPOLLIN even can be triggered next time
void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctr(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// initialize new connection
void http_conn::init(int sockfd, const sockaddr_int& addr) {
    m_sockfd = sockfd;
    m_address = addr;

    // set up port multiplexing
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // add to epoll object
    addfd(m_epollfd, sockfd, true);
    m_user_count++;


}

// close the connection
void http_conn::close_conn() {
    if(m_sockfd != -1) {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

// read in non-blocking mode
bool http_conn::read() {
    printf("read all data at once\n");
    return true;
}

// write in non-blocking mode
bool http_conn::write() {
    printf("write all data at once\n");
    return true;
}

// used by worker thread in the threadpool, to handle http request
void http_conn::process() {
    // parse http request
    printf("parse request, create response\n");
    // create response
}