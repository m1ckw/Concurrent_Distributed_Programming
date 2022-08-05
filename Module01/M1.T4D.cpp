
const uint8_t sensor1 = 2;
const uint8_t sensor2 = 3;
const uint8_t sensor3 = 4;

const uint8_t LED_green = 8;
const uint8_t LED_yellow = 9;
const uint8_t LED_blue = 11;
const uint8_t LED_red = 10;

int LEDfreq; // Using int to round the double down.

void setup()
{
  Serial.begin(9600);
  
  // Enableing the Pin Change Interupt Control Register.
  PCICR |= 0b00000111;
  // Setting the pins that will contribute to the interrupts.
  PCMSK2 |= 0b00011100; // Pins 2, 3, 4.
  
  // Initialise Outputs
  pinMode(LED_green, OUTPUT);
  pinMode(LED_yellow, OUTPUT);
  pinMode(LED_blue, OUTPUT);
  pinMode(LED_red, OUTPUT);
  
  double seconds = 4; // Enter value in seconds (MAX 4)
  startTimer(seconds);
}

// Converts the timerFrequency to timer speed in Hz for OCR1A.
double convert(double timerFrequency) {
  return (15624*timerFrequency);
  // 1024 Prescale = 15624 p/s
}  

// Starts the timer and manages its behaviour.
void startTimer(double timerFrequency){
  noInterrupts();
  
  // Clear registers
  TCCR1A = 0; // Timer/Counter Control Register A
  TCCR1B = 0; // Timer/Counter Control Register B
  TCNT1 = 0;  // Timer/Counter Register: 
  			    // Actual Timer value stored here.
  
  // Set timer Output Compare Reg.1A, 15624 = 1 seconds.
  OCR1A = convert(timerFrequency);
  
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10); //CS = Clock Source
  //TCCR1B |= B00000100; Alternative to CS12 value.
  
  // Output compare match A interrupt enable
  TIMSK1 |= (1 << OCIE1A);
  // TIMSK1 |= B00000010;
  
  // CTC mode ON - Clear Timer on Compare.
  TCCR1B |= (1 << WGM12); // Changed B to A
  // Enable interrupts.
  interrupts();
}


void sensor_1()
{
    Serial.println("SENSOR 1: detected motion!");
  	Serial.println(" ACTION: Green LED On.");
  	digitalWrite(LED_green,HIGH);
}

void sensor_2()
{
  	Serial.println("SENSOR 2: detected motion!");
  	Serial.println(" ACTION: Yellow LED On.");
  	digitalWrite(LED_yellow,HIGH);
}

void sensor_3()
{
  	Serial.println("SENSOR 3: detected motion!");
  	Serial.println(" ACTION: Blue LED On.");
  	digitalWrite(LED_blue,HIGH);
}

int count = 0;
void loop()
{
	Serial.print("     ~~ Time Kepper:");
  	Serial.println(count);
  	delay(1000);
    count++;
}


// Interrupt Service Routine 1 - Port D
ISR(PCINT2_vect)
{
  if (digitalRead(2)) {
    sensor_1();
  } 
  if (digitalRead(3)) {
    sensor_2();
  } 
  if (digitalRead(4)) {
    sensor_3();
  }
  if (digitalRead(2)==LOW && digitalRead(3)==LOW &&
     digitalRead(4)==LOW) {
    digitalWrite(LED_green,LOW);
    digitalWrite(LED_yellow,LOW);
    digitalWrite(LED_blue,LOW);
  }
}

// Interrupt Service Routine 2 - Timed Interupts
ISR(TIMER1_COMPA_vect){
	Serial.println("LED Red Timer SWITCH");
	digitalWrite(LED_red, digitalRead(LED_red) ^ 1);
}
  