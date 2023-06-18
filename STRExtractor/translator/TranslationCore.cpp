// QCHK.cpp : Defines the entry point for the console application.
//

#include "../common/stdafx.h"
#include "TranslationCore.h"

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

namespace SupportType {
	enum SupportType {
		NonEUD = 0,
		EUDEditor = 1,
		NonCPEUD = 2,
		Unsupported = 3
	};
};

namespace Modifiers {

	enum Enum {
		IsSet = 2,
		IsCleared = 3,
		Set = 4,
		Add = 8,
		Exactly = 10
	};

}

bool mapSupported(CHK* chk, SupportType::SupportType* output) {
	Section_TRIG* TRIG = ( Section_TRIG*) chk->getSection("TRIG");
	if (TRIG == nullptr) {
		*output = SupportType::NonEUD;
		return true;
	}

	bool isEUD = false;

	// First, no EUD present -> overwrite STR section possible
	for (unsigned int triggerIndex = 0; triggerIndex < TRIG->triggers.getSize(); triggerIndex++) {
		Trigger* trigger = TRIG->triggers[triggerIndex];
		for (unsigned int actionIndex = 0; actionIndex < 64; actionIndex++) {
			Action* action = &(trigger->actions[actionIndex]);
			if (action->ActionType == 45) { // Set deaths
				unsigned int pid = action->Player;
				unsigned int unit = action->UnitType;
				if (pid > 8 || unit > 228) { // EUD
					isEUD = true;
					triggerIndex = TRIG->triggers.getSize(); // End outer loop
					break;
				}
			}
		}
	}


	if (!isEUD) {
		*output = SupportType::NonEUD;
		return true;
	}

	// EUD, check if map is a product of EUD Editor -> first trigger EUD setter

	bool isProductOfEUDEDitor = false;

	Trigger* trigger = TRIG->triggers[0];
	Condition* condition = &(trigger->conditions[0]);
	if (condition->ConditionType == 0) { // No conditions
		Action* action = &(trigger->actions[0]);
		if (action->ActionType == 45) { // Set deaths
			unsigned int pid = action->Player;
			unsigned int unit = action->UnitType;
			if (pid == 4294967284 && unit == 1) {
				action = &(trigger->actions[1]);
				if (action->ActionType == 0) {
					isProductOfEUDEDitor = true;
				}
			}
		}
	}

	if (isProductOfEUDEDitor) {
		*output = SupportType::EUDEditor;
		return true;
	}

	bool containsCPTrickActions = false;
	bool containsSTRPointerAction = false;

	// EUD But not from EUD editor, check if STR is being used
	for (unsigned int triggerIndex = 0; triggerIndex < TRIG->triggers.getSize(); triggerIndex++) {
		Trigger* trigger = TRIG->triggers[triggerIndex];
		for (unsigned int actionIndex = 0; actionIndex < 64; actionIndex++) {
			Action* action = &(trigger->actions[actionIndex]);
			if (action->ActionType == 45) { // Set deaths
				unsigned int pid = action->Player;
				unsigned int unit = action->UnitType;
				unsigned int epd = ((unit * 12) + pid);
				if (epd == 203155) { // CP Trick
					containsCPTrickActions = true;
				} else if (epd == 15388) { // STR Pointer
					containsSTRPointerAction = true;
				}
			}
		}
	}

	if (containsCPTrickActions || containsSTRPointerAction) { // Could be messing with trigger lists
		*output = SupportType::Unsupported;
		return false;
	}

	// Not messing with trigger lists, relatively safe
	*output = SupportType::NonCPEUD;

	return true;
}

bool mapSupported(CHK* chk) {
	SupportType::SupportType output;
	return mapSupported(chk, &output);
}

