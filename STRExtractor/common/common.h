#pragma once

#include "stdafx.h"
#include "stdlib.h"
#include <stdarg.h>
#include "string.h"
#include "miniz.h"
#include <stdio.h>
#include <cstdint>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

namespace nsEndian {
	enum Enum {
		BigEndian,
		LittleEndian
	};
}

typedef nsEndian::Enum Endian;

//#define TRIG_PRINT
//#define INCLUDE_UNUSED_STRINGS

typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef int8_t bool8;

#define PRINT_R(ignore, format, ...) {char buffer[2048]; sprintf_s(buffer, format, __VA_ARGS__); wb->writeFixedLengthString((unsigned char*) buffer, error); if(*error) {return;}}

#define MALLOC(target, type, size, failBlock) target = (type*) malloc(sizeof(type)*(size)); if(!target){failBlock};

#define MALLOC_N(target, type, size, failBlock) type* target; MALLOC(target, type, size, failBlock);

#define COMMON_CONSTR_SEC_DER(name, base) name(unsigned char* name, unsigned int size, ReadBuffer* buffer, bool isSanc) : base(name, size, buffer, isSanc) {};

#define COMMON_CONSTR_SEC(name) name(unsigned char* name, unsigned int size, ReadBuffer* buffer) : Section(name, size, buffer) {};

#define COMMON_CONSTR_SEC_BS(name) name(unsigned char* name, unsigned int size, ReadBuffer* buffer) : BasicSection(name, size, buffer) {};

#define ENDS_WIDTH(name, suffix) (strlen(name) >= strlen(suffix) ? !strcmp(&(name[strlen(name) - strlen(suffix)]), (char*) suffix): false)

#define GET_CLONED_DATA_SZ(target, type, string, length, spec_sz, failBlock) MALLOC_N(target, type, length + spec_sz, failBlock);if(target){ memcpy(target, string, length*sizeof(type));}

#define GET_CLONED_DATA(target, type, string, length, failBlock) GET_CLONED_DATA_SZ(target, type, string, length, 0, failBlock)

#define GET_CLONED_STRING_LEN(target, string, length, failBlock) GET_CLONED_DATA_SZ(target, char, string, length, 1, failBlock); target[length] = 0

#define GET_CLONED_STRING(target, string, failBlock) GET_CLONED_STRING_LEN(target, string, strlen(string), failBlock)

#define ARRAY_DEFAULT_SIZE 64
#define ARRAY_INCREATE_FACTOR 2;

static int initValue = ARRAY_DEFAULT_SIZE;

//#define DEBUG_LOG



#ifdef DEBUG_LOG

#define __LOG_SKIP(sectionName) _skip |= !strcmp(_section, sectionName);

#define LOG_R(section, fmt, ...) {\
	bool _skip=false;\
	char* _section = (char*) section;\
	__LOG_SKIP("HP REMAPPER")\
	__LOG_SKIP("DAMAGE REMAPPER")\
	if(!_skip){do { fprintf(stderr, "[" section "] " fmt , __VA_ARGS__); } while (0);}}

#define LOG(section, fmt, ...) \
		LOG_R(section, fmt "\n", __VA_ARGS__)

#define LOG_INFO(section, fmt, ...) {\
	bool _skip=false;\
	char* _section = (char*) section;\
	if(!_skip){do { fprintf(stderr, "[" section "] " fmt "\n" , __VA_ARGS__); } while (0);}}


#else
#define LOG(section, fmt, ...)
#define LOG_R(section, fmt, ...)
#define LOG_INFO(section, fmt, ...)
#endif

#define LOG_ERROR(section, fmt, ...) fprintf(stderr, "[" section "] " fmt "\n" , __VA_ARGS__);

#define LOG_LEAK(addr)  fprintf(stderr, "Leaking 0x%X\n", ((unsigned int)addr));

