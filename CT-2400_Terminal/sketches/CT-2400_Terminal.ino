
#include "common.h"
#include "CT-1024.h"
#include "Cromemco3102.h"

#include "hardware/flash.h" // for the flash erasing and writing
#include "hardware/sync.h" // for the interrupts

#include "wchar.h"

#include <string>

#define BUFFER_SIZE		2400

Terminal* terminal = nullptr;

volatile bool wrapVertical = false;
bool wrapHorizontal = true;

volatile bool hasLowercase = true;

volatile bool resetPushed = false;

volatile bool setupComplete = false;

volatile bool offlineMode = false;

int baudRate = 300;

bool isDebug = false;

struct Position
{
	int v;
	int h;
};

struct FlashConfig
{
	FlashConfig()
	{
		
	}
	int magic_number;
	int version;
	int TerminalType;
};

struct BufferEntry
{
	wchar_t Character;
	byte VideoAttributes;
};

// Buffer goes from 1 to ROWS/COLUMNS, so column and row 0 are ignored 
BufferEntry pageBuffer[PAGES][ROWS + 1][COLUMNS + 1];  
volatile int currentPageBuffer = 0;
volatile int newPageBuffer = 0;
Position currentPageBufferPosition[PAGES];
int firstRowOfPageBuffer[PAGES];

// A BMP multi-byte character, with terminating NUL byte.
struct mbchar {
	char utf8[4];
};

// Convert a wide character to UTF-8. Only works within the BMP.
mbchar wchar_to_utf8(wchar_t c) {
	mbchar result;
	if (c < 128) {
		// 0xxx.xxxx
		result.utf8[0] = c;
		result.utf8[1] = 0;
	}
	else if (c < 2048) {
		// 110x.xxxx 10xx.xxxx
		result.utf8[0] = 0xc0 | (c >> 6);
		result.utf8[1] = 0x80 | (c & 0x3f);
		result.utf8[2] = 0;
	}
	else {
		// 1110.xxxx 10xx.xxxx 10xx.xxxx
		result.utf8[0] = 0xe0 | (c >> 12);
		result.utf8[1] = 0x80 | (c >> 6 & 0x3f);
		result.utf8[2] = 0x80 | (c & 0x3f);
		result.utf8[3] = 0;
	}
	return result;
}

int getCurrentPage()
{
	return currentPageBuffer;
}

int GetBaudRate()
{
	if (digitalRead(BAUD110) == LOW)
		return 110 ;
	else if (digitalRead(BAUD150) == LOW)
		return 150 ;
	else if (digitalRead(BAUD300) == LOW)
		return 300 ;
	else if (digitalRead(BAUD600) == LOW)
		return 600 ;
	else if (digitalRead(BAUD1200) == LOW)
		return 1200 ;
	else if (digitalRead(BAUD2400) == LOW)
		return 2400 ;
	else if (digitalRead(BAUD4800) == LOW)
		return 4800 ;
	else
		return 9600;
}
;

void SetupClock(int baudRate)
{ 
	pinMode(CLK_OUT, OUTPUT);
	analogWriteFreq(baudRate * 16);
	analogWriteResolution(8);
	
	analogWrite(CLK_OUT, 128);
	
	clock_configure_gpin(clk_peri, CLK_IN, baudRate * 16, baudRate * 16);
}

void GetCurrentScreenPosition(int& v, int& h)
{
	Serial.printf("%c[6n", 27);
	String response = Serial.readStringUntil('R');
	// ^[<v>;<h>R
	v = response.substring(response.indexOf('[') + 1, response.indexOf(';')).toInt();
	h = response.substring(response.indexOf(';') + 1, response.indexOf('R')).toInt();
}

void MoveCursor(int v, int h)
{
	if (!isDebug)
	{
		Serial.printf("%c[%d;%dH", 27, v, h);
	}
}

void MoveCursorRight()
{
	if (!isDebug)
	{
		Serial.printf("%c[C", 27);
	}
}

void MoveCursorLeft()
{
	if (!isDebug)
	{
		Serial.printf("%c[D", 27);
	}
}

