#include "MapFile.h"
#include "../storm/Storm.h"


//#define STORM_ALLOW_CHK_ADVANCED_SCAN
//#define EXTRACT_MPQ

#ifdef EXTRACT_MPQ
#include <shellapi.h>
#include <Shlobj.h>
#include <Shlobj_core.h>
#define MPQ_EXTRACT_PATH "C:\\Users\\Tom\\Desktop\\Documents\\Visual Studio 2015\\Projects\\TranslateLib\\MPQ_CONTENTS\\"
#endif

MapFile::MapFile(char* originalFilename, Array<char*>* data, Array<unsigned int>* dataLengths, Array<char*>* fileNames, bool* error) {
	this->originalInputFile = originalFilename;
	this->chk = nullptr;
	this->contents = data;
	this->fileNames = fileNames;
	this->dataLengths = dataLengths;
#ifdef EXTRACT_MPQ
	// Delete the directory
	SHFILEOPSTRUCTA tmp;
	memset(&tmp, 0, sizeof(SHFILEOPSTRUCTA));
	tmp.wFunc = FO_DELETE;
	char dirBuffer[1024];
	memset(dirBuffer, 0, 1024);
	memcpy(dirBuffer, MPQ_EXTRACT_PATH, strlen(MPQ_EXTRACT_PATH));
	tmp.pFrom = dirBuffer;
	tmp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NO_UI;
	SHFileOperationA(&tmp);

	for (unsigned int i = 0; i < fileNames->getSize(); i++) {
		char* contents = data->get(i);
		char* fileName = fileNames->get(i);
		unsigned int length = dataLengths->get(i);
		char buffer[1024];
		sprintf_s(buffer, "%s%s", MPQ_EXTRACT_PATH, fileName);
		remove(buffer);

		// Get absolute directory
		char* lastBackslashPosition = nullptr;
		unsigned int fileNameLen = strlen(buffer);
		for (unsigned int o = fileNameLen - 1; o > 0; o--) {
			char chr = buffer[o];
			if (chr == '\\') {
				lastBackslashPosition = &(buffer[o]);
				break;
			}
		}
		if (lastBackslashPosition == nullptr) {
			LOG_ERROR("MAPFILE", "Failed to extract directory of file %s", buffer);
			continue;
		}
		*lastBackslashPosition = 0; // Terminate at backslast

		// Create directories recursively
		SHCreateDirectoryExA(nullptr, buffer, nullptr);

		*lastBackslashPosition = '\\'; // Unterminate at backslast

		FILE* f;
		if (!fopen_s(&f, buffer, "wb")) {
			fwrite(contents, sizeof(char), length, f);
			fclose(f);
		}
	}
#endif
}

MapFile::~MapFile() {
	if (this->chk != nullptr) {
		delete this->chk;
		this->chk = nullptr;
	}
	if (this->contents != nullptr) {
		for (unsigned int i = 0; i < this->contents->getSize(); i++) {
			char* content = this->contents->get(i);
			if (content != nullptr) {
				free(content);
				this->contents->set(i, nullptr);
			}
		}
		delete this->contents;
	}
	if (this->fileNames != nullptr) {
		for (unsigned int i = 0; i < this->fileNames->getSize(); i++) {
			if (this->fileNames->get(i) != nullptr) {
				free(this->fileNames->get(i));
			}
		}
		delete this->fileNames;
		this->fileNames = nullptr;
	}
	if (this->dataLengths != nullptr) {
		delete this->dataLengths;
	}
}

CHK* MapFile::getCHK(CHKReadMode::CHKReadMode mode) {
	unsigned int index = 0;
	return getCHK(mode, &index);
}

static const char* SECTIONS[] = {"TYPE", "VER ", "IVER", "IVE2", "VCOD", "IOWN", "OWNR", "ERA ", "DIM ", "SIDE", "MTXM", "PUNI", "UPGR", "PTEC", "UNIT", "ISOM", "TILE", "DD2 ", "THG2", "MASK", "STR ", "UPRP", "UPUS", "MRGN", "TRIG", "MBRF", "SPRP", "FORC", "WAV ", "UNIS", "UPGS", "TECS", "SWNM", "COLR", "PUPx", "PTEx", "UNIx", "UPGx", "TECx"};

