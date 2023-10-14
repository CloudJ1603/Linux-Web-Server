#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include <signal.h>

void addsig(int sig, void(handler)(int))
{
    struct sigaction sa;
    memset(&sa,'\0', sizeof(sa));
}

int main(int argc, char* argv[])
{
    if(argc <= 1) 
    {
        /* 
            'basename' command in Unix-like OS is used to strip directory and suffix information from filenames
            example:
                basename /user/local/bin/somefile.txt
                output:
                somefile
        */
        printf("Please run according to the following format: %s [port_number]\n", basename(argv[0]));
        exit(-1);
    }

    // get the port number
    int port = atoi(argv[1]);



    return 0;
}