bool applyStrings(MapFile* mf, CHK* chk, Array<char*>* output, unsigned int useCondition, bool repack) {
	Section_STR_* STR = ( Section_STR_*) chk->getSection("STR ");
	Section_TRIG* TRIG = ( Section_TRIG*) chk->getSection("TRIG");
	Section_MBRF* MBRF = ( Section_MBRF*) chk->getSection("MBRF");
	SupportType::SupportType support;

	if (STR == nullptr || TRIG == nullptr || MBRF == nullptr || !mapSupported(chk, &support)) {
		return false;
	}

	// Append new strings
	for (unsigned int i = 0; i < output->getSize(); i++) {
		char* str = output->get(i);
		if (!STR->appendVirtualString(str)) {
			return false;
		}
	}

	CharMap wavRemapping;
	char remappedWavName[64];

	// Copy WAV strings from triggers
	for (unsigned int triggerIndex = 0; triggerIndex < TRIG->triggers.getSize(); triggerIndex++) {
		Trigger* trig = TRIG->triggers.get(triggerIndex);
		for (unsigned int actionIndex = 0; actionIndex < 64; actionIndex++) {
			Action* action = &(trig->actions[actionIndex]);
			if (action->ActionType == 0) {
				break;
			} else if (action->ActionType == 7 || action->ActionType == 8) {
				unsigned int wavIndex = action->WAVStringNumber;
				if (wavIndex > 0) { // Remap wav
					char* wavName = STR->getRawString(wavIndex, false);
					if (repack) { // Repacking, new sound name
						if (!wavRemapping.has(wavName)) { // Not remapped yet, create new 
							sprintf_s(remappedWavName, "snd_%d.wav", wavRemapping.getSize());
							if (!wavRemapping.add(wavName, remappedWavName)) {
								return false;
							}
							if (!mf->renameFile(wavName, remappedWavName)) { return false; }
						} else { // Remapped, get value
							sprintf_s(remappedWavName, "%s", wavRemapping.get(wavName));
						}
						wavName = remappedWavName;
					}
					unsigned int newIndex = 0;
					if (!STR->appendVirtualString(wavName, &newIndex)) {
						return false;
					}
					action->WAVStringNumber = newIndex;
				}
			}
		}
	}

	// Copy WAV strings from briefings
	for (unsigned int triggerIndex = 0; triggerIndex < MBRF->triggers.getSize(); triggerIndex++) {
		Trigger* trig = MBRF->triggers.get(triggerIndex);
		for (unsigned int actionIndex = 0; actionIndex < 64; actionIndex++) {
			Action* action = &(trig->actions[actionIndex]);
			if (action->ActionType == 0) {
				break;
			} else if (action->ActionType == 2) {
				unsigned int wavIndex = action->WAVStringNumber;
				if (wavIndex > 0) { // Remap wav
					char* wavName = STR->getRawString(wavIndex, false);
					if (repack) { // Repacking, new sound name
						if (!wavRemapping.has(wavName)) { // Not remapped yet, create new 
							sprintf_s(remappedWavName, "snd_%d.wav", wavRemapping.getSize());
							if (!wavRemapping.add(wavName, remappedWavName)) {
								return false;
							}
							if (!mf->renameFile(wavName, remappedWavName)) { return false; }
						} else { // Remapped, get value
							sprintf_s(remappedWavName, "%s", wavRemapping.get(wavName));
						}
						wavName = remappedWavName;
					}
					unsigned int newIndex = 0;
					if (!STR->appendVirtualString(wavName, &newIndex)) {
						return false;
					}
					action->WAVStringNumber = newIndex;
				}
			}
		}
	}

	if (support == SupportType::NonEUD || support == SupportType::NonCPEUD) { // Replace entire STR section
		STR->erasePreviousData();
	} else if (support == SupportType::EUDEditor) { // Add offset to STR and update triggers

#define EUD_STR 0x05993D4
#define ADDRESS_TO_EPD(address) ((address - 0x58A364) / 4)
#define COND_DEATHS(data, player, modifier, number, unitID) data->ConditionType = 15; data->groupNumber = player; data->Comparision = modifier; data->Quantifier = number; data->UnitID = unitID
#define ACT_SET_DEATHS(data, player, modifier, number, unitID) data->ActionType = 45; data->Player = player; data->UnitsNumber = modifier; data->Group = number; data->UnitType = unitID
#define ACT_SET_MINERALS(data, player, modifier, number) data->ActionType = 26; data->Player = player; data->UnitsNumber = modifier; data->Group = number; data->UnitType = 1
#define COND_SWITCH(data, switchIndex, modifier) data->ConditionType = 11; data->Resource = switchIndex; data->Comparision = modifier
#define ACT_SET_SWITCH(data, swichIndex, modifier) data->ActionType = 13; data->Group = switchIndex; data->UnitsNumber = modifier


		bool error = false;
		unsigned int shiftOffset = STR->getOriginalSectionBeginOffset(&error);

		if (error) { return false; }

		Trigger* trigger0 = TRIG->triggers.get(0);
		Trigger* trigger1 = TRIG->triggers.get(1);
		Trigger* trigger3 = TRIG->triggers.get(3);
		ACT_SET_DEATHS((&(trigger0->actions[1])), ADDRESS_TO_EPD(EUD_STR), Modifiers::Add, shiftOffset, 0); // Add offset to STR, void condition in all lists

		// Figure out what to use
		if (useCondition < 256) { // Switch
			unsigned int switchIndex = useCondition;
			COND_SWITCH((&(trigger0->conditions[0])), switchIndex, Modifiers::IsCleared);
			COND_SWITCH((&(trigger1->conditions[1])), switchIndex, Modifiers::IsCleared);
			COND_SWITCH((&(trigger3->conditions[0])), switchIndex, Modifiers::IsCleared);
			ACT_SET_SWITCH((&(trigger3->actions[1])), switchIndex, Modifiers::Set);
		} else { // Deaths
			unsigned int playerID = (useCondition & 0xffff) >> 8;
			unsigned int unitID = (useCondition >> 16) & 0xff;
			COND_DEATHS((&(trigger0->conditions[0])), playerID, Modifiers::Exactly, 0, unitID);
			COND_DEATHS((&(trigger1->conditions[1])), playerID, Modifiers::Exactly, 0, unitID);
			COND_DEATHS((&(trigger3->conditions[0])), playerID, Modifiers::Exactly, 0, unitID);
			ACT_SET_DEATHS((&(trigger3->actions[1])), playerID, Modifiers::Add, 1, unitID);

		}
	}
	return true;
}

