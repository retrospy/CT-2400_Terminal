
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

#define AC30_CMD	11

#define PAGE        27

#define REC_OFF     16
#define REC_ON      17
#define RDR_OFF     18
#define RDR_ON      19

#define CLK_IN      20
#define CLK_OUT     21

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

int page = 0;

char pages[PAGES][ROWS+1][COLUMNS+1];
int posCol[PAGES];
int posRow[PAGES];
int firstRow[PAGES];

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
};

void SetupClock(int baudRate)
{ 
	
	analogWriteFreq(baudRate*16);
	analogWrite(CLK_OUT, 128);
	
	clock_configure_gpin(clk_peri, CLK_IN, baudRate*16, baudRate*16);
}

void setup() {

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
	
	pinMode(AC30_CMD, INPUT_PULLUP);
	
	pinMode(PAGE, INPUT_PULLUP);
	page = digitalRead(PAGE) == HIGH ? 0 : 1;

	pinMode(REC_OFF, OUTPUT);
	digitalWrite(REC_OFF, HIGH);
	
	pinMode(REC_ON, OUTPUT);
	digitalWrite(REC_ON, HIGH);
	
	pinMode(RDR_OFF, OUTPUT);
	digitalWrite(RDR_OFF, HIGH);
	
	pinMode(RDR_ON, OUTPUT);
	digitalWrite(RDR_ON, HIGH);
	
	int baudRate = GetBaudRate();
	
	SetupClock(baudRate);
	
	firstRow[0] = 1;
	firstRow[1] = 1;
	
	for (int i = 0; i < PAGES; ++i)
		for (int j = 1; j <= ROWS; ++j)
			for (int k = 1; k <= COLUMNS; ++k)
				pages[i][j][k] = ' ';
	
	Serial1.begin(baudRate);
	Serial.begin(115200);
	
    while(!Serial);
	
	posRow[0] = 1;
	posCol[0] = 1;
	posRow[1] = 1;
	posCol[1] = 1;
	
	Serial.printf("%c[H", 27);
	Serial.printf("%c[J", 27);
	
}

int GenerateRealRowPosition(int terminalRow)
{	
	return (((firstRow[page] - 1) + (terminalRow - 1)) % ROWS) + 1;
}

bool moveCursorRight(bool virtualMove = false, char c = '\0')
{
	Serial.printf("%c[6n", 27);
	String response = Serial.readStringUntil('R');
	// ^[<v>;<h>R
	int  v = response.substring(response.indexOf('[')+1,response.indexOf(';')).toInt();
	int h = response.substring(response.indexOf(';')+1,response.indexOf('R')).toInt();
  
	if (wrapVertical && wrapHorizontal && h == COLUMNS && v == ROWS)
	{
		if (c != '\0')
		{
			Serial.write(hasLowercase ? c : toupper(c));
		}
		Serial.printf("%c[H", 27);
		if (c != '\0')
			Serial.printf("%c[J", 27);
		posCol[page] = 1;
		posRow[page] = 1;
		return true;
	}
	if (!wrapVertical && wrapHorizontal && h == COLUMNS && v == ROWS)
	{
		if (!virtualMove)
			Serial.println();
		posCol[page] = 1;
		
		for (int i = 0; i <= COLUMNS; ++i)
			pages[page][firstRow[page]][i] = ' ';
		
		firstRow[page] = (((firstRow[page] - 1) + 1) % ROWS) + 1;
	}
	else if (wrapHorizontal && h == COLUMNS && v != ROWS)
	{
		if (!virtualMove)
			Serial.printf("%c[%d;1H", 27, v+1);
		posCol[page] = 1;
		posRow[page]++;
	}
	else
	{     
		// Cursor right
		if (!virtualMove)
			Serial.printf("%c[C", 27);
		posCol[page]++;
	}  
	
	return false;
}

void moveCursorDown()
{
	Serial.printf("%c[6n", 27);
	String response = Serial.readStringUntil('R');
	// ^[<v>;<h>R
	int  v = response.substring(response.indexOf('[')+1,response.indexOf(';')).toInt();
	int h = response.substring(response.indexOf(';')+1,response.indexOf('R')).toInt();
  
	if (wrapVertical && v == ROWS)
	{
		Serial.printf("%c[1;%dH", 27, h);
		posCol[page] = 1;
		posRow[page] = 1;
	}
	else
	{
		// Cursor down
		Serial.printf("%c[B", 27);
		posRow[page]++;
	}
}

void moveCursorLeft()
{
  Serial.printf("%c[6n", 27);
  String response = Serial.readStringUntil('R');
  // ^[<v>;<h>R
  int  v = response.substring(response.indexOf('[')+1,response.indexOf(';')).toInt();
  int h = response.substring(response.indexOf(';')+1,response.indexOf('R')).toInt();
  
  if (wrapHorizontal && COLUMNS == 1)
  {
	  Serial.printf("%c[%d;%dH", 27, v, COLUMNS);
	  posCol[page] = COLUMNS;
	  
  }
  else
  {
    // Cursor left
    Serial.printf("%c[D", 27);
	  posCol[page]--;
  }
}

