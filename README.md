# nf-server

A network server runtime prototype designed to validate a tick / shard–based
server execution model with epoll-based multi-protocol (TCP / UDP / TLS) support.

## 1. Environment (Dependencies)

### OS
- Linux (Ubuntu 22.04+)

### Compiler / Build Tools
- g++ 9
- CMake
- Make

### Runtime / Scripts
- Python 3.10+

### Privileges
- `sudo` is required for dependency installation and systemd service registration.

## 2. Execution

All commands must be executed from the project root directory.

### Install
```bash
./simulator install
```

### Build
```bash
./simulator build
```

### Clean
```bash
./simulator clean
```

### Start (systemd)
```bash
./simulator start
```
- Installs the binary to /usr/bin/nf-simulator
- Registers and starts the systemd service
- Service name : nf-simulator

### Stop
```bash
./simulator stop
```
- Stops the running systemd service

### Uninstall
```bash
./simulator uninstall
```
- Uninstall only static libraries

## 3. Tech Stacks

### Network I/O
- Support multi-protocol (TCP, UDP, openSSL-based TLS)
- Epoll-based event loop
- Epoll LT (Level Triggered) mode
- Non-blocking sockets

### Socket Layer
- Raw socket management
- Manual accept / recv / send handling
- Explicit connection lifecycle control

### Packet Processing
- Packet object abstraction
- Separation of network I/O and logic execution
- Explicit packet ownership transfer via queues

### Concurrency Model
- Tick-based logic execution
- Shard-based event distribution
- In-shard ordering guarantees

### Thread Model
- Dedicated threads for network I/O, contents, shard manager
- Synchronization using mutexes and condition variables

### Execution Model
- Explicit tick boundary management
- Tick-scoped state updates
- Clear separation between ordering guarantees and parallelism

### Cryptography / Security
- Statically linked OpenSSL
- TLS handshake and encrypted channel handling
- Clear separation between network and cryptographic layers

### Logging
- spdlog-based logging
- Asynchronous log output

### Build / Deployment
- CMake-based build system
- Static third-party dependency management
- systemd-based service execution

## 4. Utilities / Operations
### Process Monitoring
```bash
ps aux | grep nf-simulator
```
```bash
top -H -p <pid>
```
### Systemd Management
```bash
systemctl status nf-simulator
```

### File-based Logs
```bash
tail -f /var/log/nf/nf-simulator.log
```

## 5. Licnese
This project is intended for research and experimental use.

Third-party libraries included or referenced in this project are governed by
their respective licenses provided by the original authors and repositories.
