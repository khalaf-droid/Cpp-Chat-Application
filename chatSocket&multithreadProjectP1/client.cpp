
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace std;

#define ID_EDIT_LOG     101
#define ID_EDIT_INPUT   102
#define ID_BTN_SEND     103
#define ID_BTN_CONNECT  104
#define ID_EDIT_IP      105
#define ID_STATIC_IP    106
constexpr int SERVER_PORT = 12345;
constexpr int BUF = 4096;

HWND hMainWindow;
HWND hEditLog;
HWND hEditInput;
HWND hBtnSend;
HWND hBtnConnect;
HWND hEditIP;
HWND hStaticIP;

atomic<bool> connected{false};
SOCKET clientSock = INVALID_SOCKET;
thread recvThread;

string s2ws(const string& str) {
    return str;
}

string ws2s(const string& str) {
    return str;
}

void LogToGui(const string& msg) {
    int len = GetWindowTextLength(hEditLog);
    SendMessage(hEditLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    string fullMsg = msg + "\r\n";
    SendMessage(hEditLog, EM_REPLACESEL, 0, (LPARAM)fullMsg.c_str());
}

void RecvLoop() {
    char buf[BUF];
    LogToGui("Please enter your username in the input field.");
    
    while (connected) {
        int n = recv(clientSock, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            if (connected) {
                LogToGui("*** Disconnected from server. ***");
            }
            connected = false;
            break;
        }

        buf[n] = '\0';
        string incoming_msg(buf);
        LogToGui(s2ws(incoming_msg));
    }
}

void AttemptConnect() {
    int len = GetWindowTextLength(hEditIP);
    if (len == 0) {
        LogToGui("Please enter a server IP address.");
        return;
    }
    
    string ip(len + 1, '\0');
    GetWindowText(hEditIP, &ip[0], len + 1);
    ip.resize(len);
    string serverIP = ip;
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LogToGui("WSAStartup failed.");
        return;
    }

    clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSock == INVALID_SOCKET) {
        LogToGui("Socket creation failed.");
        WSACleanup();
        return;
    }

    sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, serverIP.c_str(), &servAddr.sin_addr) != 1) {
        LogToGui("Invalid IP address format.");
        closesocket(clientSock);
        WSACleanup();
        return;
    }

    if (connect(clientSock, (sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        LogToGui("Connection failed. Error: " + to_string(WSAGetLastError()));
        closesocket(clientSock);
        WSACleanup();
        return;
    }

    connected = true;
    EnableWindow(hBtnConnect, FALSE);
    SetWindowText(hBtnConnect, "Disconnect");
    EnableWindow(hBtnSend, TRUE);
    EnableWindow(hEditIP, FALSE);
    LogToGui("*** Connected to Server " + ip + ":" + to_string(SERVER_PORT) + " ***");
    
    recvThread = thread(RecvLoop);
}

void AttemptDisconnect() {
    if (!connected) return;

    connected = false;
    
    string quit_msg = "/quit\n";
    send(clientSock, quit_msg.c_str(), (int)quit_msg.size(), 0);
    shutdown(clientSock, SD_SEND);

    closesocket(clientSock);
    clientSock = INVALID_SOCKET;
    
    if (recvThread.joinable()) {
        recvThread.join();
    }

    EnableWindow(hBtnConnect, TRUE);
    SetWindowText(hBtnConnect, "Connect");
    EnableWindow(hBtnSend, FALSE);
    EnableWindow(hEditIP, TRUE);
    
    WSACleanup();
    LogToGui("*** Disconnected cleanly. ***");
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static bool username_sent = false;

    switch (uMsg) {
    case WM_CREATE: {
        HINSTANCE hInstance = GetModuleHandle(NULL);

        hEditLog = CreateWindowEx(
    0, "EDIT", "",
    WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE |
    ES_AUTOVSCROLL | ES_READONLY | WS_BORDER,
    10, 10, 460, 160,   // ⬅️ بدل 200
    hwnd, (HMENU)ID_EDIT_LOG, hInstance, NULL
);
hStaticIP = CreateWindowEx(
    0, "STATIC", "Server IP:",
    WS_CHILD | WS_VISIBLE,
    10, 180, 100, 20,
    hwnd, (HMENU)ID_STATIC_IP, hInstance, NULL
);

hEditIP = CreateWindowEx(
    0, "EDIT", "127.0.0.1",
    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP,
    120, 180, 250, 20,
    hwnd, (HMENU)ID_EDIT_IP, hInstance, NULL
);

        hEditInput = CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            10, 210, 350, 30, hwnd, (HMENU)ID_EDIT_INPUT, hInstance, NULL);
        hBtnSend = CreateWindow("BUTTON", "Send", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            370, 210, 100, 30, hwnd, (HMENU)ID_BTN_SEND, hInstance, NULL);
        hBtnConnect = CreateWindow("BUTTON", "Connect", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
            10, 250, 100, 30, hwnd, (HMENU)ID_BTN_CONNECT, hInstance, NULL);
        
        EnableWindow(hBtnSend, FALSE);
        
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(hEditLog, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hEditInput, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hBtnSend, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hBtnConnect, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hEditIP, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hStaticIP, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        LogToGui("*** Welcome to the Chat Client ***");
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        
        if (id == ID_BTN_CONNECT) {
            if (!connected) {
                AttemptConnect();
            } else {
                AttemptDisconnect();
            }
        }
        else if (id == ID_BTN_SEND) {
            if (!connected) return 0;

            int len = GetWindowTextLength(hEditInput);
            if (len == 0) return 0;
             
            string text(len + 1, '\0');
            GetWindowText(hEditInput, &text[0], len + 1);
            text.resize(len);
            
            if (!username_sent) {
                string username_header = "USERNAME:" + text + "\n";
                send(clientSock, username_header.c_str(), (int)username_header.size(), 0);
                username_sent = true;
                LogToGui("Username sent: " + text);
            } else {
                string msg_to_send = text + "\n";
                if (send(clientSock, msg_to_send.c_str(), (int)msg_to_send.size(), 0) == SOCKET_ERROR) {
                    LogToGui("Send failed! Disconnecting...");
                    AttemptDisconnect();
                }
            }
            
            SetWindowText(hEditInput, "");
        }
        break;
    }

    case WM_CLOSE:
        AttemptDisconnect();
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR pCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "ClientWindow";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClass(&wc)) return 0;

    hMainWindow = CreateWindowEx(0, CLASS_NAME, "C++ Chat Client", 
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 340,
        NULL, NULL, hInstance, NULL
    );

    if (hMainWindow == NULL) return 0;

    ShowWindow(hMainWindow, nCmdShow);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}