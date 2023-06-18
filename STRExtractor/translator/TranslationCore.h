#pragma once
#include "../common/stdafx.h"
#include "../chk/CHK.h"
#include "../storm/Storm.h"
#include "TranslationSettings.h"
#include "../common/common.h"

typedef bool(*checkCondF)(Condition*, void*);
typedef bool(*checkActF)(Action*, void*);

struct MapString {
	unsigned int index;

	Array<unsigned int>* unitIndexs;
	Array<unsigned int>* switchIndexes;
	Array<unsigned int>* locationIndexes;
	Array<unsigned int>* triggerActionIndexes;
	Array<unsigned int>* triggerCommentIndexes;
	Array<unsigned int>* briefingActionIndexes;
	Array<unsigned int>* briefingCommentIndexes;
	Array<unsigned int>* forceNamesIndexes;

	bool isMapName;
	bool isMapDescription;

	char* string;
};

class MapStringIndexer {
public:

	MapStringIndexer(Section_STR_* str) {
		this->STR = str;
	}

	virtual ~MapStringIndexer() {
		for (unsigned int i = 0; i < data.getSize(); i++) {
			MapString* ms = data[i];
			delete ms->unitIndexs;
			delete ms->switchIndexes;
			delete ms->locationIndexes;
			delete ms->triggerActionIndexes;
			delete ms->triggerCommentIndexes;
			delete ms->briefingActionIndexes;
			delete ms->briefingCommentIndexes;
			delete ms->forceNamesIndexes;
			free(ms->string);
			free(ms);
		}
	}

	MapString* getForIndex(unsigned int index) {
		for (unsigned int i = 0; i < data.getSize(); i++) {
			MapString* ms = data[i];
			if (ms->index == index) {
				return ms;
			}
		}

		// Look up string
		char* str = STR->getRawString(index, false);
		if (str == nullptr) {
			return nullptr;
		}

		// Calculate offset
		unsigned int offset = STR->getOffset(str);

		// Verify that index preceeds offset
		unsigned int indexOffset = 2 + (2 * (index - 1));
		if (indexOffset >= smallestOffset || offset <= indexOffset) {
			return nullptr;
		}

		if (offset < smallestOffset) {
			smallestOffset = offset;
		}

		// Copy string
		GET_CLONED_STRING(newStr, str, { return nullptr; });

		// Create new node
		MALLOC_N(newMs, MapString, 1, { free(newStr); return nullptr; });
		
		if (!data.append(newMs)) {
			free(newStr);
			free(newMs);
			return nullptr;
		}

		newMs->index = index;
		newMs->unitIndexs = new Array<unsigned int>();
		newMs->switchIndexes = new Array<unsigned int>();
		newMs->locationIndexes = new Array<unsigned int>();
		newMs->triggerActionIndexes = new Array<unsigned int>();
		newMs->triggerCommentIndexes = new Array<unsigned int>();
		newMs->briefingActionIndexes = new Array<unsigned int>();
		newMs->briefingCommentIndexes = new Array<unsigned int>();

		newMs->forceNamesIndexes = new Array<unsigned int>();

		newMs->isMapName = false;
		newMs->isMapDescription = false;

		newMs->string = newStr;

		return newMs;
	}

	bool getSortedArray(MapString*** result, unsigned int* resultLength) {
		bool changed = true;
		unsigned int length = data.getSize();
		MALLOC_N(sorted, MapString*, length, { return false; });
		for (unsigned int i = 0; i < length; i++) {
			MapString* ms = data[i];
			sorted[i] = ms;
		}
		while (changed) {
			changed = false;
			for (unsigned int i = 1; i < length; i++) {
				MapString* p0 = data[i - 1];
				MapString* p1 = data[i];
				if (p0->index > p1->index) {
					changed = true;
					data[i - 1] = p1;
					data[i] = p0;
				}
			}
		}
		*resultLength = length;
		*result = sorted;
		return true;
	}

private:
	Array<MapString*> data;

	Section_STR_* STR;

	unsigned int smallestOffset = 0xffffffff;
};


LIBRARY_API void __cdecl realize(TranslationSettings* settings);