void MoveCursorUp()
{
	if (!isDebug)
	{
		Serial.printf("%c[A", 27);
	}
}

void MoveCursorDown()
{
	if (!isDebug)
	{
		Serial.printf("%c[B", 27);
	}
}

void MoveCursorToHome()
{
	if (!isDebug)
	{
		Serial.printf("%c[H", 27);
	}
}

void EraseToEOL()
{
	if (!isDebug)
	{
		Serial.printf("%c[K", 27);
	}
}

void EraseToEOF()
{
	if (!isDebug)
	{
		Serial.printf("%c[J", 27);
	}
}

int GenerateRealRowPosition(int terminalRow)
{	
	return (((firstRowOfPageBuffer[currentPageBuffer] - 1) + (terminalRow - 1)) % ROWS) + 1;
}

void WriteCharactorToCurrentPageBuffer(wchar_t c)
{
//	Serial.print("V=");
//	Serial.println(GenerateRealRowPosition(currentPageBufferPosition[currentPageBuffer].v));
//	Serial.print("H=");
//	Serial.println(currentPageBufferPosition[currentPageBuffer].h);
		
	pageBuffer[currentPageBuffer][GenerateRealRowPosition(currentPageBufferPosition[currentPageBuffer].v)]
		                         [currentPageBufferPosition[currentPageBuffer].h].Character = hasLowercase ? c : toupper(c);
}



bool CommandCursorRight()
{
	int v, h;
	GetCurrentScreenPosition(v, h);
  
	if (wrapVertical && wrapHorizontal && h == COLUMNS && v == ROWS)
	{
		MoveCursorToHome();
		currentPageBufferPosition[currentPageBuffer].h = 1;
		currentPageBufferPosition[currentPageBuffer].v = 1;
	}
	else if (!wrapVertical && wrapHorizontal && h == COLUMNS && v == ROWS)
	{
		Serial.println();
		currentPageBufferPosition[currentPageBuffer].h = 1;
		
		firstRowOfPageBuffer[currentPageBuffer] = (((firstRowOfPageBuffer[currentPageBuffer] - 1) + 1) % ROWS) + 1;
	}
	else if (wrapHorizontal && h == COLUMNS && v != ROWS)
	{
		MoveCursor(v + 1, 1);
		currentPageBufferPosition[currentPageBuffer].h = 1;
		currentPageBufferPosition[currentPageBuffer].v++;		
	}
	else
	{     
		// Cursor right
		MoveCursorRight();
		currentPageBufferPosition[currentPageBuffer].h++;
	}  
	
	return true;
}

bool CommandCursorDown()
{
	int v, h;
	GetCurrentScreenPosition(v, h);
  
	if (wrapVertical && v == ROWS)
	{
		MoveCursor(1, h);
		currentPageBufferPosition[currentPageBuffer].h = 1;
		currentPageBufferPosition[currentPageBuffer].v = 1;
	}
	else
	{
		// Cursor down
		MoveCursorDown();
		currentPageBufferPosition[currentPageBuffer].v++;
	}
	
	return true;
}

bool CommandCursorLeft()
{

	int v, h;
	GetCurrentScreenPosition(v, h);
  
	if (wrapHorizontal && COLUMNS == 1)
	{
		MoveCursor(v, COLUMNS);
		currentPageBufferPosition[currentPageBuffer].h = COLUMNS;
	  
	}
	else
	{
		// Cursor left
		MoveCursorLeft();
		currentPageBufferPosition[currentPageBuffer].h--;
	}

	return true;
}

bool CommandCursorUp()
{
	int v, h;
	GetCurrentScreenPosition(v, h);
  
	if (wrapVertical && v == 1)
	{
		MoveCursor(ROWS, h);
		currentPageBufferPosition[currentPageBuffer].v = ROWS;
	}
	else
	{
		// Cursor up
		MoveCursorUp();
		currentPageBufferPosition[currentPageBuffer].v--;
	}

	return true;
}

bool CommandHome()
{
	if (!isDebug)
	{
		Serial.printf("%c[H", 27);
		currentPageBufferPosition[currentPageBuffer] = { 1, 1 };
	}
	return true;
}