void clearArray(Array<char*>* data) {
	for (unsigned int i = 0, o = data->getSize(); i < o; i++) {
		char* str = data->get(i);
		if (str != nullptr) {
			free(str);
		}
	}

	for (unsigned int i = 0, o = data->getSize(); i < o; i++) {
		data->remove(0);
	}
}

bool unpackStrings(CHK* chk, char* strings, unsigned int stringsLength, Array<char*>* output) {
	Section_STR_* STR = ( Section_STR_*) chk->getSection("STR ");

	if (STR == nullptr) {
		return false;
	}

	bool error = false;

	// unpack data
	ReadBuffer* rb = new ReadBuffer(( unsigned char*) strings, stringsLength, &error);
	if (error) { delete rb; return false; }

	int totalStrings = rb->readInt(&error);
	if (error) { delete rb; return false; }

	unsigned int nextExpectedIndex = 0;

	for (int i = 0; i < totalStrings; i++) {

		// String index
		unsigned int index = rb->readInt(&error) - 1;
		if (error) { clearArray(output); delete rb; return false; }

		if (nextExpectedIndex == index) { // Sequence unbroken
			nextExpectedIndex++;
			LOG_INFO("UNPACKER", "Have valid sequence string %d (of %d total) index %d", i, totalStrings, index);
		} else if (index < nextExpectedIndex) { // Sequence was not ordered
			LOG_INFO("UNPACKER", "Have invalid sequence string %d (of %d total) index %d (expected %d), failing", i, totalStrings, index, nextExpectedIndex);
			clearArray(output);
			delete rb;
			return false;
		} else { // Fill missing strings
			LOG_INFO("UNPACKER", "Have skip sequence string %d (of %d total) index %d (expected %d)", i, totalStrings, index, nextExpectedIndex);

			for (unsigned int o = nextExpectedIndex; o < index; o++) { // Use native strings
				char* nativeString = STR->getRawString(o + 1, false);
				nativeString = "";

				if (nativeString == nullptr) { clearArray(output); delete rb; return false; }
				GET_CLONED_STRING(newNativeString, nativeString, {clearArray(output); delete rb; return false;});
				if (!output->append(newNativeString)) { free(newNativeString); clearArray(output); delete rb; return false; }
			}
			nextExpectedIndex = index + 1;
		}

		// String length
		int stringLength = rb->readInt(&error);
		if (error) { clearArray(output); delete rb; return false; }

		char* rawStr = ( char*) rb->readFixedLengthString(( unsigned int) stringLength, &error);
		if (error) { clearArray(output); delete rb; return false; }

		MALLOC_N(newStr, char, stringLength + 1, {delete rb; return false;});
		memcpy(newStr, rawStr, stringLength);
		free(rawStr);
		newStr[stringLength] = 0;

		if (!output->append(newStr)) {
			free(newStr);
			clearArray(output);
			delete rb;
			return false;
		}
	}

	delete rb;

	return true;
}

