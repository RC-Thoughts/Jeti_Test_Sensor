/*
   -----------------------------------------------------------
                Jeti Test Sensor v 1.0
   -----------------------------------------------------------

    Tero Salminen RC-Thoughts.com (c) 2017 www.rc-thoughts.com

  -----------------------------------------------------------

    Simple test sensor with two potentiometers and two 3-pos
    switches. 

    Bothe sensora are selectable to work in three ranges: 
    0 - 100, 0 - 1000 and 0 - 10000. 

  -----------------------------------------------------------
    Shared under MIT-license by Tero Salminen (c) 2017
  -----------------------------------------------------------
*/

#include <EEPROM.h>
#include <stdlib.h>
#include <SoftwareSerialJeti.h>
#include <JETI_EX_SENSOR.h>

#define prog_char char PROGMEM
#define GETCHAR_TIMEOUT_ms 20

#ifndef JETI_RX
#define JETI_RX 3
#endif

#ifndef JETI_TX
#define JETI_TX 4
#endif

#define ITEMNAME_1 F("Sensor 1")
#define ITEMTYPE_1 F("")
#define ITEMVAL_1 &sens1

#define ITEMNAME_2 F("Sensor 2")
#define ITEMTYPE_2 F("")
#define ITEMVAL_2 &sens2

#define ABOUT_1 F(" RCT Jeti Tools")
#define ABOUT_2 F("Jeti Test Sensor")

SoftwareSerial JetiSerial(JETI_RX, JETI_TX);

void JetiUartInit()
{
  JetiSerial.begin(9700);
}

void JetiTransmitByte(unsigned char data, boolean setBit9)
{
  JetiSerial.set9bit = setBit9;
  JetiSerial.write(data);
  JetiSerial.set9bit = 0;
}

unsigned char JetiGetChar(void)
{
  unsigned long time = millis();
  while ( JetiSerial.available()  == 0 )
  {
    if (millis() - time >  GETCHAR_TIMEOUT_ms)
      return 0;
  }
  int read = -1;
  if (JetiSerial.available() > 0 )
  { read = JetiSerial.read();
  }
  long wait = (millis() - time) - GETCHAR_TIMEOUT_ms;
  if (wait > 0)
    delay(wait);
  return read;
}

char * floatToString(char * outstr, float value, int places, int minwidth = 0) {
  int digit;
  float tens = 0.1;
  int tenscount = 0;
  int i;
  float tempfloat = value;
  int c = 0;
  int charcount = 1;
  int extra = 0;
  float d = 0.5;
  if (value < 0)
    d *= -1.0;
  for (i = 0; i < places; i++)
    d /= 10.0;
  tempfloat +=  d;
  if (value < 0)
    tempfloat *= -1.0;
  while ((tens * 10.0) <= tempfloat) {
    tens *= 10.0;
    tenscount += 1;
  }
  if (tenscount > 0)
    charcount += tenscount;
  else
    charcount += 1;
  if (value < 0)
    charcount += 1;
  charcount += 1 + places;
  minwidth += 1;
  if (minwidth > charcount) {
    extra = minwidth - charcount;
    charcount = minwidth;
  }
  if (value < 0)
    outstr[c++] = '-';
  if (tenscount == 0)
    outstr[c++] = '0';
  for (i = 0; i < tenscount; i++) {
    digit = (int) (tempfloat / tens);
    itoa(digit, &outstr[c++], 10);
    tempfloat = tempfloat - ((float)digit * tens);
    tens /= 10.0;
  }
  if (places > 0)
    outstr[c++] = '.';
  for (i = 0; i < places; i++) {
    tempfloat *= 10.0;
    digit = (int) tempfloat;
    itoa(digit, &outstr[c++], 10);
    tempfloat = tempfloat - (float) digit;
  }
  if (extra > 0 ) {
    for (int i = 0; i < extra; i++) {
      outstr[c++] = ' ';
    }
  }
  outstr[c++] = '\0';
  return outstr;
}

JETI_Box_class JB;

unsigned char SendFrame()
{
  boolean bit9 = false;
  for (int i = 0 ; i < JB.frameSize ; i++ )
  {
    if (i == 0)
      bit9 = false;
    else if (i == JB.frameSize - 1)
      bit9 = false;
    else if (i == JB.middle_bit9)
      bit9 = false;
    else
      bit9 = true;
    JetiTransmitByte(JB.frame[i], bit9);
  }
}

unsigned char DisplayFrame()
{
  for (int i = 0 ; i < JB.frameSize ; i++ )
  {
  }
}

uint8_t frame[10];
short value = 27;

// Switches
const int sw1Pin = 6; //Digital
const int sw2Pin = 7;
const int sw3Pin = 8;
const int sw4Pin = 9;
int sw1State = 0;
int sw2State = 0;
int sw3State = 0;
int sw4State = 0;

// Potentiometers
int pot1Pin = 4; // Analog
int pot2Pin = 5;
float pot1val = 0;
float pot2val = 0;

// For sensors
boolean sw1 = false;
boolean sw2 = false;
boolean sw3 = false;
boolean sw4 = false;
unsigned int sens1 = 0;
unsigned int sens2 = 0;

