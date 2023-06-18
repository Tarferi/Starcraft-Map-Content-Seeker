#include "Storm.h"
#include "../chk/Section.h"

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

MapFile* Storm::readSCX(char* filePath, bool* error) {

	HANDLE mapFile;
	void* scx = SFileOpenArchive(filePath, 0, 0x00000100, &mapFile);
	if (!scx) {
		LOG_ERROR("STORM", "Failed to open file %s", filePath)
			* error = true;
		return nullptr;
	}
	LOG_INFO("STORM", "Opened file %s", filePath);

	SFILE_FIND_DATA data;

	HANDLE searchHandle = SFileFindFirstFile(mapFile, "*", &data, ( const char*) 0);

	Array<char*>* fileNames = new Array<char*>();
	Array<unsigned int>* fileSizes = new Array<unsigned int>();
	Array<char*>* filesContents = new Array<char*>();

	bool hasScenarioChk = false;

	do {

		fileSizes->append(data.dwFileSize);
		GET_CLONED_STRING(fileName, data.cFileName, {fileNames->freeItems(); filesContents->freeItems(); SFileFindClose(searchHandle); SFileCloseArchive(mapFile); delete fileNames; delete fileSizes; delete filesContents; return nullptr; });
		fileNames->append(fileName);
		HANDLE fileH;
		MALLOC_N(fileContents, char, data.dwFileSize, {fileNames->freeItems(); filesContents->freeItems(); SFileFindClose(searchHandle); SFileCloseArchive(mapFile); delete fileNames; delete fileSizes; delete filesContents; return nullptr; });
		SFileOpenFileEx(mapFile, data.cFileName, 0, &fileH);
		DWORD read;
		SFileReadFile(fileH, fileContents, data.dwFileSize, &read, 0);
		LOG_INFO("STORM", "Read file %s from %s", data.cFileName, filePath);
		if (read != data.dwFileSize) {
			//LOG_ERROR("STORM", "Read only %d of %d total\n", read, data.dwFileSize);
		}
		SFileCloseFile(fileH);
		filesContents->append(fileContents);

	} while (SFileFindNextFile(searchHandle, &data));

	SFileFindClose(searchHandle);
	SFileCloseArchive(mapFile);
	LOG_INFO("STORM", "Closed file %s", filePath);
	MapFile* mf = new MapFile(filePath, filesContents, fileSizes, fileNames, error);
	return mf;
}

bool Storm::repack(char* file, MapFile* mf, bool onlyCHK) {

	unsigned int CHKIndex = 0;
	CHK* chk = mf->getCHK(CHKReadMode::Partial, &CHKIndex);
	if (chk == nullptr) { return false; }

	unsigned int files = mf->fileNames->getSize() + 1;
	if (files < 4) {
		files = 4;
	}
	remove(file);
	HANDLE fl;

	if (!SFileCreateArchive(( wchar_t*) file, 0, files, &fl)) {
		LOG("STORM", "Failed creating archive \"%s\"", file);
		return false;
	}

	{
		// Write CHK first
		char* chkFile = mf->getCHKFileName();
		char* contents;
		unsigned int fileSize;
		WriteBuffer* wb = new WriteBuffer();
		if (!chk->write(wb)) {
			SFileCloseArchive(fl);
			delete wb;
			return false;
		}
		wb->getWrittenData(( unsigned char**) (&contents), &fileSize);
		HANDLE fh;
		if (!SFileCreateFile(fl, chkFile, 0, fileSize, 0, 0x200, &fh)) {
			if (!SFileCreateFile(fl, "staredit\\scenario.chk", 0, fileSize, 0, 0x00000200, &fh)) {
				SFileCloseArchive(fl);
				delete wb;
				return false;
			}
		}
		if (!SFileWriteFile(fh, contents, fileSize, 0x2)) {
			SFileFinishFile(fh);
			SFileCloseArchive(fl);
			delete wb;
			return false;
		}
		if (!SFileFinishFile(fh)) {
			SFileCloseArchive(fl);
			delete wb;
			return false;
		}
		delete wb;
	}

	if (!onlyCHK) {
		// Write every other files
		for (unsigned int fileIndex = 0; fileIndex < mf->fileNames->getSize(); fileIndex++) {
			if (fileIndex != CHKIndex) {
				LOG("STORM", "Found file \"%s\" with index %d, exporting as \"%s\"", file->fileName, file->v2Index, STR->getRawString(file->v2Index));

				char* fileName = mf->fileNames->get(fileIndex);
				char* contents = mf->contents->get(fileIndex);
				unsigned int fileSize = mf->dataLengths->get(fileIndex);
				if (!strcmp(fileName, "(listfile)") || fileSize <= 1) {
					continue;
				}

				HANDLE fh;
				if (!SFileCreateFile(fl, fileName, 0, fileSize, 0, 0x200, &fh)) {
					LOG("STORM", "Failed creating file \"%s\" in archive \"%s\"", fileName, file->fileName);
					SFileCloseArchive(fl);
					return false;
				}
				if (!SFileWriteFile(fh, contents, fileSize, 0x2)) {
					LOG("STORM", "Failed writing file \"%s\" (%d bytes) in archive \"%s\"", fileName, fileSize, file->fileName);
					SFileFinishFile(fh);
					SFileCloseArchive(fl);
					return false;
				}
				SFileFinishFile(fh);
			}
		}
	}

	//SFileCompactArchive(fl, 0, 0);
	SFileCloseArchive(fl);
	return true;
}

