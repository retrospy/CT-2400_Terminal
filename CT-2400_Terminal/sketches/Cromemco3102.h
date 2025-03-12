#pragma once

#include "Terminal.h"

#define NUM_3102_COMMANDS 0x6F

struct Command
{
	bool(*CommandFn)();
	std::vector<char> KeysToTransmit();
};


class Cromemco3102 : public Terminal
{
public:
	virtual void AssignCommands();
	virtual bool ProcessCommand(char c, bool receive);
	virtual bool IsCommand(char c, bool isReceive);
	virtual void TerminalSetup();
	virtual void TerminalLoop1(int pins);
	virtual char* StartupMessage();
	virtual bool ShouldTransmit(char c);
};