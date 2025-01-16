void setup()
{
	Serial1.begin(300);
	Serial.begin(115200);
}

void loop()
{
	while (true)
	{
		if (Serial1.available())
		{
			Serial1.write(Serial1.read());
		}
	}
}
