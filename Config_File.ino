/***************************** GetConfigFile ***************************/
void getConfigFile() {
  if (!SD.begin(chipSelect)) {
    debugln("SD initialization failed!");
    return;
  }
  else {
    debugln("SD initialization success.");
  }

  // Open config file and list contents to serial port
  ConfigFile = SD.open("YACT6000.cfg");
  if (ConfigFile) {
    debugln("Reading YACT6000.cfg");
      // read from the file until there's nothing else in it:
    while (ConfigFile.available()) {
      Rchar = ConfigFile.read();
      InBuf += String(Rchar);
      if (Rchar == 10 || ConfigFile.available() <= 0) {
        debug("Config Line: "); debug(InBuf);
        ParseInBuf();
        InBuf = "";
      }
    }
    // close the file:
    ConfigFile.close();
  }
  else {
    // if the file didn't open, print an error:
    debugln("Error opening YACT6000.cfg");
  }
}

/***************************** ParseInBuf ***************************/
void ParseInBuf() {
  String tmpStr;
  InBuf          = InBuf.trim();
  String InBufLC = InBuf;
  InBuf          = InBuf.toUpperCase();

  if (InBuf.indexOf(";") == 0) {
    return;
  }
  if (InBuf.indexOf(";") > 0) {
    InBuf = InBuf.substring(0, InBuf.indexOf(";"));
  }
  if (InBuf.indexOf("STARTUP DELAY:") >= 0 && InSetup) {
    tmpStr       = InBuf.substring(InBuf.indexOf("STARTUP DELAY:") + 15);
    StartUpDelay = tmpStr.toInt();
    debug("Parsed StartUpDelay: "); debugln(StartUpDelay);
    return;
  }
  if (InBuf.indexOf("DEBOUNCE:") >= 0 && InSetup) {
    tmpStr      = InBuf.substring(InBuf.indexOf("DEBOUNCE:") + 10);
    Debounce = tmpStr.toInt();
    debug("Parsed Debounce: "); debugln(Debounce);
    return;
  }
  if (InBuf.indexOf("CW SIDETONE:") >= 0 && InSetup) {
    ST = InBuf.substring(InBuf.indexOf("CW SIDETONE:") + 13).trim() == "ON";
    debug("Parsed Sidetone: "); debugln(ST);
    return;
  }
  if (InBuf.indexOf("CW SIDETONE FREQ:") >= 0 && InSetup) {
    tmpStr = InBuf.substring(InBuf.indexOf("CW SIDETONE FREQ:") + 18).trim();
    STFreq = tmpStr.toInt();
    if (STFreq < 100) {STFreq = 100;}
    if (STFreq > 2000) {STFreq = 2000;}
    debug("Parsed STFreq: "); debugln(STFreq);
    return;
  }
  if (InBuf.indexOf("FLEXIP:") >= 0 && InSetup) {
    InBuf = InBuf.substring(InBuf.indexOf("FLEXIP:") + 8);
    ParseIP(InBuf, FlexIP);
    debugf4("Parsed FlexIP:  %u.%u.%u.%u\n", FlexIP[0],FlexIP[1],FlexIP[2],FlexIP[3]);
    return;
  }
  if (InBuf.indexOf("STATIC IP:") >= 0 && InSetup) {
    StaticIP = InBuf.substring(InBuf.indexOf("STATIC IP:") + 11).trim() == "TRUE";
    debug("Parsed Static IP: "); debugln(StaticIP);
    return;
  }
  if (InBuf.indexOf("TEENSYIP:") >= 0 && InSetup) {
    InBuf = InBuf.substring(InBuf.indexOf("TEENSYIP:") + 10);
    ParseIP(InBuf, CfgIP);
    debugf4("Parsed CfgIP:  %u.%u.%u.%u\n", CfgIP[0], CfgIP[1], CfgIP[2], CfgIP[3]);
    return;
  }
  if (InBuf.indexOf("TEENSYGATEWAY:") >= 0 && InSetup) {
    InBuf = InBuf.substring(InBuf.indexOf("TEENSYGATEWAY:") + 15);
    ParseIP(InBuf, CfgGateway);
    debugf4("Parsed MyGateway:  %u.%u.%u.%u\n", CfgGateway[0], CfgGateway[1], CfgGateway[2], CfgGateway[3]);
    return;
  }
  if (InBuf.indexOf("TEENSYMASK:") >= 0 && InSetup) {
    InBuf = InBuf.substring(InBuf.indexOf("TEENSYMASK:") + 12);
    ParseIP(InBuf, CfgMask);
    debugf4("Parsed MyMask:  %u.%u.%u.%u\n", CfgMask[0], CfgMask[1], CfgMask[2], CfgMask[3]);
    return;
  }
}

/***************************** ParseIP ***************************/
void ParseIP(String &IPstr, uint8_t IP[4]) {
  int DotIDX;
  String tmpStr;
  for (int i = 0; i < 4; i++) {
    if (IPstr.indexOf(".") >= 0) {
      DotIDX = IPstr.indexOf(".") + 1;
      tmpStr = IPstr.substring(0, DotIDX);
      IP[i]  = tmpStr.toInt();
      IPstr  = IPstr.substring(DotIDX);
    }
    else {
      IP[i] = IPstr.toInt();
    }
  }
}

