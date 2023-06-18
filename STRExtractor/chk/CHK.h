#pragma once

#include "../common/common.h"
#include "Section.h"
#include "../common/ReadBuffer.h"

class MapFile;

namespace CHKReadMode {
	enum CHKReadMode {
		Complete,
		Partial
	};
}

class CHK {
public:
	CHK(char* data, unsigned int size, CHKReadMode::CHKReadMode mode);
	~CHK();
	bool write(WriteBuffer* buffer);
	Section* getSection(const char* name);
	Section* getSection(const char* name, unsigned int order);
	Array<Section*> sections;
	bool isValid() {
		return this->valid;
	}
	bool removeSection(Section* section);
private:

	ReadBuffer* buffer;
	bool valid;
	bool parse(char* data, unsigned int totalDataLength, CHKReadMode::CHKReadMode mode);

	unsigned char* partialData = nullptr;
	unsigned int partialDataLength;
};
