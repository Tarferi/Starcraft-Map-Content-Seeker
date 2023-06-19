#include "Storm.h"
#include "../storm/StormLib.h"

bool decompressLib(char** data, unsigned int* size) {
	bool error = false;
	char* newData;
	unsigned int newSize;
	decompress(( char*) _acStormLib, _acStormLibLen, &newData, &newSize, &error);
	*data = newData;
	*size = newSize;
	return !error;
}

Storm::Storm(bool* error) {
	this->lib = nullptr;
	unsigned int libSize;
	if (!decompressLib(&this->decompressedLib, &libSize)) {
		LOG_ERROR("STORM", "Decompression failed");
		*error = true;
		return;
	}

	this->lib = MemoryLoadLibrary(this->decompressedLib, libSize);
	if (!this->lib) {
		*error = true;
		return;
	}
	SFileOpenArchive = ( SFileOpenArchiveF) MemoryGetProcAddress(this->lib, "SFileOpenArchive");
	SFileFindNextFile = ( SFileFindNextFileF) MemoryGetProcAddress(this->lib, "SFileFindNextFile");
	SFileFindFirstFile = ( SFileFindFirstFileF) MemoryGetProcAddress(this->lib, "SFileFindFirstFile");
	SFileFindClose = ( SFileFindCloseF) MemoryGetProcAddress(this->lib, "SFileFindClose");
	SFileExtractFile = ( SFileExtractFileF) MemoryGetProcAddress(this->lib, "SFileExtractFile");
	SFileReadFile = ( SFileReadFileF) MemoryGetProcAddress(this->lib, "SFileReadFile");
	SFileOpenFileEx = ( SFileOpenFileExF) MemoryGetProcAddress(this->lib, "SFileOpenFileEx");
	SFileCloseFile = ( SFileCloseFileF) MemoryGetProcAddress(this->lib, "SFileCloseFile");
	SFileCreateFile = ( SFileCreateFileF) MemoryGetProcAddress(this->lib, "SFileCreateFile");
	SFileWriteFile = ( SFileWriteFileF) MemoryGetProcAddress(this->lib, "SFileWriteFile");
	SFileFinishFile = ( SFileFinishFileF) MemoryGetProcAddress(this->lib, "SFileFinishFile");
	SFileCreateArchive = ( SFileCreateArchiveF) MemoryGetProcAddress(this->lib, "SFileCreateArchive");
	SFileCloseArchive = ( SFileCloseArchiveF) MemoryGetProcAddress(this->lib, "SFileCloseArchive");
	SFileRemoveFile = ( SFileRemoveFileF) MemoryGetProcAddress(this->lib, "SFileRemoveFile");
	SFileAddFileEx = ( SFileAddFileExF) MemoryGetProcAddress(this->lib, "SFileAddFileEx");
	SFileCompactArchive = ( SFileCompactArchiveF) MemoryGetProcAddress(this->lib, "SFileCompactArchive");

	LOG_INFO("STORM", "Storm library initiated");
}

Storm::~Storm() {
	if (this->decompressedLib != nullptr) {
		free(this->decompressedLib);
		this->decompressedLib = nullptr;
	}
	if (this->lib != nullptr) {
		MemoryFreeLibrary(this->lib);
		this->lib = nullptr;
	}
	LOG_INFO("STORM", "Storm library uninitiated");
}

bool ReadFixedLengthString(char* data, unsigned int dataLength, unsigned int& position, char* target, int size) {
	if (position + size <= dataLength) {
		memcpy(target, &(data[position]), size);
		position += size;
		return true;
	}
	return false;
}

bool ReadShort(char* data, unsigned int dataLength, unsigned int& position, unsigned short* target) {
	char tmp[2];
	if (ReadFixedLengthString(data, dataLength, position, tmp, 2)) {
		unsigned short x1 = tmp[0] & 0xff;
		unsigned short x2 = tmp[1] & 0xff;
		*target = (x2 << 8) + x1;
		return true;
	}
	return false;
}

