const wlanSSID wifiNetworks[] {
    {"PI4RAZ","PI4RAZ_Zoetermeer"},
    {"Loretz_Gast", "Lor_Steg_98"},
    {"MARODEKWiFi2", "MAROWiFi19052004!2"},
    {"FHanna", "Kleinsteg71"},
    {"iPhone van Robert", "0919932003"}
};

Settings settings = {
    '@',                //chkDigit
    "MARODEKWiFi",      //wifiSSID[25];
    "MAROWiFi19052004!",//wifiPass[25];
    63,                 //volume
    1,                  //isDAB
    1,                  //isStereo
    0,                  //isMuted
    0,                  //dabChannel
    0,                  //dabService
    87500,              //fmFreq
    100,                //currentBrightness
    0                   //isDebug
};

int screenRotation  = 1;                            // 0=0, 1=90, 2=180, 3=270
uint16_t calData[5] = { 378, 3473, 271, 3505, 7 };