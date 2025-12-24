#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "shared_memory.h"

#pragma comment(lib, "ws2_32.lib")
using namespace std;

constexpr int SERVER_PORT = 12345;
constexpr int BUF_SIZE = 4096;

struct Client {
    SOCKET sock;
    string name;
};

vector<Client> clients;
mutex clients_mtx;
mutex cout_mtx;

void safe_print(const string &msg) {
    lock_guard<mutex> lg(cout_mtx);
    cout << msg << endl;
}

string now_str() {
    auto t = chrono::system_clock::to_time_t(chrono::system_clock::now());
    char buf[64];
    ctime_s(buf, sizeof(buf), &t);
    string s(buf);
    if (!s.empty() && s.back() == '\n') s.pop_back();
    return s;
}

// دالة البث تضمن وجود \n في نهاية كل رسالة
void broadcast_message(const string &msg) {
    lock_guard<mutex> lg(clients_mtx);
    string formattedMsg = msg;
    if (formattedMsg.back() != '\n') formattedMsg += "\n";

    for (const auto &c : clients) {
        send(c.sock, formattedMsg.c_str(), (int)formattedMsg.size(), 0);
    }
}

void handle_client(SOCKET clientSock) {
    char buf[BUF_SIZE];
    string username = "anonymous";
    
    int n = recv(clientSock, buf, BUF_SIZE - 1, 0);
    if (n > 0) {
        buf[n] = '\0';
        string firstMsg(buf);
        if (firstMsg.find("USERNAME:") == 0) {
            username = firstMsg.substr(9);
            username.erase(remove(username.begin(), username.end(), '\n'), username.end());
            username.erase(remove(username.begin(), username.end(), '\r'), username.end());
        }
    }

    WaitForSingleObject(hMutex, INFINITE);
    shmData->connectedClients++;
    ReleaseMutex(hMutex);

    safe_print("[" + now_str() + "] " + username + " connected.");
    broadcast_message("*** " + username + " joined the chat ***");

    while (true) {
        n = recv(clientSock, buf, BUF_SIZE - 1, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        
        string msg(buf);
        msg.erase(remove(msg.begin(), msg.end(), '\n'), msg.end());
        msg.erase(remove(msg.begin(), msg.end(), '\r'), msg.end());
        
        if (msg.empty()) continue;
        if (msg.find("/quit") != string::npos) break;

        WaitForSingleObject(hMutex, INFINITE);
        strcpy_s(shmData->lastMessage, msg.substr(0, 1023).c_str());
        ReleaseMutex(hMutex);

        string out = username + ": " + msg;
        safe_print(out);
        broadcast_message(out);
    }

    {
        lock_guard<mutex> lg(clients_mtx);
        clients.erase(remove_if(clients.begin(), clients.end(), [clientSock](const Client& c){ return c.sock == clientSock; }), clients.end());
    }

    WaitForSingleObject(hMutex, INFINITE);
    shmData->connectedClients--;
    ReleaseMutex(hMutex);

    closesocket(clientSock);
    safe_print("[" + now_str() + "] " + username + " disconnected.");
}

int main() {
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
    init_shared_memory(); 
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = {AF_INET, htons(SERVER_PORT)};
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(listenSock, (sockaddr*)&addr, sizeof(addr));
    listen(listenSock, 5);

    safe_print("Server running (Final Logic Fix)...");

    while (true) {
        SOCKET clientSock = accept(listenSock, NULL, NULL);
        {
            lock_guard<mutex> lg(clients_mtx);
            clients.push_back({clientSock, "anonymous"});
        }
        thread(handle_client, clientSock).detach();
    }
    return 0;
}