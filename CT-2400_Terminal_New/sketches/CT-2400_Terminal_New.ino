
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

#define AC30_CMD	10

#define PAGE        27

#define REC_OFF     16
#define REC_ON      17
#define RDR_OFF     18
#define RDR_ON      19

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

bool wrapVertical = false;
bool wrapHorizontal = true;
bool useAC30Commands = false;
bool hasLowercase = true;

int baudRate = 300;

struct Position
{
	int v;
	int h;
};

// Buffer goes from 1 to ROWS/COLUMNS, so column and row 0 are ignored 
char pageBuffer[PAGES][ROWS + 1][COLUMNS + 1];  
int currentPageBuffer = 0;
Position currentPageBufferPosition[PAGES];
int firstRowOfPageBuffer[PAGES];

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
	
	analogWriteFreq(baudRate * 16);
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
	Serial.printf("%c[%d;%dH", 27, v, h);
}

void MoveCursorRight()
{
	Serial.printf("%c[C", 27);
}

void MoveCursorLeft()
{
	Serial.printf("%c[D", 27);
}

void MoveCursorUp()
{
	Serial.printf("%c[A", 27);
}

void MoveCursorDown()
{
	Serial.printf("%c[B", 27);
}

void MoveCursorToHome()
{
	Serial.printf("%c[H", 27);
}

void EraseToEOL()
{
	Serial.printf("%c[K", 27);
}

void EraseToEOF()
{
	Serial.printf("%c[J", 27);
}

int GenerateRealRowPosition(int terminalRow)
{	
	return (((firstRowOfPageBuffer[currentPageBuffer] - 1) + (terminalRow - 1)) % ROWS) + 1;
}

void WriteCharactorToCurrentPageBuffer(char c)
{
	Serial.print("V=");
	Serial.println(GenerateRealRowPosition(currentPageBufferPosition[currentPageBuffer].v));
	Serial.print("H=");
	Serial.println(currentPageBufferPosition[currentPageBuffer].h);
		
	pageBuffer[currentPageBuffer][GenerateRealRowPosition(currentPageBufferPosition[currentPageBuffer].v)]
		                         [currentPageBufferPosition[currentPageBuffer].h] = hasLowercase ? c : toupper(c);
}

void sendPulse(int pin)
{
	//Serial.println(clock_get_hz(clk_peri));
	digitalWrite(pin, LOW);
	delayMicroseconds(1);
	digitalWrite(pin, HIGH);
}


bool CommandCursorRight(bool virtualMove = false, char c = '\0')
{
	int v, h;
	GetCurrentScreenPosition(v, h);
  
	if (wrapVertical && wrapHorizontal && h == COLUMNS && v == ROWS)
	{
		if (c != '\0')
		{
			Serial.write(hasLowercase ? c : toupper(c));
		}
		MoveCursorToHome();
		currentPageBufferPosition[currentPageBuffer].h = 1;
		currentPageBufferPosition[currentPageBuffer].v = 1;
		return true;
	}
	
	if (!wrapVertical && wrapHorizontal && h == COLUMNS && v == ROWS)
	{
		if (!virtualMove)
			Serial.println();
		currentPageBufferPosition[currentPageBuffer].h = 1;
		
		//for (int i = 0; i <= COLUMNS; ++i)
		//	pageBuffer[currentPageBuffer][firstRowOfPageBuffer[currentPageBuffer]][i] = ' ';
		
		firstRowOfPageBuffer[currentPageBuffer] = (((firstRowOfPageBuffer[currentPageBuffer] - 1) + 1) % ROWS) + 1;
	}
	else if (wrapHorizontal && h == COLUMNS && v != ROWS)
	{
		if (!virtualMove)
			MoveCursor(v + 1, 1);
		currentPageBufferPosition[currentPageBuffer].h = 1;
		currentPageBufferPosition[currentPageBuffer].v++;
	}
	else
	{     
		// Cursor right
		if (!virtualMove)
			MoveCursorRight();
		currentPageBufferPosition[currentPageBuffer].h++;
	}  
	
	return false;
}

bool CommandCursorRight_new(bool virtualMove = false, char c = '\0')
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

bool CommandCursorDown(bool virtualMove = false, char c = '\0')
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

bool CommandCursorLeft(bool virtualMove = false, char c = '\0')
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

bool CommandCursorUp(bool virtualMove = false, char c = '\0')
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

