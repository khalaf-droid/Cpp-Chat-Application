#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

constexpr int SERVER_PORT = 12345;
constexpr int BUF_SIZE = 4096;
atomic<bool> serverRunning{true};

struct Client
{
    SOCKET sock;
    string name;
};

vector<Client> clients;
CRITICAL_SECTION clients_cs;
CRITICAL_SECTION cout_cs;

struct ScopedLock
{
    CRITICAL_SECTION *cs;
    ScopedLock(CRITICAL_SECTION *c) : cs(c) { EnterCriticalSection(cs); }
    ~ScopedLock() { LeaveCriticalSection(cs); }
};

void safe_print(const string &msg)
{
    ScopedLock lg(&cout_cs);
    cout << msg << endl;
}

string now_str()
{
    using namespace chrono;
    auto t = system_clock::now();
    time_t tt = system_clock::to_time_t(t);
    char buf[64];
    tm *ptm = localtime(&tt);
    if (ptm)
    {
        strftime(buf, sizeof(buf), "%c", ptm);
        return string(buf);
    }
    return to_string(tt);
}

void broadcast_message(const string &msg, SOCKET excludeSock = INVALID_SOCKET)
{
    vector<SOCKET> toRemove;
    {
        ScopedLock lg(&clients_cs);
        for (const auto &c : clients)
        {
            if (c.sock == excludeSock)
                continue;
            int sent = send(c.sock, msg.c_str(), (int)msg.size(), 0);
            if (sent == SOCKET_ERROR)
            {
                toRemove.push_back(c.sock);
            }
        }
    }
    if (!toRemove.empty())
    {
        ScopedLock lg(&clients_cs);
        for (SOCKET s : toRemove)
        {
            auto it = find_if(clients.begin(), clients.end(),
                              [&](const Client &cl)
                              { return cl.sock == s; });
            if (it != clients.end())
            {
                safe_print("[" + now_str() + "] remove client due to send failure");
                closesocket(it->sock);
                clients.erase(it);
            }
        }
    }
}

void handle_client(SOCKET clientSock)
{
    char buf[BUF_SIZE];
    int n;

    n = recv(clientSock, buf, BUF_SIZE - 1, 0);
    if (n <= 0)
    {
        closesocket(clientSock);
        return;
    }
    buf[n] = '\0';
    string firstMsg(buf, n);

    string username = "anonymous";
    if (firstMsg.rfind("USERNAME:", 0) == 0)
    {
        username = firstMsg.substr(9);
        while (!username.empty() && (username.back() == '\n' || username.back() == '\r'))
            username.pop_back();
    }
    else
    {
        username = firstMsg;
        while (!username.empty() && (username.back() == '\n' || username.back() == '\r'))
            username.pop_back();
    }

    {
        ScopedLock lg(&clients_cs);
        for (auto &c : clients)
        {
            if (c.sock == clientSock)
            {
                c.name = username;
                break;
            }
        }
    }

    safe_print("[" + now_str() + "] " + username + " has connected.");
    broadcast_message("*** " + username + " has joined the chat\n", clientSock);

    while (true)
    {
        n = recv(clientSock, buf, BUF_SIZE - 1, 0);
        if (n == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            safe_print("[" + now_str() + "] recv error from " + username + " code: " + to_string(err));
            break;
        }
        else if (n == 0)
        {
            safe_print("[" + now_str() + "] " + username + " disconnected.");
            break;
        }
        else
        {
            buf[n] = '\0';
            string msg(buf, n);
            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
                msg.pop_back();
            if (msg == "/quit")
            {
                safe_print("[" + now_str() + "] " + username + " requested quit.");
                break;
            }
            string out = username + ": " + msg + "\n";
            safe_print("[" + now_str() + "] " + out);
            broadcast_message(out, clientSock);
        }
    }

    {
        ScopedLock lg(&clients_cs);
        auto it = find_if(clients.begin(), clients.end(),
                          [&](const Client &c)
                          { return c.sock == clientSock; });
        if (it != clients.end())
        {
            clients.erase(it);
        }
    }

    broadcast_message("*** " + username + " has left the chat\n", clientSock);

    closesocket(clientSock);
    safe_print("[" + now_str() + "] cleaned up client: " + username);
}

DWORD WINAPI client_thread_func(LPVOID param)
{
    SOCKET sock = *(SOCKET *)param;
    delete (SOCKET *)param;
    handle_client(sock);
    return 0;
}

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET)
    {
        cerr << "socket() failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERVER_PORT);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSock, (sockaddr *)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
    {
        cerr << "bind() failed: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR)
    {
        cerr << "listen() failed: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    InitializeCriticalSection(&clients_cs);
    InitializeCriticalSection(&cout_cs);

    safe_print("[" + now_str() + "] Server listening on port " + to_string(SERVER_PORT) + " ...");

    while (serverRunning)
    {
        sockaddr_in clientAddr{};
        int addrlen = sizeof(clientAddr);
        SOCKET clientSock = accept(listenSock, (sockaddr *)&clientAddr, &addrlen);
        if (clientSock == INVALID_SOCKET)
        {
            int err = WSAGetLastError();
            safe_print("[" + now_str() + "] accept() failed: " + to_string(err));
            continue;
        }

        {
            ScopedLock lg(&clients_cs);
            clients.push_back(Client{clientSock, "anonymous"});
        }

        {
            SOCKET *pSock = new SOCKET(clientSock);
            HANDLE h = CreateThread(NULL, 0, client_thread_func, pSock, 0, NULL);
            if (h)
                CloseHandle(h);
        }

        char *ipstr = inet_ntoa(clientAddr.sin_addr);
        safe_print("[" + now_str() + "] Accepted connection from " + string(ipstr));
    }

    safe_print("[" + now_str() + "] Server shutting down...");

    {
        ScopedLock lg(&clients_cs);
        for (auto &c : clients)
        {
            closesocket(c.sock);
        }
        clients.clear();
    }

    closesocket(listenSock);
    DeleteCriticalSection(&clients_cs);
    DeleteCriticalSection(&cout_cs);
    WSACleanup();
    return 0;
}
