
const byte SENSOR_IN = 2;
const byte LED_OUT = 12;
const byte SM_OUT = 3;

void setup()
{
  Serial.begin(9600);
  pinMode(SENSOR_IN, INPUT);
  pinMode(LED_OUT, OUTPUT);
  pinMode(SM_OUT, OUTPUT);
}

void ledAction() 
{
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

void motionDetected()
{
  Serial.println("SENSOR: detected motion!"); 
  ledAction();
}

void loop()
{
  digitalWrite(SM_OUT, LOW);
  int read = digitalRead(SENSOR_IN);
  if(read == HIGH)
  {
    motionDetected();
  }
}

  