void processMap(TranslationSettings* settings) {
	bool error = false;
	Storm* storm = new Storm(&error);

	MapFile* v3F = storm->readSCX(( char*) settings->inputFilePath, &error);
	if (error || v3F == nullptr) {
		SET_ERROR_LOAD_FILE
			settings->outputFilePath = nullptr;
		if (v3F != nullptr) {
			delete v3F;
		}
		delete storm;
		return;
	}

	CHK* v3 = v3F->getCHK(CHKReadMode::Partial);

#ifdef TRIG_PRINT
	Section_TRIG* T = ( Section_TRIG*) v3->getSection("TRIG");
	Section_STR_* S = ( Section_STR_*) v3->getSection("STR ");

	WriteBuffer wb;
	if (T->print(S, &wb)) {
		wb.writeByte(0, &error);
		if (!error) {
			unsigned int length;
			char* data;
			wb.getWrittenData(( unsigned char**) & data, &length);
			FILE* f;
			if (!fopen_s(&f, "C:\\Users\\Tom\\Desktop\\Documents\\Visual Studio 2015\\Projects\\TranslateLib\\triggers.txt", "wb")) {
				fprintf(f, "%s", data);
				fclose(f);
			}
		}
	}
#endif

	if (v3 == nullptr) {
		SET_ERROR_LOAD_FILE
			delete v3F;
		delete storm;
		return;
	}

	Array<char*> strings;
	if (!unpackStrings(v3, ( char*) settings->stringData, settings->stringDataLength, &strings)) {
		SET_ERROR_LOAD_FILE
			delete v3F;
		delete storm;
		return;
	}

	bool ms = mapSupported(v3);

	if (!applyStrings(v3F, v3, &strings, settings->useCondition, settings->repack) || !ms) {
		SET_ERROR_LOAD_FILE
			delete v3F;
		delete storm;
		clearArray(&strings);
		return;
	}

	clearArray(&strings);

	v3F->writeToFile(storm, ( char*) settings->outputFilePath, &error, settings->repack == 1, false);

	if (error) {
		SET_ERROR_PROCESS
	}



	delete v3F;
	delete storm;
}

bool serialize(Section_UNIS* section, MapStringIndexer* mi) {
	for (unsigned int i = 0; i < 228; i++) {
		unsigned int index = section->data->name[i];
		if (index > 0) {
			MapString* ms = mi->getForIndex(index);
			if (ms == nullptr) { return true; }
			if (!ms->unitIndexs->append(i)) { return false; }
		}
	}
	return true;
}

bool serialize(Section_UNIx* section, MapStringIndexer* mi) {
	for (unsigned int i = 0; i < 228; i++) {
		unsigned int index = section->data->str_unit_name[i];
		if (index > 0) {
			MapString* ms = mi->getForIndex(index);
			if (ms == nullptr) { return true; }
			if (!ms->unitIndexs->append(i)) { return false; }
		}
	}
	return true;
}

