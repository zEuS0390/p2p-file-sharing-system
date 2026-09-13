# Peer-to-Peer (P2P) File Sharing System

This project is a modern C++ file-sharing system library capable of transferring both small and large files. It acts as a peer that functions as both a server and a client within a decentralized network. In the meantime, it only works on Linux because it uses epoll, a platform-specific interface for efficient I/O event notification, to handle large numbers of connections.

## 🔨 How to Build?
```sh
cmake -S ./ -B ./build/ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

```sh
cmake --build ./build/
```