bool CommandHome(bool virtualMove = false, char c = '\0')
{
	Serial.printf("%c[H", 27);
	currentPageBufferPosition[currentPageBuffer] = { 1, 1 };
	
	return true;
}

bool CommandEraseToEOL(bool virtualMove = false, char c = '\0')
{
	Serial.printf("%c[K", 27);
	
	return true;
}

bool CommandEraseToEOF(bool virtualMove = false, char c = '\0')
{
	Serial.printf("%c[J", 27);
	
	return true;
}

bool CommandReaderOn(bool virtualMove = false, char c = '\0')
{
	sendPulse(RDR_ON);
	
	return true;
}

bool CommandRecordOn(bool virtualMove = false, char c = '\0')
{
	sendPulse(REC_ON);
	
	return true;
}

bool CommandReaderOff(bool virtualMove = false, char c = '\0')
{
	sendPulse(RDR_OFF);
	
	return true;
}

bool CommandRecordOff(bool virtualMove = false, char c = '\0')
{
	sendPulse(REC_OFF);
	
	return true;
}

static bool(*CommandAssigment[2][8])(bool, char);

void AssignCommands()
{
	// Default Command Set
	CommandAssigment[0][0] = CommandCursorRight_new; // ^P
	CommandAssigment[0][1] = CommandCursorDown; // ^Q
	CommandAssigment[0][2] = CommandCursorLeft; // ^R
	CommandAssigment[0][3] = CommandHome; // ^S
	CommandAssigment[0][4] = CommandEraseToEOL; // ^T
	CommandAssigment[0][5] = CommandCursorUp; // ^U
	CommandAssigment[0][6] = CommandEraseToEOF; // ^V
	CommandAssigment[0][7] = nullptr; // ^W

	// AC-30 Command Set
	CommandAssigment[1][0] = CommandHome; // ^P
	CommandAssigment[1][1] = CommandReaderOn; // ^Q
	CommandAssigment[1][2] = CommandRecordOn; // ^R
	CommandAssigment[1][3] = CommandReaderOff; // ^S
	CommandAssigment[1][4] = CommandRecordOff; // ^T
	CommandAssigment[1][5] = CommandEraseToEOL; // ^U
	CommandAssigment[1][6] = CommandEraseToEOF; // ^V
	CommandAssigment[1][7] = CommandCursorRight_new; // ^W	
}

void ProcessCommand_New(char c)
{
	if (CommandAssigment[useAC30Commands == false ? 0 : 1][c - 0x10] != nullptr)
		CommandAssigment[useAC30Commands == false ? 0 : 1][c - 0x10](false, c);
}

bool ProcessCommand(char c, bool receive = false)
{
	// Handle Commands
	if (c >= 0x10 && c <= 0x17)
	{
		if (CommandAssigment[useAC30Commands == false ? 0 : 1][c - 0x10] != nullptr)
			return CommandAssigment[useAC30Commands == false ? 0 : 1][c - 0x10](false, c);
	}
	
	if (receive)
	{
		if (c >= 0x21 && c <= 0x7e)
		{
			WriteCharactorToCurrentPageBuffer(c);
			return CommandCursorRight(true, c);
		}
	}
    
	return false;
}

static bool eatNextLifeFeed = false;
 
void ProcessReceivedByte(char c)
{
	//Serial.print("Receiving: ");
	//Serial.println(c, HEX);
	
	if (c == CR)
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
		return;
	}
		
	if (c >= 0x10 && c <= 0x17)
		ProcessCommand_New(c);
	else if (c >= 0x21 && c <= 0x7e)
	{
		WriteCharactorToCurrentPageBuffer(c);

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
			Serial.print(pageBuffer[currentPageBuffer][j][k]);
		}
		
	for (int j = 1; j <= firstRowOfPageBuffer[currentPageBuffer] - 1; ++j)
		for (int k = 1; k <= COLUMNS; ++k)
		{
			Serial.print(pageBuffer[currentPageBuffer][j][k]);
		}
		
	MoveCursor(currentPageBufferPosition[currentPageBuffer].v, currentPageBufferPosition[currentPageBuffer].h);
}

void ProcessSentByte(char c)
{
	bool eatCharacter = ProcessCommand(c, false);

	if (!eatCharacter)
	{
		//Serial.print("Sending: ");
		//Serial.println(c, HEX);
		Serial1.write(hasLowercase ? c : toupper(c));
	}	

	if (digitalRead(LOCAL_ECHO) == HIGH)
	{
		ProcessReceivedByte(c);	
	}
}

