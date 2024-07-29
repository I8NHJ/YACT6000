// MORCONI for Flex
// MOrse COde Network Interface for 6X00 and 8X00 Flex radio series.
// Ver 0.1 N5NHJ 4/24
//

#define APP_VERSION 1.0

//#define CONFIG_DEBUG
#define debug(x)                        Serial.print(x)
#define debugln(x)                      Serial.println(x)
#define debugf6(Str, A, B, C, D, E, F)  Serial.printf(Str, A, B, C, D, E, F)
#define debugf4(Str, A, B, C, D)        Serial.printf(Str, A, B, C, D)

/* LIBRARIES INCLUSION */
#include <SD.h>
#include "NativeEthernet.h"
#include <Audio.h>

/* RADIO AND PROTOCOL DEFINITIONS */
// #include <FlexRigTeensy.h>
// FlexRig FlexRadio;
// #define TCP_API_PORT 4992
// #define UDP_DISCOVERING_PORT 4992
// #define UDP_TX_PACKET_MAX_SIZE_FLEX 16384	// V3 = 596, V2 = 350
// #define UDP_VITA49_PORT 4991
// #define UDP_VITA49_PACKET_MAX_SIZE 16384	// V3 = 16384, V2 = 4992
// #define RESPONSE_PACKET_LIST_SIZE 1024	// V3 = 1024, V2 = 25

/* SIDETONE VARIABLE DEFINITIONS */
AudioSynthWaveform SideTone;
AudioOutputPWM Speaker (1,2);                                        //PWM pins output for audio
AudioConnection patchCord0 (SideTone, Speaker);

/* ETHERNET CHANNELS AND BUFFERS */
EthernetClient RadioTCPChannel;
String RadioTCPBuffer;
// EthernetClient RadioUDPChannel;
// String RadioUDPBuffer;
// EthernetUDP VITA49;
// char VITA49Buffer[UDP_TX_PACKET_MAX_SIZE];
char MyHandle[]={"00000000"};
String ConnectionHandle;
String ClientHandle="";

/* GLOBAL VARIABLE DEFINITIONS */
const int chipSelect          = BUILTIN_SDCARD;
const int BuiltInLED          = LED_BUILTIN;
const int KeyInPin            = 0; //33
unsigned long CWIndex         = 1;
unsigned long SEQ             = 1;

/* TIMING VARIABLES */
unsigned int PingTimeInterval = 1000;
unsigned long LastPingTime    = 0;
unsigned long ThisLoopTime    = 0;
unsigned long TimeIt;

String RadioCommand;
bool PreviousKeying           = false;
bool FlexConnected            = false;

/* SD Card parameters - Assign defaults in case some lines are missing */
char Rchar;
String InBuf;
bool InSetup                  = true;

unsigned int StartUpDelay     = 250;
unsigned int Debounce         = 30;
bool SidetoneActive           = true;
float SidetoneFrequency       = 600.0;
float SidetoneVolume          = 0.5;
bool StaticIP                 = false;
unsigned int FlexPort         = 4992;
unsigned int FlexDelay        = 3000;
bool TeensyDebug              = false;
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
  pinMode(KeyInPin, INPUT_PULLUP);
  pinMode(BuiltInLED, OUTPUT);
  digitalWrite(BuiltInLED, HIGH);
  
  AudioMemory (8);
  SideTone.begin(0.0, SidetoneFrequency, WAVEFORM_SINE);                                    //SINE, PULSE, SAWTOOTH, SQUARE, TRIANGLE

  #ifdef CONFIG_DEBUG
    OpenSerialMonitor();
  #endif

  getConfigFile();

  if (TeensyDebug) {
    #ifndef CONFIG_DEBUG
      OpenSerialMonitor();
    #endif
  }

  getIpAddress();

  if (TeensyDebug) {
    debugf4 ("Teensy IP: %u.%u.%u.%u\n", MyIP[0], MyIP[1], MyIP[2], MyIP[3]);
    debugf6 ("Teensy MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", MyMAC[0], MyMAC[1], MyMAC[2], MyMAC[3], MyMAC[4], MyMAC[5]);
    debugf4 ("Teensy MASK: %u.%u.%u.%u\n", MyMask[0], MyMask[1], MyMask[2], MyMask[3]);
    debugf4 ("Teensy GATEWAY: %u.%u.%u.%u\n", MyGateway[0], MyGateway[1], MyGateway[2], MyGateway[3]);
    debugf4 ("Teensy DNS: %u.%u.%u.%u\n", MyDNS[0], MyDNS[1], MyDNS[2], MyDNS[3]);
    debugf4 ("Flex IP: %u.%u.%u.%u\n", FlexIP[0], FlexIP[1], FlexIP[2], FlexIP[3]);
  }

  while (!FlexConnected) {                        //Retry connecting forever
    if (TeensyDebug) {
      debugln("Connecting Radio");
    }
    FlexConnected=connect(RadioIP, FlexPort);
    if (!FlexConnected) {
      delay(FlexDelay);                                //Timeout is set to 2s, wait 3s more before retry.
    }
  }

  FlexInit();

  send_K();

  if (TeensyDebug){
    debugln("Radio connected");
  }
} //END setup()

