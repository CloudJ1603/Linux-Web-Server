# Linux Web Server

- Developed a high-performance linux web server in C++, capable of handling over ten thousand concurrent connection data exchange per second, as proven by Webbench stress testing.
- Implemented a concurrent model incorporating a thread pool, non-blocking socket, epoll (both edge-triggered and level-triggered) and event handling (simulated proactor)
- Implemented a state machine for parsing HTTP request message
- Implemented a test website utilizing MySQL for user registration and login, enabling image and video file retrieval to assess server performance and data handling efficiency.

## Pressure Test
Testing environment:
- Memory: 8 GB
- Number of Processors: 2
- Number of Cores per processor: 2
- Operating System: Ubunto 20.04.6 LTS
- CPU: i7-9750H

Test 1: 100 clients, running 10 sec.
```bash
./webbench -c 100 -t 10 http://192.168.87.128:9999/index.html
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://192.168.87.128:9999/index.html
100 clients, running 10 sec.

Speed=970530 pages/min, 2571904 bytes/sec.
Requests: 161755 susceed, 0 failed.
```

Test 2: 1000 clients, running 10 sec.
```bash
./webbench -c 1000 -t 10 http://192.168.87.128:9999/index.html
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://192.168.87.128:9999/index.html
1000 clients, running 10 sec.

Speed=991344 pages/min, 2626998 bytes/sec.
Requests: 165224 susceed, 0 failed.
```

Test 3: 5000 clients, running 10 sec.
```bash
./webbench -c 5000 -t 10 http://192.168.87.128:9999/index.html
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://192.168.87.128:9999/index.html
5000 clients, running 10 sec.

Speed=1070370 pages/min, 2836258 bytes/sec.
Requests: 178395 susceed, 0 failed.
```

## Build and Run Instructions
Compile under /Linux-Web-Server directory
```bash
francis@francis-VM:~/Linux-Web-Server$ g++ *.cpp -pthread
```
Run the resulting executable a.out, replace '8888' with your choice of port number
```bash
francis@francis-VM:~/Linux-Web-Server$ ./a.out 8888
create the 0th thread
create the 1th thread
create the 2th thread
create the 3th thread
create the 4th thread
create the 5th thread
create the 6th thread
create the 7th thread
```
Open any browser, and enter the URL consisting of the IP address of the Linux machine, port number, and the web file. 
For example: http://192.168.68.128:8888/index.html

You can use command-line scripts to retrieve the IP address of the Linux server
```bash
francis@francis-VM:~/Linux-Web-Server$ ip_address=$(hostname -I | awk '{print $1}')
francis@francis-VM:~/Linux-Web-Server$ echo "IP Address: $ip_address"
IP Address: 192.168.87.128
```
If you can see the welcome text together with an image, you have successfully connection to the server. 

## TODO
- Log system
- handle POST HTTP method

## Dependencies for Running Locally






