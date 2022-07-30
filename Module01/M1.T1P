const byte SENSOR_IN = 2;
const byte LED_OUT = 3;

void setup()
{
  // Initialise Serial Output
  Serial.begin(9600);
  // Initialise Pin Functions
  pinMode(SENSOR_IN, INPUT);
  pinMode(LED_OUT, OUTPUT);
}

// Procedure for controlling the LED
void ledAction() 
{
  // Just for fun to make the LED blink.
  for (int i=0; i<4; i++) 
  {
    digitalWrite(LED_OUT,HIGH);
    Serial.println(" ACTION: Led FLASH.");
  	delay(200);
  	digitalWrite(LED_OUT,LOW);
  	delay(200);
  }
  digitalWrite(LED_OUT, LOW);
}

// Procedure for motion Detector.
void motionDetected()
{
  Serial.println("SENSOR: detected motion!"); 
  ledAction();
}

// Using Polling to update.
void loop()
{
  // Reads the state of the sensor.
  int read = digitalRead(SENSOR_IN);
  if(read == HIGH)
  {
    motionDetected();
  }
}
