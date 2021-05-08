/*
  Cullen Jennings - May 2021

  Blue and yellow button can be used to set limits of vacuum range

  Holding down both yellow and blue during boot up will put readings in
  raw mode that can be used to collect data for calibration.

  To use, hold down yellow button until upper preasure reached then hold
  down blue button until lower pressure reached.

  There is a spreadsheet to build the calibration formula for raw
  values. The calibration formula can be put into the "calib" function
  below.
*/

#include <Wire.h>


const byte dispAddr = 0x71;
const int sensorPin = A0;
const int relayPin = 13;
const int blueButtonPin = 2;
const int yellowButtonPin = 3;

volatile int blueValVol;
volatile int yellowValVol;

//#define SDEBUG

bool displayRaw = false;

float calib( float x ) {
  // y = 4.14687E-07x3 - 9.27494E-04x2 + 6.97722E-01x - 1.78920E+02
  float x2 = x * x;
  float x3 = x2 * x;
  return 4.14687E-7 * x3 - 9.27494E-4 * x2 + 6.97722E-1 * x - 1.78920E+2 ;
}

void blueButtonIntr() {
  blueValVol = analogRead(sensorPin);
}

void yellowButtonIntr() {
  yellowValVol = analogRead(sensorPin);
}

void setup() {
  Wire.begin();

#ifdef SDEBUG
  Serial.begin(115200);
#endif

  pinMode(relayPin , OUTPUT);
  digitalWrite(relayPin , LOW);

  analogReference( EXTERNAL );

  pinMode(blueButtonPin, INPUT_PULLUP);
  pinMode(yellowButtonPin, INPUT_PULLUP);
  digitalRead( blueButtonPin );
  digitalRead( yellowButtonPin );

  delay(500); // wait for input, sertial, and display arudino to all startup

  if ( (digitalRead( blueButtonPin ) == 0) && (digitalRead( yellowButtonPin ) == 0)  ) {
    displayRaw = true;
  }

  //attachInterrupt(digitalPinToInterrupt(blueButtonPin), blueButtonIntr, FALLING);
  //attachInterrupt(digitalPinToInterrupt(yellowButtonPin), yellowButtonIntr, FALLING);

  delay(100); // wait for interupts to stabalize then setup values

  // change these values to set the default min and max pressure

  blueValVol = 1024; // 492;
  yellowValVol = 1024; // 525;

  Wire.beginTransmission(dispAddr);
  Wire.write(0x76);  // clear display
  Wire.endTransmission();
}

void loop() {
  delay(100);

  int val = analogRead(sensorPin);
  // val = 500; // TODO REMOVE - use for testing

  float pressure = 0.0;
  if ( val < 1010 ) {
    unsigned long timeStart = micros();

    pressure = calib( float(val) );

    unsigned long timeDelta =  micros() - timeStart;
    //Serial.print(" calcTime(us)="); Serial.println(timeDelta);
  }

  static char dispBuffer[10];
  if ( displayRaw ) {
    sprintf(dispBuffer, "%4d", val);
    Wire.beginTransmission(dispAddr);
    Wire.write(0x76);  // clear display
    for (int i = 0; i < 4; i++)
    {
      Wire.write(dispBuffer[i]);
    }
    Wire.endTransmission();
  }
  else {
    float v = pressure * 10.0;
    val = int( v - 0.5 ); // round for negative numbers
    sprintf(dispBuffer, "%4d", val);
    Wire.beginTransmission(dispAddr);
    for (int i = 0; i < 4; i++)
    {
      Wire.write(dispBuffer[i]);
    }
    Wire.write(0x77); Wire.write(0b00000100); // turn on decimal point
    Wire.endTransmission();
  }

#if 0
  if ( (digitalRead( blueButtonPin ) == 0) && (digitalRead( yellowButtonPin ) == 0)  ) {
    displayRaw = true;
  }
#endif

  // turn on if blue button pressed
  if ( digitalRead( blueButtonPin ) == 0 ) {
    digitalWrite(relayPin , HIGH);

    blueValVol = val;

    if ( yellowValVol <= val ) {
      yellowValVol=val+10;
    }
#ifdef SDEBUG
    Serial.println("blue");
#endif
    return;
  }

  // turn on if yellow button pressed
  if ( digitalRead( yellowButtonPin ) == 0 ) {
    digitalWrite(relayPin , HIGH);
    yellowValVol = val;

    if ( blueValVol >= val ) {
      blueValVol = val-10;
    }
#ifdef SDEBUG
    Serial.println("yellow");
#endif
    return;
  }

  int blueVal = blueValVol;
  int yellowVal = yellowValVol;

#ifdef SDEBUG
  Serial.print( " blue=" );
  Serial.print( blueVal );
  Serial.print( " yellow=" );
  Serial.print( yellowVal );

  Serial.print( " val=" );
  Serial.print( val );
  Serial.print( "  pressure=" );
  Serial.println( pressure );
#endif

  //  pressure < min pressure  then turn off
  if ( val <= blueVal ) {
    digitalWrite(relayPin , LOW);
  }
  else {
    // pressure > max pressure  then turn on
    if ( val > yellowVal ) {
      digitalWrite(relayPin , HIGH);
    }
  }
}
