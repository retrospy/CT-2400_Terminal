#include "common.h"
#include "Cromemco3102.h"

#define		STX		0x02
#define		ENQ		0x05
#define		BS		0x08
#define		ESC		0x1B

#define		CTRL_K	0x0B
#define		CTRL_J	0x0A
#define		CTRL_L	0x0C

#define		UP		CTRL_K
#define		DOWN	CTRL_J
#define		LEFT	BS
#define		RIGHT	CTRL_L

                                        // 12345678901234567890123456789012345678901234567890123456789012345678901234567890
static char startupMessage[COLUMNS + 1] = "                    ******   Cromemco 3102 Mode   ******                        ";

bool static ProcessPositionCursor(bool receive)
{
	int lineNumber = g_consumedKeys[0] - 0x1F;
	int columnNumber = g_consumedKeys[1] - 0x1F;

#ifdef DEBUG
	Serial.print("M: ");
	Serial.print(lineNumber);
	Serial.print(" ");
	Serial.println(columnNumber);
#endif
	
	if (lineNumber <= ROWS && columnNumber <= COLUMNS)
		MoveCursor(lineNumber, columnNumber);
	
	return true;
}

bool static CommandErase()
{
	CommandEraseAll();
	CommandHome();
	
	return true;
}

bool static CommandPositionCursor()
{
	g_keysToConsume = 2;
	g_keysConsumedCallback = ProcessPositionCursor;
	
	return true;
}

static bool(*CommandAssignment[NUM_3102_COMMANDS])();

void Cromemco3102::AssignCommands()
{
	for (int i = 0; i < NUM_3102_COMMANDS; ++i)
	{
		CommandAssignment[i] = nullptr;		
	}
	
	CommandAssignment['A'] = CommandCursorUp;
	CommandAssignment['B'] = CommandCursorDown;
	CommandAssignment['C'] = CommandCursorRight;
	CommandAssignment['D'] = CommandCursorLeft;
	CommandAssignment['E'] = CommandErase;
	CommandAssignment['F'] = CommandPositionCursor;
	CommandAssignment['H'] = CommandHome;
	CommandAssignment['J'] = CommandEraseToEOF;
	CommandAssignment['K'] = CommandEraseToEOL;
	CommandAssignment['L'] = CommandInsertLine;
	CommandAssignment['M'] = CommandDeleteLine;
	CommandAssignment['Z'] = CommandCursorToggle;
	CommandAssignment['l'] = CommandStartBlink;
	CommandAssignment['m'] = CommandNormalVideo;
	//CommandAssignment['['] = CommandVT100EscapeCode;	
}

static bool ProcessEscapeCode(bool isReceive)
{
	char c = g_consumedKeys[0];
	
	if (!isReceive)
		return true;
	
	if (CommandAssignment[c] != nullptr && c >= 0x20 && c <= 0x6F)
	{
		CommandAssignment[c]();		
	}
	
	return true;
}

bool Cromemco3102::ProcessCommand(char& c, bool receive)
{
	if (!receive)
	{
		switch (c)
		{
		case ARROW_UP:
			c = UP;  
			break;
		case ARROW_DOWN:
			c = DOWN; 
			break;
		case ARROW_LEFT:
			c = LEFT; 
			break;	
		case ARROW_RIGHT:
			c = RIGHT;
			break;	
		}
		
		return true;
	}
	else
	{
		if (c == ESC)
		{	
			g_keysToConsume = 1;
			g_keysConsumedCallback = ProcessEscapeCode;
		}
		else if (c == BS)
		{
			int h, v;
		
			GetCurrentScreenPosition(v, h);
			MoveCursor(v, h - 1);
		}
		else if (c == ENQ)
		{
			Serial1.write(STX);
			Serial1.write(STX);
			Serial1.write('3');
			Serial1.write('1');
			Serial1.write('0');
			Serial1.write('2');
		}
		return true;
	}
}

bool Cromemco3102::IsCommand(char c, bool isReceive)
{
	return (isReceive && (c == ESC || c == BS || c == ENQ || c == '\0'))
				|| (!isReceive && ((unsigned char)c) >= ARROW_UP && ((unsigned char)c) <= ARROW_LEFT);
}

void Cromemco3102::TerminalSetup()
{
	
}

void Cromemco3102::TerminalLoop1(int pins)
{

}

char* Cromemco3102::StartupMessage()
{
	return startupMessage;
}

bool Cromemco3102::ShouldTransmit(char c)
{
	return !IsLocalEchoOn();
}