bool CommandMoveCursor(int v, int h)
{
	if (!isDebug)
	{
		MoveCursor(v, h);
		currentPageBufferPosition[currentPageBuffer] = { v, h };
	}
	return true;
}

bool CommandEraseToEOL()
{
	if (!isDebug)
	{
		Serial.printf("%c[K", 27);
	
		int v, h;
		GetCurrentScreenPosition(v, h);
	
		for (int i = h; i <= COLUMNS; ++i)
		{
			pageBuffer[currentPageBuffer][GenerateRealRowPosition(v)][i] = {' ', VA_NORMAL};
		}
	}	
	
	return true;
}

bool CommandEraseToEOF()
{
	if (!isDebug)
	{
		Serial.printf("%c[J", 27);
	
		int v, h;
		GetCurrentScreenPosition(v, h);
	
		for (int i = h; i <= COLUMNS; ++i)
			pageBuffer[currentPageBuffer][GenerateRealRowPosition(v)][i] = {' ', VA_NORMAL};
	
		for (int j = firstRowOfPageBuffer[currentPageBuffer] + 1; j <= ROWS; ++j)
			for (int k = 1; k <= COLUMNS; ++k)
			{
				pageBuffer[currentPageBuffer][j][k] = { ' ', VA_NORMAL };
			}
		
		for (int j = 1; j <= firstRowOfPageBuffer[currentPageBuffer] - 1; ++j)
			for (int k = 1; k <= COLUMNS; ++k)
			{
				pageBuffer[currentPageBuffer][j][k] = { ' ', VA_NORMAL };
			}
	}
	
	return true;
}

bool CommandEraseAll()
{

	if (!isDebug)
	{
		Serial.printf("%c[2J", 27);
	
		for (int j = 1; j <= ROWS; ++j)
			for (int k = 1; k <= COLUMNS; ++k)
			{
				pageBuffer[currentPageBuffer][j][k] = {' ', VA_NORMAL};
			}
	}
	
	return true;
}

bool CommandEraseLine()
{
	if (!isDebug)
	{
		Serial.printf("%c[2K", 27);
	
		int v, h;
		GetCurrentScreenPosition(v, h);
	
		for (int i = 0; i <= COLUMNS; ++i)
			pageBuffer[currentPageBuffer][GenerateRealRowPosition(v)][i] = {' ', VA_NORMAL};
	}
	
	return true;
}

bool CommandCursorOn()
{
	Serial.printf("%c[?25h", 27);
	
	return true;
}

bool CommandCursorOff()
{
	Serial.printf("%c[?25l", 27);
	
	return true;
}

bool cursorOn = true;

bool CommandCursorToggle()
{
	if (cursorOn)
		CommandCursorOff();
	else
		CommandCursorOn();
	
	cursorOn = !cursorOn;
	
	return true;
}

bool CommandStartVideoAttribute(byte attributes, int v, int h)
{
	//if (attributes == pageBuffer[currentPageBuffer][v][h].VideoAttributes)
	//	return true;
	
	pageBuffer[currentPageBuffer][v][h].VideoAttributes = attributes;
	
	if (attributes == VA_NORMAL)
	{
		Serial.printf("%c[m", 27);
		return true;
	}

	bool firstAttribute = true;
	std::string command;
	
	command.push_back(27);
	command.push_back('[');
	
	if ((attributes & VA_HALF_INTENSITY) != 0)
	{
		if (!firstAttribute)
			command.push_back(';');
		command.push_back('2');
		firstAttribute = false;
	}
	if ((attributes & VA_UNDERLINE) != 0)
	{
		if (!firstAttribute)
			command.push_back(';');
		command.push_back('4');
		firstAttribute = false;
	}
	if ((attributes & VA_BLINK) != 0)
	{
		if (!firstAttribute)
			command.push_back(';');
		command.push_back('5');
		firstAttribute = false;
	}
	if ((attributes & VA_REVERSE) != 0)
	{
		if (!firstAttribute)
			command.push_back(';');
		command.push_back('7');
		firstAttribute = false;
	}
	if ((attributes & VA_INVISIBLE) != 0)
	{
		if (!firstAttribute)
			command.push_back(';');
		command.push_back('8');
		firstAttribute = false;
	}
	command.push_back('m');
	
	Serial.write(command.c_str());
	
	return true;
}

