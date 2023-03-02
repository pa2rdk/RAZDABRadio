////////////////////////////////////////////////////////////
// V0.1
//
//
//  *********************************
//  **   Display connections       **
//  *********************************
//  |------------|------------------|
//  |Display 2.8 |      ESP32       |
//  |  ILI9341   |                  |
//  |------------|------------------|
//  |   Vcc      |     3V3          |  
//  |   GND      |     GND          |  
//  |   CS       |     15           |  
//  |   Reset    |      4           |  
//  |   D/C      |      2           | 
//  |   SDI      |     23           | 
//  |   SCK      |     18           | 
//  |   LED Coll.|     To Be arr.   | 
//  |   SDO      |                  | 
//  |   T_CLK    |     18           |
//  |   T_CS     |      5           |
//  |   T_DIN    |     23           | 
//  |   T_DO     |     19           |
//  |   T_IRQ    |     34           |
//  |------------|------------------|
////////////////////////////////////////////////////////////
#include <SPI.h>
#include <WiFi.h>
#include <WifiMulti.h>
#include <DABShield.h>
#include <EEPROM.h>
#include <TFT_eSPI.h>       // https://github.com/Bodmer/TFT_eSPI

#define offsetEEPROM       32
#define EEPROM_SIZE        2048

const char mode_0[]  = "Dual";
const char mode_1[]  = "Mono";
const char mode_2[]  = "Stereo";
const char mode_3[]  = "Joint Stereo";
const char *const audiomode[]  = {mode_0,mode_1,mode_2,mode_3};

#define BTN_NAV        32768
#define BTN_NEXT       16384
#define BTN_PREV        8192
#define BTN_CLOSE       4096
#define BTN_ARROW       2048
#define BTN_NUMERIC     1024

//#define DISPLAYLEDPIN   14

#define TFT_GREY 0x5AEB
#define TFT_LIGTHYELLOW 0xFF10
#define TFT_DARKBLUE 0x016F
#define TFT_SHADOW 0xE71C
#define TFT_BUTTONCOLOR 0xB5FE

typedef struct  // WiFi Access
{
  const char *SSID;
  const char *PASSWORD;
} wlanSSID;

typedef struct // Buttons
{
    const char *name;     // Buttonname
    const char *caption;  // Buttoncaption
    char waarde[12];  // Buttontext
    uint16_t pageNo;
    uint16_t xPos;          
    uint16_t yPos; 
    uint16_t width;
    uint16_t height;
    uint16_t btnColor;
    uint16_t bckColor;
} Button;

