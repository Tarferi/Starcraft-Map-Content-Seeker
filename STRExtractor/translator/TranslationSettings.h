#pragma once

#include "../common/common.h"

#ifndef _DEBUG
#define EUDSettings PeniStruct
#endif

#define SET_ERROR(flag){unsigned int val = settings->result; val |= (1 << flag); settings->result = val;}

#define SET_ERROR_LOAD_FILE SET_ERROR(1)
#define SET_ERROR_LOAD_SECTION SET_ERROR(2)
#define SET_ERROR_PROCESS SET_ERROR(3)
#define SET_NO_ERROR settings->result = 0;

#pragma pack(push, 1)
_declspec(align(1)) // Disable alignment
struct TranslationSettings {

	uint8 action;

	void* inputFilePath;
	void* outputFilePath;

	void* stringData;
	int32 stringDataLength;

	int32 useCondition;
	int8 repack;

	uint8 result;

};
#pragma pack(pop)