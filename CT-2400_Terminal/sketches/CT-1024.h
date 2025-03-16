#pragma once

#include "Terminal.h"

#define NUM_1024_COMMAND_SETS	2
#define NUM_1024_COMMANDS		8

class CT_1024 : public Terminal
{
public:
	virtual void AssignCommands();
	virtual bool ProcessCommand(char& c, bool receive);
	virtual bool IsCommand(char c, bool isReceive);
	virtual void TerminalSetup();
	virtual void TerminalLoop1(int pins);
	virtual char* StartupMessage();
	virtual bool ShouldTransmit(char c);
		
private:
	bool(*CommandAssignment[NUM_1024_COMMAND_SETS][NUM_1024_COMMANDS])();
};