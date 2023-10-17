#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
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

    static const int FILENAME_LEN = 200;         // the maximum length of filename
    static const int READ_BUFFER_SIZE = 2048;    // read buffer size
    static const int WRITE_BUFFER_SIZE = 1024;   // write buffer size

    // HTTP Request Method
    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT};
    
    /*
        解析客户端请求时，主状态机的状态
        The state of the main state machine when parsing HTTP request
        CHECK_STATE_REQUESTLINE:当前正在分析请求行
        CHECK_STATE_HEADER:当前正在分析头部字段
        CHECK_STATE_CONTENT:当前正在解析请求体
    */
    enum CHECK_STATE { 
        CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    
    /*
        服务器处理HTTP请求的可能结果，报文解析的结果
        NO_REQUEST          :   请求不完整，需要继续读取客户数据
        GET_REQUEST         :   表示获得了一个完成的客户请求
        BAD_REQUEST         :   表示客户请求语法错误
        NO_RESOURCE         :   表示服务器没有资源
        FORBIDDEN_REQUEST   :   表示客户对资源没有足够的访问权限
        FILE_REQUEST        :   文件请求,获取文件成功
        INTERNAL_ERROR      :   表示服务器内部错误
        CLOSED_CONNECTION   :   表示客户端已经关闭连接了
    */
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    
    // 从状态机的三种可能状态，即行的读取状态，分别表示
    // the state of the side state machine
    // 1.读取到一个完整的行 2.行出错 3.行数据尚且不完整
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
    http_conn() {};
    ~http_conn() {};

    void process();  // process request from client end
    
    void init(int sockfd, const sockaddr_in& addr);  // initialize new connection
    void close_conn();   // close connection
    bool read();   // read in non-blocking mode
    bool write();  // write in non-blocking mode

    



private:
    int m_sockfd;            // the socket connected with this HTTP
    sockaddr_in m_address;  // the address of the socket

    char m_read_buf[READ_BUFFER_SIZE];   // read buffer
    int m_read_index;    // the position right AFTER the last byte of the content in read buffer

    int m_checked_index;   // the position of the byte being parsed currently
    int m_start_line;     // the starting position of the line being parsed currently
    
    // HTTP request line
    char* m_url;        // requested resource path
    char* m_version;    // HTTP version
    METHOD m_method;    // HTTP method

    char* m_host;    // the target host and the port where the request is being sent
    bool m_linger;   // it suggests whether the client would like to keep the connection open for potential further request


    CHECK_STATE m_check_state;  // the current state of the main state machine


    void init();      // 初始化连接
    
    bool process_write( HTTP_CODE ret );    // fill http response  


    HTTP_CODE process_read();    // parse http request - the main state machine
    // The following function will be called by process_read() to parse HTTP request      
    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    LINE_STATUS parse_line();
    char* get_line() { return m_read_buf + m_start_line; }
};


#endif