bool ReadInt(char* data, unsigned int dataLength, unsigned int& position, unsigned int* target) {
	char tmp[4];
	if (ReadFixedLengthString(data, dataLength, position, tmp, 4)) {
		unsigned int x1 = tmp[0] & 0xff;
		unsigned int x2 = tmp[1] & 0xff;
		unsigned int x3 = tmp[2] & 0xff;
		unsigned int x4 = tmp[3] & 0xff;
		*target = (x4 << 24) + (x3 << 16) + (x2 << 8) + x1;
		return true;
	}
	return false;
}

void GetSTR(char* data, int dataLength, char** dataStr, int* dataStrLength, int* dataRequired) {
	unsigned int position = 0;

	bool error = false;
	while (!error) {
		char name[] = { 0,0,0,0,0 };
		unsigned int length = 0;

		error |= !ReadFixedLengthString(data, dataLength, position, name, 4);
		if (error) { break; }
		error |= !ReadInt(data, dataLength, position, &length);
		if (error) { break; }

		if (!strcmp(name, "STR ")) {
			unsigned int remainingLength = dataLength - position;
			*dataRequired = remainingLength;
			unsigned int strRealLength = remainingLength < length ? remainingLength : length;
			if (strRealLength > 0) {
				MALLOC_N(STR, char, strRealLength, { return; });
				memcpy(STR, &(data[position]), strRealLength);
				*dataStr = STR;
				*dataStrLength = strRealLength;
				return;
			}
		} else {
			if (position + length <= (unsigned int)dataLength) {
				position += length;
			} else {
				break;
			}
		}
	}
}

bool Storm::readSTR(char* filePath, char** strContent, int* strContentSize, int* dataRequired) {

	HANDLE mapFile;
	void* scx = SFileOpenArchive(filePath, 0, 0x00000100, &mapFile);
	if (!scx) {
		LOG_ERROR("STORM", "Failed to open file %s", filePath);
		return false;
	}
	LOG_INFO("STORM", "Opened file %s", filePath);
		
	char* STRDATA = nullptr;
	int STRDATALength = 0;

	auto forEachFile = [this, &mapFile](auto cb) {
		SFILE_FIND_DATA data;
		HANDLE searchHandle = SFileFindFirstFile(mapFile, "*", &data, (const char*)0);

		do {
			int fileSize = data.dwFileSize;

			auto reader = [&]() -> char* {
				HANDLE fileH;
				MALLOC_N(fileContents, char, data.dwFileSize, { SFileFindClose(searchHandle); return nullptr; });
				SFileOpenFileEx(mapFile, data.cFileName, 0, &fileH);
				DWORD read;
				SFileReadFile(fileH, fileContents, data.dwFileSize, &read, 0);
				LOG_INFO("STORM", "Read file %s from %s", data.cFileName, filePath);
				if (read != data.dwFileSize) {
					SFileCloseFile(fileH);
					free(fileContents);
					return nullptr;
				} else {
					SFileCloseFile(fileH);
					return fileContents;
				}
			};

			if (strcmp(data.cFileName, "(listfile)")) {
				cb(data.cFileName, fileSize, reader);
			}

		} while (SFileFindNextFile(searchHandle, &data));
		SFileFindClose(searchHandle);
	};

	auto lookForCHK = [&](char* fileName, int fileSize, auto reader) {
		if (!STRDATA && !strcmp(fileName, "staredit\\scenario.chk")) {
			char* chk = reader();
			if (chk) {
				GetSTR(chk, fileSize, &STRDATA, &STRDATALength, dataRequired);
				free(chk);
			}
		}
	};

	auto lookForAnyCHK = [&](char* fileName, int fileSize, auto reader) {
		if (!STRDATA) {
			char* data = reader();
			if (data) {
				GetSTR(data, fileSize, &STRDATA, &STRDATALength, dataRequired);
				free(data);
			}
		}
	};

	forEachFile(lookForCHK);
	if (!STRDATA) {
		forEachFile(lookForAnyCHK);
	}

	SFileCloseArchive(mapFile);
	if (STRDATA) {
		*strContent = STRDATA;
		*strContentSize = STRDATALength;
		return true;
	}
	return false;
}
