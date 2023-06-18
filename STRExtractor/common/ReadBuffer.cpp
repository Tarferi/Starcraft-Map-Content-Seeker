#include "ReadBuffer.h"
#include <stdio.h>

ReadBuffer::ReadBuffer(FILE* file, bool* error) {
	this->size = 0;
	this->dataSize = 0;
	this->position = 0;
	this->rawData = nullptr;
	unsigned char buffer[READBUFFER_BUFFER_SIZE];
	while (true) {
		size_t read = fread(buffer, sizeof(char), READBUFFER_BUFFER_SIZE, file);
		if (read == 0) {
			break;
		} else {
			this->append(buffer, read, error);
		}
	}

}

ReadBuffer::ReadBuffer(unsigned char* buffer, unsigned int size, bool* error) {
	this->size = 0;
	this->dataSize = 0;
	this->position = 0;
	this->rawData = nullptr;
	this->append(buffer, size, error);
}

ReadBuffer::~ReadBuffer() {
	if (this->size != 0) {
		free(this->rawData);
		this->size = 0;
		this->position = 0;
		this->dataSize = 0;
		this->rawData = nullptr;
	}
}

unsigned char ReadBuffer::readByte(bool* error) {
	if (this->dataSize == this->position) {
		*error = true;
		return 0;
	}
	unsigned char byte = this->rawData[this->position];
	this->position++;
	return byte;
}

unsigned short ReadBuffer::readShort(bool* error) {
	if (this->dataSize + 1 == this->position) {
		*error = true;
		return 0;
	}
	position += 2;
	if (endian == Endian::LittleEndian) {
		return (this->rawData[this->position - 2]) + (this->rawData[this->position - 1] << 8);
	} else {
		return (this->rawData[this->position - 1]) + (this->rawData[this->position - 2] << 8);
	}
}

unsigned int ReadBuffer::readInt(bool* error) {
	if (this->dataSize + 3 == this->position) {
		*error = true;
		return 0;
	}
	position += 4;
	if (endian == Endian::LittleEndian) {
		return (this->rawData[this->position - 4]) + (this->rawData[this->position - 3] << 8) + (this->rawData[this->position - 2] << 16) + (this->rawData[this->position - 1] << 24);
	} else {
		return (this->rawData[this->position - 1]) + (this->rawData[this->position - 2] << 8) + (this->rawData[this->position - 3] << 16) + (this->rawData[this->position - 4] << 24);
	}
}

unsigned char* ReadBuffer::readArray(unsigned int length, bool* error) {
	if (this->position + length > this->dataSize) {
		*error = true;
		return 0;
	}
	MALLOC_N(data, unsigned char, length, {*error = true; return 0;});
	memcpy(data, &(this->rawData[this->position]), sizeof(unsigned char)* length);
	this->position += length;
	return data;
}

unsigned char* ReadBuffer::readFixedLengthString(unsigned int length, bool* error) {
	if (this->position + length > this->dataSize) {
		*error = true;
		return 0;
	}
	MALLOC_N(data, unsigned char, length + 1, {*error = true; return 0;});
	memcpy(data, &(this->rawData[this->position]), sizeof(unsigned char)* length);
	this->position += length;
	data[length] = 0;
	return data;
}

unsigned int ReadBuffer::getPosition() {
	return this->position;
}

void ReadBuffer::setPosition(unsigned int position) {
	if (position > this->dataSize) {
		throw 1;
	}
	this->position = position;
}

bool ReadBuffer::isDone() {
	return this->position >= this->dataSize;
}

void ReadBuffer::append(unsigned char* buffer, int size, bool* error) {
	if (this->dataSize + size > this->size) { // Not enough space
		if (this->rawData != nullptr) { // There is something already
			unsigned char* toFree = this->rawData;
			unsigned int newSize = this->size * 2;
			MALLOC(this->rawData, unsigned char, newSize, {*error = true; free(toFree); this->rawData = nullptr; this->size = 0; return;});
			memcpy(this->rawData, toFree, this->size);
			this->size = newSize;
			free(toFree);
			this->append(buffer, size, error);
		} else { // Not enough space, but no previous data
			unsigned int newSize = READBUFFER_BUFFER_SIZE;
			MALLOC(this->rawData, unsigned char, newSize, {*error = true;  this->rawData = nullptr; this->size = 0; return;});
			this->size = newSize;
			this->append(buffer, size, error);
		}
	} else { // Enough space
		memcpy(&(this->rawData[this->dataSize]), buffer, size);
		this->dataSize += size;
	}
}
