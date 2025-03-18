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
//                                             12345678901234567890123456789012345678901234567890123456789012345678901234567890
static wchar_t startupMessage[COLUMNS + 1] = L"                    ******   Cromemco 3102 Mode   ******                        ";

bool static ProcessPositionCursor(bool receive)
{
	int lineNumber = min(ROWS, g_consumedKeys[0] - 0x1F);
	int columnNumber = g_consumedKeys[1] - 0x1F;

	if (isDebug)
	{
		Serial.print("M: ");
		Serial.print(lineNumber);
		Serial.print(" ");
		Serial.println(columnNumber);
	}
	
	if (lineNumber <= ROWS && columnNumber <= COLUMNS)
		CommandMoveCursor(lineNumber, columnNumber);
	
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
	switch (g_consumedKeys[0])
	{
	case '@':
		CommandNormalVideo();
		break;
	case 'A':
		CommandStartVideoAttribute(VA_HALF_INTENSITY);
		break;
	case 'B':
		CommandStartVideoAttribute(VA_BLINK);
		break;
	case 'C':
		CommandStartVideoAttribute(VA_HALF_INTENSITY | VA_BLINK);
		break;
	case 'P':
		CommandStartVideoAttribute(VA_REVERSE);
		break;
	case 'Q':
		CommandStartVideoAttribute(VA_REVERSE | VA_HALF_INTENSITY);
		break;
	case 'R':
		CommandStartVideoAttribute(VA_REVERSE | VA_BLINK);
		break;
	case 'S':
		CommandStartVideoAttribute(VA_REVERSE | VA_HALF_INTENSITY | VA_BLINK);
		break;
	case '`':
		CommandStartVideoAttribute(VA_UNDERLINE);
		break;
	case 'a':
		CommandStartVideoAttribute(VA_UNDERLINE | VA_HALF_INTENSITY);
		break;
	case 'b':
		CommandStartVideoAttribute(VA_UNDERLINE | VA_BLINK);
		break;
	case 'c':
		CommandStartVideoAttribute(VA_UNDERLINE | VA_HALF_INTENSITY | VA_BLINK);
		break;
	case 'p':
		CommandStartVideoAttribute(VA_UNDERLINE | VA_REVERSE);
		break;
	case 'q':
		CommandStartVideoAttribute(VA_UNDERLINE | VA_REVERSE | VA_HALF_INTENSITY);
		break;
	case 'r':
		CommandStartVideoAttribute(VA_UNDERLINE | VA_REVERSE | VA_BLINK);
		break;
	case 's':
		CommandStartVideoAttribute(VA_UNDERLINE | VA_REVERSE | VA_HALF_INTENSITY | VA_BLINK);
		break;
	case '$':
		CommandStartVideoAttribute(VA_INVISIBLE);
		break;
	case '4':
		CommandStartVideoAttribute(VA_INVISIBLE | VA_REVERSE);
		break;
	case '5':
		CommandStartVideoAttribute(VA_INVISIBLE | VA_REVERSE | VA_HALF_INTENSITY);
		break;
	case '6':
		CommandStartVideoAttribute(VA_INVISIBLE | VA_REVERSE | VA_BLINK);
		break;
	case '7':
		CommandStartVideoAttribute(VA_INVISIBLE | VA_REVERSE | VA_BLINK | VA_HALF_INTENSITY);
		break;
	}
		
	return true;
}

