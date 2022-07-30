
// Pin Declarations
const uint8_t SENSOR_IN = 2;
const uint8_t LED_OUT = 3;

// Global Variables 
uint8_t led_state = LOW;
int message_switch = 1;

void setup()
{
  // Initialise Serial Output
  Serial.begin(9600);
  // Initialise Pin Functions
  pinMode(SENSOR_IN, INPUT_PULLUP);
  pinMode(LED_OUT, OUTPUT);
  // Attaching Interupt to pin
  attachInterrupt(digitalPinToInterrupt(SENSOR_IN), interrupt, CHANGE); 
}

void loop()
{
  //
}

// Interrupt Routine.
void interrupt()
{
  // As the sensor stays on for 1 seconds.
  // I have configured the interrupt to trigger on CHANGE.
  // This allows for the LED to go on for 1 second, then off.
  if (message_switch%2 != 0)
  {
    Serial.println("SENSOR: detected motion!");
  	led_state = !led_state;
  	Serial.println(" ACTION: LED On.");
  	digitalWrite(LED_OUT,led_state);
  }
  else 
  {
    Serial.println("SENSOR: detactivated.");
  	led_state = !led_state;
  	Serial.println(" ACTION: LED Off.");
  	digitalWrite(LED_OUT,led_state);
  }
  message_switch++;
}

  