#pragma once

#include "Terminal.h"

#define NUM_3102_COMMANDS 0x70

struct Command
{
	bool(*CommandFn)();
	std::vector<wchar_t> KeysToTransmit();
};


class Cromemco3102 : public Terminal
{
public:
	virtual void AssignCommands();
	virtual bool ProcessCommand(wchar_t& c, bool receive);
	virtual bool IsCommand(wchar_t c, bool isReceive);
	virtual wchar_t TransformReceived(wchar_t c);
	virtual void TerminalSetup();
	virtual void TerminalLoop1(int pins);
	virtual wchar_t* StartupMessage();
	virtual bool ShouldTransmit(wchar_t c);
	virtual int NextTabStop(int h);
	virtual bool EraseLineOnLineFeed();
};