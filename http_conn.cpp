#include "http_conn.h"

// Alias
using METHOD = http_conn::METHOD;
using CHECK_STATE = http_conn::CHECK_STATE;
using HTTP_CODE = http_conn::HTTP_CODE;
using LINE_STATUS = http_conn::LINE_STATUS;

// 定义HTTP响应的一些状态信息
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";

// root directory of the website
const char* doc_root = "/home/francis/Linux-Web-Server/resources";

int http_conn::m_epollfd = -1;      // events from all sockets are registered to the same epoll object
int http_conn::m_user_count = 0;    // number of users

// set FD as non-blocking
int setnonblocking(int fd) {
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
    return old_flag;
}


// add file descriptor which require listening to epoll
void addfd(int epollfd, int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP;

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
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
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
    // initialize the state of the main state machine 
    m_check_state = http_conn::CHECK_STATE_REQUESTLINE;  
    
    // initialize the some indices for different positions in the read buffer
    m_checked_index = 0;
    m_start_line = 0;
    m_read_index = 0;
    m_write_index = 0;

    // initialize the HTTP Request Line data
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_linger = false;
    m_content_length = 0;
    m_host = 0;


    // set 'READ_BUFFER_SIZE' bytes of 'm_read_buf' to 0
    bzero(m_read_buf, READ_BUFFER_SIZE);
    bzero(m_write_buf,WRITE_BUFFER_SIZE);
    bzero(m_real_file,FILENAME_LEN);
}