bool CommandStartVideoAttribute(byte attributes)
{
	int v, h;
	GetCurrentScreenPosition(v, h);
	
	return CommandStartVideoAttribute(attributes, v, h);
}


bool CommandStartBlink()
{
	return CommandStartVideoAttribute(VA_BLINK);
	
	return true;
}

bool CommandNormalVideo()
{
	return CommandStartVideoAttribute(VA_NORMAL);
}

bool CommandDeleteLine()
{
	
	int v;
	int h;
	GetCurrentScreenPosition(v, h);
	
	int deletedRow = GenerateRealRowPosition(v);

	MoveCursor(v, 1);
	EraseToEOF();
	
	for (int j = v + 1; j <= ROWS; ++j)
		for (int k = 1; k <= COLUMNS; ++k)
		{
			pageBuffer[currentPageBuffer][GenerateRealRowPosition(j - 1)][k] = 
						pageBuffer[currentPageBuffer][GenerateRealRowPosition(j)][k];
			Serial.write(wchar_to_utf8(pageBuffer[currentPageBuffer][GenerateRealRowPosition(j)][k].Character).utf8);
			CommandStartVideoAttribute(pageBuffer[currentPageBuffer][GenerateRealRowPosition(j)][k].VideoAttributes, GenerateRealRowPosition(j), k);
		}

	for (int k = 1; k <= COLUMNS; ++k)
	{
		pageBuffer[currentPageBuffer][GenerateRealRowPosition(ROWS)][k] = { ' ', VA_NORMAL };
		Serial.write(wchar_to_utf8(pageBuffer[currentPageBuffer][GenerateRealRowPosition(ROWS)][k].Character).utf8);
		CommandStartVideoAttribute(pageBuffer[currentPageBuffer][GenerateRealRowPosition(ROWS)][k].VideoAttributes, GenerateRealRowPosition(ROWS), k);
	}
		
	MoveCursor(v, h);
	
	return true;
}

bool CommandInsertLine()
{
	int v;
	int h;
	GetCurrentScreenPosition(v, h);
	
	for (int j = ROWS - 1; j >= v; --j)
	{
		for (int k = 1; k <= COLUMNS; ++k)
		{
			pageBuffer[currentPageBuffer][GenerateRealRowPosition(j + 1)][k] = 
						pageBuffer[currentPageBuffer][GenerateRealRowPosition(j)][k];
		}
	}

	for (int k = 1; k <= COLUMNS; ++k)
	{
		pageBuffer[currentPageBuffer][GenerateRealRowPosition(v)][k] = { ' ', VA_NORMAL };
	}
	
	MoveCursor(v, 1);
	EraseToEOF();
	for (int i = v; i <= ROWS; ++i)
	{
		for (int j = 1; j <= COLUMNS; ++j)
		{
			Serial.write(wchar_to_utf8(pageBuffer[currentPageBuffer][GenerateRealRowPosition(i)][j].Character).utf8);
			CommandStartVideoAttribute(pageBuffer[currentPageBuffer][GenerateRealRowPosition(i)][j].VideoAttributes, GenerateRealRowPosition(i), j);
		}
	}
	
	MoveCursor(v, h);
	
	return true;
}

int g_keysToConsume = 0;
std::vector<wchar_t> g_consumedKeys;
bool(*g_keysConsumedCallback)(bool receive) = nullptr;

bool ProcessVT100EscapeCode(bool receive)
{
	switch (g_consumedKeys[0])
	{
	case 'A':
		CommandCursorUp();
		break;
	case 'B':
		CommandCursorDown();
		break;
	case 'C':
		CommandCursorRight();
		break;
	case 'D':
		CommandCursorLeft();
		break;
	}
	
	return true;
}

bool CommandVT100EscapeCode()
{
	g_keysToConsume = 1;
	g_keysConsumedCallback = ProcessVT100EscapeCode;
	
	return false;
}

