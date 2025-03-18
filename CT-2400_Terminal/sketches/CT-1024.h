#pragma once

#include "Terminal.h"

#define NUM_1024_COMMAND_SETS	2
#define NUM_1024_COMMANDS		8

class CT_1024 : public Terminal
{
public:
	virtual void AssignCommands();
	virtual bool ProcessCommand(wchar_t& c, bool receive);
	virtual bool IsCommand(wchar_t c, bool isReceive);
	virtual wchar_t TransformReceived(wchar_t c);
	virtual void TerminalSetup();
	virtual void TerminalLoop1(int pins);
	virtual char* StartupMessage();
	virtual bool ShouldTransmit(wchar_t c);
	virtual int NextTabStop(int h);
		
private:
	bool(*CommandAssignment[NUM_1024_COMMAND_SETS][NUM_1024_COMMANDS])();
};