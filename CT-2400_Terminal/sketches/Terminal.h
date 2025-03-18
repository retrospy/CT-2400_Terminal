#pragma once

#include "common.h"

class Terminal
{
public:
	virtual ~Terminal()
	{}
	virtual void AssignCommands() = 0;
	virtual bool ProcessCommand(wchar_t& c, bool isReceive) = 0;
	virtual bool IsCommand(wchar_t c, bool isReceive) = 0;
	virtual wchar_t TransformReceived(wchar_t c) = 0;
	virtual bool EraseLineOnLineFeed() = 0;
	virtual void TerminalSetup() = 0;
	virtual void TerminalLoop1(int pins) = 0;
	virtual wchar_t* StartupMessage() = 0;
	virtual bool ShouldTransmit(wchar_t c) = 0;
	
	virtual int NextTabStop(int h) = 0;
};