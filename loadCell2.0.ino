#include <SPI.h>
#include <SD.h>
#include <Bounce2.h>
#include <HX711.h>

//device objects
HX711 cell;


//global variables
unsigned long cycle = 0;
String dataString;
unsigned long timeStamp = 0;


//pin declarations
const byte Sensor1Pin = 2; //direction check sensor
const byte DOut = A1; // DOut pin for HX711
const byte Pd_sck = A0; // Pd_sck pin for HX711
const byte LogSwitchPin = 4; // pin to switch between testing and logging mode
const byte chipSelectPin = 4;

bool logSwitchPos = LOW;

//debouncer objects
Bounce toggleBouncer = Bounce();
Bounce sensor1Bouncer = Bounce();

int updateBouncers() {
  //return the number of bouncers that updated
  return toggleBouncer.update() + sensor1Bouncer.update(); // yes, this is sneaky. Each update is called and the sum is returned in case we want to check how many updated for debugging
}

void globalReset() {
  cell.tare();
  setup();
  cycle = 0;
}

int logLoadValue() {
  timeStamp = millis() / 1000; // number of seconds since we started
  dataString = String(cycle) + "," + String(timeStamp) + "," + String(cell.get_units(10)); // this will average 10 readings
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  
  if (dataFile) {
    dataFile.println(dataString);
  } else {
    Serial.println("Data Write Error");
  }
}

void setup() {
  
  pinMode(LED_BUILTIN, OUTPUT); //enable the built-in LED, in case servos are too much fun

  //setup pins with debouncers
  pinMode(Sensor1Pin, INPUT_PULLUP);
  sensor1Bouncer.attach(Sensor1Pin);
  sensor1Bouncer.interval(5);
  
  pinMode(LogSwitchPin, INPUT_PULLUP);
  toggleBouncer.attach(LogSwitchPin);
  toggleBouncer.interval(5);
  
  // Start serial comm and set up load cell and SD card
  Serial.begin(9600);
  Serial.print("Initializing Load Cell...");
  cell.begin(DOut, Pd_sck);
  Serial.println("Done");
  
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelectPin)) {
    Serial.println("FAIL");
  }
  Serial.println("Done");
  
  Serial.print("Tare and debounce...");
  cell.set_scale(22773.73); // this scales output to Newtons
  cell.tare(); // I don't know why it needs to tare twice, but doing it twice with a 5s gap seems to work really well for an accurate zero.
  delay(5000);
  cell.tare();
  Serial.println("Ready");
}

void loop() {
  updateBouncers();
  logSwitchPos = toggleBouncer.read();
  switch (logSwitchPos) {
    case LOW:
    if (!LED_BUILTIN) digitalWrite(LED_BUILTIN, HIGH); // turn on the LED to indicate testing mode
    Serial.println(cell.get_units());
    case HIGH:
      digitalWrite(LED_BUILTIN, LOW); // start with LED off to indicate logging mode
      if (!sensor1Bouncer.read()) { // check if sensor1 is active to trip a logging event
        delay(2000); // wait 2 seconds to make sure everything is level
        logLoadValue();
        digitalWrite(LED_BUILTIN, HIGH); // this and the next two lines just flash the LED to show a successful log, so we can make sure it happened before the arm moves
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        cycle++; // increment the number of cycles
        delay(600000); // wait 10 minutes
      }
  }
}