const Button buttons[] = {
    {"ToLeft","<<","",  BTN_ARROW,  2,208,154,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"ToRight",">>","", BTN_ARROW,162,208,154,30, TFT_BLUE, TFT_BUTTONCOLOR}, 

    {"Vol","Vol","",            1,  2,100, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Mute","Mute","",          1, 82,100, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Mode","Mode","",          1,162,100, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Off","Off","",            1,242,100, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},

    {"Tune","Tune","",          1,  2,136, 74,30, TFT_BLUE, TFT_WHITE},
    {"Service","Service","",    1, 82,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"MEM","MEM","",            1,162,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Save","Save","",        1,242,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},

    {"Info","Info","",          1,  2,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Light","Light","",        1, 82,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Calibrate","Calibrate","",1,162,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR}, 
    // {"Scan","Scan","",          1, 82,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    // {"Time","Time","",          1,162,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Stereo","Stereo","",      1,242,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},

    {"A001","1","",     BTN_NUMERIC, 42,100,74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"A002","2","",     BTN_NUMERIC,122,100,74,30, TFT_BLUE, TFT_BUTTONCOLOR},    
    {"A003","3","",     BTN_NUMERIC,202,100,74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"A004","4","",     BTN_NUMERIC, 42,136,74,30, TFT_BLUE, TFT_BUTTONCOLOR},  
    {"A005","5","",     BTN_NUMERIC,122,136,74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"A006","6","",     BTN_NUMERIC,202,136,74,30, TFT_BLUE, TFT_BUTTONCOLOR},    
    {"A007","7","",     BTN_NUMERIC, 42,172,74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"A008","8","",     BTN_NUMERIC,124,172,74,30, TFT_BLUE, TFT_BUTTONCOLOR}, 
    {"A009","9","",     BTN_NUMERIC,202,172,74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Clear","Clear","",BTN_NUMERIC, 42,208,74,30, TFT_BLUE, TFT_BUTTONCOLOR}, 
    {"A000","0","",     BTN_NUMERIC,122,208,74,30, TFT_BLUE, TFT_BUTTONCOLOR},       
    {"Enter","Enter","",BTN_NUMERIC,202,208,74,30, TFT_BLUE, TFT_BUTTONCOLOR},  

    // {"Prev","Prev","",     BTN_PREV,  2,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    // {"Next","Next","",     BTN_NEXT,242,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    // {"ToLeft","<<","",      BTN_NAV,  2,208,154,30, TFT_BLUE, TFT_BUTTONCOLOR},
    // {"ToRight",">>","",     BTN_NAV,162,208,154,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Navigate","Freq","",  BTN_NAV,  2,208,314,30, TFT_BLUE, TFT_BUTTONCOLOR},   
    {"Close","Close","",  BTN_CLOSE,122,208, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},     
};

typedef struct {
  byte chkDigit;
  char wifiSSID[25];
  char wifiPass[25];
  byte volume;
  bool isDab;
  bool isStereo;
  byte isMuted;
  uint8_t dabChannel;
  byte dabService;
  uint32_t fmFreq;
  byte currentBrightness;
  bool isDebug;
} Settings;

const int ledFreq          = 5000;
const int ledResol         = 8;
const int ledChannelforTFT = 0;

#include "config.h";     

DAB Dab;
TFT_eSPI tft = TFT_eSPI();            // Invoke custom library
DABTime dabtime;
WiFiMulti wifiMulti;

const byte slaveSelectPin = 12;
int actualPage = 1;
int lastPage = 2;
uint8_t activeBtn = -1;
int32_t keyboardNumber = 0;
char buf[300] = "\0";
long lastTime = millis();
long saveTime = millis();
long startTime = millis();
bool wifiAvailable = false;
bool wifiAPMode = false;

void setup() {
  // pinMode(DISPLAYLEDPIN, OUTPUT);
  // digitalWrite(DISPLAYLEDPIN, 0);

  Serial.begin(115200);
  while(!Serial);

  Serial.print(F("PI4RAZ DAB\n\n")); 
  Serial.print(F("Initializing....."));

  // ledcSetup(ledChannelforTFT, ledFreq, ledResol);
  // ledcAttachPin(DISPLAYLEDPIN, ledChannelforTFT);

  //Enable SPI
  pinMode(slaveSelectPin, OUTPUT);
  digitalWrite(slaveSelectPin, HIGH);
  SPI.begin(); 

  tft.init();
  tft.setRotation(screenRotation);

  tft.setTouch(calData);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  //DAB Setup
  Dab.setCallback(DrawServiceData);
  Dab.begin();

  if(Dab.error != 0)
  {
    Serial.print(F("ERROR: "));
    Serial.print(Dab.error);
    Serial.print(F("\nCheck DABShield is Connected and SPI Communications\n"));
  }
  else  
  {
    Serial.print(F("done\n\n")); 

    Serial.print(F("DAB>"));
  }

  if (!EEPROM.begin(EEPROM_SIZE)){
    DrawButton(80,120,160,30,"EEPROM Failed","",TFT_RED,TFT_WHITE,"");
    Serial.println("failed to initialise EEPROM"); 
    while(1);
  } 

  if (!LoadConfig()){
    Serial.println(F("Writing defaults"));
    SaveConfig();
    // Memory myMemory = {0, 0, 0, 0, 0, 0};
    // for (int x=0;x<10;x++){
    //   memories[x] = myMemory;
    // }
    // SaveMemories();
  }
  
  LoadConfig();
  // LoadMemories();

  // add Wi-Fi networks from All_Settings.h
  for (int i = 0; i < sizeof(wifiNetworks)/sizeof(wifiNetworks[0]); i++ ){
    wifiMulti.addAP(wifiNetworks[i].SSID, wifiNetworks[i].PASSWORD);
    Serial.printf("Wifi:%s, Pass:%s.",wifiNetworks[i].SSID, wifiNetworks[i].PASSWORD);
    Serial.println();
  }
  DrawButton(80,80,160,30,"Connecting to WiFi","",TFT_RED,TFT_WHITE,"");
  wifiMulti.addAP(settings.wifiSSID,settings.wifiPass);
  Serial.printf("Wifi:%s, Pass:%s.",settings.wifiSSID,settings.wifiPass);
  Serial.println();
  if (Connect2WiFi()){
    wifiAvailable=true;
    DrawButton(80,80,160,30,"Connected to WiFi",WiFi.SSID(),TFT_RED,TFT_WHITE,"");
    delay(1000);
  } else {
    wifiAPMode=true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP("APRSTRX", NULL);
  } 

  settings.isMuted=0;
  Dab.vol(settings.volume);
  Dab.mono(!settings.isStereo);
  if (settings.isDab){
    Dab.begin(0);
    Dab.tune(settings.dabChannel);
    Dab.set_service(settings.dabService);
    activeBtn=FindButtonIDByName("Service");
  } else {
    Dab.begin(1);
    Dab.tune((uint16_t)(settings.fmFreq/10));
    activeBtn=FindButtonIDByName("Tune");
  }
  DrawScreen();
}

bool Connect2WiFi(){
  startTime = millis();
  Serial.print("Connect to Multi WiFi");
  while (wifiMulti.run() != WL_CONNECTED && millis()-startTime<30000){
    // esp_task_wdt_reset();
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  return (WiFi.status() == WL_CONNECTED);
}

void loop() {
  delay(10);

  if (millis()-lastTime>1000 && actualPage<lastPage){
    DrawTime();
    lastTime= millis();
  }

  if (millis()-saveTime>10000){
    saveTime = millis();
    if (!CompareConfig()) SaveConfig();
  }

  uint16_t x = 0, y = 0;
  bool pressed = tft.getTouch(&x, &y);
  if (pressed) {
    int showVal = ShowControls();
    for (int i=0; i<sizeof(buttons)/sizeof(buttons[0]); i++) {
      if ((buttons[i].pageNo&showVal)>0){
        if (x >= buttons[i].xPos && x <= buttons[i].xPos+buttons[i].width && y >= buttons[i].yPos && y <= buttons[i].yPos+buttons[i].height){
          delay(50);
          HandleButton(buttons[i],x,y);
        }
      }
    }
  }

  Dab.task();
}


/***************************************************************************************
**            Draw screen
***************************************************************************************/
void DrawScreen(){
  DrawScreen(false);
}

void DrawScreen(bool drawAll){
  tft.fillScreen(TFT_BLACK);
  if (actualPage<lastPage || drawAll) DrawFrequency();
  if (actualPage==BTN_NUMERIC) DrawKeyboardNumber(false);
  DrawButtons();
}

void DrawTime(){
  if (settings.isDab){
    Dab.time(&dabtime);
    sprintf(buf,"%02d/%02d/%02d %02d:%02d", dabtime.Days,dabtime.Months,dabtime.Year, dabtime.Hours,dabtime.Minutes);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextPadding(tft.textWidth(buf));  // String width + margin
    tft.drawString(buf, 2, 4, 1);
  }
}

void DrawServiceData(){
  if (actualPage==1){
    tft.setTextDatum(ML_DATUM);
    if (settings.isDab)
    {
      sprintf(buf,"%s", Dab.ServiceData);
    }
    else
    {
      sprintf(buf, "%02d/%02d/%04d %02d:%02d", Dab.Days, Dab.Months, Dab.Year, Dab.Hours, Dab.Minutes);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setTextPadding(tft.textWidth(buf));  // String width + margin
      tft.drawString(buf, 2, 4, 1);
      sprintf(buf, "%s - %s", Dab.ps, Dab.ServiceData);
    }
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextPadding(tft.textWidth(buf));  // String width + margin
    tft.drawString(buf, 2, 14, 1);
  }
}

void DrawFrequency(){
  if (actualPage<lastPage){
    tft.fillRect(0,0,320,99,TFT_BLACK);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(TFT_GOLD, TFT_BLACK);
    if (settings.isDab && Dab.freq_index>0){
      sprintf(buf, "%03d.%03d", (uint16_t)(Dab.freq_khz(Dab.freq_index) / 1000),(uint16_t)(Dab.freq_khz(Dab.freq_index) % 1000));
      tft.setTextPadding(tft.textWidth(buf));
      tft.drawString(buf, 260,48,7);
      tft.setTextPadding(tft.textWidth("MHz"));
      tft.drawString("MHz", 315,50,4);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      if (Dab.numberofservices>0) tft.drawString(Dab.service[settings.dabService].Label, 315,85,4);
    }
    if (!settings.isDab){
        sprintf(buf, "%3d.%1d", (uint16_t)Dab.freq / 100, (uint16_t)(Dab.freq % 100)/10); 
        tft.setTextPadding(tft.textWidth(buf));
        tft.drawString(buf, 260,48,7);
        tft.setTextPadding(tft.textWidth("MHz"));
        tft.drawString("MHz", 315,50,4);
    }
    DrawStatus();

    // if (wifiAvailable || wifiAPMode){
    //   tft.setTextPadding(tft.textWidth(WiFi.SSID()));
    //   tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
    //   tft.drawString(WiFi.SSID(),315,4,1);
    // }
  }
}

void DrawPatience(){
  tft.fillRect(0,0,320,99,TFT_BLACK);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  sprintf(buf, "Even geduld...");
  tft.drawString(buf, 0,30,4);
}

void DrawStatus(){
  Serial.println("Status");
  if (!settings.isDab){
    sprintf(buf,"RSSI:%d, SNR:%d", Dab.signalstrength, Dab.snr);
  }

  if (settings.isDab){
    Dab.status();
    sprintf(buf,"RSSI:%d, SNR:%d, Quality:%d\%", Dab.signalstrength, Dab.snr, Dab.quality);
    //Dab.status();
    //sprintf(buf,"PTY = %S (%d), Bit Rate = %d kHz, Sample Rate = %d Hz, Audio Mode = %S (%d), Service Mode = %s, RSSI = %d, SNR = %d, Quality = %d%", pgm_read_word(&pty[Dab.pty]), Dab.pty, Dab.bitrate, Dab.samplerate, pgm_read_word(&audiomode[Dab.mode]), Dab.mode, Dab.dabplus == true ? PSTR("dab+") : PSTR("dab"), Dab.signalstrength, Dab.snr, Dab.quality);
  }

  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(buf));  // String width + margin
  tft.drawString(buf, 315, 4, 1);
  Serial.println(buf);
}

void DrawButtons(){
  for (int i=0; i<sizeof(buttons)/sizeof(buttons[0]); i++) {
    int showVal = ShowControls();
    if ((buttons[i].pageNo&showVal)>0){
      Button button = FindButtonInfo(buttons[i]);
      button.bckColor = TFT_BUTTONCOLOR;
      if (String(button.name) == FindButtonNameByID(activeBtn)) button.bckColor = TFT_WHITE;
      DrawButton(button.xPos,button.yPos,button.width,button.height,button.caption,button.waarde,button.btnColor,button.bckColor,button.name);
    }
  }
}

void DrawButton(String btnName){
  DrawButton(btnName,0);
}

void DrawButton(String btnName, uint16_t btnColor){
  int showVal = ShowControls();
  for (int i=0; i<sizeof(buttons)/sizeof(buttons[0]); i++) {
    if (String(buttons[i].name)==btnName && ((buttons[i].pageNo&showVal)>0)){
      Button button = FindButtonInfo(buttons[i]);
      if (btnColor==0) btnColor = button.btnColor; 
      if (String(button.name) == FindButtonNameByID(activeBtn)) button.bckColor = TFT_WHITE;
      DrawButton(button.xPos,button.yPos,button.width,button.height,button.caption,button.waarde,btnColor,button.bckColor,button.name);
    }
  }
}

void DrawButton(int xPos, int yPos, int width, int height, String caption, String waarde, uint16_t btnColor, uint16_t bckColor, String Name){
  tft.setTextDatum(MC_DATUM);
  DrawBox(xPos, yPos, width, height);

  tft.fillRoundRect(xPos + 2,yPos + 2, width-4, (height/2)+1, 3, bckColor);
  tft.setTextPadding(tft.textWidth(caption));
  tft.setTextColor(TFT_BLACK, bckColor);
  tft.drawString(caption, xPos + (width/2), yPos + (height/2)-5, 2);

  if (Name=="Navigate"){
    tft.setTextDatum(ML_DATUM);
    tft.setTextPadding(tft.textWidth("     <<     <"));
    tft.setTextColor(TFT_BLACK, bckColor);
    tft.drawString("     <<     <", 5, yPos + (height/2)-5, 2);
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth(">     >>     "));
    tft.setTextColor(TFT_BLACK, bckColor);
    tft.drawString(">     >>     ", 309, yPos + (height/2)-5, 2);
  }

  tft.fillRoundRect(xPos + 2,yPos + 2 + (height/2), width-4, (height/2)-4, 3,btnColor);
  tft.fillRect(xPos + 2,yPos + 2 + (height/2), width-4, 2, TFT_DARKBLUE);

  if (waarde!=""){
    tft.setTextPadding(tft.textWidth(waarde));
    tft.setTextColor(TFT_WHITE, btnColor);
    tft.drawString(waarde, xPos + (width/2), yPos + (height/2)+9, 1);
  }
}

int ShowControls(){
  int retVal = actualPage;
  if (actualPage == 1) retVal = retVal + BTN_ARROW + BTN_NAV;
  // if (actualPage >1 && actualPage <lastPage) retVal = retVal + BTN_ARROW + BTN_NEXT + BTN_PREV;
  if (actualPage == lastPage) retVal = retVal + BTN_CLOSE;
  return retVal;
}

Button FindButtonByName(String Name){
  for (int i=0; i<sizeof(buttons)/sizeof(buttons[0]); i++) {
    if (String(buttons[i].name)==Name) return FindButtonInfo(buttons[i]);
  }
  return buttons[0];
}

int8_t FindButtonIDByName(String Name){
  for (int i=0; i<sizeof(buttons)/sizeof(buttons[0]); i++) {
    if (String(buttons[i].name)==Name) return i;
  }
  return -1;
}

String FindButtonNameByID(int8_t id){
  return String(buttons[id].name);
}

Button FindButtonInfo(Button button){
  char buttonBuf[10] = "\0";

  if (button.name=="Vol") {
    sprintf(buttonBuf,"%d",settings.volume);
    strcpy(button.waarde,buttonBuf);
  }

  if (button.name=="Mute"){
    if (settings.isMuted==0) strcpy(button.waarde,"");
    if (settings.isMuted==1) strcpy(button.waarde,"Right");
    if (settings.isMuted==2) strcpy(button.waarde,"Left");
    if (settings.isMuted==3) strcpy(button.waarde,"Muted");
    button.btnColor = settings.isMuted?TFT_RED:TFT_BLUE;
  }

  if (button.name=="Mode") {  
    sprintf(buttonBuf,"%s",settings.isDab?"DAB":"FM");
    strcpy(button.waarde,buttonBuf);
    button.btnColor = settings.isDab?TFT_RED:TFT_GREEN;
  }

  if (button.name=="Tune") {
    if (settings.isDab){
      sprintf(buttonBuf,"%d",settings.dabChannel);
      strcpy(button.waarde,buttonBuf);
    }
  }

  if (button.name=="Service") {
    sprintf(buttonBuf,"        ");
    if (settings.isDab && Dab.numberofservices>0){
      sprintf(buttonBuf,"%d/%d",settings.dabService,Dab.numberofservices - 1);
    } 
    strcpy(button.waarde,buttonBuf);
    button.btnColor = settings.isDab?TFT_RED:TFT_BLUE;
  }

  if (button.name=="Stereo") {  
    sprintf(buttonBuf,"%s",settings.isStereo?"Stereo":"Mono");
    strcpy(button.waarde,buttonBuf);
    button.btnColor = settings.isStereo?TFT_GREEN:TFT_BLUE;
  }

  if (button.name=="Light") {
    sprintf(buttonBuf,"%d",settings.currentBrightness);
    strcpy(button.waarde,buttonBuf);
  }

  if (button.name=="Navigate") {
    sprintf(buttonBuf,"%s", FindButtonNameByID(activeBtn));
    button.caption = buttonBuf;
  }
  return button;
}

void DrawBox(int xPos, int yPos, int width, int height){
  tft.drawRoundRect(xPos+2,yPos+2,width,height, 4, TFT_SHADOW);
  tft.drawRoundRect(xPos,yPos,width,height, 3, TFT_WHITE);
  tft.fillRoundRect(xPos + 1,yPos + 1, width-2, height-2, 3, TFT_BLACK);
}

void DrawKeyboardNumber(bool doReset){
  tft.setTextDatum(MC_DATUM);
  sprintf(buf,"%s",FindButtonNameByID(activeBtn));
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(buf,162,15,4);

  tft.setTextDatum(MR_DATUM);
  if (doReset) keyboardNumber = 0;
  int f1 = floor(keyboardNumber/10000);
  int f2 = keyboardNumber-(f1*10000);
  sprintf(buf,"%1d",keyboardNumber);
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(buf, 282,60,7);
}
/***************************************************************************************
**            Handle button
***************************************************************************************/
void HandleButton(String buttonName){
  HandleButton(buttonName, true);
}

void HandleButton(String buttonName, bool doDraw){
  Button button = FindButtonByName(buttonName);
  HandleButton(button,0,0, doDraw);
}

void HandleButton(String buttonName, int x, int y){
  HandleButton(buttonName, x, y, true);
}

void HandleButton(String buttonName, int x, int y, bool doDraw){
  Button button = FindButtonByName(buttonName);
  HandleButton(button, x, y, doDraw);
}

void HandleButton(Button button, int x, int y){
  HandleButton(button, x, y, true);
}

void HandleButton(Button button, int x, int y, bool doDraw){
  if (button.name=="ToRight"){
    if (activeBtn==FindButtonIDByName("Tune")){
      DrawPatience();
      if (settings.isDab && settings.dabChannel<DAB_FREQS - 1){
        settings.dabChannel++;
        if (doDraw) DrawButton("Tune");
        Dab.tune(settings.dabChannel);
        while (Dab.numberofservices<1 && settings.dabChannel<DAB_FREQS - 1){
          settings.dabChannel++;
          if (doDraw) DrawButton("Tune");
          Dab.tune(settings.dabChannel);
        }
        if (Dab.numberofservices<1 && settings.dabChannel==DAB_FREQS - 1){
          while (Dab.numberofservices<1 && settings.dabChannel>0){
            settings.dabChannel--;
            if (doDraw) DrawButton("Tune");
            Dab.tune(settings.dabChannel);
          }
        }
        settings.dabService = 0;
        Dab.set_service(settings.dabService);
        activeBtn=FindButtonIDByName("Service");
        Serial.printf("Tune:%d, Service:%d, Channels:%d",settings.dabChannel,settings.dabService,DAB_FREQS);
      } 
      if (!settings.isDab){
        Dab.seek(1,1);
        settings.fmFreq = Dab.freq*10;
        Serial.println(settings.fmFreq);
      }
      if (doDraw) DrawButtons();
      DrawFrequency();
      delay(200);
    }
    else if (activeBtn==FindButtonIDByName("Service")){
      if (settings.isDab && settings.dabService<Dab.numberofservices - 1) settings.dabService++;
      Dab.set_service(settings.dabService);
      if (doDraw) DrawButton("Service");
      DrawFrequency();
      delay(200);
    }
    else if (activeBtn==FindButtonIDByName("Vol")){
      if (settings.volume<63) settings.volume++;
      Dab.vol(settings.volume);
      settings.isMuted=0;
      if (doDraw) DrawButton("Vol");
      if (doDraw) DrawButton("Mute");
    }
    else if (activeBtn==FindButtonIDByName("Light")){
      if (settings.currentBrightness<100) settings.currentBrightness+=5;
      if (settings.currentBrightness>100) settings.currentBrightness=100;
      if (doDraw) DrawButton("Light");
      //ledcWrite(ledChannelforTFT, 256-(settings.currentBrightness*2.56));
    }
    if (doDraw) DrawButton("Navigate");
  }

  if (button.name=="ToLeft"){
    if (activeBtn==FindButtonIDByName("Tune")){
      DrawPatience();
      if (settings.isDab && settings.dabChannel>0){
        settings.dabChannel--;
        Dab.tune(settings.dabChannel);
        while (Dab.numberofservices<1 && settings.dabChannel>0){
          settings.dabChannel--;
          if (doDraw) DrawButton("Tune");
          Dab.tune(settings.dabChannel);
        }
        if (Dab.numberofservices<1 && settings.dabChannel==0){
          while (Dab.numberofservices<1 && settings.dabChannel<DAB_FREQS - 1){
            settings.dabChannel++;
            if (doDraw) DrawButton("Tune");
            Dab.tune(settings.dabChannel);
          }
        }
        settings.dabService = 0;
        Dab.set_service(settings.dabService);
        activeBtn=FindButtonIDByName("Service");
        Serial.printf("Tune:%d, Service:%d, Channels:%d",settings.dabChannel,settings.dabService,DAB_FREQS);
      } 
      if (!settings.isDab){
        Dab.seek(0,1);
        settings.fmFreq = Dab.freq*10;
      }
      if (doDraw) DrawButtons();
      DrawFrequency();
      delay(200);
    }
    else if (activeBtn==FindButtonIDByName("Service")){
      if (settings.isDab && settings.dabService>0) settings.dabService--;
      Dab.set_service(settings.dabService);
      if (doDraw) DrawButton("Service");
      DrawFrequency();
      delay(200);
    }
    else if (activeBtn==FindButtonIDByName("Vol")){
      if (settings.volume>0) settings.volume--;
      Dab.vol(settings.volume);
      settings.isMuted=0;
      if (doDraw) DrawButton("Vol");
      if (doDraw) DrawButton("Mute");
    }
    else if (activeBtn==FindButtonIDByName("Light")){
      if (settings.currentBrightness>5) settings.currentBrightness-=5;
      if (settings.currentBrightness<5) settings.currentBrightness=5;
      if (doDraw) DrawButton("Light");
      //ledcWrite(ledChannelforTFT, 256-(settings.currentBrightness*2.56));
    }
    if (doDraw) DrawButton("Navigate");
  }

  if (button.name=="Vol"){
    if (activeBtn==FindButtonIDByName("Vol")){
      keyboardNumber = settings.volume;
      actualPage = BTN_NUMERIC;
      if (doDraw) DrawScreen();
    } else {
      activeBtn=FindButtonIDByName("Vol");
      if (doDraw) DrawButtons();
    }
  }

  if (button.name=="Mute") {
    settings.isMuted++;
    if (settings.isMuted>3) settings.isMuted=0;
    if (settings.isMuted==0) Dab.mute(false,false);
    if (settings.isMuted==1) Dab.mute(true,false);
    if (settings.isMuted==2) Dab.mute(false,true);
    if (settings.isMuted==3) Dab.mute(true,true);
    if (doDraw) DrawButton("Mute");
  }

  if (button.name=="Mode") {
    DrawPatience();
    settings.isDab = !settings.isDab;
    if (settings.isDab){
      Dab.begin(0);
      Dab.tune(settings.dabChannel);
      Dab.set_service(settings.dabService);
      activeBtn=FindButtonIDByName("Service");
    } else {
      Dab.begin(1);
      Dab.tune((uint16_t)(settings.fmFreq/10));
      activeBtn=FindButtonIDByName("Tune");
    }
    if (doDraw) DrawScreen();
  }

  if (button.name=="Tune"){
    if (settings.isDab){
      if (activeBtn==FindButtonIDByName("Tune")){
        keyboardNumber = settings.dabChannel;
        actualPage = BTN_NUMERIC;
        if (doDraw) DrawScreen();
      } else {
        activeBtn=FindButtonIDByName("Tune");
        if (doDraw) DrawButtons();
      }
    } else {
      if (activeBtn==FindButtonIDByName("Tune")){
        keyboardNumber = 0;
        actualPage = BTN_NUMERIC;
        if (doDraw) DrawScreen();
      } else {
        activeBtn=FindButtonIDByName("Tune");
        if (doDraw) DrawButtons();
      }
    }
  }

  if(button.name =="Info"){
    if (settings.isDab){
      actualPage=lastPage;
      DrawScreen();

      tft.fillRect(2,2,320,200,TFT_BLACK);
      tft.setTextDatum(ML_DATUM);
      tft.setTextPadding(50);
      tft.setTextColor(TFT_YELLOW,TFT_BLACK);

      tft.drawString("Service:",2,10,2);
      tft.setTextColor(TFT_GREEN,TFT_BLACK);
      for (int i=0;i<Dab.numberofservices;i++){
        sprintf(buf,"%2d = %s",i, Dab.service[i].Label);
        tft.drawString(buf,2,25+(i*8),1);
      }
    }
  }

  if (button.name=="Service"){
    if (settings.isDab){
      if (activeBtn==FindButtonIDByName("Service")){
        keyboardNumber = settings.dabService;
        actualPage = BTN_NUMERIC;
        if (doDraw) DrawScreen();
      } else {
        activeBtn=FindButtonIDByName("Service");
        if (doDraw) DrawButtons();
      }
    }
  }

  if (button.name=="Stereo") {
    settings.isStereo = !settings.isStereo;
    Dab.mono(!settings.isStereo);
    if (doDraw) DrawButton("Stereo");
  }

  if (button.name=="Calibrate"){
    TouchCalibrate();
    actualPage=1;
    if (doDraw) DrawScreen();
  }

  if (button.name=="Light"){
    activeBtn=FindButtonIDByName("Light");
    if (doDraw) DrawButtons();
  }

  if (button.name=="Next") {
    actualPage = actualPage<<1;
    if (actualPage>lastPage) actualPage=1;
    if (doDraw) DrawScreen();
  }

  if (button.name=="Prev") {
    if (actualPage>1) actualPage = actualPage>>1;
    if (doDraw) DrawScreen();
  }

  if (button.name=="Close") {
    actualPage=1;
    if (doDraw) DrawScreen();
  }

  if (button.name=="Off") {
    delay(500);
  }

  if (String(button.name).substring(0,3) =="A00") {
    int i = String(button.name).substring(3).toInt();
    keyboardNumber = (keyboardNumber*10)+i;

    if (activeBtn==FindButtonIDByName("Tune")){
      if (settings.isDab && keyboardNumber>=DAB_FREQS) keyboardNumber=0;
      if (!settings.isDab && keyboardNumber>107900) keyboardNumber=0;
    } 
    if (activeBtn==FindButtonIDByName("Vol")){
      if (keyboardNumber>63) keyboardNumber=0;
    } 
    if (activeBtn==FindButtonIDByName("Service")){
      if (keyboardNumber>Dab.numberofservices - 1) keyboardNumber=0;
    } 
    if (doDraw) DrawKeyboardNumber(false);
  }

  if (button.name=="Enter") {
    if (activeBtn==FindButtonIDByName("Vol")){
      settings.volume=keyboardNumber;
      Dab.vol(settings.volume);
    }
    if (activeBtn==FindButtonIDByName("Tune")){
      if (settings.isDab){
        DrawPatience();
        settings.dabChannel=keyboardNumber;
        settings.dabService=0;
        Dab.tune(settings.dabChannel);
        Dab.set_service(settings.dabService);
      }
      if (!settings.isDab){
        while (keyboardNumber<87500) keyboardNumber*=10;
        if (keyboardNumber>=87500 && keyboardNumber<=107900){
          DrawPatience();
          settings.fmFreq = keyboardNumber;
          Dab.tune((uint16_t)(settings.fmFreq/10));
        }
      }
    } 
    if (activeBtn==FindButtonIDByName("Service")){
      if (settings.isDab){
        settings.dabService=keyboardNumber;
        Dab.set_service(settings.dabService); 
      }
    } 

    actualPage = 1;
    if (doDraw) DrawScreen();
  }

  if (button.name=="Clear") {
    actualPage = 1;
    if (doDraw) DrawScreen();
  }
}

/***************************************************************************************
**            EEPROM Routines
***************************************************************************************/
bool SaveConfig() {
  for (unsigned int t = 0; t < sizeof(settings); t++)
    EEPROM.write(offsetEEPROM + t, *((char*)&settings + t));
  EEPROM.commit();
  Serial.println("Configuration:saved");
  return true;
}

bool LoadConfig() {
  bool retVal = true;
  if (EEPROM.read(offsetEEPROM + 0) == settings.chkDigit){
    for (unsigned int t = 0; t < sizeof(settings); t++)
      *((char*)&settings + t) = EEPROM.read(offsetEEPROM + t);
  } else retVal = false;
  Serial.println("Configuration:" + retVal?"Loaded":"Not loaded");
  return retVal;
}

bool CompareConfig() {
  bool retVal = true;
  for (unsigned int t = 0; t < sizeof(settings); t++)
    if (*((char*)&settings + t) != EEPROM.read(offsetEEPROM + t)) retVal = false;
  return retVal;
}

/***************************************************************************************
**            De rest
***************************************************************************************/
void DABSpiMsg(unsigned char *data, uint32_t len)
{
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));    //2MHz for starters...
  digitalWrite (slaveSelectPin, LOW);
  SPI.transfer(data, len);
  digitalWrite (slaveSelectPin, HIGH);
  SPI.endTransaction();
}

/***************************************************************************************
**            Calibrate touch
***************************************************************************************/
void TouchCalibrate(){
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // Calibrate
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 0);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.println("Touch corners as indicated");

  tft.setTextFont(1);
  tft.println();

  tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

  Serial.println(); Serial.println();
  Serial.println("// Use this calibration code in setup():");
  Serial.print("  uint16_t calData[5] = ");
  Serial.print("{ ");

  for (uint8_t i = 0; i < 5; i++)
  {
    Serial.print(calData[i]);
    if (i < 4) Serial.print(", ");
  }

  Serial.println(" };");
  Serial.print("  tft.setTouch(calData);");
  Serial.println(); Serial.println();

  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Calibration complete!");
  tft.println("Calibration code sent to Serial port.");

  delay(4000);
}
