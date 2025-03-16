#pragma once

#include "Arduino.h"

//#define CROMEMCO3102
//#define CT1024

#define LF			0x0A
#define CR			0x0D

#define BAUD110     2
#define BAUD150     3
#define BAUD300     4
#define BAUD600     5
#define BAUD1200    6
#define BAUD2400	11
#define BAUD4800	12
#define BAUD9600	13

#define SCROLL		7

#define LOCAL_ECHO  8

#define LOWER_CASE  9

#define PAGE        27

#define CLK_IN      20
#define CLK_OUT     21

#define RESET		28

// TeraTerm Default 80x24
// CT-1024 32x16
// CT-64 64x16
// CT-82 82x16
#define COLUMNS 80
#define ROWS	24
#define PAGES	 2

#define ARROW_KEY_OFFSET	0xFA
#define ARROW_UP			ARROW_KEY_OFFSET
#define ARROW_DOWN			ARROW_KEY_OFFSET + 1
#define ARROW_RIGHT			ARROW_KEY_OFFSET + 2
#define ARROW_LEFT			ARROW_KEY_OFFSET + 3

//#define DEBUG

extern bool CommandCursorUp();
extern bool CommandCursorDown();
extern bool CommandCursorRight();
extern bool CommandCursorLeft();
extern bool CommandEraseAll();
extern bool CommandHome();
extern bool CommandEraseToEOF();
extern bool CommandEraseToEOL();
extern bool CommandInsertLine();
extern bool CommandDeleteLine();
extern bool CommandCursorToggle();
extern bool CommandStartBlink();
extern bool CommandNormalVideo();
extern bool CommandVT100EscapeCode();
extern void MoveCursor(int v, int h);
extern bool IsLocalEchoOn();
extern void TerminalSetup();
extern void GetCurrentScreenPosition(int& v, int& h);

extern int g_keysToConsume;
extern std::vector<char> g_consumedKeys;
extern bool(*g_keysConsumedCallback)(bool receive);
