#include "Section.h"

Section::Section(unsigned char * name, unsigned int size, ReadBuffer * buffer) {
	this->name = (char*)name;
	this->size = size;
	this->processed = false;
	this->buffer = buffer;
	this->bufferBeginPosition = buffer->getPosition();
	this->buffer->setPosition(this->bufferBeginPosition + this->size);
}

Section::~Section() {
	if (this->name != nullptr) {
		free(this->name);
		this->name = nullptr;
	}
}

bool Section::process() {
	if (this->processed) {
		return true;
	} else {
		unsigned int position = this->buffer->getPosition();
		this->buffer->setPosition(this->bufferBeginPosition);
		bool b = this->parse();
		this->buffer->setPosition(position);
		if (b) {
			this->processed = true;
		}
		return b;
	}
}

Section_TRIG::~Section_TRIG() {
	for (unsigned int i = 0; i < this->triggers.getSize(); i++) {
		if (this->triggers[i] != nullptr) {
			free(this->triggers[i]);
			this->triggers[i] = (Trigger*) nullptr;
		}
	};
}

#ifdef TRIG_PRINT

void Section_TRIG::printAction(Action * action, Section_STR_* STR, WriteBuffer* wb, bool* error) {
	TriggerAction* ta = TriggerAction::get(action);
	ta->print(STR, wb, error);
	delete ta;
}

void Section_TRIG::printCondition(Condition * condition, Section_STR_* STR, WriteBuffer* wb, bool* error) {
	TriggerCondition* tc = TriggerCondition::get(condition);
	tc->print(STR, wb, error);
	delete tc;
}

void Section_TRIG::printPlayers(Trigger * trigger, Section_STR_* STR, WriteBuffer* wb, bool* error) {
	int zeroes = 0;
	for (unsigned int playerIndex = 0; playerIndex < 28; playerIndex++) {
		if (trigger->players[playerIndex] == 1) {
			zeroes++;
		}
	}
	int done = 0;
	//const char* players[] = { "Player1","Player2","Player3","Player4","Player5","Player6","Player7","Player8","Player9","Player10","Player11","Player12","CurrentPlayer","Foes","Allies","NeutralPlayers","AllPlayers","Force1","Force2","Force3","Force4","NonAlliedVictoryPlayers" };
	for (unsigned int playerIndex = 0; playerIndex < 28; playerIndex++) {
		if (trigger->players[playerIndex] == 1) {
			done++;
			bool isLast = done == zeroes;
			FIELDTYPE_PLAYER(playerIndex).print(STR, wb, error);
			if (*error) {
				return;
			}
			if (!isLast) {
				PRINT_R("TRIGGERS", ", ");
			}
		}
	}
}

TriggerCondition* TriggerCondition::get(Condition * rawCondition) {
	unsigned short condition = rawCondition->ConditionType;
	if (condition == 0) {
		return new CONDITION_NOCONDITION(rawCondition);
	} else if (condition == 1) {
		return new CONDITION_COUNTDOWNTIMER(rawCondition);
	} else if (condition == 2) {
		return new CONDITION_COMMAND(rawCondition);
	} else if (condition == 3) {
		return new CONDITION_BRING(rawCondition);
	} else if (condition == 4) {
		return new CONDITION_ACCUMULATE(rawCondition);
	} else if (condition == 5) {
		return new CONDITION_KILL(rawCondition);
	} else if (condition == 6) {
		return new CONDITION_COMMADNTHEMOST(rawCondition);
	} else if (condition == 7) {
		return new CONDITION_COMMANDSTHEMOSTAT(rawCondition);
	} else if (condition == 8) {
		return new CONDITION_MOSTKILLS(rawCondition);
	} else if (condition == 9) {
		return new CONDITION_HIGHESTSCORE(rawCondition);
	} else if (condition == 10) {
		return new CONDITION_MOSTRESOURCES(rawCondition);
	} else if (condition == 11) {
		return new CONDITION_SWITCH(rawCondition);
	} else if (condition == 12) {
		return new CONDITION_ELAPSEDTIME(rawCondition);
	} else if (condition == 13) {
		return new CONDITION_NACD(rawCondition);
	} else if (condition == 14) {
		return new CONDITION_OPPONENTS(rawCondition);
	} else if (condition == 15) {
		return new CONDITION_DEATHS(rawCondition);
	} else if (condition == 16) {
		return new CONDITION_COMMANDTHELEAST(rawCondition);
	} else if (condition == 17) {
		return new CONDITION_COMMANDTHELEASTAT(rawCondition);
	} else if (condition == 18) {
		return new CONDITION_LEASTKILLS(rawCondition);
	} else if (condition == 19) {
		return new CONDITION_LOWESTSCORE(rawCondition);
	} else if (condition == 20) {
		return new CONDITION_LEASTRESOURCES(rawCondition);
	} else if (condition == 21) {
		return new CONDITION_SCORE(rawCondition);
	} else if (condition == 22) {
		return new CONDITION_ALWAYS(rawCondition);
	} else if (condition == 23) {
		return new CONDITION_NEVER(rawCondition);
	} else {
		return new CONDITION_INVALIDCONDITION(rawCondition);
	}

}

