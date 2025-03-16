
#include "CT-1024.h"

#define AC30_CMD	10

#define REC_OFF     16
#define REC_ON      17
#define RDR_OFF     18
#define RDR_ON      19

									  // 12345678901234567890123456789012345678901234567890123456789012345678901234567890
static char startupMessage[COLUMNS+1] = "                    ******   SWTPC CT-1024 Mode   ******                        ";

static volatile bool useAC30Commands = false;

static void sendPulse(int pin)
{
	digitalWrite(pin, LOW);
	delayMicroseconds(1);
	digitalWrite(pin, HIGH);
}

static bool CommandReaderOn()
{
	sendPulse(RDR_ON);
	
	return true;
}

static bool CommandRecordOn()
{
	sendPulse(REC_ON);
	
	return true;
}

static bool CommandReaderOff()
{
	sendPulse(RDR_OFF);
	
	return true;
}

static bool CommandRecordOff()
{
	sendPulse(REC_OFF);
	
	return true;
}

void CT_1024::AssignCommands()
{
	for (int i = 0; i < NUM_1024_COMMAND_SETS; ++i)
		for (int j = 0; j < NUM_1024_COMMANDS; ++j)
			CommandAssignment[i][j] = nullptr;
	
	CommandAssignment[0][0] = CommandCursorRight; // ^P
	CommandAssignment[0][1] = CommandCursorDown; // ^Q
	CommandAssignment[0][2] = CommandCursorLeft; // ^R
	CommandAssignment[0][3] = CommandHome; // ^S
	CommandAssignment[0][4] = CommandEraseToEOL; // ^T
	CommandAssignment[0][5] = CommandCursorUp; // ^U
	CommandAssignment[0][6] = CommandEraseToEOF; // ^V
	CommandAssignment[0][7] = nullptr; // ^W

	// AC-30 Command Set
	CommandAssignment[1][0] = CommandHome; // ^P
	CommandAssignment[1][1] = CommandReaderOn; // ^Q
	CommandAssignment[1][2] = CommandRecordOn; // ^R
	CommandAssignment[1][3] = CommandReaderOff; // ^S
	CommandAssignment[1][4] = CommandRecordOff; // ^T
	CommandAssignment[1][5] = CommandEraseToEOL; // ^U
	CommandAssignment[1][6] = CommandEraseToEOF; // ^V
	CommandAssignment[1][7] = CommandCursorRight; // ^W	
	
}

bool CT_1024::ProcessCommand(char& c, bool isReceive)
{
	if (isReceive)
	{
		if (CommandAssignment[1][c - 0x10] != nullptr)
			CommandAssignment[1][c - 0x10]();
	}
	else
	{
		if (CommandAssignment[useAC30Commands == false ? 0 : 1][c - 0x10] != nullptr)
			CommandAssignment[useAC30Commands == false ? 0 : 1][c - 0x10]();		
	}
	
	return false;
}

bool CT_1024::IsCommand(char c, bool isReceive)
{
	return c >= 0x10 && c <= 0x17;
}

void CT_1024::TerminalSetup()
{
	pinMode(AC30_CMD, INPUT_PULLUP);
	useAC30Commands = digitalRead(AC30_CMD) == HIGH;
	
	pinMode(REC_OFF, OUTPUT);
	digitalWrite(REC_OFF, HIGH);
	
	pinMode(REC_ON, OUTPUT);
	digitalWrite(REC_ON, HIGH);
	
	pinMode(RDR_OFF, OUTPUT);
	digitalWrite(RDR_OFF, HIGH);
	
	pinMode(RDR_ON, OUTPUT);
	digitalWrite(RDR_ON, HIGH);
}

void CT_1024::TerminalLoop1(int pins)
{
	useAC30Commands = (pins & (1 << AC30_CMD)) != 0;
}

char* CT_1024::StartupMessage()
{
	return startupMessage;
}

bool CT_1024::ShouldTransmit(char c)
{
	return true;
}