bool serialize(Section_SWNM* section, MapStringIndexer* mi) {
	for (unsigned int i = 0; i < 256; i++) {
		unsigned int index = section->data->switchNames[i];
		if (index > 0) {
			MapString* ms = mi->getForIndex(index);
			if (ms == nullptr) { return true; } // Invalid switch name?
			if (!ms->switchIndexes->append(i)) { return false; }
		}
	}
	return true;
}

bool serialize(Section_MRGN* section, MapStringIndexer* mi) {
	for (unsigned int i = 0; i < section->locations.getSize(); i++) {
		unsigned int index = section->locations.get(i)->str_description;
		if (index > 0) {
			MapString* ms = mi->getForIndex(index);
			if (ms == nullptr) { return true; } // Invalid location name?
			if (!ms->locationIndexes->append(i)) { return false; }
		}
	}
	return true;
}

bool serialize(Section_FORC* section, MapStringIndexer* mi) {
	for (unsigned int i = 0; i < 4; i++) {
		unsigned int index = section->data->str_names[i];
		if (index > 0) {
			MapString* ms = mi->getForIndex(index);
			if (ms == nullptr) { return false; }
			if (!ms->forceNamesIndexes->append(i)) { return false; }
		}
	}
	return true;
}

bool serialize(Section_SPRP* section, MapStringIndexer* mi) {
	unsigned int mapName = section->str_scenarioName;

	MapString* msName = mi->getForIndex(mapName);
	if (msName == nullptr) { return false; }
	msName->isMapName = true;

	unsigned int mapDesciption = section->str_scenarioDescription;
	MapString* msDescr = mi->getForIndex(mapDesciption);
	if (msDescr == nullptr) { return false; }
	msDescr->isMapDescription = true;


	return true;
}

typedef bool(*hasTextF)(Action*);
inline bool hasTrigText(Action* action) { return action->ActionType == 7 || action->ActionType == 9 || action->ActionType == 12 || action->ActionType == 17 || action->ActionType == 18 || action->ActionType == 19 || action->ActionType == 20 || action->ActionType == 21 || action->ActionType == 33 || action->ActionType == 34 || action->ActionType == 35 || action->ActionType == 36 || action->ActionType == 37 || action->ActionType == 41 || action->ActionType == 47; }
inline bool hasBriefText(Action* action) { return action->ActionType == 3 || action->ActionType == 4 || action->ActionType == 8; }

bool serialize(Section_TRIG* section, MapStringIndexer* mi, bool TRIG) {
	hasTextF hasText = TRIG ? hasTrigText : hasBriefText;

	for (unsigned int triggerIndex = 0; triggerIndex < section->triggers.getSize(); triggerIndex++) {
		Trigger* trigger = section->triggers[triggerIndex];
		for (unsigned int actionIndex = 0; actionIndex < 64; actionIndex++) {
			Action* action = &(trigger->actions[actionIndex]);
			if (action->ActionType == 0) {
				break;
			} else if (hasText(action)) {
				MapString* ms = mi->getForIndex(action->TriggerText);
				if (ms == nullptr) { continue; }
				if (TRIG) {
					if (action->ActionType == 47) {
						if (!ms->triggerCommentIndexes->append(triggerIndex)) { return false; }
					} else {
						if (!ms->triggerActionIndexes->append(triggerIndex)) { return false; }
					}
				} else {
					if (!ms->briefingActionIndexes->append(triggerIndex)) { return false; }
				}
				break;
			}
		}

	}
	return true;
}

bool writeMapString(Array<unsigned int>* items, WriteBuffer* wb) {
	bool error = false;
	wb->writeInt(items->getSize(), &error);
	if (error) { return false; }
	for (unsigned int i = 0; i < items->getSize(); i++) {
		unsigned int item = items->get(i);
		wb->writeInt(item, &error);
		if (error) { return false; }
	}
	return true;
}

