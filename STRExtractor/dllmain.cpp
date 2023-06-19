//#define STOCK_STORM
//#define BUILD_EXE

#include "common/common.h"
#include <Windows.h>
#ifdef STOCK_STORM
#include "storm/TinyStorm.h"
#else
#include "storm/Storm.h"
#endif

#ifndef STOCK_STORM
static Storm* GlobalStorm = nullptr;

void UnloadStorm() {
    if (GlobalStorm != nullptr) {
        delete GlobalStorm;
        GlobalStorm = nullptr;
    }
}

bool LoadStorm() {
    UnloadStorm();
    bool error = false;
    GlobalStorm = new Storm(&error);
    return !error;
}
#endif

void realize0(char* input, char* output, uint32 outputSize, uint32* writteLength, uint32* requiredLength) {
    bool error = false;
    *writteLength = 0;
    char* str = nullptr;
    int strlength = 0;
    int reqLength = 0;

#ifdef STOCK_STORM
    TinyStorm ts;
    if (!ts.IsValid()) {
        return;
    }
    if(ts.readSTR(input, &str, &strlength, &reqLength)) {
#else
    if (GlobalStorm->readSTR(input, &str, &strlength, &reqLength)) {
#endif
        unsigned int toCopy = outputSize < (uint32)strlength ? outputSize : strlength;
        memcpy(output, str, toCopy);
        *writteLength = toCopy;
        *requiredLength = reqLength;
        free(str);
    }
}

LIBRARY_API void __cdecl realize(void* input, void* output, void* outputSize, void* writteLength, void* requiredLength) {
    realize0((char*)input, (char*)output, (uint32)outputSize, (uint32*)writteLength, (uint32*)requiredLength);
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
#ifndef STOCK_STORM
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        LoadStorm();
        break;
    case DLL_PROCESS_DETACH:
        UnloadStorm();
        break;
    }
#endif
    return TRUE;
}

#ifdef BUILD_EXE

static char tmp[0x7ffff];

int main() {
#ifndef STOCK_STORM
    LoadStorm();
#endif
    uint32 sz = 0;
    uint32 szreq = 0;
    realize0((char*)"C:\\Users\\Tom\\Documents\\StarCraft\\Maps\\moje mapy\\test\\Raccoon City Remastered.scx", tmp, sizeof(tmp), &sz, &szreq);
#ifndef STOCK_STORM
    UnloadStorm();
#endif
    return 0;
}

#endif