bool isCHK(char* data, unsigned int dataLength) {
	if (dataLength > 4) { // Could verify CHK
		uint8 sectionName[5] = {0, 0, 0, 0, 0};
		memcpy(sectionName, data, 4);
		for (uint32 i = 0; i < sizeof(SECTIONS) / sizeof(char*); i++) {
			if (!memcmp(SECTIONS[i], sectionName, 4)) { // Possible CHK, Patch
				return true;
			}
		}
	}
	return false;
}

CHK* MapFile::getCHK(CHKReadMode::CHKReadMode mode, unsigned int* fileIndex) {
	if (this->chk == nullptr) {
		for (unsigned int i = 0; i < this->fileNames->getSize(); i++) {
			if (!strcmp(this->fileNames->get(i), "staredit\\scenario.chk")) {
				sprintf_s(this->chkFileName, "staredit\\scenario.chk");
				this->chk = new CHK(this->contents->get(i), this->dataLengths->get(i), mode);
				CHKIndex = i;
				*fileIndex = i;
				return this->chk;
			}
		}
		// No chk?
		for (unsigned int ii = 0; ii < this->fileNames->getSize(); ii++) {
			char buffer1[1024];
			char buffer2[1024];
			sprintf_s(buffer1, "%d.xxx", ii);
			sprintf_s(buffer2, "File00000000.xxx");
			unsigned int len1 = strlen(buffer1);
			unsigned int len2 = strlen(buffer2);
			memcpy(buffer2 + (len2 - len1), buffer1, len1);
			sprintf_s(this->chkFileName, buffer2);
			for (unsigned int i = 0; i < this->fileNames->getSize(); i++) {
				if (!strcmp(this->fileNames->get(i), this->chkFileName) && isCHK(this->contents->get(i), this->dataLengths->get(i))) { // Cheatio
					this->chk = new CHK(this->contents->get(i), this->dataLengths->get(i), mode);
					CHKIndex = i;
					*fileIndex = i;
					return this->chk;
				}
			}
		}
		CHKIndex = 0;
#ifdef STORM_ALLOW_CHK_ADVANCED_SCAN
		for (unsigned int iii = 0; iii < this->fileNames->getSize(); iii++) {
			if (isCHK(this->contents->get(iii), this->dataLengths->get(iii))) {
				sprintf_s(this->chkFileName, this->fileNames->get(iii));
				this->chk = new CHK(this->contents->get(iii), this->dataLengths->get(iii), mode);
				CHKIndex = iii;
				*fileIndex = iii;
				return this->chk;
			}
		}
#endif
	}
	*fileIndex = CHKIndex;
	return this->chk;
}

bool hasFile(MapFile* mf, char* name) {
	for (unsigned int wavIndex = 0; wavIndex < mf->fileNames->getSize(); wavIndex++) {
		char* wavFile = mf->fileNames->get(wavIndex);
		if (!strcmp(wavFile, name)) {
			return true;
		}
	}
	return false;
}

void MapFile::writeToFile(Storm* storm, char* name, bool* error, bool repack, bool onlyCHK) {

	// Create new mapfile and write it
	if (!storm->writeSCX(name, this, repack, onlyCHK)) {
		*error = true;
	}

}

bool MapFile::renameFile(char* fromName, char* toName) {
	for (unsigned int wavIndex = 0; wavIndex < fileNames->getSize(); wavIndex++) {
		char* fileName = fileNames->get(wavIndex);
		if (!strcmp(fileName, fromName)) {
			GET_CLONED_STRING(newName, toName, {return false; });
			fileNames->remove(wavIndex);
			if (!fileNames->insert(wavIndex, newName)) { free(newName); return false; }
			free(fileName);
		}
	}
	return true;
}