TriggerAction * TriggerAction::get(Action * rawAction) {
	unsigned short action = rawAction->ActionType;
	if (action == 0) {
		return new ACTION_NOACTION(rawAction);
	} else if (action == 1) {
		return new ACTION_VICTORY(rawAction);
	} else if (action == 2) {
		return new ACTION_DEFEAT(rawAction);
	} else if (action == 3) {
		return new ACTION_PRESERVETRIGGER(rawAction);
	} else if (action == 4) {
		return new ACTION_WAIT(rawAction);
	} else if (action == 5) {
		return new ACTION_PAUSEGAME(rawAction);
	} else if (action == 6) {
		return new ACTION_UNPAUSEGAME(rawAction);
	} else if (action == 7) {
		return new ACTION_TRANSMISSION(rawAction);
	} else if (action == 8) {
		return new ACTION_PLAYWAV(rawAction);
	} else if (action == 9) {
		return new ACTION_DISPLAYTEXTMESSAGE(rawAction);
	} else if (action == 10) {
		return new ACTION_CENTERVIEW(rawAction);
	} else if (action == 11) {
		return new ACTION_CREATEUNITWITHPROPERTIES(rawAction);
	} else if (action == 12) {
		return new ACTION_SETMISSIONOBJECTIVES(rawAction);
	} else if (action == 13) {
		return new ACTION_SETSWITCH(rawAction);
	} else if (action == 14) {
		return new ACTION_SETCOUNTDOWNTIMER(rawAction);
	} else if (action == 15) {
		return new ACTION_RUNAISCRIPT(rawAction);
	} else if (action == 16) {
		return new ACTION_RUNAISCRIPTATLOCATION(rawAction);
	} else if (action == 17) {
		return new ACTION_LEADERBOARDCONTROL(rawAction);
	} else if (action == 18) {
		return new ACTION_LEADERBOARDCONTROLATLOCATION(rawAction);
	} else if (action == 19) {
		return new ACTION_LEADERBOARDRECOURCES(rawAction);
	} else if (action == 20) {
		return new ACTION_LEADERBOARDKILLS(rawAction);
	} else if (action == 21) {
		return new ACTION_LEADERBOARDPOINTS(rawAction);
	} else if (action == 22) {
		return new ACTION_KILLUNIT(rawAction);
	} else if (action == 23) {
		return new ACTION_KILLUNITATLOCATION(rawAction);
	} else if (action == 24) {
		return new ACTION_REMOVEUNIT(rawAction);
	} else if (action == 25) {
		return new ACTION_REMOVEUNITATLOCATION(rawAction);
	} else if (action == 26) {
		return new ACTION_SETRESOURCES(rawAction);
	} else if (action == 27) {
		return new ACTION_SETSCORE(rawAction);
	} else if (action == 28) {
		return new ACTION_MINIMAPPING(rawAction);
	} else if (action == 29) {
		return new ACTION_TALKINGPORTRAIT(rawAction);
	} else if (action == 30) {
		return new ACTION_MUTEUNITSPEECH(rawAction);
	} else if (action == 31) {
		return new ACTION_UNMUTEUNITSPEECH(rawAction);
	} else if (action == 32) {
		return new ACTION_LEADERBOARDCOMPUTERPLAYERS(rawAction);
	} else if (action == 33) {
		return new ACTION_LEADERBOARDGOALCONTROL(rawAction);
	} else if (action == 34) {
		return new ACTION_LEADERBOARDGOALCONTROLATLOCATION(rawAction);
	} else if (action == 35) {
		return new ACTION_LEADERBOARDGOALRESOURCES(rawAction);
	} else if (action == 36) {
		return new ACTION_LEADERBOARDGOALKILLS(rawAction);
	} else if (action == 37) {
		return new ACTION_LEADERBOARDGOALPOINTS(rawAction);
	} else if (action == 38) {
		return new ACTION_MOVELOCATION(rawAction);
	} else if (action == 39) {
		return new ACTION_MOVEUNIT(rawAction);
	} else if (action == 40) {
		return new ACTION_LEADERBOARDGREED(rawAction);
	} else if (action == 41) {
		return new ACTION_SETNEXTSCENARIO(rawAction);
	} else if (action == 42) {
		return new ACTION_SETDOODADSTATE(rawAction);
	} else if (action == 43) {
		return new ACTION_SETINVINCIBILLITY(rawAction);
	} else if (action == 44) {
		return new ACTION_CREATEUNIT(rawAction);
	} else if (action == 45) {
		return new ACTION_SETDEATHS(rawAction);
	} else if (action == 46) {
		return new ACTION_ORDER(rawAction);
	} else if (action == 47) {
		return new ACTION_COMMENT(rawAction);
	} else if (action == 48) {
		return new ACTION_GIVEUNITSTOPLAYER(rawAction);
	} else if (action == 49) {
		return new ACTION_MODIFYUNITHITPOINTS(rawAction);
	} else if (action == 50) {
		return new ACTION_MODIFYUNITENERGY(rawAction);
	} else if (action == 51) {
		return new ACTION_MODIFYUNITSHIELDPOINTS(rawAction);
	} else if (action == 52) {
		return new ACTION_MODIFYUNITRESOURCEAMOUNT(rawAction);
	} else if (action == 53) {
		return new ACTION_MODIFYUNITHANGERCOUNT(rawAction);
	} else if (action == 54) {
		return new ACTION_PAUSETIMER(rawAction);
	} else if (action == 55) {
		return new ACTION_UNPAUSETIMER(rawAction);
	} else if (action == 56) {
		return new ACTION_DRAW(rawAction);
	} else if (action == 57) {
		return new ACTION_SETALLIANCESTATUS(rawAction);
	} else {
		return new ACTION_INVALIDACTION(rawAction);
	}
}