bool Storm::writeSCX(char* ffile, MapFile* mf, bool _repack, bool onlyCHK) {
	GET_CLONED_STRING(file, ffile, {});
	for (unsigned int cI = 0; cI < strlen(file); cI++) {
		if (file[cI] == '\\') {
			file[cI] = '/';
		}
	}
	remove(file);
	if (_repack) {
		if (repack(file, mf, onlyCHK)) {
			free(file);
			return true;
		}
		free(file);
		return false;
	}
	{
		FILE* f;
		if (fopen_s(&f, mf->originalInputFile, "rb")) {
			free(file);
			LOG_ERROR("STORM", "Failed to open original file");
			return false;
		}
		bool error = false;
		ReadBuffer rb(f, &error);
		fclose(f);
		if (error || !rb.good) {
			free(file);
			return false;
		}
		unsigned int dataSize = rb.getDataSize();
		unsigned char* data = rb.readArray(dataSize, &error);
		WriteBuffer wb;
		wb.writeArray(data, dataSize, &error);
		if (error) {
			free(file);
			return false;
		}
		delete data;
		wb.writeToFile(file, &error);
		if (error) {
			free(file);
			return false;
		}
	}

	char* contents;
	unsigned int fileSize;
	WriteBuffer wb;
	CHK* chk = mf->getCHK(CHKReadMode::Partial);
	if (!chk->write(&wb)) {
		free(file);
		return false;
	}
	wb.getWrittenData(( unsigned char**) (&contents), &fileSize);

	HANDLE mapFile = nullptr;
	if (!SFileOpenArchive(file, 0, 0x00000200, &mapFile)) {
		LOG_ERROR("STORM", "Failed to open file %s", file)
			free(file);
		return false;
	}
	LOG_INFO("STORM", "Opened file %s for writing", file);

	HANDLE newFileH;
	char* fName = mf->getCHKFileName();

	bool removed = SFileRemoveFile(mapFile, fName, 0);

	if (!SFileCreateFile(mapFile, fName, 0, fileSize, 0, 0x80000000 | 0x00000200, &newFileH)) {
		if (!SFileCreateFile(mapFile, fName, 0, fileSize, 0, 0x80000000, &newFileH)) {



			if (!SFileCreateFile(mapFile, fName, 0, fileSize, 0, 0x00000200, &newFileH)) {
				if (!SFileCreateFile(mapFile, fName, 0, fileSize, 0, 0, &newFileH)) {

					if (!SFileCreateFile(mapFile, "staredit\\scenario.chk", 0, fileSize, 0, 0x00000200, &newFileH)) {
						if (!SFileCreateFile(mapFile, "staredit\\scenario.chk", 0, fileSize, 0, 0, &newFileH)) {
							SFileCloseArchive(mapFile);
							free(file);
							return false;
							//return repack(ffile, mf);
						}
					}
				}
			}
		}
	}

	if (!SFileWriteFile(newFileH, contents, fileSize, 0x08 | 0x02)) {
		SFileFinishFile(newFileH);
		SFileCloseArchive(mapFile);
		free(file);
		return false;
	}
	if (!SFileFinishFile(newFileH)) {
		SFileCloseArchive(mapFile);
		free(file);
		return false;
	}

	SFileCompactArchive(mapFile, 0, 0);
	SFileCloseArchive(mapFile);
	free(file);
	return true;
}