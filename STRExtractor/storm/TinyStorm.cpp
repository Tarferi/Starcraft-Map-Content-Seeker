#include "TinyStorm.h"
#include "Storm.h"
#include "stormdll.h"

TinyStorm::TinyStorm() {
	mod = MemoryLoadLibrary(stormdll, stormdll_size);
	if (mod) {
		SFileOpenArchive = (SFileOpenArchiveF)MemoryGetProcAddress(mod, (LPCSTR)(266));
		SFileOpenFileEx = (SFileOpenFileExF)MemoryGetProcAddress(mod, (LPCSTR)(268));
		SFileGetFileSize = (SFileGetFileSizeF)MemoryGetProcAddress(mod, (LPCSTR)(265));
		SFileReadFile = (SFileReadFileF)MemoryGetProcAddress(mod, (LPCSTR)(269));
		SFileCloseFile = (SFileCloseFileF)MemoryGetProcAddress(mod, (LPCSTR)(253));
		SFileCloseArchive = (SFileCloseArchiveF)MemoryGetProcAddress(mod, (LPCSTR)(252));
		SFileDestroy = (SFileDestroyF)MemoryGetProcAddress(mod, (LPCSTR)(262));
		SFileSetIoErrorMode = (SFileSetIoErrorModeF)MemoryGetProcAddress(mod, (LPCSTR)(274));
	}
}

TinyStorm::~TinyStorm() {
	if (mod != NULL) {
		MemoryFreeLibrary(mod);
		mod = NULL;
	}
}

bool TinyStorm::IsValid() {
	bool error = false;
	error |= mod == nullptr;
	error |= SFileOpenArchive == nullptr;
	error |= SFileOpenFileEx == nullptr;
	error |= SFileGetFileSize == nullptr;
	error |= SFileReadFile == nullptr;
	error |= SFileCloseFile == nullptr;
	error |= SFileCloseArchive == nullptr;
	error |= SFileDestroy == nullptr;
	error |= SFileSetIoErrorMode == nullptr;
	return !error;
}

bool TinyStorm::readSTR(char* filePath, char** strContent, int* strContentSize, int* dataRequired) {
	bool ok = false;
	if (IsValid()) {
		HANDLE hMpq = NULL;
		this->SFileDestroy();
		this->SFileSetIoErrorMode(0, NULL);
		BOOL resultScx = this->SFileOpenArchive(filePath, 0, 0, &hMpq);
		if (resultScx) {
			HANDLE hFile = NULL;
			BOOL resultChk = this->SFileOpenFileEx(NULL, "staredit\\scenario.chk", 0, &hFile);
			if (resultChk) {
				LONG filesize = this->SFileGetFileSize(hFile, NULL);
				if (filesize >= 0 && filesize <= 1024 * 1024 * 300) { // 300 MB Limit
					MALLOC_N(tmp, char, filesize, { ; });
					if (tmp) {
						DWORD bytesread = 0;
						this->SFileReadFile(hFile, tmp, filesize, &bytesread, NULL);
						if (bytesread == filesize) {
							GetSTR(tmp, filesize, strContent, strContentSize, dataRequired);
							ok = true;
						}
						free(tmp);
					}
				}
				this->SFileCloseFile(hFile);
			}
		}
		this->SFileCloseArchive(hMpq);
	}
	return ok;
}