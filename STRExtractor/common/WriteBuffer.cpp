#include "WriteBuffer.h"



WriteBuffer::WriteBuffer() {
	this->size = 0;
	this->position = 0;
	this->dataSize = 0;
	this->rawData = nullptr;
}

WriteBuffer::~WriteBuffer() {
	if (this->size != 0) {
		if (this->rawData != nullptr) {
			free(this->rawData);
			this->rawData = nullptr;
		}
		this->size = 0;
		this->dataSize = 0;
		this->position = 0;
	}
}

void WriteBuffer::writeByte(unsigned char value, bool* error) {
	this->ensureEnoughSpace(256, error);
	if (*error) {
		return;
	}
	this->rawData[this->position] = value;
	this->position++;
	if (this->position > this->dataSize) {
		this->dataSize = this->position;
	}
}

void WriteBuffer::writeShort(unsigned short value, bool* error) {
	if (endian == Endian::LittleEndian) {
		this->writeByte((value >> 0) & 0xff, error);
		if (*error) {
			return;
		}
		this->writeByte((value >> 8) & 0xff, error);
	} else {
		this->writeByte((value >> 8) & 0xff, error);
		if (*error) {
			return;
		}
		this->writeByte((value >> 0) & 0xff, error);
	}
}

void WriteBuffer::writeInt(unsigned int value, bool* error) {
	if (endian == Endian::LittleEndian) {
		this->writeShort((value >> 0) & 0xffff, error);
		if (*error) {
			return;
		}
		this->writeShort((value >> 16) & 0xffff, error);
	} else {
		this->writeShort((value >> 16) & 0xffff, error);
		if (*error) {
			return;
		}
		this->writeShort((value >> 0) & 0xffff, error);
	}
}

void WriteBuffer::writeArray(unsigned char* data, unsigned int length, bool* error) {
	this->ensureEnoughSpace(length, error);
	if (*error) {
		return;
	}
	memcpy(&(this->rawData[this->dataSize]), data, length);
	this->position += length;
	if (this->position > this->dataSize) {
		this->dataSize = this->position;
	}
}

void WriteBuffer::writeFixedLengthString(unsigned char* string, bool* error) {
	int length = strlen(( char*) string);
	this->writeArray(string, length, error);
}

void WriteBuffer::writeZeroDelimitedString(unsigned char* string, bool* error) {
	this->writeFixedLengthString(string, error);
	if (*error) {
		return;
	}
	this->writeByte(0, error);
}

unsigned int WriteBuffer::getPosition() {
	return this->position;
}

void WriteBuffer::setPosition(unsigned int position) {
	this->position = position;
}

void WriteBuffer::getWrittenData(unsigned char** dataPtr, unsigned int* lengthPtr) {
	*dataPtr = this->rawData;
	*lengthPtr = this->dataSize;
}

void WriteBuffer::writeToFile(char* file, bool* error) {
	FILE* f;
	if (!fopen_s(&f, file, "wb")) {
		fwrite(this->rawData, 1, this->dataSize, f);
		fclose(f);
		return;
	}
	*error = true;
}

void WriteBuffer::expandBuffer(bool* error) {

	if (this->rawData != nullptr) { // There is something already
		unsigned char* toFree = this->rawData;
		unsigned int newSize = this->size * WRITEBUFFER_INCREASE_FACTOR;
		MALLOC(this->rawData, unsigned char, newSize, {free(toFree); *error = true; return; });
		memcpy(this->rawData, toFree, this->size);
		this->size = newSize;
		free(toFree);
	} else { // Not enough space, but no previous data
		unsigned int newSize = WRITEBUFFER_BUFFERSIZE;
		MALLOC(this->rawData, unsigned char, newSize, {*error = true; return; });
		this->size = newSize;
	}
}

void WriteBuffer::ensureEnoughSpace(unsigned int neededLength, bool* error) {
	if (this->dataSize + neededLength > this->size) {
		this->expandBuffer(error);
		if (*error) {
			return;
		}
		this->ensureEnoughSpace(neededLength, error);
	}
}


