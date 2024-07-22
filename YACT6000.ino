// YACT6000 - Yet Another Cw Tool for using FlexRadio 6X00/8X00 remotelly
// Ver 0.1 N5NHJ 4/24
//

#define DEBUG
#ifdef DEBUG
  #define BAUD_RATE 115200 
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

/* LIBRARIES INCLUSION */
#include <SD.h>
#include "NativeEthernet.h"
#include <FlexRigTeensy.h>
// #include <EEPROM.h>

/* RADIO AND PROTOCOL DEFINITIONS */
FlexRig FlexRadio;
#define TCP_API_PORT 4992
#define UDP_DISCOVERING_PORT 4992
#define UDP_TX_PACKET_MAX_SIZE_FLEX 16384	// V3 = 596, V2 = 350
#define UDP_VITA49_PORT 4991
#define UDP_VITA49_PACKET_MAX_SIZE 16384	// V3 = 16384, V2 = 4992
#define RESPONSE_PACKET_LIST_SIZE 1024	// V3 = 1024, V2 = 25

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

/* ETHERNET CHANNELS AND BUFFERS */
EthernetClient RadioTCPChannel;
String RadioTCPBuffer;
EthernetClient RadioUDPChannel;
String RadioUDPBuffer;
EthernetUDP VITA49;
char VITA49Buffer[UDP_TX_PACKET_MAX_SIZE];
char handle[9];

/* GLOBAL VARIABLE DEFINITIONS */
const int chipSelect         = BUILTIN_SDCARD;
const int BuiltInLED         = LED_BUILTIN;
const int KeyInPin           = 0; //33
unsigned long CWIndex        = 1;
unsigned long SEQ            = 1;
String ConnectionHandle;

unsigned int PingInterval = 1000;
unsigned long LastPing;
unsigned long TimeIt;
char Rchar;
String InBuf;
String RadioCommand;
bool PreviousKeying          = false;
bool FlexConnected           = false;

/* SD Card parameters - Assign defaults in case some lines are missing */
bool InSetup                  = true;
int StartUpDelay              = 250;
int Debounce                  = 30;
bool ST                       = true;
unsigned int STFreq           = 600;
bool StaticIP                 = false;
unsigned int FlexPort         = 4992;
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
IPAddress RadioIP;
IPAddress ipAddress;
/* END GLOBAL VARIABLE DEFINITIONS  */

void setup() {
  #ifdef DEBUG
  Serial.begin(BAUD_RATE);
  while (!Serial) {               // wait for Serial Monitor 
    if (millis() > 2000) { 
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
  digitalWrite(BuiltInLED, HIGH);

  getConfigFile();
  getIpAddress();

  debugf4 ("Teensy IP: %u.%u.%u.%u\n", MyIP[0], MyIP[1], MyIP[2], MyIP[3]);
  debugf6 ("Teensy MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", MyMAC[0], MyMAC[1], MyMAC[2], MyMAC[3], MyMAC[4], MyMAC[5]);
  debugf4 ("Teensy MASK: %u.%u.%u.%u\n", MyMask[0], MyMask[1], MyMask[2], MyMask[3]);
  debugf4 ("Teensy GATEWAY: %u.%u.%u.%u\n", MyGateway[0], MyGateway[1], MyGateway[2], MyGateway[3]);
  debugf4 ("Teensy DNS: %u.%u.%u.%u\n", MyDNS[0], MyDNS[1], MyDNS[2], MyDNS[3]);
  debugf4 ("Flex IP: %u.%u.%u.%u\n", FlexIP[0], FlexIP[1], FlexIP[2], FlexIP[3]);

  while (!FlexConnected) {
    debugln("Connecting Radio");
    FlexConnected=connect(RadioIP, FlexPort);
    delay(3000); //Wait 3 second if both connected or not connected yet. 
  }

  //ipAddress=RadioIP;
  //FlexRadio.connect();

  ConnectionHandle=getHandler();
  debugln(ConnectionHandle);
  FlexInit();
  send_K();
  digitalWrite(BuiltInLED, LOW);

  debugln("Radio connected");

} //END setup()

void loop() {
  if (digitalRead(KeyInPin)) {                                        //It is high, I'm not transmitting
    if (PreviousKeying) {
      RadioCommand="C" + String(SEQ).trim() + "|cw key immediate 0\n";
      //RadioCommand="C"+ String(SEQ).trim() + "|cw key 0 time=0x" + String(millis() % 0xFFFF, HEX).trim() + " index=" + String(CWIndex).trim() + " client_handle=0x" + ConnectionHandle.trim() + "\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      CWIndex++;
      SEQ++;
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
      RadioCommand="C" + String(SEQ).trim() + "|cw key immediate 1\n";
    //  RadioCommand="C"+ String(SEQ).trim() + "|cw key 1 time=0x" + String(millis() % 0xFFFF, HEX).trim() + " index=" + String(CWIndex).trim() + " client_handle=0x" + ConnectionHandle.trim() + "\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      CWIndex++;
      SEQ++;
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

  if ( (millis()-LastPing) > PingInterval) {
    RadioCommand = "C" + String(SEQ).trim() + "|ping ms_timestamp=" + String(millis()).trim()+".0000\n";
    RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
    LastPing = millis();
    SEQ++;
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

void FlexInit() {
      RadioCommand="C1|client ip\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      RadioCommand="C2|client program YACT6000\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      RadioCommand="C3|client start_persistence 0\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C4|client bind client_ID\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C5|info\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C6|version\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C7|ant list\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C8|mic list\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C9|profile global info\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C10|profile tx info\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C11|profile mic info\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C12|profile displayinfo\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C13|sub client all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C14|sub tx all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C15|sub atu all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C16|sub amplifier all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C17|sub meter all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C18|sub pan all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C19|sub slice all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C20|sub gps all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C21|sub audio_stream all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C22|sub cwx all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C23|sub xvtr all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C24|sub memories all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C25|sub daxiq all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C26|sub dax all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C27|sub usb_cable all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C28|sub spot all\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      RadioCommand="C29|client set enforce_network_mtu=1 network_mtu=1500\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      RadioCommand="C30|client set send_reduced_bw_dax=1\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C31|client udpport 4995\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      RadioCommand="C32|stream create netcw\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C33|display panf rfgain_info 0x0\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      SEQ=34;
  }

bool connect(IPAddress IP, uint16_t Port) {
  if (RadioTCPChannel.connect(IP, Port)) {
    return true;
    digitalWrite(BuiltInLED, LOW);
  }
  else {
  return false;
  }
}

String getHandler () {
  while (RadioTCPChannel.available()) {
    char c=RadioTCPChannel.read();
    debug(c);
    if (c=='H') {
      for (unsigned int i=0;i<8;i++) {
        if (RadioTCPChannel.available()) {
          handle[i]=RadioTCPChannel.read();
          debug("-->");
          debug(handle[i]);
        }
        else {
          i--;
        }
      }
    handle[8]='\0';
    debugln(handle);
    return handle;
    }
  }

}