void ProcessReceivedByte(wchar_t c)
{
	if (isDebug)
	{
		Serial.print("R: ");
		Serial.println(c, HEX);
	}
	
	// Handle position only characters
	if (g_keysToConsume > 0)
	{
		g_consumedKeys.push_back(c);
		g_keysToConsume--;
		if (g_keysToConsume == 0)
		{
			g_keysConsumedCallback(true);
			g_consumedKeys.clear();
		}
		return;
	}
	else if (terminal->IsCommand(c, true))
	{
		terminal->ProcessCommand(c, true);
		return;
	}
	else if (c == CR)
	{
		Serial.write(CR);
		currentPageBufferPosition[currentPageBuffer].h = 1;
		return;
	}
	else if (c == LF)
	{
		if (currentPageBufferPosition[currentPageBuffer].v != ROWS)
		{
			Serial.write(LF);
			
			currentPageBufferPosition[currentPageBuffer].v += 1;
		}
		else if (wrapVertical && currentPageBufferPosition[currentPageBuffer].v == ROWS)
		{
			int v, h;
			GetCurrentScreenPosition(v, h);

			MoveCursor(1, h);
			currentPageBufferPosition[currentPageBuffer].v = 1;
		}
		else if (!wrapVertical && currentPageBufferPosition[currentPageBuffer].v == ROWS)
		{
			Serial.write(LF);
				
			firstRowOfPageBuffer[currentPageBuffer] = (((firstRowOfPageBuffer[currentPageBuffer] - 1) + 1) % ROWS) + 1;				
		}
		
		//CommandEraseLine();
		
		return;
	}
	else if (c == TAB)
	{
		int v, h;
		GetCurrentScreenPosition(v, h);
		
		bool wrapped = false;
		h = terminal->NextTabStop(h);
		
		if (h > 1)
		{
			MoveCursor(v, h);
		}
		else
		{
			if (currentPageBufferPosition[currentPageBuffer].v != ROWS)
			{
				Serial.write(CR);
				Serial.write(LF);
			
				currentPageBufferPosition[currentPageBuffer].v += 1;
				currentPageBufferPosition[currentPageBuffer].h = 1;
			}
			else if (wrapVertical && currentPageBufferPosition[currentPageBuffer].v == ROWS)
			{
				MoveCursor(1, 1);
				currentPageBufferPosition[currentPageBuffer].v = 1;
				currentPageBufferPosition[currentPageBuffer].h = 1;
			}
			else if (!wrapVertical && currentPageBufferPosition[currentPageBuffer].v == ROWS)
			{
				Serial.write(CR);
				Serial.write(LF);
				
				currentPageBufferPosition[currentPageBuffer].h = 1;
				
				firstRowOfPageBuffer[currentPageBuffer] = (((firstRowOfPageBuffer[currentPageBuffer] - 1) + 1) % ROWS) + 1;				
			}		
		}
		
	}
		
	if (c >= 0x20 && c <= 0x7f)
	{
		int v, h;
		GetCurrentScreenPosition(v, h);

		wchar_t charToDisplay = terminal->TransformReceived(c);

		Serial.write(wchar_to_utf8(hasLowercase ? charToDisplay : toupper(charToDisplay)).utf8);
		
		WriteCharactorToCurrentPageBuffer(charToDisplay);
		
		if (wrapVertical && wrapHorizontal && h == COLUMNS && v == ROWS)
		{
			MoveCursorToHome();
			currentPageBufferPosition[currentPageBuffer].h = 1;
			currentPageBufferPosition[currentPageBuffer].v = 1;
		}
		else if (!wrapVertical && wrapHorizontal && h == COLUMNS && v == ROWS)
		{
			Serial.write(CR);
			Serial.write(LF);
			currentPageBufferPosition[currentPageBuffer].h = 1;
		
			firstRowOfPageBuffer[currentPageBuffer] = (((firstRowOfPageBuffer[currentPageBuffer] - 1) + 1) % ROWS) + 1;
		}
		else if (wrapHorizontal && h == COLUMNS && v != ROWS)
		{
			MoveCursor(v + 1, 1);
			currentPageBufferPosition[currentPageBuffer].h = 1;
			currentPageBufferPosition[currentPageBuffer].v++;		
		}
		else
		{     
			// Cursor right
			MoveCursor(v, h + 1);
			currentPageBufferPosition[currentPageBuffer].h++;
		}  
	}
}

