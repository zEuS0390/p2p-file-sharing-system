# Peer-to-Peer (P2P) File Sharing System

This project is a modern C++ file-sharing system library capable of transferring files. It acts as a peer that functions as both a server and a client within a decentralized network. In the meantime, it only works on Linux because it uses epoll, a platform-specific interface for efficient I/O event notification, to handle large numbers of connections.

## Features
- Send and receive messages
- Download and upload multiple files of any size concurrently

## Classes
- ConnectionManager
- MessageHandler
- IOMultiplexer

## Types
- Connection
- Endpoint
- EventCommand
- FileErrorCode
- IncomingFileTransfer
- MessageHeaders
- MessageType
- OutgoingFileTransfer

## 🔨 How to Build?
The build process is very straightforward on Linux. It does not require any third-party libraries, as it was built entirely from scratch. Just make sure you have CMake and the GNU Compiler Collection (GCC) installed on your system.
```sh
cmake -S ./ -B ./build/ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

```sh
cmake --build ./build/
```
