// YACT6000 - Yet Another Cw Tool for using FlexRadio 6X00/8X00 remotelly
// Ver 0.1 N5NHJ 4/24
//

// #define DEBUG
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

#include <SD.h>
#include "NativeEthernet.h"
// #include <EEPROM.h>

/* SIDETONE VARIABLE DEFINITIONS */
//#define AUDIOSYNT                                                     //High definition audiolibrary to be used instead of the Tone function to generate Sidetone.
//#ifdef AUDIOSYNT
  #include <Audio.h>
  AudioSynthWaveform SideTone;
  AudioOutputPWM Speaker (1,2);                                        //PWM pins output for audio
  AudioConnection patchCord0 (SideTone, Speaker);
//#else
//  const int Speaker            = 1;                                   //34
//#endif

#define BAUD_RATE 115200

/* GLOBAL VARIABLE DEFINITIONS */
const int chipSelect = BUILTIN_SDCARD;
const int BuiltInLED         = LED_BUILTIN;
const int KeyInPin           = 0; //33

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

  //#ifdef AUDIOSYNT
    AudioMemory (8);
    SideTone.begin(0.0, STFreq, WAVEFORM_SINE);                                    //SINE, PULSE, SAWTOOTH, SQUARE, TRIANGLE
  //#else
  //  pinMode(Speaker,OUTPUT);
  //#endif
  
  pinMode(KeyInPin, INPUT_PULLUP);
  pinMode(BuiltInLED, OUTPUT);

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

  send_K();
} //END setup()

void loop() {
  if (digitalRead(KeyInPin)) {                                        //It is high, I'm not transmitting
    if (PreviousKeying) {
      PreviousKeying = false;
      digitalWrite(BuiltInLED, LOW);                                  // LED off
      debugln("RX");
      if (ST) {
        //#ifdef AUDIOSYNT
          SideTone.amplitude(0);
        //#else
        //  noTone(Speaker);
        //#endif
      }
    }
    else {
    }
  }
  else {                                                              //It is low, I'm transmitting
    if (PreviousKeying) {
    }
    else {
      PreviousKeying = true;
      if (ST) {
        //#ifdef AUDIOSYNT
          SideTone.amplitude(1.0);
        //#else
        //  tone(Speaker,STFreq);
        //#endif
      } 
      digitalWrite(BuiltInLED, HIGH);                                 // LED on
      debugln("TX");
    }
  }
} //END loop()

//---------------------------------------------
// Element (dot) length = 60000 (milliseconds in a minute) / (50 (elements of the world PARIS) * Speed (word per minute))
// @20WMP: 60000 / (50*20) = 60ms
// @30WPM: 60000 / (50*30) = 40ms
// @40WPM: 60000 / (50*40) = 30ms
void send_dot(int speed) {

}
void send_dash(int speed) {

}
void send_element_space(int speed) {

}
void send_letter_space (int speed){

}
void send_word_space (int speed){

}

void send_K() {                                                       // _._ at speed of 40WPM
  //#ifdef AUDIOSYNT
    SideTone.frequency(600.0);
    SideTone.amplitude(1.0);
  //#else
  //  tone (Speaker, 600);
  //#endif
  delay (90);

  //#ifdef AUDIOSYNT
    SideTone.amplitude(0.0);
  //#else
  //  noTone(Speaker);
  //#endif
  delay (30);

  //#ifdef AUDIOSYNT
    SideTone.amplitude(1.0);
  //#else
  //  tone (Speaker, 600);
  //#endif
  delay (30);

  //#ifdef AUDIOSYNT
    SideTone.amplitude(0.0);
  //#else
  //  noTone(Speaker);
  //#endif
  delay (30);

  //#ifdef AUDIOSYNT
    SideTone.amplitude(1.0);
  //#else
  //  tone (Speaker, 600);
  //#endif
  delay (90);

  //#ifdef AUDIOSYNT
    SideTone.amplitude(0);
    SideTone.frequency(STFreq);
  //#else
  //  noTone(Speaker);
  //#endif
  }

void send_C_tone() {                                                  // _._.
  //#ifdef AUDIOSYNT
  //#else
  //  tone (Speaker, 600);
  delay (90);
  //noTone(Speaker);
  delay (30);
  //tone (Speaker, 600);
  delay (30);
  //noTone(Speaker);
  delay (30);
  //tone (Speaker, 600);
  delay (90);
  //noTone(Speaker);
  delay (30);
  //tone (Speaker, 600);
  delay (30);
  //noTone(Speaker);
  //#endif
}