bool writeMapString(MapString* ms, WriteBuffer* wb) {
	bool error = false;

	wb->writeInt(ms->index, &error);
	if (error) { return false; }

	// First write string length without delim
	wb->writeInt(strlen(ms->string), &error);
	if (error) { return false; }

	// Next, write string itself
	wb->writeFixedLengthString(( unsigned char*) ms->string, &error);
	if (error) { return false; }

	// Next map description bool
	wb->writeByte(ms->isMapDescription ? 1 : 0, &error);
	if (error) { return false; }

	// Map name bool
	wb->writeByte(ms->isMapName ? 1 : 0, &error);
	if (error) { return false; }

	// Arrays
	if (!writeMapString(ms->unitIndexs, wb)) { return false; }
	if (!writeMapString(ms->switchIndexes, wb)) { return false; }
	if (!writeMapString(ms->locationIndexes, wb)) { return false; }
	if (!writeMapString(ms->triggerActionIndexes, wb)) { return false; }
	if (!writeMapString(ms->triggerCommentIndexes, wb)) { return false; }
	if (!writeMapString(ms->briefingActionIndexes, wb)) { return false; }
	if (!writeMapString(ms->briefingCommentIndexes, wb)) { return false; }
	if (!writeMapString(ms->forceNamesIndexes, wb)) { return false; }
	return true;
}

void getAllStrings(TranslationSettings* settings) {
	bool error = false;
	Storm* storm = new Storm(&error);

	MapFile* v3F = storm->readSCX(( char*) settings->inputFilePath, &error);
	if (error || v3F == nullptr) {
		SET_ERROR_LOAD_FILE;
		settings->outputFilePath = nullptr;
		if (v3F != nullptr) {
			delete v3F;
		}
		delete storm;
		return;
	}

	CHK* v3 = v3F->getCHK(CHKReadMode::Complete);

	if (v3 == nullptr) {
		SET_ERROR_LOAD_FILE;
		delete v3F;
		delete storm;
		return;
	}

	Section_STR_* STR = ( Section_STR_*) v3->getSection("STR ");
	if (STR == nullptr || !mapSupported(v3)) {
		SET_ERROR_LOAD_FILE;
		delete v3F;
		delete storm;
		return;
	}

	MapStringIndexer indexer(STR);

#ifdef INCLUDE_UNUSED_STRINGS
	Array<char*> allStrings;
	if (!STR->getAllStrings(&allStrings)) {
		SET_ERROR_LOAD_FILE;
		delete v3F;
		delete storm;
		return;
	}
	for (unsigned int i = 0; i < allStrings.getSize(); i++) {
		char* string = allStrings.get(i);
		MapString* mi = indexer.getForIndex(i + 1);
		if (mi != nullptr) {
			if (!mi->triggerActionIndexes->append(i + 1)) {
				SET_ERROR_LOAD_FILE;
				delete v3F;
				delete storm;
				return;
			}
		}
	}
	bool ok = true;

#else

	// First get names of all units
	Section_UNIS* UNIS = ( Section_UNIS*) v3->getSection("UNIS");
	Section_UNIx* UNIx = ( Section_UNIx*) v3->getSection("UNIx");
	if (UNIx != nullptr) {
		if (!serialize(UNIx, &indexer)) {
			SET_ERROR_LOAD_FILE;
			delete v3F;
			delete storm;
			return;
		}
	} else if (UNIS != nullptr) {
		if (!serialize(UNIS, &indexer)) {
			SET_ERROR_LOAD_FILE;
			delete v3F;
			delete storm;
			return;
		}
	} else {
		SET_ERROR_LOAD_FILE;
		delete v3F;
		delete storm;
		return;
	}

	// Next, switch names
	Section_SWNM* SWNM = ( Section_SWNM*) v3->getSection("SWNM");
	if (SWNM != nullptr) {
		serialize(SWNM, &indexer);
	}

	// Next, location names
	Section_MRGN* MRGN = ( Section_MRGN*) v3->getSection("MRGN");
	if (MRGN != nullptr) {
		serialize(MRGN, &indexer);
	}

	// Next, force names
	Section_FORC* FORC = ( Section_FORC*) v3->getSection("FORC");
	if (FORC != nullptr) {
		serialize(FORC, &indexer);
	}

	// Next, map name and description
	Section_SPRP* SPRP = ( Section_SPRP*) v3->getSection("SPRP");
	if (SPRP != nullptr) {
		serialize(SPRP, &indexer);
	}

	// Trigger and mission briefings
	Section_TRIG* TRIG = ( Section_TRIG*) v3->getSection("TRIG");
	Section_MBRF* MBRF = ( Section_MBRF*) v3->getSection("MBRF");
	bool ok = true;
	if (TRIG != nullptr) {
		ok &= serialize(TRIG, &indexer, true);
	}
	if (MBRF != nullptr) {
		ok &= serialize(MBRF, &indexer, false);
	}


#endif

	// Get strings sorted by index
	unsigned int totalUsedStrings;
	MapString** usedStrings;
	if (!indexer.getSortedArray(&usedStrings, &totalUsedStrings) || !ok) {
		SET_ERROR_LOAD_FILE;
		delete v3F;
		delete storm;
		return;
	}

	// Serialize
	WriteBuffer wb;
	wb.writeInt(totalUsedStrings, &error);
	if (error) {
		SET_ERROR_LOAD_FILE;
		free(usedStrings);
		delete v3F;
		delete storm;
		return;
	}

	for (unsigned int i = 0; i < totalUsedStrings; i++) {
		MapString* ms = usedStrings[i];
		if (!writeMapString(ms, &wb)) {
			SET_ERROR_LOAD_FILE;
			free(usedStrings);
			delete v3F;
			delete storm;
			return;
		}
	}
	free(usedStrings);
	delete v3F;
	delete storm;

	// Output
	unsigned char* outData;
	unsigned int outDataLength;
	wb.getWrittenData(&outData, &outDataLength);
	GET_CLONED_DATA(resultData, char, outData, outDataLength, {SET_ERROR_LOAD_FILE; return;});
	settings->stringData = resultData;
	settings->stringDataLength = outDataLength;

	LOG_LEAK(settings->stringData);

	return;
}

