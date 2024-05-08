// YACT6000 - Yet Another Cw Toll for FlexRadio 6X00 series in remote
// Ver 0.1 N5NHJ 4/24
//
#include <SD.h>
#include "NativeEthernet.h"
// #include <EEPROM.h>
// #define DEBUG
#define BAUD_RATE 115200

#ifdef DEBUG 
  #define debug(x)                        Serial.print(x)
  #define debugln(x)                      Serial.println(x)
  #define debugf6(Str, A, B, C, D, E, F)  Serial.printf(Str, A, B, C, D, E, F)
  #define debugf4(Str, A, B, C, D)        Serial.printf(Str, A, B, C, D)
#else
  #define debug(x)
  #define debugln(x)
  #define debugf6(Str, A, B, C, D, E, F)
  #define debugf4(Str, A, B, C, D)
#endif

/* GLOBAL VARIABLE DEFINITIONS */
const int chipSelect = BUILTIN_SDCARD;
const int BuiltInLED         = LED_BUILTIN;
const int KeyInPin           = 0; //33
const int Speaker            = 1; //34

unsigned long TimeIt;
char Rchar;
String InBuf;
bool PreviousKeying = false;

/* SD Card parameters - Assign defaults in case some lines are missing */
bool InSetup                  = true;
int StartUpDelay              = 250;
int Debounce                  = 30;
bool ST                       = true;
unsigned int STFreq           = 800;
bool StaticIP                 = false;
uint8_t FlexIP[4]             = {0, 0, 0, 0 };
uint8_t CfgGateway[4]         = {0, 0, 0, 0 };
uint8_t CfgIP[4]              = {0, 0, 0, 0 };
uint8_t CfgMask[4]            = {0, 0, 0, 0 };

File ConfigFile;

/* Ethernet */
byte MyMAC[6];
IPAddress MyIP;
IPAddress MyGateway;
IPAddress MyMask;
IPAddress MyDNS;
/* END GLOBAL VARIABLE DEFINITIONS  */

void setup() {
  #ifdef DEBUG
  Serial.begin(BAUD_RATE);
  while (!Serial) {
    // wait for Serial Monitor 
    if (millis() > 2000) { // Don't wait too long...
      break;
    }
  }
  #endif
  getConfigFile();
  getIpAddress(); 

  // if (Ethernet.linkStatus() == 1) {
    debugf6 ("Teensy MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", MyMAC[0], MyMAC[1], MyMAC[2], MyMAC[3], MyMAC[4], MyMAC[5]);
    debugf4 ("Teensy IP: %u.%u.%u.%u\n", MyIP[0], MyIP[1], MyIP[2], MyIP[3]);
    debugf4 ("Teensy MASK: %u.%u.%u.%u\n", MyMask[0], MyMask[1], MyMask[2], MyMask[3]);
    debugf4 ("Teensy GATEWAY: %u.%u.%u.%u\n", MyGateway[0], MyGateway[1], MyGateway[2], MyGateway[3]);
    debugf4 ("Teensy DNS: %u.%u.%u.%u\n", MyDNS[0], MyDNS[1], MyDNS[2], MyDNS[3]);
    debugf4 ("Flex IP: %u.%u.%u.%u\n", FlexIP[0], FlexIP[1], FlexIP[2], FlexIP[3]);
  // }

  pinMode(KeyInPin, INPUT_PULLUP);
  pinMode(BuiltInLED, OUTPUT);
  pinMode(Speaker,OUTPUT);
  send_K();

} //END setup()

void loop() {
  if (digitalRead(KeyInPin)) {//It is high, I'm not transmitting
    if (PreviousKeying) {
      PreviousKeying = false;
      digitalWrite(BuiltInLED, LOW);    // LED off
      debugln("RX");
      if (ST) {noTone(Speaker);}
    }
    else {
    }
  }
  else {  //It is low, I'm transmitting
    if (PreviousKeying) {
    }
    else {
      PreviousKeying = true;
      if (ST) {tone(Speaker,STFreq);} 
      digitalWrite(BuiltInLED, HIGH);   // LED on
      debugln("TX");
    }
  }
} //END loop()

//---------------------------------------------
void send_K() { // _._
  tone (Speaker, 600);
  delay (90);
  noTone(Speaker);
  delay (30);
  tone (Speaker, 600);
  delay (30);
  noTone(Speaker);
  delay (30);
  tone (Speaker, 600);
  delay (90);
  noTone(Speaker);
}

void send_C() { // _._.
  tone (Speaker, 600);
  delay (90);
  noTone(Speaker);
  delay (30);
  tone (Speaker, 600);
  delay (30);
  noTone(Speaker);
  delay (30);
  tone (Speaker, 600);
  delay (90);
  noTone(Speaker);
  delay (30);
  tone (Speaker, 600);
  delay (30);
  noTone(Speaker);
}