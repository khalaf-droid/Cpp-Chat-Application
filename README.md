# 🗨️ C++ Chat System  
### TCP Socket Chat & Shared Memory IPC

---

## 📌 Project Overview

This project implements a **multi-client chat system in C++** demonstrating two fundamental **Inter-Process Communication (IPC)** techniques:

1. **TCP Socket Programming (Client–Server Model)**
2. **Shared Memory Communication (IPC on the same machine)**

The project is designed for **Operating Systems / Distributed Systems coursework**, focusing on:
- Networking fundamentals
- Concurrency and synchronization
- System-level programming using C++
- Practical comparison between IPC mechanisms

---

## 🎯 Project Objectives

- Build a **multi-threaded TCP chat server**
- Support **multiple concurrent clients**
- Implement **message broadcasting**
- Design a **GUI-based client (WinAPI)**
- Implement a **Shared Memory chat system**
- Compare **Sockets vs Shared Memory** in terms of performance and design

---

## 🧩 Project Structure

Chat-System/
│
├── Socket-Version/
│ ├── server.cpp
│ ├── client.cpp
│ └── README.md
│
├── SharedMemory-Version/
│ ├── processA.cpp
│ ├── processB.cpp
│ └── shared_memory.h
│
├── diagrams/
│ ├── architecture.png (optional)
│ └── flowchart.png (optional)
│
└── README.md


---

# 🧱 Part 1: TCP Socket Chat System

## 🔧 Technologies Used
- C++
- Winsock2 (Windows)
- TCP/IP (AF_INET, SOCK_STREAM)
- Multi-threading (`std::thread`)
- WinAPI (GUI Client)

---

## 🧱 System Architecture (Socket Version)

The system follows a **Client–Server Architecture**:

- One server listens on a known port.
- Multiple clients connect using the server IP.
- Each client is handled by a **dedicated thread**.
- Messages are broadcast to all connected clients.

### 📐 Architecture Diagram

      ┌───────────────┐
      │   Client 1    │
      │ (GUI + TCP)   │
      └───────┬───────┘
              │
      ┌───────▼───────┐
      │   Client 2    │
      │ (GUI + TCP)   │
      └───────┬───────┘
              │
    ┌─────────▼─────────┐
    │    TCP Server     │
    │ (Winsock2, C++)   │
    │  - Listen         │
    │  - Accept         │
    │  - Thread/Client  │
    │  - Broadcast      │
    └─────────▲─────────┘
              │
      ┌───────┴───────┐
      │   Client N    │
      │ (GUI + TCP)   │
      └───────────────┘


---

## 🔄 Server Flowchart

Start
│
▼
WSAStartup
│
▼
Create Socket
│
▼
Bind (IP = 0.0.0.0, Port)
│
▼
Listen
│
▼
Accept Client
│
▼
Create Thread
│
▼
Receive Username
│
▼
Receive Messages
│
▼
Broadcast
│
▼
Client Disconnect?


---

## 🔄 Client Flowchart

Start
│
▼
Enter Server IP
│
▼
Connect
│
▼
Send Username
│
▼
Send / Receive Messages
│
▼
Disconnect


---

## ✅ Features (Socket Version)

- Multi-threaded server
- Unlimited clients
- Username system
- Message broadcasting
- GUI client
- Proper error handling
- Graceful disconnect

---

# 🧠 Part 2: Shared Memory Communication System

## 🔧 Technologies Used
- C++
- Shared Memory (IPC)
- Mutex / Synchronization primitives
- Same-machine process communication

---

## 📌 Shared Memory Overview

Shared Memory allows **multiple processes to access the same memory region** directly, providing **fast IPC** without kernel-level message passing.

This version demonstrates:
- Memory creation and attachment
- Concurrent read/write operations
- Synchronization between processes

---

## 🧱 Shared Memory Architecture

┌───────────────┐
│ Process A │
│ (Writer) │
└───────┬───────┘
│
│ Shared Memory Segment
│ (Messages + Flags)
│
┌───────▼───────┐
│ Process B │
│ (Reader) │
└───────────────┘


---

## 🔄 Shared Memory Flowchart



Start
│
▼
Create Shared Memory
│
▼
Attach Memory
│
▼
Lock Mutex
│
▼
Write / Read Data
│
▼
Unlock Mutex
│
▼
Detach & Cleanup


---

## ⚠️ Synchronization & Safety

- Mutex used to prevent race conditions
- Only one process accesses shared memory at a time
- Proper cleanup to avoid memory leaks

---

## 📊 Socket vs Shared Memory Comparison

| Feature           | TCP Sockets            | Shared Memory          |
|------------------|------------------------|------------------------|
| Scope            | Different machines     | Same machine only      |
| Speed            | Slower (network)       | Very fast              |
| Complexity       | Higher                 | Lower                  |
| Scalability      | High                   | Limited                |
| Use Case         | Distributed systems    | Local IPC              |

---

## 🧪 How to Run

### Socket Version
1. Run `server.cpp`
2. Note server IP and port
3. Run multiple instances of `client.cpp`
4. Enter server IP and chat

### Shared Memory Version
1. Run `processA`
2. Run `processB`
3. Observe shared data exchange

---

## 📚 Educational Value

This project demonstrates:
- TCP/IP networking
- Multi-threading
- IPC mechanisms
- Synchronization
- System-level C++ programming

---

## 👨‍💻 Author

**Student Name:** _Your Name Here_  
**Course:** Operating Systems / Distributed Systems  
**Language:** C++  
**Platform:** Windows  

---

## 🏁 Conclusion

This project provides a practical comparison between **network-based communication (TCP Sockets)** and **local IPC (Shared Memory)**, highlighting trade-offs in performance, scalability, and design complexity.

