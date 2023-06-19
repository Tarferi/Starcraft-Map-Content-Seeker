#pragma once
#include "../common/common.h"
#include "../common/Mem.h"
#include <Windows.h>
#ifndef STORMAPI
#define STORMAPI __stdcall
#endif

class TinyStorm {

	typedef BOOL (STORMAPI* SFileOpenArchiveF) (const char* szMpqName, DWORD dwPriority, DWORD dwFlags, HANDLE* phMpq);

	typedef BOOL (STORMAPI* SFileOpenFileExF) (HANDLE hMpq, const char* szFileName, DWORD dwSearchScope, HANDLE* phFile);

	typedef LONG (STORMAPI* SFileGetFileSizeF) (HANDLE hFile, LPDWORD lpFileSizeHigh);

	typedef BOOL (STORMAPI* SFileReadFileF) (HANDLE hFile, void* buffer, DWORD nNumberOfBytesToRead, DWORD* read, LONG lpDistanceToMoveHigh);

	typedef BOOL (STORMAPI* SFileCloseFileF) (HANDLE hFile);

	typedef BOOL (STORMAPI* SFileCloseArchiveF) (HANDLE hArchive);
	
	typedef BOOL (STORMAPI* SFileDestroyF) ();

	typedef BOOL (STORMAPI* SFileSetIoErrorModeF)(DWORD , BOOL(STORMAPI*)(const char*, DWORD, DWORD));

public:

	TinyStorm();

	~TinyStorm();

	bool IsValid();
	
	bool readSTR(char* filePath, char** strContent, int* strContentSize, int* dataRequired);

private:

	SFileOpenArchiveF SFileOpenArchive = nullptr;
	SFileOpenFileExF SFileOpenFileEx = nullptr;
	SFileGetFileSizeF SFileGetFileSize = nullptr;
	SFileReadFileF SFileReadFile = nullptr;
	SFileCloseFileF SFileCloseFile = nullptr;
	SFileCloseArchiveF SFileCloseArchive = nullptr;
	SFileDestroyF SFileDestroy = nullptr;
	SFileSetIoErrorModeF SFileSetIoErrorMode = nullptr;

	HMEMORYMODULE mod = NULL;

};