void moveCursorUp()
{
  Serial.printf("%c[6n", 27);
  String response = Serial.readStringUntil('R');
  // ^[<v>;<h>R
  int  v = response.substring(response.indexOf('[')+1,response.indexOf(';')).toInt();
  int h = response.substring(response.indexOf(';')+1,response.indexOf('R')).toInt();
  
  if (wrapVertical && v == 1)
  {
    Serial.printf("%c[%d;%dH", 27, ROWS, h);
	  posRow[page] = ROWS;
  }
  else
  {
    // Cursor up
    Serial.printf("%c[A", 27);
	  posRow[page]--;
  }
}

void sendPulse(int pin)
{
    //Serial.println(clock_get_hz(clk_peri));
    digitalWrite(pin, LOW);
    delayMicroseconds(1);
    digitalWrite(pin, HIGH);
}

bool ProcessCharAC30(char c, bool receive = false)
{
	if (c == 0x10)  // ^P
	{
		// Home
		Serial.printf("%c[H", 27);
		posCol[page] = 1;
		posRow[page] = 1;
	}
	else if (c == 0x11)  // ^Q
	{
		// Reader On
		sendPulse(RDR_ON);
	}
	else if (c == 0x12)  // ^R
	{
		// Record On
		sendPulse(REC_ON);
	}
	else if (c == 0x13)  // ^S
	{
		// Reader Off
		sendPulse(RDR_OFF);
	}
	else if (c == 0x14) // ^T
	{
		// Record Off
		sendPulse(REC_OFF);
	}
	else if (c == 0x15)  // ^U
	{
		// Erase to EOL
		Serial.printf("%c[K", 27);
	}
	else if (c == 0x16) // ^V
	{
		// Erase to EOF
		Serial.printf("%c[J", 27);
	}
	else if (c == 0x17) // ^W
	{
		// Cursor right
		moveCursorRight();
	}
	else if (receive && c == 0x0A) // line feed
	{
		if (posRow[page] != ROWS)
		{
			posRow[page] += 1;
		}
		else if (wrapVertical && posRow[page] == ROWS )
		{
			Serial.printf("%c[6n", 27);
			String response = Serial.readStringUntil('R');
			// ^[<v>;<h>R
			int  v = response.substring(response.indexOf('[') + 1, response.indexOf(';')).toInt();
			int h = response.substring(response.indexOf(';') + 1, response.indexOf('R')).toInt();

			Serial.printf("%c[%d;%dH", 27, 1, h);
			posRow[page] = 1;
			
		}
		return false;
	}
	else if (receive && c == 0x0D) // carriage return
	{
		posCol[page] = 1;
		return false;
	}
	else if (receive && c >= 0x21 && c <= 0x7e)
	{
		pages[page][GenerateRealRowPosition(posRow[page])][posCol[page]] = hasLowercase ? c : toupper(c);
		return moveCursorRight(true, c);
	}
	else // Do you intercept, or pass through
	{
		return false;
	}

	return true;
}

bool ProcessCharDefault(char c, bool receive = false)
{
	if (c == 0x10)  // ^P
	{
		moveCursorRight();
	}
	else if (c == 0x11)  // ^Q
	{
		moveCursorDown();
	}
	else if (c == 0x12)  // ^R
	{
		moveCursorLeft();
	}
	else if (c == 0x13)  // ^S
	{
		// Home
		Serial.printf("%c[H", 27);
		posCol[page] = 1;
		posRow[page] = 1;
	}
	else if (c == 0x14) // ^T
	{
		// Erase to EOL
		Serial.printf("%c[K", 27);
	}
	else if (c == 0x15)  // ^U
	{
		moveCursorUp();
	}
	else if (c == 0x16) // ^V
	{
		// Erase to EOF
		Serial.printf("%c[J", 27);
	}
	//    else if (c == 0x17) // ^W
	//    {
	//      // Cursor right
	//      Serial.printf("%c[C", 27);
	//    }
	else if (receive && c == 0x0A) // line feed
	{
		if (posRow[page] != ROWS)
		{
			posRow[page] += 1;
		}
		else if (wrapVertical && posRow[page] == ROWS )
		{
			Serial.printf("%c[6n", 27);
			String response = Serial.readStringUntil('R');
			// ^[<v>;<h>R
			int  v = response.substring(response.indexOf('[') + 1, response.indexOf(';')).toInt();
			int h = response.substring(response.indexOf(';') + 1, response.indexOf('R')).toInt();

			Serial.printf("%c[%d;%dH", 27, 1, h);
			posRow[page] = 1;
			
		}
		return false;
	}
	else if (receive && c == 0x0D) // carriage return
	{
		posCol[page] = 1;
		return false;
	}
	else if (receive && c >= 0x21 && c <= 0x7e)
	{
		pages[page][GenerateRealRowPosition(posRow[page])][posCol[page]] = hasLowercase ? c : toupper(c);
		return moveCursorRight(true, c);
	}
    else // Do you intercept, or pass through
    {
      return false;
    }

    return true;
}