char startupMessage[3][COLUMNS];

void setup() {

	AssignCommands();
	
	pinMode(BAUD110, INPUT_PULLUP);
	pinMode(BAUD150, INPUT_PULLUP);
	pinMode(BAUD300, INPUT_PULLUP);
	pinMode(BAUD600, INPUT_PULLUP);
	pinMode(BAUD1200, INPUT_PULLUP);
	pinMode(BAUD2400, INPUT_PULLUP);
	pinMode(BAUD4800, INPUT_PULLUP);
	pinMode(BAUD9600, INPUT_PULLUP);
	
	pinMode(SCROLL, INPUT_PULLUP);
	
	pinMode(LOCAL_ECHO, INPUT_PULLUP);
	
	pinMode(LOWER_CASE, INPUT_PULLUP);
	hasLowercase = digitalRead(LOWER_CASE) == HIGH;
	
	pinMode(AC30_CMD, INPUT_PULLUP);
	
	pinMode(PAGE, INPUT_PULLUP);
	currentPageBuffer = digitalRead(PAGE) == HIGH ? 0 : 1;

	pinMode(REC_OFF, OUTPUT);
	digitalWrite(REC_OFF, HIGH);
	
	pinMode(REC_ON, OUTPUT);
	digitalWrite(REC_ON, HIGH);
	
	pinMode(RDR_OFF, OUTPUT);
	digitalWrite(RDR_OFF, HIGH);
	
	pinMode(RDR_ON, OUTPUT);
	digitalWrite(RDR_ON, HIGH);
	
	pinMode(RESET, INPUT_PULLUP);
	
	int baudRate = GetBaudRate();
	
	SetupClock(baudRate);
	
	firstRowOfPageBuffer[0] = 1;
	firstRowOfPageBuffer[1] = 1;
	
	                         // 12345678901234567890123456789012345678901234567890123456789012345678901234567890
	strncpy(startupMessage[0], "                    ******  RetroSpy Technologies  ******                       ", 80);
	strncpy(startupMessage[1], "                                   CT-2400                                      ", 80);
	strncpy(startupMessage[2], "                               Terminal System                                  ", 80);
	
	for (int i = 0; i < PAGES; ++i)
		for (int j = 1; j <= ROWS; ++j)
			for (int k = 1; k <= COLUMNS; ++k)
			{
				if (i == currentPageBuffer)
				{
					if (j == 1)
						pageBuffer[i][j][k] = hasLowercase ? startupMessage[0][k] : toupper(startupMessage[0][k]);		
					else if (j == 3)
						pageBuffer[i][j][k] = hasLowercase ? startupMessage[1][k] : toupper(startupMessage[1][k]);	
					else if (j == 5)
						pageBuffer[i][j][k] = hasLowercase ? startupMessage[2][k] : toupper(startupMessage[2][k]);
					else
					{
						pageBuffer[i][j][k] = ' ';					
					}
				}
				else
				{
					pageBuffer[i][j][k] = ' ';	
				}
			}
	
	currentPageBufferPosition[0] = { 1, 1 };
	currentPageBufferPosition[1] = { 1, 1 };
	
	Serial1.begin(baudRate);
	Serial.begin(115200);
	
	while (!Serial) ;
	
	MoveCursorToHome();
	EraseToEOF();
	
	for (int j = 1; j <= ROWS; ++j)
		for (int k = 1; k <= COLUMNS; ++k)
		{
			Serial.print(pageBuffer[currentPageBuffer][j][k]);
		}
	
	MoveCursor(7, 1);
	currentPageBufferPosition[currentPageBuffer] = { 7, 1 };
}

void loop() 
{
	if (digitalRead(RESET) == LOW)
	{
		MoveCursorToHome();
		EraseToEOF();
		*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)) = 0x5FA0004;
	}
	
	hasLowercase = digitalRead(LOWER_CASE) == HIGH;
	useAC30Commands = digitalRead(AC30_CMD) == HIGH;
	wrapVertical = digitalRead(SCROLL) == LOW;
	
	int currentPage = digitalRead(PAGE) == HIGH ? 0 : 1;
	if (currentPage != currentPageBuffer)
	{
		SwapPages();
	}
	
	char c;
	if (Serial.available()) 
	{      
		c = Serial.read();
		ProcessSentByte(c);
	}
	
	if (Serial1.available())
	{
		c = Serial1.read();
		ProcessReceivedByte(c);
	}
}
