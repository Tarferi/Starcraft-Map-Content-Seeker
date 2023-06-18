#pragma once
#include "../common/common.h"
#include "CHK.h"

class Storm;

class MapFile {
public:
	MapFile(char* originalInputFile, Array<char*>* data, Array<unsigned int>* dataLengths, Array<char*>* fileNames, bool* error);
	~MapFile();

	CHK* getCHK(CHKReadMode::CHKReadMode mode, unsigned int* fileIndex);

	CHK* getCHK(CHKReadMode::CHKReadMode mode);
	
	char* getCHKFileName() {
		return chkFileName;
	}

	void writeToFile(Storm* storm, char* name, bool* error, bool repack, bool onlyCHK);

	Array<char*>* contents;
	Array<unsigned int>* dataLengths;
	Array<char*>* fileNames;

	bool renameFile(char* fromName, char* toName);

	char* originalInputFile;

private:
	CHK* chk;
	char chkFileName[1024] = {0};

	unsigned int CHKIndex = 0;
};