void SwapPages()
{
	currentPageBuffer = (currentPageBuffer + 1) % PAGES;
	if (wrapVertical)
	{
		MoveCursorToHome();
		EraseToEOF();
	}
	else
	{
		MoveCursor(ROWS, 1);
		Serial.println();
	}
		
	for (int j = firstRowOfPageBuffer[currentPageBuffer]; j <= ROWS; ++j)
		for (int k = 1; k <= COLUMNS; ++k)
		{
			Serial.write(wchar_to_utf8(pageBuffer[currentPageBuffer][j][k].Character).utf8);
			CommandStartVideoAttribute(pageBuffer[currentPageBuffer][j][k].VideoAttributes, j, k);
		}
		
	for (int j = 1; j <= firstRowOfPageBuffer[currentPageBuffer] - 1; ++j)
		for (int k = 1; k <= COLUMNS; ++k)
		{
			Serial.write(wchar_to_utf8(pageBuffer[currentPageBuffer][j][k].Character).utf8);
			CommandStartVideoAttribute(pageBuffer[currentPageBuffer][j][k].VideoAttributes, j, k);
		}
		
	MoveCursor(currentPageBufferPosition[currentPageBuffer].v, currentPageBufferPosition[currentPageBuffer].h);
}

wchar_t startupMessage[3][COLUMNS];
std::deque<wchar_t> inputBuffer;

void initScreen()
{
	CommandCursorOn();
	MoveCursorToHome();
	EraseToEOF();
	
	for (int j = 1; j <= ROWS; ++j)
		for (int k = 1; k <= COLUMNS; ++k)
		{
			Serial.write(wchar_to_utf8(pageBuffer[currentPageBuffer][j][k].Character).utf8);
			CommandStartVideoAttribute(pageBuffer[currentPageBuffer][j][k].VideoAttributes, j, k);
		}
	
	MoveCursor(9, 1);
	currentPageBufferPosition[currentPageBuffer] = { 9, 1 };
}

void initPageBuffer()
{
	firstRowOfPageBuffer[0] = 1;
	firstRowOfPageBuffer[1] = 1;
	
	for (int i = 0; i < PAGES; ++i)
		for (int j = 1; j <= ROWS; ++j)
			for (int k = 1; k <= COLUMNS; ++k)
			{
				if (i == currentPageBuffer)
				{
					if (j == 1)
						pageBuffer[i][j][k] = { hasLowercase ? startupMessage[0][k - 1] : toupper(startupMessage[0][k - 1]), VA_NORMAL };	
					else if (j == 3)
						pageBuffer[i][j][k] = { hasLowercase ? startupMessage[1][k - 1] : toupper(startupMessage[1][k - 1]), VA_NORMAL };
					else if (j == 5)
						pageBuffer[i][j][k] = { hasLowercase ? startupMessage[2][k - 1] : toupper(startupMessage[2][k - 1]), VA_NORMAL };
					else if (j == 7)
						pageBuffer[i][j][k] = { hasLowercase ? terminal->StartupMessage()[k - 1] : toupper(terminal->StartupMessage()[k - 1]), VA_NORMAL };
					else
					{
						pageBuffer[i][j][k] = { ' ', VA_NORMAL };					
					}
				}
				else
				{
					pageBuffer[i][j][k] = { ' ', VA_NORMAL };		
				}
			}
	
	currentPageBufferPosition[0] = { 1, 1 };
	currentPageBufferPosition[1] = { 1, 1 };
}

void reinit()
{
	inputBuffer.clear();
	
	terminal->AssignCommands();
	
	int baudRate = GetBaudRate();
	Serial1.end();
	
	SetupClock(baudRate);
	Serial1.begin(baudRate);
	
	initPageBuffer();
	
	while (!Serial) ;
	
	initScreen();
}

static FlashConfig config;

