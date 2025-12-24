#ifndef UNICODE
#define UNICODE
#endif 

#define _WIN32_WINNT 0x0600 
#include <winsock2.h>
#include <ws2tcpip.h> 
#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <algorithm> 

using namespace std;
#define ID_BTN_SEND 103
#define ID_BTN_CONNECT 104
HWND hMainWindow, hEditLog, hEditInput, hBtnSend, hBtnConnect;
atomic<bool> connected{false};
SOCKET clientSock;
bool username_sent = false;

wstring s2ws(const string& s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    wstring ws(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
    return ws;
}

string ws2s(const wstring& ws) {
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, NULL, 0, NULL, NULL);
    string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], len, NULL, NULL);
    return s;
}

void LogToGui(wstring msg) {
    if (msg.empty()) return;
    int len = GetWindowTextLength(hEditLog);
    SendMessage(hEditLog, EM_SETSEL, len, len);
    // \r\n ضرورية جداً لنظام ويندوز للنزول لسطر جديد في الـ EDIT control
    SendMessage(hEditLog, EM_REPLACESEL, 0, (LPARAM)(msg + L"\r\n").c_str());
}

// دالة الاستقبال المحسنة لحل مشكلة تداخل الأسطر
void RecvLoop() {
    char buf[4096];
    string messageBuffer = ""; // مخزن لتجميع البيانات المقطوعة
    
    while (connected) {
        int n = recv(clientSock, buf, 4095, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        
        messageBuffer += string(buf);
        
        size_t pos;
        // البحث عن كل سطر جديد وفصله ومعالجته منفرداً
        while ((pos = messageBuffer.find('\n')) != string::npos) {
            string token = messageBuffer.substr(0, pos);
            
            // تنظيف النص من أي علامات \r
            token.erase(remove(token.begin(), token.end(), '\r'), token.end());

            if (!token.empty()) {
                LogToGui(s2ws(token));
            }
            
            // مسح الجزء الذي تمت طباعته من المخزن
            messageBuffer.erase(0, pos + 1);
        }
    }
    connected = false;
    EnableWindow(hBtnSend, FALSE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_COMMAND && LOWORD(wp) == ID_BTN_CONNECT) {
        WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
        clientSock = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr = {AF_INET, htons(12345)};
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        if (connect(clientSock, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR) {
            connected = true; username_sent = false;
            EnableWindow(hBtnSend, TRUE); 
            LogToGui(L"*** Connected! Please enter your username first ***");
            thread(RecvLoop).detach();
        } else {
            LogToGui(L"*** Connection Failed! ***");
        }
    } else if (msg == WM_COMMAND && LOWORD(wp) == ID_BTN_SEND) {
        wchar_t buf[1024]; GetWindowText(hEditInput, buf, 1024);
        string s = ws2s(buf);
        if (s.empty()) return 0;

        if (!username_sent) {
            string h = "USERNAME:" + s;
            send(clientSock, h.c_str(), (int)h.size(), 0);
            username_sent = true;
        } else {
            send(clientSock, s.c_str(), (int)s.size(), 0);
        }
        SetWindowText(hEditInput, L"");
    } else if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hwnd, msg, wp, lp);
}

int main() {
    HINSTANCE hi = GetModuleHandle(NULL);
    WNDCLASS wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hi; wc.lpszClassName = L"CW";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1); RegisterClass(&wc);
    hMainWindow = CreateWindow(L"CW", L"Chat Client", WS_OVERLAPPEDWINDOW|WS_VISIBLE, 100, 100, 500, 400, NULL, NULL, hi, NULL);
    hEditLog = CreateWindow(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_READONLY|WS_BORDER, 10, 10, 460, 250, hMainWindow, NULL, hi, NULL);
    hEditInput = CreateWindow(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER, 10, 270, 350, 30, hMainWindow, NULL, hi, NULL);
    hBtnSend = CreateWindow(L"BUTTON", L"Send", WS_CHILD|WS_VISIBLE, 370, 270, 100, 30, hMainWindow, (HMENU)ID_BTN_SEND, hi, NULL);
    hBtnConnect = CreateWindow(L"BUTTON", L"Connect", WS_CHILD|WS_VISIBLE, 10, 310, 100, 30, hMainWindow, (HMENU)ID_BTN_CONNECT, hi, NULL);
    
    EnableWindow(hBtnSend, FALSE); 
    MSG m; while(GetMessage(&m, NULL, 0, 0)) { TranslateMessage(&m); DispatchMessage(&m); }
    return 0;
}