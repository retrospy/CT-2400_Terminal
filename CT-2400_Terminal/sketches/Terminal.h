#pragma once

#include "common.h"

class Terminal
{
public:
	virtual ~Terminal()
	{}
	virtual void AssignCommands() = 0;
	virtual bool ProcessCommand(char c, bool isReceive) = 0;
	virtual bool IsCommand(char c, bool isReceive) = 0;
	virtual void TerminalSetup() = 0;
	virtual void TerminalLoop1(int pins) = 0;
	virtual char* StartupMessage() = 0;
	virtual bool ShouldTransmit(char c) = 0;
};