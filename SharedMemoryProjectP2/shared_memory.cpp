#include "shared_memory.h"
#include <cstring>

HANDLE hMapFile = NULL;
SharedChatData* shmData = nullptr;
HANDLE hMutex = NULL;

bool init_shared_memory() {
    hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedChatData), SHM_NAME);
    if (!hMapFile) return false;

    shmData = (SharedChatData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedChatData));
    if (!shmData) return false;

    hMutex = CreateMutexA(NULL, FALSE, SHM_MUTEX);
    if (!hMutex) return false;

    shmData->connectedClients = 0;
    strcpy_s(shmData->lastMessage, "No messages yet");
    return true;
}

void cleanup_shared_memory() {
    if (shmData) UnmapViewOfFile(shmData);
    if (hMapFile) CloseHandle(hMapFile);
    if (hMutex) CloseHandle(hMutex);
}