static void SaveConfigToFlash()
{	
	uint32_t interrupts = save_and_disable_interrupts();
	//multicore_lockout_start_blocking();
	flash_range_erase((PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
	flash_range_program((PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), (uint8_t*)&config, FLASH_PAGE_SIZE);
	//multicore_lockout_end_blocking();
	restore_interrupts(interrupts);
}

static int LoadTerminalType()
{
	const uint8_t* flash_target_contents = (const uint8_t *)(XIP_BASE + (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE));
	memcpy(&config, flash_target_contents, sizeof(config));
	
	if (config.magic_number != 1337 || config.version != 0 || config.TerminalType < 0 || config.TerminalType > 1)
	{
		config.magic_number = 1337;
		config.version = 0;
		config.TerminalType = 0;
	}
	
	return config.TerminalType;
}

static void SetTerminalType(int terminalType, bool callingFromSetup)
{
	config.TerminalType = terminalType;
	
	if (!callingFromSetup)
		SaveConfigToFlash();
	
	delete terminal;
	terminal = nullptr;
		
	switch (config.TerminalType)
	{
	case 1:
		terminal = new Cromemco3102();
		break;
	default:
		terminal = new CT_1024();
		break;
	}
	
	if (!callingFromSetup)
		reinit();

}

static void ToggleTerminalType()
{
	config.TerminalType = (config.TerminalType + 1) % 2;
	SetTerminalType(config.TerminalType, false);
}

void ProcessSentByte(wchar_t c)
{
	if (isDebug)
	{
		Serial.print("S: ");
		Serial.println(c, HEX);
	}
	
	// Handle position only characters
	if (c == 0x06) // ^F
	{
		ToggleTerminalType();
		return;
	}
	if (c == 0x04) // ^D
	{
		isDebug = !isDebug;
		return;
	}
	if (g_keysToConsume > 0)
	{
		g_consumedKeys.push_back(c);
		g_keysToConsume--;
		if (g_keysToConsume == 0)
		{
			bool passCharToHost = g_keysConsumedCallback(false);
			if (passCharToHost)
			{
				for (auto& it : g_consumedKeys)
				{  
					if (isDebug)
					{
						Serial.print("Send: ");
						Serial.println(hasLowercase ? c : toupper(c), HEX);
					}
					Serial1.write(hasLowercase ? c : toupper(c));
				}
				g_consumedKeys.clear();
			}
		}
		return;
	}
	else if (terminal->IsCommand(c, false))
	{
		bool passCharToHost = terminal->ProcessCommand(c, false);
		
		if (passCharToHost)
		{
			if (isDebug)
			{
				Serial.print("Send: ");
				Serial.println(hasLowercase ? c : toupper(c), HEX);
			}
			Serial1.write(hasLowercase ? c : toupper(c));
		}
		
		return;
	}
	if (isDebug)
	{
		Serial.print("Send: ");
		Serial.println(hasLowercase ? c : toupper(c), HEX);
	}
	Serial1.write(hasLowercase ? c : toupper(c));

	if (offlineMode)
	{
		ProcessReceivedByte(c);	
	}
}

bool IsLocalEchoOn()
{
	return offlineMode;
}

void setup() 
{
	Serial.begin(115200);
	
	while (!Serial) ;
	
	SetTerminalType(LoadTerminalType(), true);
	
	terminal->AssignCommands();
	
	pinMode(BAUD110, INPUT_PULLUP);
	pinMode(BAUD150, INPUT_PULLUP);
	pinMode(BAUD300, INPUT_PULLUP);
	pinMode(BAUD600, INPUT_PULLUP);
	pinMode(BAUD1200, INPUT_PULLUP);
	pinMode(BAUD2400, INPUT_PULLUP);
	pinMode(BAUD4800, INPUT_PULLUP);
	pinMode(BAUD9600, INPUT_PULLUP);
	
	pinMode(SCROLL, INPUT_PULLUP);
	wrapVertical = digitalRead(SCROLL) == LOW;

	pinMode(LOCAL_ECHO, INPUT_PULLUP);
	offlineMode = digitalRead(LOCAL_ECHO) == HIGH;

	pinMode(LOWER_CASE, INPUT_PULLUP);
	hasLowercase = digitalRead(LOWER_CASE) == HIGH;

	pinMode(PAGE, INPUT_PULLUP);
	currentPageBuffer = digitalRead(PAGE) == HIGH ? 0 : 1;
	newPageBuffer = currentPageBuffer;
	
	pinMode(RESET, INPUT_PULLUP);
	resetPushed = false;
	
	terminal->TerminalSetup();
	                          // 12345678901234567890123456789012345678901234567890123456789012345678901234567890
	wcsncpy(startupMessage[0], L"                    ******  RetroSpy Technologies  ******                       ", 80);
	wcsncpy(startupMessage[1], L"                                   CT-2400                                      ", 80);
	wcsncpy(startupMessage[2], L"                               Terminal System                                  ", 80);
	
	initPageBuffer();
	
	int baudRate = GetBaudRate();
	SetupClock(baudRate);
	
	Serial1.setFIFOSize(BUFFER_SIZE);
	Serial1.begin(baudRate);
	
	setupComplete = true;
	
	initScreen();

}

//void setup1()
//{
//	while (!Serial || !setupComplete) ;
//	//multicore_lockout_victim_init();  // This locks up the core when executed.  BUG?
//}

//void loop1()
//{
//	while (!Serial || !setupComplete) ;
//
//	resetPushed = digitalRead(RESET) == LOW;
//	hasLowercase = digitalRead(LOWER_CASE) == HIGH;
//	wrapVertical = digitalRead(SCROLL) == LOW;
//	newPageBuffer = digitalRead(PAGE) == HIGH ? 0 : 1;
//	offlineMode = digitalRead(LOCAL_ECHO) == HIGH;
//	
//	terminal->TerminalLoop1();
//}

void HandleSend()
{
	if (Serial.available() >= 3)
	{
		if (Serial.peek() == 0x1B)
		{
			wchar_t c = Serial.read();
			if (Serial.peek() == '[')
			{
				Serial.read();
				c = Serial.read();
				switch (c)  // VT-100 Command to Follow
				{
				case 'A':
				case 'B':
				case 'C':
				case 'D':
					if (cursorOn)
						ProcessSentByte(c - 'A' + ARROW_KEY_OFFSET);
					break;
				default:
					ProcessSentByte(0x1B);
					ProcessSentByte('[');
					ProcessSentByte(c);
					break;
				}
			}
			else
			{
				ProcessSentByte(c);
			}
		}
		else
		{
			ProcessSentByte(Serial.read());
		}
	}
	else
	{
		ProcessSentByte(Serial.read());
	}
}

void loop() 
{
	int pins = gpio_get_all();
	resetPushed = (pins & (1 << RESET)) == 0;
	hasLowercase = (pins & (1 << LOWER_CASE)) != 0;
	wrapVertical = (pins & (1 << SCROLL)) == 0;
	newPageBuffer = (pins & (1 << PAGE)) != 0 ? 0 : 1;
	offlineMode = (pins & (1 << LOCAL_ECHO)) != 0;
	
	terminal->TerminalLoop1(pins);
	
	if (resetPushed)
	{
		reinit();
	}
	
	if (newPageBuffer != currentPageBuffer)
	{
		SwapPages();
	}

//	if (Serial1.available())
//	{
//		wchar_t temp_buffer[BUFFER_SIZE];
//		int count = min(BUFFER_SIZE, Serial1.available());
//		
//		for(int i = 0; i < count; ++i)
//			temp_buffer[i] = Serial1.read();
//		inputBuffer.insert(inputBuffer.cend(), temp_buffer, temp_buffer + count);
//	}
	
	if (Serial.available()) 
	{   	
		HandleSend();
	}
	
	if (Serial1.available())
	{
		ProcessReceivedByte(Serial1.read());
	}
	
//	if (!inputBuffer.empty())
//	{
//		int count = inputBuffer.size();
//		for (int i = 0; i < count; ++i)	
//		{	
//			c = inputBuffer.front();
//			inputBuffer.pop_front();
//			ProcessReceivedByte(c);
//		}
//	}
}
