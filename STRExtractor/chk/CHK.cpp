#include "CHK.h"
#include "../common/common.h"

CHK::CHK(char* data, unsigned int size, CHKReadMode::CHKReadMode mode) {
	bool error = false;
	this->buffer = new ReadBuffer((unsigned char*) data, size, &error);
	if (error) {
		this->valid = false;
		return;
	}
	this->valid = this->parse(data, size, mode);
}

CHK::~CHK() {
	for (unsigned int i = 0; i < this->sections.getSize(); i++) {
		Section* section = this->sections[i];
		if (section != nullptr) {
			if (!strcmp(section->getName(), "TRIG")) {
				delete section;
				continue;
			}
			delete section;
		}
		section = nullptr;
	};
	if (partialData != nullptr) {
		free(partialData);
		partialData = nullptr;
	}
	delete this->buffer;
}

bool CHK::write(WriteBuffer* buffer) {
	for (unsigned int i = 0; i < this->sections.getSize(); i++) {
		Section* section = this->sections[i];
		bool error = false;
		buffer->writeFixedLengthString((unsigned char*) section->getName(), &error);
		if (error) {
			LOG_ERROR("CHK", "Failed to write section \"%s\"", section->getName());
			return false;
		}
		unsigned int prePosition = buffer->getPosition();
		buffer->writeInt(0, &error); // Later replace this with size
		if (error) {
			LOG_ERROR("CHK", "Failed to write section \"%s\"", section->getName());
			return false;
		}
		section->write(buffer);
		unsigned int postPosition = buffer->getPosition();
		unsigned int sectionSize = (postPosition - prePosition) - 4;
		buffer->setPosition(prePosition);
		buffer->writeInt(sectionSize, &error);
		if (error) {
			LOG_ERROR("CHK", "Failed to write section \"%s\"", section->getName());
			return false;
		}
		buffer->setPosition(postPosition);
	}
	if (this->partialData != nullptr) {
		bool error = false;
		buffer->writeArray(this->partialData, this->partialDataLength, &error);
		if (error) { return false; }
	}
	return true;
}

Section* CHK::getSection(const char* name) {
	return getSection(name, 0);
}

Section* CHK::getSection(const char* name, unsigned int order) {
	for (unsigned int i = 0; i < this->sections.getSize(); i++) {
		Section* section = this->sections[i];
		char* sname = section->getName();
		if (!strcmp(name, sname)) {
			if (order == 0) {
				return section;
			} else {
				order--;
			}
		}
	};
	return nullptr;
}

bool CHK::parse(char* data, unsigned int totalDataLength, CHKReadMode::CHKReadMode mode) {
	LOG_INFO("CHK", "BEGIN PARSING of %d bytes", this->buffer->getDataSize());
	this->buffer->endian = Endian::LittleEndian;
	bool hasSTR = false;
	bool hasTRIG = false;
	bool hasMBRF = false;
	unsigned int begin = this->buffer->getPosition();
	while (!this->buffer->isDone()) {
		bool error = false;
		char* name = (char*) this->buffer->readFixedLengthString(4, &error);
		if (error) {
			LOG_ERROR("CHK", "Error reading section name");
			return false;
		}
		unsigned int size = this->buffer->readInt(&error);
		if (error) {
			LOG_ERROR("CHK", "Error reading section size");
			free(name);
			return false;
		}
		unsigned int restLength = this->buffer->getDataSize() - this->buffer->getPosition();
		if (size > restLength) {
			LOG_INFO("CHK", "Found section \"%s\" of size %d which exceeds the file", name, size);
			free(name);
			return false;
		}
		LOG_INFO("CHK", "Found section \"%s\" of size %d", name, size);
		Section* section;
		unsigned int end = this->buffer->getPosition();

		if (!strcmp(name, "STR ")) {
			section = new Section_STR_((unsigned char*) name, size, this->buffer);
			hasSTR = true;
		} else if (!strcmp(name, "TRIG")) {
			section = new Section_TRIG((unsigned char*) name, size, this->buffer);
			hasTRIG = true;
		} else if (!strcmp(name, "MRGN")) {
			section = new Section_MRGN((unsigned char*) name, size, this->buffer);
		} else if (!strcmp(name, "MBRF")) {
			section = new Section_MBRF((unsigned char*) name, size, this->buffer);
			hasMBRF = true;
		} else if (!strcmp(name, "SPRP")) {
			section = new Section_SPRP((unsigned char*) name, size, this->buffer);
		} else if (!strcmp(name, "UNIS")) {
			section = new Section_UNIS((unsigned char*) name, size, this->buffer);
		} else if (!strcmp(name, "UNIx")) {
			section = new Section_UNIx((unsigned char*) name, size, this->buffer);
		} else if (!strcmp(name, "FORC")) {
			section = new Section_FORC((unsigned char*) name, size, this->buffer);
		} else if (!strcmp(name, "SWNM")) {
			section = new Section_SWNM((unsigned char*) name, size, this->buffer);
		} else if (!strcmp(name, "DIM ")) {
			section = new Section_DIM_((unsigned char*) name, size, this->buffer);
		} else if (!strcmp(name, "MTXM")) {
			section = new Section_MTXM((unsigned char*) name, size, this->buffer);
		} else if (!strcmp(name, "THG2")) {
			section = new Section_THG2((unsigned char*) name, size, this->buffer);
		} else if (!strcmp(name, "ERA ")) {
			section = new Section_ERA_((unsigned char*) name, size, this->buffer);
		} else if (!strcmp(name, "UNIT")) {
			section = new Section_UNIT((unsigned char*) name, size, this->buffer);
		} else if (!strcmp(name, "ISOM")) {
			section = new Section_ISOM((unsigned char*) name, size, this->buffer);
		} else if (!strcmp(name, "TILE")) {
			section = new Section_TILE((unsigned char*) name, size, this->buffer);
		} else if (!strcmp(name, "CMP ")) {
			continue;
		} else {
			section = new BasicSection((unsigned char*) name, size, this->buffer);
		}

		updateBufferWithinSection(section, data, totalDataLength, end - begin);
		this->sections.append(section);
		if (!section->process()) {
			LOG_ERROR("CHK", "Failed to process section \"%s\" (size %d)", name, size);
			return false;
		}
		if (mode == CHKReadMode::Partial && hasSTR && hasTRIG && hasMBRF) { // Has everything
			unsigned int restLength = this->buffer->getDataSize() - this->buffer->getPosition();
			unsigned char* data = this->buffer->readArray(restLength, &error);
			if (error) { return false; }
			this->partialDataLength = restLength;
			this->partialData = data;
			return true;
		}
	}
	return true;
}

bool CHK::removeSection(Section* section) {
	for (unsigned int i = 0; i < this->sections.getSize(); i++) {
		Section* sect = this->sections[i];
		if (sect == section) {
			this->sections.remove(i);
			return true;
		}
	}
	return false;
}
