# Shared Memory Module Documentation

## 1. Overview

Shared memory is a form of inter-process communication (IPC) that allows multiple processes to access a common region of physical memory. In this C++ project, shared memory facilitates efficient data exchange between the chat server and client processes without the overhead of traditional IPC methods like sockets or pipes for certain shared state information.

The shared memory approach enables low-latency access to global chat state, such as the count of connected clients and the most recent message, allowing processes to synchronize and display updated information in real-time.

## 2. Design & Architecture

The shared memory segment is structured as a `SharedChatData` struct containing:
- `connectedClients`: An integer tracking the number of active client connections.
- `lastMessage`: A character array (1024 bytes) storing the most recent chat message.

Processes attach to the shared memory segment using Windows API functions `CreateFileMappingA` and `MapViewOfFile`, which map the shared region into the process's virtual address space. Detachment occurs via `UnmapViewOfFile` and `CloseHandle`.

Synchronization is achieved through a named mutex (`ChatSharedMemoryMutex`) created with `CreateMutexA`, ensuring atomic access to the shared data and preventing race conditions during read/write operations.

## 3. Implementation Details

Shared memory creation is handled by `init_shared_memory()`, which:
1. Creates a file mapping object with `CreateFileMappingA` using the name "ChatSharedMemory".
2. Maps the shared region into the process's address space with `MapViewOfFile`.
3. Initializes a mutex for synchronization.
4. Initializes the shared data structure with default values.

Access to shared memory is mediated through the global pointer `shmData`, which points to the mapped `SharedChatData` struct. Read operations directly access struct members, while write operations are protected by mutex acquisition.

Data consistency is maintained through mutex-based locking before any modification, ensuring that concurrent accesses do not corrupt the shared state.

## 4. Concurrency & Safety

Race conditions are prevented through the use of a named mutex that must be acquired before accessing or modifying shared data. The mutex ensures mutual exclusion, allowing only one process at a time to perform operations on the shared memory segment.

Multiple processes coordinate access by attempting to acquire the mutex; if unavailable, processes wait until it is released, implementing a simple critical section mechanism.

## 5. Error Handling

The `init_shared_memory()` function returns a boolean indicating success or failure of shared memory creation. Failures can occur due to:
- Insufficient system resources.
- Permission issues.
- Existing shared memory conflicts.

Cleanup is performed by `cleanup_shared_memory()`, which unmaps the view, closes handles, and releases system resources. This prevents memory leaks and ensures proper resource deallocation when processes terminate.

## 6. Use Case in the Project

The Shared Memory module serves as a communication bridge between the multithreaded chat server and GUI client processes. The server updates the `connectedClients` count and `lastMessage` content, while clients read this information to display current chat state.

A typical usage flow:
1. Server process initializes shared memory on startup.
2. As clients connect, server increments `connectedClients` under mutex protection.
3. When messages are received, server updates `lastMessage`.
4. Client processes periodically read the shared data to update their GUI displays.

## 7. Limitations & Notes

This implementation is limited to Windows platforms due to its reliance on Windows-specific APIs. The fixed-size message buffer (1024 bytes) constrains message length, and the simple mutex-based synchronization may not scale efficiently for high-concurrency scenarios.

Platform dependency restricts portability, and the approach assumes a small number of processes with moderate access frequency for optimal performance.</content>
<parameter name="filePath">d:\down from g\os\os\SharedMemoryDocumentation.md