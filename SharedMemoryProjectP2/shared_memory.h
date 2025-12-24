#pragma once
#include <windows.h>

// استخدمنا نصوص عادية (ANSI) لتجنب مشاكل التوافق مع المترجم
#define SHM_NAME   "ChatSharedMemory"
#define SHM_MUTEX  "ChatSharedMemoryMutex"

struct SharedChatData {
    int connectedClients;
    char lastMessage[1024];
};

extern HANDLE hMapFile;
extern SharedChatData* shmData;
extern HANDLE hMutex;

bool init_shared_memory();
void cleanup_shared_memory();