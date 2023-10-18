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
   
    static int m_epollfd;      // all events on the sockets are registered under the same epoll object
    static int m_user_count;   // number of users
    static const int FILENAME_LEN = 200;         // the maximum length of filename
    static const int READ_BUFFER_SIZE = 2048;    // read buffer size
    static const int WRITE_BUFFER_SIZE = 1024;   // write buffer size

    // HTTP Request Method
    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT};
    
    // The state of the main state machine when parsing HTTP request
    enum CHECK_STATE { 
        CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    
    /*
        The possible responses after the server parses the HTTP request:
        NO_REQUEST          :   The request is incomplete; need to continue reading client data.
        GET_REQUEST         :   Indicates that a complete client request has been received.
        BAD_REQUEST         :   Indicates a syntax error in the client request.
        NO_RESOURCE         :   Indicates the server has no resources.
        FORBIDDEN_REQUEST   :   Indicates the client does not have sufficient access rights to the resource.
        FILE_REQUEST        :   File request; file retrieval successful.
        INTERNAL_ERROR      :   Indicates an internal server error.
        CLOSED_CONNECTION   :   Indicates the client has already closed the connection.
    */

    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    
    // the state of the side state machine (the state when parsing each line)
    // 1.get a complete line 2.error 3.the line data is incomplete
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
    http_conn() {};
    ~http_conn() {};

    void init(int sockfd, const sockaddr_in& addr);  // initialize new connection
    void close_conn();   // close connection
    bool read();   // read in non-blocking mode
    bool write();  // write in non-blocking mode
    void process();  // process request from client end

private:
    int m_sockfd;            // the socket connected with this HTTP
    sockaddr_in m_address;   // the address of the socket
    CHECK_STATE m_check_state;  // the current state of the main state machine

    // ---------------------------- read related variables ------------------------------
    char m_read_buf[READ_BUFFER_SIZE];   // read buffer
    int m_read_index;      // the position right AFTER the last byte of the content in read buffer

    int m_checked_index;   // the position of the byte being parsed currently
    int m_start_line;      // the starting position of the line being parsed currently
    
    // HTTP status line
    char* m_url;            // requested resource path
    char* m_version;        // HTTP version
    METHOD m_method;        // HTTP method
    char* m_host;           // the target host and the port where the request is being sent
    bool m_linger;          // it suggests whether the client would like to keep the connection open for potential further request

    // HTTP header
    int m_content_length;   // the length of HTTP request content
    
    // do_request
    char m_real_file[FILENAME_LEN];     // the complete path of the file requested by the client

    // ---------------------------- write related variables ------------------------------
    char m_write_buf[ WRITE_BUFFER_SIZE ];  // write buffer
    int m_write_index;                        // the number of bytes need to write in the buffer
    char* m_file_address;                   // the starting point for the target file requested by the client 'mmapped' in memory
    /*
        the state of the target file. we can determine:
            - the existance of teh file
            - whether it is a directory or not
            - whether it is readable or not
            - the size of the file
    */ 
    struct stat m_file_stat;                
    struct iovec m_iv[2];                   
    int m_iv_count;                        // the number of memory block being written

    void init();      // 初始化连接
    
    HTTP_CODE process_read();    // parse http request - the main state machine
    // The following functions are called by process_read() to parse HTTP request      
    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    LINE_STATUS parse_line();
    char* get_line() { return m_read_buf + m_start_line; }

    bool process_write( HTTP_CODE ret );    // generate http response 
    void unmap();
    // The following functions are called by process_write() to generate HTTP response
    
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
        // add_content_length, add_content_type, add_linger, add_blank_line are called by add_header
    bool add_content_length( int content_length );
    bool add_content_type();
    bool add_linger();
    bool add_blank_line();   
    bool add_content( const char* content );
    bool add_response( const char* format, ... );
    
};


#endif