void loop() {
  ThisLoopTime=millis();
  if (digitalRead(KeyInPin)) {                                        //It is high, I'm not transmitting
    if (PreviousKeying) {
      RadioCommand="C"+ String(SEQ).trim() + "|cw key 0 time=0x" + String(millis() % 0xFFFF, HEX).trim() + " index=" + String(CWIndex).trim() + " client_handle=0x" + ClientHandle + "\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      CWIndex++;
      SEQ++;
      PreviousKeying = false;
      digitalWrite(BuiltInLED, LOW);                                  // LED off
      if (TeensyDebug) {
        debugln("RX");
      }
      if (SidetoneActive) {
        SideTone.amplitude(0);
      }
    }
//    else {
//    }
  }
  else {                                                              //It is low, I'm transmitting
    if (PreviousKeying) {
    }
    else {
      RadioCommand="C"+ String(SEQ).trim() + "|cw key 1 time=0x" + String(millis() % 0xFFFF, HEX).trim() + " index=" + String(CWIndex).trim() + " client_handle=0x" + ClientHandle + "\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      CWIndex++;
      SEQ++;
      PreviousKeying = true;
      if (SidetoneActive) {
        SideTone.amplitude(SidetoneVolume);
      } 
      digitalWrite(BuiltInLED, HIGH);                                 // LED on
      if (TeensyDebug) {
        debugln("TX");
      }
    }
  }
  if (RadioTCPChannel.connected()) {
    if ( (ThisLoopTime-LastPingTime) > PingTimeInterval) {
      LastPingTime = ThisLoopTime;
      RadioCommand = "C" + String(SEQ).trim() + "|ping ms_timestamp=" + String(millis()).trim()+".0000\n";
      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
      SEQ++;
    }
  }
  else {
    RadioTCPChannel.stop();
    digitalWrite(BuiltInLED, HIGH);
    debugln("Radio disconnected");
    FlexConnected=false;
    ClientHandle="";

    while (!FlexConnected) { 
      if (TeensyDebug) {                       //Retry connecting forever
        debugln("Connecting Radio");
      }
      FlexConnected=connect(RadioIP, FlexPort);
      if (!FlexConnected) {
        delay(FlexDelay);                           
      }
    }
    FlexInit();
    send_K();
  }
} //END loop()

//---------------------------------------------
// Element (dot or space) length = 60000 (milliseconds in a minute) / (50 (elements of the world PARIS) * Speed (word per minute))
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

void send_K() {                                                  // _._ at speed of 40WPM
  SideTone.frequency(600.0);

  SideTone.amplitude(SidetoneVolume);
  delay (90);
  SideTone.amplitude(0.0);
  delay (30);
  SideTone.amplitude(SidetoneVolume);
  delay (30);
  SideTone.amplitude(0.0);
  delay (30);
  SideTone.amplitude(SidetoneVolume);
  delay (90);
  SideTone.amplitude(0.0);

  SideTone.frequency(SidetoneFrequency);
  }

void send_C() {                                                  // _._.
    SideTone.frequency(600.0);
    SideTone.amplitude(SidetoneVolume);
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
        ConnectionHandle=getMyHandler();
//      RadioCommand="C1|client ip\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  RadioCommand="C1|client program YACT6000\n";
  RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C3|client start_persistence 0\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
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
  while (ClientHandle=="") {
    if (TeensyDebug) {
      debugln("Waiting for SMARTSdr");
    }
    RadioCommand="C2|sub client\n";
    RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
    getClientHandler();
  }
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
//      RadioCommand="C29|client set enforce_network_mtu=1 network_mtu=1500\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C30|client set send_reduced_bw_dax=1\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C31|client udpport 4995\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C32|stream create netcw\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
//      RadioCommand="C33|display panf rfgain_info 0x0\n";
//      RadioTCPChannel.write(RadioCommand.c_str(), RadioCommand.length());
  digitalWrite(BuiltInLED, LOW);
  SEQ=3;
}

bool connect(IPAddress IP, uint16_t Port) {
  if (RadioTCPChannel.connect(IP, Port, 2000)) {
    return true;
  }
  else {
    return false;
  }
}

String getMyHandler () {
  delay (3000);
  while (RadioTCPChannel.available()) {
    char c=RadioTCPChannel.read();
    debug(c);
    if (c=='H') {
      for (unsigned int i=0;i<8;i++) {
        if (RadioTCPChannel.available()) {
          MyHandle[i]=RadioTCPChannel.read();
        }
        else {
          i--;
        }
      }
    MyHandle[8]='\0';
    debugln(MyHandle);
    return MyHandle;
    }
  }
}

String getClientHandler () {
  delay (FlexDelay);                           //Give some time to the radio to sync
  int index;
  String buffer;
  while (RadioTCPChannel.available()) {
    buffer=RadioTCPChannel.readStringUntil('\n');
    if (TeensyDebug) {
      debugln(buffer);
    }
    index = buffer.indexOf(F("|client 0x"));
    if (index>=0) {
      if (TeensyDebug) {
        debug("Index: ");
        debugln(index);
      }
      ClientHandle=buffer.substring(index+10,index+18);
    }
  }
  debugln(ClientHandle);
  return ClientHandle;
}

void OpenSerialMonitor() {
  Serial.begin(115200);
  while (!Serial) { 
    if (millis() > 2000) { 
      break;
    } 
  }
  Serial.print(F("MORCONI for Flex 6/8X00 series radios V."));
  Serial.print((APP_VERSION));
  Serial.println(F(" by N5NHJ"));
  Serial.println(F("----------------------"));
}