bool static CommandEnterVideoAttribute()
{
	g_keysToConsume = 1;
	g_keysConsumedCallback = ProcessEnterVideoAttribute;
	
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
	CommandAssignment['e'] = CommandNormalVideo;
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
			CommandMoveCursor(v, h - 1);
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

wchar_t* Cromemco3102::StartupMessage()
{
	return startupMessage;
}

bool Cromemco3102::ShouldTransmit(wchar_t c)
{
	return !IsLocalEchoOn();
}

wchar_t Cromemco3102::TransformReceived(wchar_t c)
{
	
	if (!isGraphicsMode || c < 0x40 || c > 0x6B)
		return c;
	
	switch (c)
	{
	// Upper Left Corner
	case '@':
		return 0x250F;
		break;
	case 'A':
		c = 0x250F;
		CommandStartVideoAttribute(VA_HALF_INTENSITY);
		break;
	case 'B':
		c = 0x250F;
		CommandStartVideoAttribute(VA_BLINK);
		break;
	case 'C':
		c = 0x250F;
		CommandStartVideoAttribute(VA_HALF_INTENSITY | VA_BLINK);
		break;
	// Upper Right Corner
	case 'D':
		return 0x2513;
		break;
	case 'E':
		c = 0x2513;
		CommandStartVideoAttribute(VA_HALF_INTENSITY);
		break;
	case 'F':
		c = 0x2513;
		CommandStartVideoAttribute(VA_BLINK);
		break;
	case 'G':
		c = 0x2513;
		CommandStartVideoAttribute(VA_HALF_INTENSITY | VA_BLINK);
		break;
	// Lower Left Corner
	case 'H':
		return 0x2517;
		break;
	case 'I':
		c = 0x2517;
		CommandStartVideoAttribute(VA_HALF_INTENSITY);
		break;
	case 'J':
		c = 0x2517;
		
		break;
	case 'K':
		c = 0x2517;
		CommandStartVideoAttribute(VA_HALF_INTENSITY | VA_BLINK);
		break;
	// Lower Right Corner
	case 'L':
		return 0x251B;
		break;
	case 'M':
		c = 0x251B;
		CommandStartVideoAttribute(VA_HALF_INTENSITY);
		break;
	case 'N':
		c = 0x251B;
		CommandStartVideoAttribute(VA_BLINK);
		break;
	case 'O':
		c = 0x251B;
		CommandStartVideoAttribute(VA_HALF_INTENSITY | VA_BLINK);
		break;
	// Upper 'T'
	case 'P':
		return 0x2533;
		break;
	case 'Q':
		c = 0x2533;
		CommandStartVideoAttribute(VA_HALF_INTENSITY);
		break;
	case 'R':
		c = 0x2533;
		CommandStartVideoAttribute(VA_BLINK);
		break;
	case 'S':
		c = 0x2533;
		CommandStartVideoAttribute(VA_HALF_INTENSITY | VA_BLINK);
		break;
	// Right 'T'
	case 'T':
		return 0x252B;
		break;
	case 'U':
		c = 0x252B;
		CommandStartVideoAttribute(VA_HALF_INTENSITY);
		break;
	case 'V':
		c = 0x252B;
		CommandStartVideoAttribute(VA_BLINK);
		break;
	case 'W':
		c = 0x252B;
		CommandStartVideoAttribute(VA_HALF_INTENSITY | VA_BLINK);
		break;
	// Left 'T'
	case 'X':
		return 0x2523;
		break;
	case 'Y':
		c = 0x2523;
		CommandStartVideoAttribute(VA_HALF_INTENSITY);
		break;
	case 'Z':
		c = 0x2523;
		CommandStartVideoAttribute(VA_BLINK);
		break;
	case '[':
		c = 0x2523;
		CommandStartVideoAttribute(VA_HALF_INTENSITY | VA_BLINK);
		break;
	// Bottom 'T'
	case '\\':
		return 0x253B;
		break;
	case ']':
		c = 0x253B;
		CommandStartVideoAttribute(VA_HALF_INTENSITY);
		break;
	case '^':
		c = 0x253B;
		CommandStartVideoAttribute(VA_BLINK);
		break;
	case '_':
		c = 0x253B;
		CommandStartVideoAttribute(VA_HALF_INTENSITY | VA_BLINK);
		break;
	// Horiztonal Line
	case '`':
		return 0x2501;
		break;
	case 'a':
		c = 0x2501;
		CommandStartVideoAttribute(VA_HALF_INTENSITY);
		break;
	case 'b':
		c = 0x2501;
		CommandStartVideoAttribute(VA_BLINK);
		break;
	case 'c':
		c = 0x2501;
		CommandStartVideoAttribute(VA_HALF_INTENSITY | VA_BLINK);
		break;
	// Vertical Line
	case 'd':
		return 0x2503;
		break;
	case 'e':
		c = 0x2503;
		CommandStartVideoAttribute(VA_HALF_INTENSITY);
		break;
	case 'f':
		c = 0x2503;
		CommandStartVideoAttribute(VA_BLINK);
		break;
	case 'g':
		c = 0x2503;
		CommandStartVideoAttribute(VA_HALF_INTENSITY | VA_BLINK);
		break;
	// Cross
	case 'h':
		return 0x254B;
		break;
	case 'i':
		c = 0x254B;
		CommandStartVideoAttribute(VA_HALF_INTENSITY);
		break;
	case 'j':
		c = 0x254B;
		CommandStartVideoAttribute(VA_BLINK);
		break;
	case 'k':
		c = 0x254B;
		CommandStartVideoAttribute(VA_HALF_INTENSITY | VA_BLINK);
		break;
	}
	
	return c;
}


int Cromemco3102::NextTabStop(int h)
{
	h -= 1;
	
	h = (h + 8) - (h % 8);
	
	if (h > 72)
	{
		h = 0;
	}
	
	h += 1;

	return h;
}


bool Cromemco3102::EraseLineOnLineFeed()
{
	return false;
}