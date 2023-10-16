#include "http_conn.h"

// Alias
using METHOD = http_conn::METHOD;
using CHECK_STATE = http_conn::CHECK_STATE;
using HTTP_CODE = http_conn::HTTP_CODE;
using LINE_STATUS = http_conn::LINE_STATUS;


int http_conn::m_epollfd = -1;      // events from all sockets are registered to the same epoll object
int http_conn::m_user_count = 0;    // number of users

// set FD as non-blocking
int setnonblocking(int fd) 
{
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
    return old_flag;
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

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    // set up the fd as non-blocking
    setnonblocking(fd);

}

// remove file descriptor which require listening from epoll
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

// modify file descriptor in epoll
// reset EPOLLONESHOT to ensure EPOLLIN even can be triggered next time
void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    // event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    event.events = ev | EPOLLONESHOT | EPOLLET | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// initialize new connection
void http_conn::init(int sockfd, const sockaddr_in& addr) {
    m_sockfd = sockfd;
    m_address = addr;

    // set up port multiplexing
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // add to epoll object
    addfd(m_epollfd, sockfd, true);
    m_user_count++;
    init();

}

void http_conn::init() {
    m_check_state = http_conn::CHECK_STATE_REQUESTLINE;   // initialize the state of the main state machine 
    m_checked_index = 0;
    m_start_line = 0;
    m_read_index = 0;
}

// close the connection
void http_conn::close_conn() {
    if(m_sockfd != -1) {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

// read in non-blocking mode, keep looping until there is no more data or the connection is closd
bool http_conn::read() {
    // printf("read all data at once\n");

    // the read buffer is full
    if(m_read_index >= READ_BUFFER_SIZE) {
        return false;
    }

    int bytes_read = 0;    // number of bytes that have been read
    while(true) {


        /*
            read from socket 'm_sockfd' to buffer pointed to by 'm_read_buf + m_read_index'
            READ_BUFFER_SIZE - m_read_index: the maximum number of bytes that can be received
        */
        bytes_read = recv(m_sockfd, m_read_buf + m_read_index, READ_BUFFER_SIZE - m_read_index, 0);

        // return -1 : an error occurs
        if (bytes_read == -1) {
            if( errno == EAGAIN || errno == EWOULDBLOCK ) {  // there is no more to read
                break;
            }
            return false;   
        } else if (bytes_read == 0) {   // the connection is closed
            return false;
        }

        m_read_index += bytes_read;  
    }

    printf("data retrived: %s\n", m_read_buf);
    return true;
}

// write in non-blocking mode
bool http_conn::write() {
    printf("write all data at once\n");
    return true;
}

// the main state machine
HTTP_CODE http_conn::process_read() {

    LINE_STATUS line_status = http_conn::LINE_OK;     // initialize the state of the side state machine
    HTTP_CODE ret = http_conn::NO_REQUEST;            // initialize the return value

    char* text = 0;

    /*

    */
    while (((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK))
                || ((line_status = parse_line()) == LINE_OK)) {
        // 获取一行数据
        text = get_line();
        m_start_line = m_checked_index;
        printf( "got 1 http line: %s\n", text );

        switch ( m_check_state ) {
            case CHECK_STATE_REQUESTLINE: {
                ret = parse_request_line( text );
                if ( ret == BAD_REQUEST ) {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER: {
                ret = parse_headers( text );
                if ( ret == BAD_REQUEST ) {
                    return BAD_REQUEST;
                } else if ( ret == GET_REQUEST ) {
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT: {
                ret = parse_content( text );
                if ( ret == GET_REQUEST ) {
                    return do_request();
                }
                line_status = LINE_OPEN;
                break;
            }
            default: {
                return INTERNAL_ERROR;
            }
        }
    }
    return NO_REQUEST;
}

// The following function will be called by process_read() to parse HTTP request      
HTTP_CODE http_conn::parse_request_line( char* text ) {
    return http_conn::NO_REQUEST;
}

HTTP_CODE http_conn::parse_headers( char* text ) {
    return http_conn::NO_REQUEST;
}

HTTP_CODE http_conn::parse_content( char* text ) {
    return http_conn::NO_REQUEST;
}

HTTP_CODE http_conn::do_request() {
    return http_conn::NO_REQUEST;
}

// 
LINE_STATUS http_conn::parse_line() {
    return http_conn::LINE_OK;
}

// used by worker thread in the threadpool, to handle http request
void http_conn::process() {
    // parse http request
    printf("parse request, create response\n");

    // parse http request
    // HTTP_CODE read_ret = process_read();
    // if ( read_ret == NO_REQUEST ) {
    //     modfd( m_epollfd, m_sockfd, EPOLLIN );
    //     return;
    // }
    
    // // generate http response
    // bool write_ret = process_write( read_ret );
    // if ( !write_ret ) {
    //     close_conn();
    // }
    // modfd( m_epollfd, m_sockfd, EPOLLOUT);
}