#define MAX_SCREEN 2
#define MAX_CONFIG 1
#define COND_LES_EQUAL 1
#define COND_MORE_EQUAL 2

void setup()
{
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  // Pinmodes for switches
  pinMode(sw1Pin, INPUT);
  pinMode(sw2Pin, INPUT);
  pinMode(sw3Pin, INPUT);
  pinMode(sw4Pin, INPUT);

  pinMode(JETI_RX, OUTPUT);
  JetiUartInit();

  JB.JetiBox(ABOUT_1, ABOUT_2);
  JB.Init(F("RCT-Test"));
  JB.addData(ITEMNAME_1, ITEMTYPE_1);
  JB.addData(ITEMNAME_2, ITEMTYPE_2);
  JB.setValueBig(1, ITEMVAL_1);
  JB.setValueBig(2, ITEMVAL_2);

  do {
    JB.createFrame(1);
    SendFrame();
    delay(GETCHAR_TIMEOUT_ms);
  }
  while (sensorFrameName != 0);
  digitalWrite(13, LOW);
}

int header = 0;
int lastbtn = 240;
int current_screen = 0;
int current_config = 0;
char temp[LCDMaxPos / 2];
char msg_line1[LCDMaxPos / 2];
char msg_line2[LCDMaxPos / 2];

void process_screens()
{
  switch (current_screen)
  {
    case 0 : {
        JB.JetiBox(ABOUT_1, ABOUT_2);
        break;
      }
    case MAX_SCREEN : {
        JB.JetiBox(ABOUT_1, ABOUT_2);
        break;
      }
  }
}

void loop()
{
  // Read switches
  sw1State = digitalRead(sw1Pin);
  sw2State = digitalRead(sw2Pin);
  sw3State = digitalRead(sw3Pin);
  sw4State = digitalRead(sw4Pin);

  // Read potentiometers
  pot1val = analogRead(pot1Pin);
  pot2val = analogRead(pot2Pin);

  // Set switches true if on
  if (sw1State == HIGH) {
    sw1 = true;
  } else {
    sw1 = false;
  }
  if (sw2State == HIGH) {
    sw2 = true;
  } else {
    sw2 = false;
  }
  if (sw3State == HIGH) {
    sw3 = true;
  } else {
    sw3 = false;
  }
  if (sw4State == HIGH) {
    sw4 = true;
  } else {
    sw4 = false;
  }

  // Define ranges for sensor 1:
  // Switch UP = 0-100
  // Switch middle = 0-1000
  // Switch down = 0-10000
  if (sw1) {
    sens1 = (pot1val * 100) / 1023; // 0-100
  } else if (sw2) {
    sens1 = (pot1val * 10000) / 1023; // 0-10000
    //sens1 = round(sens1 * 100) / 100;
  } else {
    sens1 = (pot1val * 1000) / 1023; // 0-1000
    //sens1 = round(sens1 * 100) / 100;
  }

  // Define ranges for sensor 2:
  // Switch UP = 0-100
  // Switch middle = 0-1000
  // Switch down = 0-10000
  if (sw3) {
    sens2 = (pot2val * 100) / 1023; // 0-100
  } else if (sw4) {
    sens2 = (pot2val * 10000) / 1023; // 0-10000
    //sens2 = round(sens2 * 100) / 100;
  } else {
    sens2 = (pot2val * 1000) / 1023; // 0-1000
    //sens2 = round(sens2 * 100) / 100;
  }

  // Serial debug - Print switch and potentiometer states
  Serial.print("Inputs: "); Serial.print(sw1);
  Serial.print(" "); Serial.print(sw2);
  Serial.print(" "); Serial.print(sw3);
  Serial.print(" "); Serial.print(sw4);

  Serial.print(" PotVals: ");
  Serial.print(pot1val); Serial.print(" ");
  Serial.print(pot2val);

  // Serial debug - Print sensor values
  Serial.print(" Sensors: ");
  Serial.print(sens1); Serial.print(" ");
  Serial.println(sens2);

  unsigned long time = millis();
  SendFrame();
  time = millis();
  int read = 0;
  pinMode(JETI_RX, INPUT);
  pinMode(JETI_TX, INPUT_PULLUP);

  JetiSerial.listen();
  JetiSerial.flush();

  while ( JetiSerial.available()  == 0 )
  {

    if (millis() - time >  5)
      break;
  }

  if (JetiSerial.available() > 0 )
  { read = JetiSerial.read();

    if (lastbtn != read)
    {
      lastbtn = read;
      switch (read)
      {
        case 224 : // RIGHT
          break;
        case 112 : // LEFT
          break;
        case 208 : // UP
          break;
        case 176 : // DOWN
          break;
        case 144 : // UP+DOWN
          break;
        case 96 : // LEFT+RIGHT
          break;
      }
    }
  }

  if (current_screen != MAX_SCREEN)
    current_config = 0;
  process_screens();
  header++;
  if (header >= 5)
  {
    JB.createFrame(1);
    header = 0;
  }
  else
  {
    JB.createFrame(0);
  }

  long wait = GETCHAR_TIMEOUT_ms;
  long milli = millis() - time;
  if (milli > wait)
    wait = 0;
  else
    wait = wait - milli;
  pinMode(JETI_TX, OUTPUT);
}
