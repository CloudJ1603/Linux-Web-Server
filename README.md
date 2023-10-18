# Linux Web Server

- Developed a high-performance linux web server in C++, capable of handling over ten thousand concurrent connection data exchange per second, as proven by Webbench stress testing.
- Implemented a concurrent model incorporating a thread pool, non-blocking socket, epoll (both edge-triggered and level-triggered) and event handling (simulated proactor)
- Implemented a state machine for parsing HTTP request message
- Implemented a test website utilizing MySQL for user registration and login, enabling image and video file retrieval to assess server performance and data handling efficiency.

## Pressure Test

```bash
./webbench -c 100 -t 10 http://192.168.87.128:9999/index.html
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://192.168.87.128:9999/index.html
100 clients, running 10 sec.

Speed=970530 pages/min, 2571904 bytes/sec.
Requests: 161755 susceed, 0 failed.

./webbench -c 1000 -t 10 http://192.168.87.128:9999/index.html
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://192.168.87.128:9999/index.html
1000 clients, running 10 sec.

Speed=991344 pages/min, 2626998 bytes/sec.
Requests: 165224 susceed, 0 failed.

./webbench -c 5000 -t 10 http://192.168.87.128:9999/index.html
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://192.168.87.128:9999/index.html
5000 clients, running 10 sec.

Speed=1070370 pages/min, 2836258 bytes/sec.
Requests: 178395 susceed, 0 failed.

```

## TODO



## Dependencies for Running Locally
* cmake >= 3.7
  * All OSes: [click here for installation instructions](https://cmake.org/install/)
* make >= 4.1 (Linux, Mac), 3.81 (Windows)
  * Linux: make is installed by default on most Linux distros
  * Mac: [install Xcode command line tools to get make](https://developer.apple.com/xcode/features/)
  * Windows: [Click here for installation instructions](http://gnuwin32.sourceforge.net/packages/make.htm)
* SDL2 >= 2.0
  * All installation instructions can be found [here](https://wiki.libsdl.org/Installation)
  >Note that for Linux, an `apt` or `apt-get` installation is preferred to building from source. 
* gcc/g++ >= 5.4
  * Linux: gcc / g++ is installed by default on most Linux distros
  * Mac: same deal as make - [install Xcode command line tools](https://developer.apple.com/xcode/features/)
  * Windows: recommend using [MinGW](http://www.mingw.org/)

## Basic Build Instructions



