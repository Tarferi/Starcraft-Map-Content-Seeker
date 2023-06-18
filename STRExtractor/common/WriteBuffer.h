#pragma once
#include "common.h"

#define WRITEBUFFER_BUFFERSIZE 1024*10

#define WRITEBUFFER_INCREASE_FACTOR 2

class WriteBuffer {
public:
	WriteBuffer();
	~WriteBuffer();

	void writeByte(unsigned char value, bool* error);
	void writeShort(unsigned short value, bool* error);
	void writeInt(unsigned int value, bool* error);
	void writeArray(unsigned char* data, unsigned int length, bool* error);
	void writeFixedLengthString(unsigned char* string, bool* error);
	void writeZeroDelimitedString(unsigned char* string, bool* error);

	unsigned int getPosition();
	void setPosition(unsigned int position);
	void getWrittenData(unsigned char** dataPtr, unsigned int* lengthPtr);

	void writeToFile(char* file, bool* error);

	Endian endian = Endian::LittleEndian;
private:
	unsigned char* rawData;
	unsigned int size;
	unsigned int dataSize;
	unsigned int position;

	void expandBuffer(bool* error);
	void ensureEnoughSpace(unsigned int neededLength, bool* error);
};

