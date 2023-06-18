// dllmain.cpp : Defines the entry point for the DLL application.
#include "common/common.h"
#include "storm/Storm.h"
#include <regex>
#include <Windows.h>

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

void realize0(char* input, char* output, uint32 outputSize, uint32* writteLength) {
    bool error = false;
    *writteLength = 0;
    MapFile* mf = GlobalStorm->readSCX(input, &error);
    if (mf) {
        CHK* chk = mf->getCHK(CHKReadMode::Partial);
        if (chk) {
            Section* str = chk->getSection("STR ");
            if (str) {
                WriteBuffer wb;
                if (str->writeToBuffer(&wb)) {
                    unsigned char* data = nullptr;
                    unsigned int length = 0;
                    wb.getWrittenData(&data, &length);
                    unsigned int toCopy = outputSize < length ? outputSize : length;
                    memcpy(output, data, toCopy);
                    *writteLength = toCopy;
                }
            }
        }
        delete mf;
    }
}

LIBRARY_API void __cdecl realize(void* input, void* output, void* outputSize, void* writteLength) {
    realize0((char*)input, (char*)output, (uint32)outputSize, (uint32*)writteLength);
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        LoadStorm();
        break;
    case DLL_PROCESS_DETACH:
        UnloadStorm();
        break;
    }
    return TRUE;
}

