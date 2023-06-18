#pragma once

#include "common.h"

#define READBUFFER_BUFFER_SIZE 1024*10

class ReadBuffer {

public:
	ReadBuffer(FILE* file, bool* error);
	ReadBuffer(unsigned char* buffer, unsigned int size, bool* error);
	~ReadBuffer();

	unsigned char readByte(bool* error);
	unsigned short readShort(bool* error);
	unsigned int readInt(bool* error);
	unsigned char* readArray(unsigned int length, bool* error);
	unsigned char* readFixedLengthString(unsigned int length, bool* error);

	unsigned int getPosition();
	void setPosition(unsigned int position);
	bool isDone();

	bool good = true;

	unsigned int getDataSize() {
		return this->dataSize;
	}

	Endian endian = Endian::LittleEndian;
private:

	unsigned int size = 0;
	unsigned int position = 0;
	unsigned int dataSize = 0;
	unsigned char* rawData = nullptr;

	void append(unsigned char* buffer, int size, bool* error);
};