#define PRINT_RR(i) {types[i]->print(STR, &(wbs[i]), error);if(*error) {return;};wbs[i].writeByte(0, error);if(*error) {return;}unsigned char* tmp;unsigned int tmpI;wbs[i].getWrittenData(&tmp, &tmpI);tmps[i] = (char*) tmp;}
#define PRINT_RRR(i) if(this->dataCount > i) {PRINT_RR(i);}
#define PRINT_RRRR(i, ...) if(this->dataCount == i+1) {PRINT_R("TRIGGERS", templaes[i], __VA_ARGS__); return;}

void TriggerContents::print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
	FIELDTYPE* types[] = { data1, data2, data3, data4, data5, data6, data7, data8, data9 };
	WriteBuffer wbs[9];
	char* tmps[9];
	char* templaes[] = {
		"(%s);",
		"(%s, %s);",
		"(%s, %s, %s);",
		"(%s, %s, %s, %s);",
		"(%s, %s, %s, %s, %s);",
		"(%s, %s, %s, %s, %s, %s);",
		"(%s, %s, %s, %s, %s, %s, %s);",
		"(%s, %s, %s, %s, %s, %s, %s, %s);",
		"(%s, %s, %s, %s, %s, %s, %s, %s, %s);"
	};

	char* source = (char*)(this->templateStr == nullptr ? this->invalidTemplateStr : this->templateStr);
	if (this->dataCount == 0) {
		PRINT_R("TRIGGERS", "%s", source);
		return;
	}


	unsigned int bpos = (strstr(source, "(") - source);
	for (unsigned int i = 0; i < bpos; i++) {
		char c = source[i];
		PRINT_R("TRIGGERS", "%c", c);
	}

	// Print arguments into separate strings
	PRINT_RRR(0);
	PRINT_RRR(1);
	PRINT_RRR(2);
	PRINT_RRR(3);
	PRINT_RRR(4);
	PRINT_RRR(5);
	PRINT_RRR(6);
	PRINT_RRR(7);
	PRINT_RRR(8);

	PRINT_RRRR(0, tmps[0]);
	PRINT_RRRR(1, tmps[0], tmps[1]);
	PRINT_RRRR(2, tmps[0], tmps[1], tmps[2]);
	PRINT_RRRR(3, tmps[0], tmps[1], tmps[2], tmps[3]);
	PRINT_RRRR(4, tmps[0], tmps[1], tmps[2], tmps[3], tmps[4]);
	PRINT_RRRR(5, tmps[0], tmps[1], tmps[2], tmps[3], tmps[4], tmps[5]);
	PRINT_RRRR(6, tmps[0], tmps[1], tmps[2], tmps[3], tmps[4], tmps[5], tmps[6]);
	PRINT_RRRR(7, tmps[0], tmps[1], tmps[2], tmps[3], tmps[4], tmps[5], tmps[6], tmps[7]);
	PRINT_RRRR(8, tmps[0], tmps[1], tmps[2], tmps[3], tmps[4], tmps[5], tmps[6], tmps[7], tmps[8]);
	return;
}

TriggerContents::~TriggerContents() {
	if (this->data1 != nullptr) {
		delete this->data1;
		this->data1 = nullptr;
	}
	if (this->data2 != nullptr) {
		delete this->data2;
		this->data2 = nullptr;
	}
	if (this->data3 != nullptr) {
		delete this->data3;
		this->data3 = nullptr;
	}
	if (this->data4 != nullptr) {
		delete this->data4;
		this->data4 = nullptr;
	}
	if (this->data5 != nullptr) {
		delete this->data5;
		this->data5 = nullptr;
	}
	if (this->data6 != nullptr) {
		delete this->data6;
		this->data6 = nullptr;
	}
	if (this->data7 != nullptr) {
		delete this->data7;
		this->data7 = nullptr;
	}
	if (this->data8 != nullptr) {
		delete this->data8;
		this->data8 = nullptr;
	}
	if (this->data9 != nullptr) {
		delete this->data9;
		this->data9 = nullptr;
	}
}

#endif

void updateBufferWithinSection(Section* section, char * originalCHKBuffer, unsigned int originalCHKBufferLength, unsigned int positionWithinBuffer) {
	section->completeCHKBuffer = originalCHKBuffer;
	section->totalInputBufferLength = originalCHKBufferLength;
	section->positionInInputBuffer = positionWithinBuffer;
}
