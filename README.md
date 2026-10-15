# Peer-to-Peer (P2P) File Sharing System

This project is a file-sharing system for transferring files across the distributed network written in modern C++. It functions as a node or peer that can operate as both a server and a client.

When I first implemented a typical client-server architecture using socket programming, I often used multithreading to divide a single process into smaller, concurrent units of execution called threads. These threads were used to accept incoming connections and handle communication with each client. Although this approach is reliable, after learning how it works internally, I discovered that it can introduce performance bottlenecks, especially when handling a large number of concurrent connections.

The most efficient method that I found was I/O multiplexing. It is essentially a low-level operating system mechanism that allows a single thread to monitor multiple file descriptors simultaneously to check if any of them are ready for reading or writing. Since file descriptors can also represent network sockets, I pass them to the OS kernel and wait for events using **select**, **poll**, or **epoll**. I initially used **poll** to handle events, but it must scan the entire list of file descriptors, which can become inefficient as the number of connections grows. Eventually, I switched to **epoll**, which returns only file descriptors that are ready for I/O, eliminating the need to scan the entire list on every call and improving efficiency when handling a large number of connections.

## Features

### Current Features
- Send and receive text messages.
- Download and upload multiple files.
- Register custom message event callbacks.

### Planned Features
- Peer Discovery with DHT

## Diagrams

### Sequence Diagram of the Upload and Download Process
<img width="665" height="593" alt="upload_and_download_file_sequence_daigram" src="https://github.com/user-attachments/assets/49bb8ec3-05e2-4ef3-a309-fd543637ea4e" />

## Building

The building process is very straightforward on Linux. It does not require any third-party libraries, as it was built entirely from scratch. 

### Prerequisites
- A C++ compiler (preferably from GCC)
- CMake

### Build Instructions
```sh
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```