// close the connection
void http_conn::close_conn() {
    if(m_sockfd != -1) {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}


// write in non-blocking mode
bool http_conn::write()
{

    // printf("write all data at once\n");
    // return true;
    int temp = 0;
    int bytes_have_send = 0;    
    int bytes_to_send = m_write_index;
    
    if ( bytes_to_send == 0 ) {
        /*
            Calls 'modfd' to modify the file descriptor in the epoll event to wait for incoming data.
            This essentially sets the server up to listen for the next request from the same client
        */
        modfd( m_epollfd, m_sockfd, EPOLLIN ); 
        init();     // reset the internal state of the 'http_conn' object                             
        return true;
    }

    while(true) {
        /*
            Uses 'writev' to perform scatter-write operation. 'writev' is used to write data from 
            multiple buffers ('m_iv' in this case) into a single file descriptor ('m_sockfd'). This
            is often used for efficiency when writing data from different memory locations.
        */
        temp = writev(m_sockfd, m_iv, m_iv_count);
        if ( temp <= -1 ) {  
            /*
                If the error is 'EAGAIN', it means that the TCP write buffer has no space at the moment, 
                so it modifies the file descriptor in the epoll event to wait for the socket to become writable (EPOLLOUT).
                This allows the server to wait for the next opportunity to send data when the TCP buffer has space.
                The server cannot immidiately receive the request from the next client. 
                If it encounters any other error, call unmap() to release any resources associated memory mapping.
            */
            if( errno == EAGAIN ) {
                modfd( m_epollfd, m_sockfd, EPOLLOUT );
                return true;
            }
            unmap();
            return false;
        }

        bytes_to_send -= temp;
        bytes_have_send += temp;
        if ( bytes_to_send <= bytes_have_send ) {
            unmap();
            /*
                Depending on the value of m_linger, it decides whether to keep the connection open for potential 
                future request or close it.
                    - m_linger is true: call init() to reset the internal state, modify the epoll event to wait for incoming data
                    - m_linger is false: modify the epoll event to wait for incoming data, return false to indicate the connection should be closed
            */
            if(m_linger) {
                init();
                modfd( m_epollfd, m_sockfd, EPOLLIN );
                return true;
            } else {
                modfd( m_epollfd, m_sockfd, EPOLLIN );
                return false;
            } 
        }
    }
}

// unmap the memory mapping 
void http_conn::unmap() {
    if( m_file_address )
    {
        munmap( m_file_address, m_file_stat.st_size );
        m_file_address = 0;
    }
}

// Generate an HTTP response based on the given HTTP_CODE (the result of processing the request)
bool http_conn::process_write(HTTP_CODE ret) {
    // set up the status line, headers and content (if appicable) for the response
    switch (ret)
    {
        case INTERNAL_ERROR:
            add_status_line( 500, error_500_title );
            add_headers( strlen( error_500_form ) );
            if ( ! add_content( error_500_form ) ) {
                return false;
            }
            break;
        case BAD_REQUEST:
            add_status_line( 400, error_400_title );
            add_headers( strlen( error_400_form ) );
            if ( ! add_content( error_400_form ) ) {
                return false;
            }
            break;
        case NO_RESOURCE:
            add_status_line( 404, error_404_title );
            add_headers( strlen( error_404_form ) );
            if ( ! add_content( error_404_form ) ) {
                return false;
            }
            break;
        case FORBIDDEN_REQUEST:
            add_status_line( 403, error_403_title );
            add_headers(strlen( error_403_form));
            if ( ! add_content( error_403_form ) ) {
                return false;
            }
            break;
        case FILE_REQUEST:
            add_status_line(200, ok_200_title );
            add_headers(m_file_stat.st_size);
            // sets up two 'iovec' structure to send both the headers and the file content
            m_iv[ 0 ].iov_base = m_write_buf;         // header
            m_iv[ 0 ].iov_len = m_write_index;          // length of the header
            m_iv[ 1 ].iov_base = m_file_address;      // file content
            m_iv[ 1 ].iov_len = m_file_stat.st_size;  // length of the file content
            m_iv_count = 2;
            return true;
        default:
            return false;
    }

    // set up m_iv to contain only the header
    m_iv[ 0 ].iov_base = m_write_buf;
    m_iv[ 0 ].iov_len = m_write_index;
    m_iv_count = 1;
    return true;
}

/*
    The following functions are called by process_write() to generate HTTP response
        void unmap();
        bool add_response( const char* format, ... );
        bool add_content( const char* content );
        bool add_content_type();
        bool add_status_line( int status, const char* title );
        bool add_headers( int content_length );
        bool add_content_length( int content_length );
        bool add_linger();
        bool add_blank_line();   
*/ 

bool http_conn::add_status_line( int status, const char* title ) {
    return add_response( "%s %d %s\r\n", "HTTP/1.1", status, title );
}

bool http_conn::add_headers(int content_len) {
    // add_content_length(content_len);
    // add_content_type();
    // add_linger();
    // add_blank_line();
    return add_content_length(content_len) && add_content_type()
            && add_linger() && add_blank_line();
}

bool http_conn::add_content_length(int content_len) {
    return add_response( "Content-Length: %d\r\n", content_len );
}

bool http_conn::add_content_type() {
    // FUTURE IMPROVEMENTS: extend the content type here
    return add_response("Content-Type:%s\r\n", "text/html");
}

bool http_conn::add_linger() {
    return add_response( "Connection: %s\r\n", ( m_linger == true ) ? "keep-alive" : "close" );
}

bool http_conn::add_blank_line() {
    return add_response( "%s", "\r\n" );
}

bool http_conn::add_content( const char* content ) {
    return add_response( "%s", content );
}

// add a response to the server's write buffer
bool http_conn::add_response( const char* format, ... ) {
    // ensure the current position within the write buffer does not exceed the maximum write buffer size
    if( m_write_index >= WRITE_BUFFER_SIZE ) {
        return false;
    }
    
    /*
        va_list: This is a data type that represents a list of variable arguments

        va_start(): initialize arg_list to point to the first optional argument ('format' in this case) 
        after which additional arguments can be accessed.

        vsnprinf(): variable-argument 'snprintf'. It is used for formatting and writing data to a string buffer
        while allowing for variable-length argument list.
            - m_write_buf + m_write_index: destination buffer m_write_buf with starting position m_write_index
            - WRITE_BUFFER_SIZE - 1 - m_write_index: the max number of characters that can be written to the buffer
            - format: This is a format string that specifies how the data should be formatted. 
            - arg_list: This is a 'va_list' object

    */
    va_list arg_list;                 
    va_start( arg_list, format );     
    int len = vsnprintf( m_write_buf + m_write_index, WRITE_BUFFER_SIZE - 1 - m_write_index, format, arg_list );
    if( len >= ( WRITE_BUFFER_SIZE - 1 - m_write_index ) ) {
        return false;
    }
    m_write_index += len;
    va_end( arg_list );   // clean up the 'arg_list"
    return true;
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

    // printf("data retrived: %s\n", m_read_buf);
    return true;
}


// the main state machine
http_conn::HTTP_CODE http_conn::process_read() {
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = 0;
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

/*
    The following function will be called by process_read() to parse HTTP request  
    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    LINE_STATUS parse_line();
    char* get_line() { return m_read_buf + m_start_line; }
*/ 

/*
    parse HTTP request line, fetch the following:
        1. HTTP method
        2. target URL
        3. HTTP version
*/
HTTP_CODE http_conn::parse_request_line( char* text ) {
    /*
        GET /index.html HTTP/1.1
        Find the first occurrence of either a space or a tab character in the string pointed by text, 
        which points to the start of the HTTP request line. The result is stored in the 
        'm_url' pointer, which points to the first character after the HTTP method, 
        in this case, is the space ' ' after 'GET'
    */ 
    
    m_url = strpbrk(text, " \t"); 
    if (! m_url) { 
        return BAD_REQUEST;
    }

    /*
        GET\0/index.html HTTP/1.1
        Place a null character at the position pointed to by 'm_url',
        splitting the request line into two parts: the HTTP method, and the URL.
        Then, increment the m_url pointer to point to the start of the URL
    */

    *m_url++ = '\0';    // place a null character, then incremt m_url
    char* method = text;
    if ( strcasecmp(method, "GET") == 0 ) { // compare two strings, ignoring case 
        m_method = GET;
    } else {
        return BAD_REQUEST;
    }

    /*
        /index.html HTTP/1.1
        Find the first occurrence of either a space of a tab in the string 
    */

    m_version = strpbrk( m_url, " \t" );
    if (!m_version) {
        return BAD_REQUEST;
    }
    *m_version++ = '\0';
    if (strcasecmp( m_version, "HTTP/1.1") != 0 ) {
        return BAD_REQUEST;
    }
    /*
     * http://192.168.110.129:10000/index.html
    */
    if (strncasecmp(m_url, "http://", 7) == 0 ) {   
        m_url += 7;
        // 在参数 str 所指向的字符串中搜索第一次出现字符 c（一个无符号字符）的位置。
        m_url = strchr( m_url, '/' );
    }
    if ( !m_url || m_url[0] != '/' ) {
        return BAD_REQUEST;
    }
    m_check_state = CHECK_STATE_HEADER; // update the state to CHECK_STATE_HEADER
    return NO_REQUEST;
}

// parse HTTP request header
HTTP_CODE http_conn::parse_headers(char* text) {   
    // Encounter null character (a blank line), indicating the parsing of header is finished
    if( text[0] == '\0' ) {    
        // If there exists HTTP message body, then read the message of length 'm_content_length'
        // transfer the state of the maine state machine to CEHCK_STATE_CONTENT
        if ( m_content_length != 0 ) {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        // else, all conponents of HTTP request have been parsed
        return GET_REQUEST;
    } else if ( strncasecmp( text, "Connection:", 11 ) == 0 ) {  // parsing HTTP header
        // handle connection, for example: 'Connection: keep-alive'
        text += 11;
        text += strspn( text, " \t" );
        if ( strcasecmp( text, "keep-alive" ) == 0 ) {
            m_linger = true;
        }
    } else if ( strncasecmp( text, "Content-Length:", 15 ) == 0 ) {
        // handle Content-Length, for example: "Content-Length: 1000"
        text += 15;
        text += strspn( text, " \t" );
        m_content_length = atol(text);
    } else if ( strncasecmp( text, "Host:", 5 ) == 0 ) {
        // handle Host, for example: 'Host: 192.168.193.128:10000'
        text += 5;
        text += strspn( text, " \t" );
        m_host = text;
    } else {
        printf( "Unknow Header %s\n", text );
    }
    return NO_REQUEST;
}

// parse HTTP request content
HTTP_CODE http_conn::parse_content( char* text ) {
    if ( m_read_index >= ( m_content_length + m_checked_index ) )
    {
        text[ m_content_length ] = '\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

/*
    When receiving a complete and valid HTTP request, we will analyze the attributes of the target file.
    If the target file exists, and readable by all users, and is not a directory, we use mmap to map it to
    the memory address at m_file_address, and inform the caller that the file retrieval was successful.
*/
HTTP_CODE http_conn::do_request()
{
    // "/home/nowcoder/webserver/resources"
    strcpy( m_real_file, doc_root );  // copy the root directory path to the m_real_file buffer
    int len = strlen( doc_root );     // calculate the length of the root directory path
    // append the requested URL. It ensures that the resulting path does not exceed the buffer size.
    strncpy( m_real_file + len, m_url, FILENAME_LEN - len - 1 );
    // fetch m_real_file related info，-1: failure，0: success
    if ( stat( m_real_file, &m_file_stat ) < 0 ) {
        return NO_RESOURCE;
    }

    // check access permissions
    if ( ! ( m_file_stat.st_mode & S_IROTH ) ) {
        return FORBIDDEN_REQUEST;
    }

    // check if it is a directory
    if ( S_ISDIR( m_file_stat.st_mode ) ) {
        return BAD_REQUEST;
    }

    // open the file in read-only mode
    int fd = open( m_real_file, O_RDONLY );
    // establish memory mapping
    m_file_address = ( char* )mmap( 0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
    close( fd );
    return FILE_REQUEST;
}

// parse one line
LINE_STATUS http_conn::parse_line() {
    char temp;
    for ( ; m_checked_index < m_read_index; ++m_checked_index ) {
        temp = m_read_buf[ m_checked_index ];
        if ( temp == '\r' ) {
            if ( ( m_checked_index + 1 ) == m_read_index ) {
                return LINE_OPEN;
            } else if ( m_read_buf[ m_checked_index + 1 ] == '\n' ) {
                m_read_buf[ m_checked_index++ ] = '\0';
                m_read_buf[ m_checked_index++ ] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        } else if( temp == '\n' )  {
            if( ( m_checked_index > 1) && ( m_read_buf[ m_checked_index - 1 ] == '\r' ) ) {
                m_read_buf[ m_checked_index-1 ] = '\0';
                m_read_buf[ m_checked_index++ ] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

// used by worker thread in the threadpool, to handle http request
void http_conn::process() {
    // parse http request
    // printf("parse request, create response\n");

    // parse http request
    HTTP_CODE read_ret = process_read();
    if ( read_ret == NO_REQUEST ) {
        modfd( m_epollfd, m_sockfd, EPOLLIN );
        return;
    }
    
    // generate http response
    bool write_ret = process_write( read_ret );
    if ( !write_ret ) {
        close_conn();
    }
    modfd( m_epollfd, m_sockfd, EPOLLOUT);
}