void freeData(TranslationSettings* settings) {
	if (settings->stringData != nullptr) {
		free(settings->stringData);
		settings->stringData = nullptr;
	}
}

void getSupportType(TranslationSettings* settings) {
	bool error = false;
	Storm* storm = new Storm(&error);

	MapFile* v3F = storm->readSCX(( char*) settings->inputFilePath, &error);
	if (error || v3F == nullptr) {
		SET_ERROR_LOAD_FILE
			settings->outputFilePath = nullptr;
		if (v3F != nullptr) {
			delete v3F;
		}
		delete storm;
		return;
	}

	CHK* v3 = v3F->getCHK(CHKReadMode::Partial);

	if (v3 == nullptr) {
		SET_ERROR_LOAD_FILE
			delete v3F;
		delete storm;
		return;
	}

	SupportType::SupportType supportE;
	mapSupported(v3, &supportE);
	unsigned int support = supportE;
	settings->useCondition = support;

	if (error) {
		SET_ERROR_PROCESS
	}

	delete v3F;
	delete storm;
}

bool checkTriggersSwitchAct(Action* action, void* param) {
	unsigned int switchIndex = ( unsigned int) param;
	return action->ActionType == 13 && action->Group == switchIndex;
}

bool checkTriggersSwitchCond(Condition* condition, void* param) {
	unsigned int switchIndex = ( unsigned int) param;
	return condition->ConditionType == 11 && condition->Resource == switchIndex;
}

bool checkTriggersDeathAct(Action* action, void* param) {
	unsigned int playerID = ((( unsigned int) param) & 0xffff) >> 8;
	unsigned int unitID = ((( unsigned int) param) >> 16) & 0xff;
	return action->ActionType == 45 && action->Player == playerID && action->UnitType == unitID;
}

bool checkTriggersDeathCond(Condition* condition, void* param) {
	unsigned int playerID = ((( unsigned int) param) & 0xffff) >> 8;
	unsigned int unitID = ((( unsigned int) param) >> 16) & 0xff;
	return condition->ConditionType == 15 && condition->groupNumber == playerID && condition->UnitID == unitID;
}