bool eatLineFeed = false;

void loop() {

	hasLowercase = digitalRead(LOWER_CASE) == HIGH;
	useAC30Commands = digitalRead(AC30_CMD) == LOW;
	wrapVertical = digitalRead(SCROLL) == LOW;
	
	int currentPage = digitalRead(PAGE) == HIGH ? 0 : 1;
	if (currentPage != page)
	{
		page = (page + 1) % PAGES;
		if (wrapVertical)
		{
			Serial.printf("%c[H", 27);
			Serial.printf("%c[J", 27);
		}
		else
		{
			Serial.printf("%c[%d;1H", 27, ROWS);
			Serial.println();
		}
		
		for (int j = firstRow[page]; j <= ROWS; ++j)
			for (int k = 1; k <= COLUMNS; ++k)
			{
				Serial.print(pages[page][j][k]);
			}
		
		for (int j = 1; j <= firstRow[page] - 1; ++j)
			for (int k = 1; k <= COLUMNS; ++k)
			{
				Serial.print(pages[page][j][k]);
			}
		
		Serial.printf("%c[%d;%dH", 27, posRow[page], posCol[page]);
	}
	
	char c;
	if (Serial.available()) 
	{      
		c = Serial.read();

		bool retVal;
		if (!useAC30Commands)
			retVal = ProcessCharDefault(c);
		else
			retVal = ProcessCharAC30(c);

		if (!retVal)
		{
			Serial1.write(hasLowercase ? c : toupper(c));
		}	

		if (digitalRead(LOCAL_ECHO) == LOW)
		{
			if (c == 0x0D)
			{
				Serial.write(0x0D);
				if (posRow[page] != ROWS)
				{
					Serial.write(0x0A);
					posRow[page] += 1;
				}
				else if (wrapVertical && posRow[page] == ROWS)
				{
					Serial.printf("%c[6n", 27);
					String response = Serial.readStringUntil('R');
					// ^[<v>;<h>R
					int  v = response.substring(response.indexOf('[') + 1, response.indexOf(';')).toInt();
					int h = response.substring(response.indexOf(';') + 1, response.indexOf('R')).toInt();

					Serial.printf("%c[%d;%dH", 27, 1, h);
					posRow[page] = 1;
					Serial.printf("%c[J", 27);
				}
				else if (!wrapVertical && posRow[page] == ROWS)
				{
					Serial.write(0x0A);
					posCol[page] = 1;
					
					for (int i = 0; i <= COLUMNS; ++i)
						pages[page][firstRow[page]][i] = ' ';
					
					firstRow[page] = (((firstRow[page] - 1) + 1) % ROWS) + 1;
				}
			
				eatLineFeed = true;
				return;
			}
			else if (eatLineFeed && c == 0x0A)
			{
				eatLineFeed = false;
				return;
			}
			else if (eatLineFeed)
			{
				eatLineFeed = false;
			}
		
			bool retVal;

			if (!useAC30Commands)
				retVal = ProcessCharDefault(c, true); 
			else
				retVal = ProcessCharAC30(c);

			if (!retVal)
				Serial.write(hasLowercase ? c : toupper(c));		
		}
	}
	
	if (Serial1.available())
	{
		c = Serial1.read();
	  
		if (c == 0x0D)
		{
			Serial.write(0x0D);
			if (posRow[page] != ROWS)
			{
				Serial.write(0x0A);
				posCol[page] = 1;
				posRow[page] += 1;
			}
			else if (wrapVertical && posRow[page] == ROWS)
			{
				Serial.printf("%c[6n", 27);
				String response = Serial.readStringUntil('R');
				// ^[<v>;<h>R
				int  v = response.substring(response.indexOf('[') + 1, response.indexOf(';')).toInt();
				int h = response.substring(response.indexOf(';') + 1, response.indexOf('R')).toInt();

				Serial.printf("%c[%d;%dH", 27, 1, h);
				posRow[page] = 1;
				Serial.printf("%c[J", 27);
			}
			else if (!wrapVertical && posRow[page] == ROWS)
			{
				Serial.write(0x0A);
				posCol[page] = 1;
				
				for (int i = 0; i <= COLUMNS; ++i)
					pages[page][firstRow[page]][i] = ' ';
				
				firstRow[page] = (((firstRow[page] - 1) + 1) % ROWS) + 1;
			}
			
			eatLineFeed = true;
			return;
		}
		else if (eatLineFeed && c == 0x0A)
		{
			eatLineFeed = false;
			return;
		}
		else if (eatLineFeed)
		{
			eatLineFeed = false;
		}
		
		bool retVal;

		if (!useAC30Commands)
			retVal = ProcessCharDefault(c, true); 
		else
			retVal = ProcessCharAC30(c);

		if (!retVal)
			Serial.write(hasLowercase ? c : toupper(c));
	}
}
