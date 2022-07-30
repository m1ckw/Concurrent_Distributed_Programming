#include <Servo.h>


// Pin Declarations
const uint8_t SENSOR_IN = 3;
const uint8_t LED_OUT = 4;
const uint8_t SENSOR_2 = 2;
const uint8_t SERVO_GO = 9;

// Global Variables 
uint8_t led_state = LOW;
int message_switch = 1; // To alternate Serial output msg.
int message_switch2 = 1; // To alternate Serial output msg2.
int pos = 0; // Servo initial position.

Servo myservo;  // creating a servo object to control the servo

void setup()
{
  // Initialise Serial Output
  Serial.begin(9600);
  // Initialise Pin Functions
  pinMode(SENSOR_IN, INPUT_PULLUP);
  pinMode(LED_OUT, OUTPUT);
  pinMode(SENSOR_2, INPUT_PULLUP);
  pinMode(SERVO_GO, OUTPUT);
  // Intialise Servo Pin
  myservo.attach(SERVO_GO);
  // Attaching Interupts to pins
  attachInterrupt(digitalPinToInterrupt(SENSOR_IN), interrupt, CHANGE); 
  attachInterrupt(digitalPinToInterrupt(SENSOR_2), interrupt_2, CHANGE); 
}

void loop()
{
  // Nothing to see here.
}

// Interrupt Routine1.
void interrupt()
{
  // As the sensor stays on for 1 seconds.
  // I have configured the interrupt to trigger on CHANGE.
  // This allows for the LED to go on for 1 second, then off.
  if (message_switch%2 != 0)
  {
    Serial.println("SENSOR 1: detected motion!");
  	led_state = !led_state;
  	Serial.println(" ACTION: LED On.");
  	digitalWrite(LED_OUT,led_state);
  }
  else 
  {
    Serial.println("SENSOR 1: detactivated.");
  	led_state = !led_state;
  	Serial.println(" ACTION: LED Off.");
  	digitalWrite(LED_OUT,led_state);
  }
  message_switch++;
}

// Interrupt Routine2 - Similar to above.
void interrupt_2()
{
  if (message_switch2%2 != 0)
  {
    Serial.println("SENSOR 2: detected motion!");
    Serial.println(" ACTION: Servo Open.");
  	myservo.write(0);
  }
  else
  {
    Serial.println("SENSOR 2: detactivated!");
    Serial.println(" ACTION: Servo Close.");
  	myservo.write(90);
  }
  message_switch2++;
}

 

  