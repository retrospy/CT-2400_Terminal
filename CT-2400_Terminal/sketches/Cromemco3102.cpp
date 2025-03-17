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

	if (isDebug)
	{
		Serial.print("M: ");
		Serial.print(lineNumber);
		Serial.print(" ");
		Serial.println(columnNumber);
	}
	
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

bool isGraphicsMode = false;

bool static CommandGraphicsModeOn()
{
	isGraphicsMode = true;
	return true;
}

bool static CommandGraphicsModeOff()
{
	isGraphicsMode = false;
	return true;
}

bool static ProcessEnterVideoAttribute(bool receive)
{	
	return true;
}


bool static CommandEnterVideoAttribute()
{
	g_keysToConsume = 1;
	g_keysConsumedCallback = ProcessEnterVideoAttribute;
	
	return true;
}

bool static CommandDeleteVideoAttribute()
{	
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
	CommandAssignment['R'] = CommandGraphicsModeOn;
	CommandAssignment['S'] = CommandGraphicsModeOff;
	CommandAssignment['Z'] = CommandCursorToggle;
	CommandAssignment['d'] = CommandEnterVideoAttribute;
	CommandAssignment['e'] = CommandDeleteVideoAttribute;
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

bool Cromemco3102::ProcessCommand(wchar_t& c, bool receive)
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

bool Cromemco3102::IsCommand(wchar_t c, bool isReceive)
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

bool Cromemco3102::ShouldTransmit(wchar_t c)
{
	return !IsLocalEchoOn();
}

wchar_t Cromemco3102::TransformReceived(wchar_t c)
{
	if (!isGraphicsMode)
		return c;
	
	switch (c)
	{
	// Upper Left Corner
	case '@':
		return 0x250F;
		break;
	case 'A':
		return 0x250C;
		break;
	case 'B':
		return 0x250F;
		break;
	case 'C':
		return 0x250C;
		break;
	// Upper Right Corner
	case 'D':
		return 0x2513;
		break;
	case 'E':
		return 0x2510;
		break;
	case 'F':
		return 0x2513;
		break;
	case 'G':
		return 0x2510;
		break;
	// Lower Left Corner
	case 'H':
		return 0x2517;
		break;
	case 'I':
		return 0x2514;
		break;
	case 'J':
		return 0x2517;
		break;
	case 'K':
		return 0x2514;
		break;
	// Lower Right Corner
	case 'L':
		return 0x251B;
		break;
	case 'M':
		return 0x2518;
		break;
	case 'N':
		return 0x251B;
		break;
	case 'O':
		return 0x2518;
		break;
	// Upper 'T'
	case 'P':
		return 0x2533;
		break;
	case 'Q':
		return 0x252C;
		break;
	case 'R':
		return 0x2533;
		break;
	case 'S':
		return 0x252C;
		break;
	// Right 'T'
	case 'T':
		return 0x252B;
		break;
	case 'U':
		return 0x2524;
		break;
	case 'V':
		return 0x252B;
		break;
	case 'W':
		return 0x2524;
		break;
	// Left 'T'
	case 'X':
		return 0x2523;
		break;
	case 'Y':
		return 0x251C;
		break;
	case 'Z':
		return 0x2523;
		break;
	case '[':
		return 0x251C;
		break;
	// Bottom 'T'
	case '\\':
		return 0x253B;
		break;
	case ']':
		return 0x2534;
		break;
	case '^':
		return 0x253B;
		break;
	case '_':
		return 0x2534;
		break;
	// Horiztonal Line
	case '`':
		return 0x2501;
		break;
	case 'a':
		return 0x2500;
		break;
	case 'b':
		return 0x2501;
		break;
	case 'c':
		return 0x2500;
		break;
	// Vertical Line
	case 'd':
		return 0x2503;
		break;
	case 'e':
		return 0x2502;
		break;
	case 'f':
		return 0x2503;
		break;
	case 'g':
		return 0x2502;
		break;
	// Cross
	case 'h':
		return 0x254B;
		break;
	case 'i':
		return 0x253C;
		break;
	case 'j':
		return 0x254B;
		break;
	case 'k':
		return 0x253C;
		break;
	default:
		return c;
		break;
	}
}