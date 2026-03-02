# Sentinel - A Process Supervisor

## What is Sentinel?

Sentinel is a lightweight process supervisor for Linux. Think of it like a guardian that watches over your applications and makes sure they're running smoothly. If something goes wrong, Sentinel can help manage and restart processes as needed.

Imagine you have an important application running on a server. You want to make sure it stays healthy and running 24/7. That's where Sentinel comes in - it monitors your processes and helps keep them alive.

## Key Features

- **Process Supervision**: Start and manage child processes
- **Inter-Process Communication (IPC)**: Built-in pipe-based communication between processes
- **Event-Driven Architecture**: Uses an event loop to handle process events efficiently
- **Graceful Shutdown**: Handles clean shutdown of supervised processes
- **Multi-threaded Support**: Uses threading for concurrent process management

## Core Components

### Supervisor
The heart of Sentinel. It handles the overall lifecycle management - starting up the supervision system and shutting it down gracefully. The supervisor keeps track of whether it's running or shutting down.

### Child Process Spawner
This component is responsible for creating and launching new child processes. When you want Sentinel to manage a new application, this is what actually starts it.

### Event Loop
The brain of the operations. It continuously listens for events from managed processes - like "process crashed", "process finished", or "new message available". When events happen, the event loop responds appropriately.

### IPC Pipes
Processes often need to talk to each other. This component sets up communication channels (pipes) so that child processes can send messages back to the supervisor or to each other.

## How It Works

1. **Initialization**: Sentinel starts up and gets ready to manage processes
2. **Spawning**: You tell it to start a new application
3. **Monitoring**: The event loop continuously watches for any activity or problems
4. **Communication**: Child processes can send updates or status messages through IPC pipes
5. **Graceful Shutdown**: When needed, Sentinel cleanly shuts down all managed processes

## Project Structure

```
sentinel/
├── include/
│   └── sentinel.h          # Main header file with core definitions
├── src/
│   ├── main.c              # Entry point - starts the supervisor
│   ├── supervisor.c        # Supervisor initialization and shutdown logic
│   ├── child_spawn.c       # Child process spawning functionality
│   ├── event_loop.c        # Main event handling loop
│   └── ipc_pipe.c          # Inter-process communication via pipes
├── Makefile                # Build configuration
└── README.c                # This file
```

## Building Sentinel

To compile and build Sentinel:

```bash
make
```

This will compile all C source files and create the `sentinel` executable.

To clean up build artifacts:

```bash
make clean
```

## Requirements

- **GCC** - GNU C Compiler (or compatible C compiler)
- **POSIX-compatible Linux system** - for process and pipe APIs
- **POSIX Threads (pthread)** - for multi-threaded support

## Current Status

This is a skeleton implementation - the foundational structure is in place, but the core functionality is still being developed. The basic framework for process supervision is ready, and components are waiting to be fully implemented.

## Use Cases

- **Application Monitoring**: Keep critical services running
- **Process Management**: Manage multiple related processes as a group
- **Automatic Restart**: Automatically restart crashed applications
- **Logging and Monitoring**: Track application behavior and events
- **Daemon Management**: Manage long-running background services

## Future Development

The project is designed to scale and include features like:
- Process restart policies
- Resource monitoring and limits
- Detailed logging system
- Configuration files for managing multiple processes
- Health checks and status reporting