bool checkTriggers(CHK* chk, checkCondF cond, checkActF act, void* data, bool* error) {
	Section_TRIG* TRIG = ( Section_TRIG*) chk->getSection("TRIG");
	if (TRIG == nullptr) {
		*error = true;
		return false;
	}

	for (unsigned int triggerIndex = 0; triggerIndex < TRIG->triggers.getSize(); triggerIndex++) {
		Trigger* trigger = TRIG->triggers[triggerIndex];
		bool hasValidPlayers = false;
		for (unsigned int i = 0; i < 8; i++) {
			hasValidPlayers |= trigger->players[i] == 1;
		}
		for (unsigned int i = 0; i < 4; i++) {
			hasValidPlayers |= trigger->players[18 + i] == 1; // Forces
		}
		hasValidPlayers |= trigger->players[17] == 1; // All players

		if (hasValidPlayers) {
			bool condsEnabled = true;
			for (unsigned int conditionIndex = 0; conditionIndex < 16; conditionIndex++) {
				Condition* condition = &(trigger->conditions[conditionIndex]);
				if ((condition->Flags & 2) == 0) {
					if (condition->ConditionType == 0) {
						break;
					} else if (condition->ConditionType == 23) { // Never
						condsEnabled = false;
					}
				}
			}
			if (condsEnabled) {
				for (unsigned int conditionIndex = 0; conditionIndex < 16; conditionIndex++) {
					Condition* condition = &(trigger->conditions[conditionIndex]);
					if ((condition->Flags & 2) == 0) {
						if (condition->ConditionType == 0) {
							break;
						} else {
							if (cond(condition, data)) {
								return true;
							}
						}
					}
				}
				for (unsigned int actionIndex = 0; actionIndex < 64; actionIndex++) {
					Action* action = &(trigger->actions[actionIndex]);
					if ((action->Flags & 2) == 0) {
						if (action->ActionType == 0) {
							break;
						} else {
							if (act(action, data)) {
								return true;
							}
						}
					}
				}
			}
		}
	}
	return false;
}

void checkUsedCondition(TranslationSettings* settings) {
	bool error = false;
	Storm* storm = new Storm(&error);

	MapFile* v3F = storm->readSCX(( char*) settings->inputFilePath, &error);
	if (error || v3F == nullptr) {
		SET_ERROR_LOAD_FILE
			settings->outputFilePath = nullptr;
		if (v3F != nullptr) {
			delete v3F;
		}
		delete storm;
		return;
	}

	CHK* v3 = v3F->getCHK(CHKReadMode::Partial);

	if (v3 == nullptr) {
		SET_ERROR_LOAD_FILE
			delete v3F;
		delete storm;
		return;
	}

	Section_TRIG* TRIG = ( Section_TRIG*) v3->getSection("TRIG");
	if (TRIG == nullptr) {
		SET_ERROR_LOAD_FILE
			delete v3F;
		delete storm;
		return;
	}

	bool used = false;
	if (settings->useCondition < 256) { // Switch
		used = checkTriggers(v3, checkTriggersSwitchCond, checkTriggersSwitchAct, ( void*) settings->useCondition, &error);
	} else { // Deaths
		used = checkTriggers(v3, checkTriggersDeathCond, checkTriggersDeathAct, ( void*) settings->useCondition, &error);
	}

	if (error) {
		SET_ERROR_PROCESS
	}
	if (used) {
		settings->useCondition = 1;
	} else {
		settings->useCondition = 0;
	}

	delete v3F;
	delete storm;
}

LIBRARY_API void __cdecl realize(TranslationSettings* settings) {
	fprintf(stderr, "QCHK Entry\n");
	LOG("QCHK", "Received request to perform action %d", settings->action);
	if (settings->action == 1) { // The main action
		processMap(settings);
	} else if (settings->action == 2) { // Query strings
		getAllStrings(settings);
	} else if (settings->action == 3) { // Free result
		freeData(settings);
	} else if (settings->action == 4) { // Get support type
		getSupportType(settings);
	} else if (settings->action == 5) { // Check for condition availability
		checkUsedCondition(settings);
	}

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtDumpMemoryLeaks();
#endif
}
