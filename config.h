const wlanSSID wifiNetworks[] {
    {"PI4RAZ","PI4RAZ_Zoetermeer"},
    {"Loretz_Gast", "Lor_Steg_98"}
};

Settings settings = {
    '$',                //chkDigit
    "YourSSID",         //wifiSSID[25];
    "WiFiPassword",     //wifiPass[25];
    63,                 //volume
    1,                  //isDAB
    1,                  //isStereo
    0,                  //isMuted
    0,                  //dabChannel
    0,                  //dabServiceID
    87500,              //fmFreq
    0,                  //memoryChannel
    0,                  //activeBtn
    100,                //currentBrightness
    0,                  //dabChannelsCount
    0,                  //dabChannelSelected
    0,                  //showOnlyCachedLogos
    0,                  //rotateScreen
    0,                  //rotateTouch
    0                   //isDebug
};

int screenRotation  = 3;                            // 0=0, 1=90, 2=180, 3=270
uint16_t calData[5] = { 378, 3473, 271, 3505, 7 };