template<typename type> class Array {

	class ArrayProxy {
		Array* array;
		int index;
	public:
		ArrayProxy(Array* array, int index) {
			this->array = array;
			this->index = index;
		}
		type operator= (type value) { array->set(index, value); return array->get(index); }
		operator type() { return array->get(index); }

	};

public:

	void remove(unsigned int index) {
		for (unsigned int i = index; i < this->dataSize - 1; i++) {
			this->rawData[i] = this->rawData[i + 1];
		}
		this->rawData[this->dataSize - 1] = (type) nullptr;
		this->dataSize--;
	}

	bool set(unsigned int index, type value) {
		bool error = false;
		if (index > this->size) {
			this->ensureAdditionalSize(this->size - index, &error);
		}
		if (error) {
			return false;
		}
		this->rawData[index] = value;
		if (index > this->dataSize) {
			this->dataSize = index + 1;
		}
		return true;
	}

	type get(unsigned int index) {
		return this->rawData[index];
	}

	unsigned int getSize() {
		return this->dataSize;
	}

	bool append(type value) {
		bool error = false;
		if (this->dataSize + sizeof(type) >= this->size) {
			this->ensureAdditionalSize(32 * sizeof(type), &error);
		}
		if (error) {
			return false;
		}
		this->rawData[this->dataSize] = value;
		this->dataSize++;
		return true;
	}

	bool insert(unsigned int index, type value) {
		if (!append(value)) { return false; }
		for (unsigned int i = this->dataSize - 1; i > index; i--) {
			this->rawData[i] = this->rawData[i - 1];
		}
		this->rawData[index] = value;
		return true;
	}

	void freeItems() {
		for (unsigned int i = 0; i < this->getSize(); i++) {
			type fn = this->get(i);
			free(fn);
		}
	}

	ArrayProxy operator[] (unsigned int index) {
		return ArrayProxy(this, index);
	}

	~Array() {
		if (this->rawData != nullptr) {
			free(this->rawData);
			this->rawData = nullptr;
		}
	}


private:

	type* rawData = nullptr;

	unsigned int size = 0;

	unsigned  int dataSize = 0;

	void ensureAdditionalSize(unsigned int size, bool* error) {
		if (this->dataSize + size > this->size) {
			if (this->rawData != nullptr) {
				void* toFree = this->rawData;
				unsigned int newSize = this->size * ARRAY_INCREATE_FACTOR;
				MALLOC(this->rawData, type, newSize, {free(toFree); *error = true; return; });
				memset(this->rawData, 0, newSize * sizeof(type));
				memcpy(this->rawData, toFree, this->size * sizeof(type));
				this->size = newSize;
				free(toFree);
			} else {
				unsigned int newSize = ARRAY_DEFAULT_SIZE;
				MALLOC(this->rawData, type, newSize, {*error = true; return; });
				memset(this->rawData, 0, newSize * sizeof(type));
				this->size = newSize;
				this->dataSize = 0;
			}
			this->ensureAdditionalSize(size, error);
		}
	}

};

#define LIBRARY_API __declspec(dllexport)

void compress(char* data, unsigned int length, char** outputData, unsigned int* outputLength, bool* error);

void compress(char* data, unsigned int length, char** outputData, unsigned int* outputLength, bool* error, int level);

void decompress(char* data, unsigned int dataLength, char** outputData, unsigned int* outputLength, bool* error);

struct MapFileStr {
	char* fileName;
	unsigned char* contents;
	unsigned int contentsLength;
};

void destroyFileArray(Array<MapFileStr*>* array);

class CharMap {

public:

	bool has(char* index) {
		return get(index) != nullptr;
	}

	char* get(char* index) {
		for (unsigned int i = 0; i < indexes.getSize(); i++) {
			char* indexx = indexes.get(i);
			if (!strcmp(indexx, index)) {
				return values.get(i);
			}
		}
		return nullptr;
	}

	bool add(char* index, char* value) {
		if (!has(index)) {
			if (!indexes.append(index)) { return false; }
			GET_CLONED_STRING(newValue, value, {return false; });
			if (!values.append(newValue)) { free(newValue); return false; }
		}
		return true;
	}

	virtual ~CharMap() {
		for (unsigned int i = 0; i < values.getSize(); i++) {
			char* value = values.get(i);
			free(value);
		}
	}

	unsigned int getSize() {
		return indexes.getSize();
	}

private:
	Array<char*> indexes;
	Array<char*> values;

};