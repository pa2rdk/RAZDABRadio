////////////////////////////////////////////////////////////
// V0.63 SNR meter
// V0.62 Solved volume bug
// V0.61 Solved error in memory display
// V0.6 Select memory position
// V0.5 Layout changes, Memories, Meters
// V0.4 Layout changes, Memories, Extended info
// V0.3 Better tuning
// V0.2 BackLight
// V0.1 Initial
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
//  |   LED Coll.|     13.   | 
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

#define DISPLAYLEDPIN   13

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

typedef struct {
  bool isDab;
  bool isStereo;
  uint8_t dabChannel;
  uint16_t dabServiceID;
  uint32_t fmFreq;
} Memory;

typedef struct {
  byte chkDigit;
  char wifiSSID[25];
  char wifiPass[25];
  byte volume;
  bool isDab;
  bool isStereo;
  byte isMuted;
  uint8_t dabChannel;
  uint16_t dabServiceID;
  uint32_t fmFreq;
  byte memoryChannel;
  byte activeBtn;
  byte currentBrightness;
  bool isDebug;
} Settings;

const Button buttons[] = {
    {"ToLeft","<<","",  BTN_ARROW,  2,208,154,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"ToRight",">>","", BTN_ARROW,162,208,154,30, TFT_BLUE, TFT_BUTTONCOLOR}, 

    {"Vol","Vol","",            1,  2,100, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Mute","Mute","",          1, 82,100, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Mode","Mode","",          1,162,100, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Off","Off","",            1,242,100, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},

    {"Tune","Scan","",          1,  2,136, 74,30, TFT_BLUE, TFT_WHITE},
    {"Service","Select","",    1, 82,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
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

const int ledFreq          = 5000;
const int ledResol         = 8;
const int ledChannelforTFT = 0;

#include "rdk_config.h";                  // Change to config.h  

DAB Dab;
TFT_eSPI tft = TFT_eSPI();            // Invoke custom library
DABTime dabtime;
WiFiMulti wifiMulti;

const byte slaveSelectPin = 12;
int actualPage = 1;
int lastPage = 2;
int32_t keyboardNumber = 0;
char buf[300] = "\0";
long lastTime = millis();
long saveTime = millis();
long startTime = millis();
long pressTime = millis();
bool wifiAvailable = false;
bool wifiAPMode = false;
bool isOn = true;
char actualInfo[100] = "\0";
char lastInfo[100] = "\0";
int services[20] = {};

bool actualIsDab = false;
byte actualDabVol = 0;
bool actualIsStereo = true;
byte actualDabChannel = 0;
int actualDabService = 0; 

uint32_t actualFmFreq = 87500;
Memory memories[10] = {};

void setup() {
  pinMode(DISPLAYLEDPIN, OUTPUT);
  digitalWrite(DISPLAYLEDPIN, 0);

  Serial.begin(115200);
  while(!Serial);

  Serial.print(F("PI4RAZ DAB\n\n")); 
  Serial.print(F("Initializing....."));

  ledcSetup(ledChannelforTFT, ledFreq, ledResol);
  ledcAttachPin(DISPLAYLEDPIN, ledChannelforTFT);

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
    DrawButton(80,120,160,30,"DAB Error","",TFT_RED,TFT_WHITE,"");
    Serial.println("failed to initialise DAB shield"); 
    while(1);
  }

  if (!EEPROM.begin(EEPROM_SIZE)){
    DrawButton(80,120,160,30,"EEPROM Failed","",TFT_RED,TFT_WHITE,"");
    Serial.println("failed to initialise EEPROM"); 
    while(1);
  } 

  if (!LoadConfig()){
    Serial.println(F("Writing defaults"));
    SaveConfig();
    Memory myMemory = {0,1,0,0,87500};
    for (int x=0;x<10;x++){
      memories[x] = myMemory;
    }
    SaveMemories();
  }
  
  LoadConfig();
  LoadMemories();

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
  SetRadio(true);
  ledcWrite(ledChannelforTFT, 256-(settings.currentBrightness*2.56));
  DrawScreen();
}

void DisplaySettings(String displayText){
  Serial.println();
  Serial.println(displayText);
  Serial.println();
  Serial.printf("Active Button = %d",settings.activeBtn); Serial.println();
  Serial.printf("Brighness = %d",settings.currentBrightness); Serial.println();  
  Serial.printf("Dab channel = %d",settings.dabChannel); Serial.println();    
  Serial.printf("Dab serviceID = %d",settings.dabServiceID); Serial.println();
  Serial.printf("Dab service = %d",actualDabService); Serial.println();
  Serial.printf("FM Freq = %d",settings.fmFreq); Serial.println();  
  Serial.printf("Is Dab = %s",settings.isDab?"True":"False"); Serial.println();
  Serial.printf("Is Stereo = %s",settings.isStereo?"True":"False"); Serial.println();
  Serial.printf("Is Muted = %d",settings.isMuted); Serial.println(); 
}

byte findChannelOnDabServiceID(uint16_t dabServiceID){
  Serial.printf("DabService:%d, QTY:%d",dabServiceID, Dab.numberofservices);
  Serial.println();
  for (int i=0;i<Dab.numberofservices;i++){
    if (Dab.service[i].ServiceID == dabServiceID) return i;
  }
  return 0;
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
  if (millis()-lastTime>1000 && actualPage<lastPage){
    DrawTime();
    DrawStatus();
    lastTime= millis();
  }

  if (millis()-saveTime>2000){
    saveTime = millis();
    if (!CompareConfig()) SaveConfig();
  }

  if (millis()-pressTime>100){
    uint16_t x = 0, y = 0;
    bool pressed = tft.getTouch(&x, &y);
    if (pressed) {
      int showVal = ShowControls();
      for (int i=0; i<sizeof(buttons)/sizeof(buttons[0]); i++) {
        if ((buttons[i].pageNo&showVal)>0){
          if (x >= buttons[i].xPos && x <= buttons[i].xPos+buttons[i].width && y >= buttons[i].yPos && y <= buttons[i].yPos+buttons[i].height){
            HandleButton(buttons[i],x,y);
            pressTime = millis();
          }
        }
      }
    }
  }

  Dab.task();
  WaitForWakeUp();
}


/***************************************************************************************
**            Draw screen
***************************************************************************************/
void DrawScreen(){
  DrawScreen(false);
  DisplaySettings("Draw Screen");
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
      sprintf(actualInfo,"%s", Dab.ServiceData);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setTextPadding(tft.textWidth(actualInfo));  // String width + margin
      tft.fillRect(2,60,318,10,TFT_BLACK);
      tft.drawString(actualInfo, 2, 65, 2);
    }
    else
    {
      sprintf(actualInfo, "%02d/%02d/%04d %02d:%02d", Dab.Days, Dab.Months, Dab.Year, Dab.Hours, Dab.Minutes);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setTextPadding(tft.textWidth(actualInfo));  // String width + margin
      tft.drawString(actualInfo, 2, 4, 1);

      sprintf(actualInfo, "%s - %s", Dab.ps, Dab.ServiceData);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setTextPadding(tft.textWidth(actualInfo));  // String width + margin
      tft.fillRect(2,11,318,6,TFT_BLACK);
      tft.drawString(actualInfo, 2, 14, 1);
    }
    if (strcmp(lastInfo,actualInfo)!=0){
      Serial.println("Old and new differ");
      strcpy(lastInfo,actualInfo);
      Serial.println(actualInfo);
    } else {
      //Serial.println("Old and new are same");
    }
  }
}

void DrawFrequency(){
  if (actualPage<lastPage){
    tft.fillRect(0,0,320,99,TFT_BLACK);

    if (settings.isDab && Dab.freq_index>0){
      tft.setTextColor(TFT_BLUE, TFT_BLACK);
      tft.setTextDatum(ML_DATUM);
      sprintf(buf, "%03d.%03d MHz", (uint16_t)(Dab.freq_khz(Dab.freq_index) / 1000),(uint16_t)(Dab.freq_khz(Dab.freq_index) % 1000));
      tft.setTextPadding(tft.textWidth(buf));
      tft.drawString(buf, 2,14,1);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      Serial.printf("Draw fequency:%d,%d,%s\r\n",Dab.service[actualDabService].ServiceID,actualDabService,Dab.service[actualDabService].Label);
      if (Dab.numberofservices>0) tft.drawString(Dab.service[actualDabService].Label, 2,45,4);
    }
    if (!settings.isDab){
        tft.setTextColor(TFT_GOLD, TFT_BLACK);
        tft.setTextDatum(MR_DATUM);
        sprintf(buf, "%3d.%1d", (uint16_t)Dab.freq / 100, (uint16_t)(Dab.freq % 100)/10); 
        tft.setTextPadding(tft.textWidth(buf));
        tft.drawString(buf, 260,48,7);
        tft.setTextPadding(tft.textWidth("MHz"));
        tft.drawString("MHz", 315,50,4);
    }
    DrawStatus();
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
  if (!settings.isDab){
    sprintf(buf,"   RSSI:%d, SNR:%d", Dab.signalstrength, Dab.snr);
  }

  if (settings.isDab){
    Dab.status();
    sprintf(buf,"   RSSI:%d, SNR:%d, Quality:%d\%", Dab.signalstrength, Dab.snr, Dab.quality);
  }
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(buf));  // String width + margin
  tft.drawString(buf, 315, 4, 1);

  DrawMeter(2,78,154,16,Dab.signalstrength,10,75,50,75,"RSSI:");
  DrawMeter(162,78,154,16,Dab.snr,0,20,50,75,"SNR:");
}

void DrawButtons(){
  for (int i=0; i<sizeof(buttons)/sizeof(buttons[0]); i++) {
    int showVal = ShowControls();
    if ((buttons[i].pageNo&showVal)>0){
      Button button = FindButtonInfo(buttons[i]);
      button.bckColor = TFT_BUTTONCOLOR;
      if (String(button.name) == FindButtonNameByID(settings.activeBtn)) button.bckColor = TFT_WHITE;
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
      if (String(button.name) == FindButtonNameByID(settings.activeBtn)) button.bckColor = TFT_WHITE;
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

  if (button.name=="MEM") {
    sprintf(buttonBuf,"%d",settings.memoryChannel);
    strcpy(button.waarde,buttonBuf);
  }

  if (button.name=="Save") {
    button.btnColor = settings.activeBtn==FindButtonIDByName("MEM")?TFT_GREY:TFT_BLUE;
  }

  if (button.name=="Service") {
    sprintf(buttonBuf,"        ");
    if (settings.isDab && Dab.numberofservices>0){
      sprintf(buttonBuf,"%d/%d",actualDabService,Dab.numberofservices - 1);
    } 
    strcpy(button.waarde,buttonBuf);
    button.btnColor = settings.isDab?TFT_BLUE:TFT_GREY;
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
    sprintf(buttonBuf,"%s", FindButtonNameByID(settings.activeBtn));
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
  sprintf(buf,"%s",FindButtonNameByID(settings.activeBtn));
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(buf,162,15,4);

  tft.setTextDatum(MR_DATUM);
  if (doReset) keyboardNumber = 0;
  int f1 = floor(keyboardNumber/10000);
  int f2 = keyboardNumber-(f1*10000);
  sprintf(buf,"%8d",keyboardNumber);
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(buf, 282,60,7);
}

/***************************************************************************************
**            Draw meter
***************************************************************************************/
void DrawMeter(int xPos, int yPos, int width, int height, int value, int minValue, int maxValue, int orangeColorPCT, int greenColorPCT,String dispText){

  tft.setTextDatum(MC_DATUM);
  DrawBox(xPos, yPos, width, height);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  int scale = ((width - 6)*10) / (maxValue-minValue);
  for (int i = 0; i<maxValue-minValue;i++){
    float mPos = ((i*scale)/10)+3+xPos;
    uint16_t signColor = TFT_RED;
    if (i<=value){
      if (i>(((maxValue-minValue)*orangeColorPCT)/100)) signColor = TFT_ORANGE;
      if (i>(((maxValue-minValue)*greenColorPCT)/100)) signColor = TFT_GREEN;
      tft.fillRect(mPos, yPos+2, (scale/10)+1, height-4, signColor);
    }
  }
  sprintf(buf,"%s%d",dispText,value);
  tft.setTextDatum(ML_DATUM);
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.drawString(buf, xPos+5,yPos+(height/2),1);
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
    if (settings.activeBtn==FindButtonIDByName("Tune")){
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
        actualDabChannel = settings.dabChannel;
        actualDabService = 0;
        settings.dabServiceID = Dab.service[actualDabService].ServiceID;
        SetRadio();
        settings.activeBtn=FindButtonIDByName("Service");
        Serial.printf("Tune:%d, Service:%d, Channels:%d",settings.dabChannel,actualDabService,DAB_FREQS);
      } 
      if (!settings.isDab){
        Dab.seek(1,1);
        settings.fmFreq = Dab.freq*10;
        Serial.println(settings.fmFreq);
        DrawFrequency();
      }
      if (doDraw) DrawButtons();
    }
    else if (settings.activeBtn==FindButtonIDByName("Service")){
      if (settings.isDab && actualDabService<Dab.numberofservices - 1){
        actualDabService++;
        settings.dabServiceID = Dab.service[actualDabService].ServiceID;
        SetRadio();
      }
      if (actualDabService==Dab.numberofservices - 1){
        settings.activeBtn=FindButtonIDByName("Tune");
      }
      if (doDraw) DrawButtons();
    }
    else if (settings.activeBtn==FindButtonIDByName("Vol")){
      if (settings.volume<63) settings.volume++;
      Dab.vol(settings.volume);
      settings.isMuted=0;
      if (doDraw) DrawButton("Vol");
      if (doDraw) DrawButton("Mute");
    }
    else if (settings.activeBtn==FindButtonIDByName("MEM")){
      if (settings.memoryChannel<9){
        settings.memoryChannel++;
        SetRadioFromMemory(settings.memoryChannel);
        if (doDraw) DrawFrequency();
        if (doDraw) DrawButton("MEM");
      }
    }
    else if (settings.activeBtn==FindButtonIDByName("Light")){
      if (settings.currentBrightness<100) settings.currentBrightness+=5;
      if (settings.currentBrightness>100) settings.currentBrightness=100;
      if (doDraw) DrawButton("Light");
      ledcWrite(ledChannelforTFT, 256-(settings.currentBrightness*2.56));
    }
    if (doDraw) DrawButton("Navigate");
  }

  if (button.name=="ToLeft"){
    if (settings.activeBtn==FindButtonIDByName("Tune")){
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
        actualDabChannel = settings.dabChannel;
        actualDabService = Dab.numberofservices - 1;
        settings.dabServiceID = Dab.service[actualDabService].ServiceID;
        SetRadio();
        settings.activeBtn=FindButtonIDByName("Service");
        Serial.printf("Tune:%d, Service:%d, Channels:%d",settings.dabChannel,actualDabService,DAB_FREQS);
      } 
      if (!settings.isDab){
        Dab.seek(0,1);
        settings.fmFreq = Dab.freq*10;
        DrawFrequency();
      }
      if (doDraw) DrawButtons();
    }
    else if (settings.activeBtn==FindButtonIDByName("Service")){
      if (settings.isDab && actualDabService>0){
        actualDabService--;
        settings.dabServiceID = Dab.service[actualDabService].ServiceID;
        SetRadio();
      }
      if (actualDabService==0){
        settings.activeBtn=FindButtonIDByName("Tune");
      }
      if (doDraw) DrawButtons();
    }
    else if (settings.activeBtn==FindButtonIDByName("Vol")){
      if (settings.volume>0) settings.volume--;
      Dab.vol(settings.volume);
      settings.isMuted=0;
      if (doDraw) DrawButton("Vol");
      if (doDraw) DrawButton("Mute");
    }
    else if (settings.activeBtn==FindButtonIDByName("MEM")){
      if (settings.memoryChannel>0){
        settings.memoryChannel--;
        SetRadioFromMemory(settings.memoryChannel);
        if (doDraw) DrawFrequency();
        if (doDraw) DrawButton("MEM");
      }
    }
    else if (settings.activeBtn==FindButtonIDByName("Light")){
      if (settings.currentBrightness>5) settings.currentBrightness-=5;
      if (settings.currentBrightness<5) settings.currentBrightness=5;
      if (doDraw) DrawButton("Light");
      ledcWrite(ledChannelforTFT, 256-(settings.currentBrightness*2.56));
    }
    if (doDraw) DrawButton("Navigate");
  }

  if (button.name=="Vol"){
    if (settings.activeBtn==FindButtonIDByName("Vol")){
      keyboardNumber = settings.volume;
      actualPage = BTN_NUMERIC;
      if (doDraw) DrawScreen();
    } else {
      settings.activeBtn=FindButtonIDByName("Vol");
      if (doDraw) DrawButtons();
    }
  }

  if (button.name=="Mute") {
    settings.isMuted++;
    SetMute();
    if (doDraw) DrawButton("Mute");
  }

  if (button.name=="Mode") {
    DrawPatience();
    settings.isDab = !settings.isDab;
    SetRadio();
    if (settings.isDab) 
      settings.activeBtn=FindButtonIDByName("Service");
    else
      settings.activeBtn=FindButtonIDByName("Tune");
    if (doDraw) DrawButtons();
  }

  if (button.name=="Tune"){
    if (settings.isDab){
      if (settings.activeBtn==FindButtonIDByName("Tune")){
        keyboardNumber = 0; //settings.dabChannel;
        actualPage = BTN_NUMERIC;
        if (doDraw) DrawScreen();
      } else {
        settings.activeBtn=FindButtonIDByName("Tune");
        if (doDraw) DrawButtons();
      }
    } else {
      if (settings.activeBtn==FindButtonIDByName("Tune")){
        keyboardNumber = 0;
        actualPage = BTN_NUMERIC;
        if (doDraw) DrawScreen();
      } else {
        settings.activeBtn=FindButtonIDByName("Tune");
        if (doDraw) DrawButtons();
      }
    }
  }

  if(button.name =="Info"){
    actualPage=lastPage;
    DrawScreen();

    tft.fillRect(2,2,320,200,TFT_BLACK);
    tft.setTextDatum(ML_DATUM);
    tft.setTextPadding(50);
    tft.setTextColor(TFT_YELLOW,TFT_BLACK);

    if (wifiAvailable || wifiAPMode){
      tft.setTextColor(TFT_YELLOW,TFT_BLACK);
      tft.drawString("WiFi:",2,10,2);
      tft.setTextColor(TFT_GREEN,TFT_BLACK);
      tft.drawString("SSID        :" + String(WiFi.SSID()),2,25,1);
      sprintf(buf,"IP Address  :%d.%d.%d.%d", WiFi.localIP()[0],WiFi.localIP()[1],WiFi.localIP()[2],WiFi.localIP()[3]);
      tft.drawString(buf,2,33,1);
      tft.drawString("RSSI        :" + String(WiFi.RSSI()),2,41,1); 
    }

    if (settings.isDab){
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.drawString("Service:",2,60,2);
      tft.setTextColor(TFT_GREEN,TFT_BLACK);
      for (int i=0;i<Dab.numberofservices;i++){
        sprintf(buf,"%2d=(%d) %s",i, Dab.service[i].ServiceID, Dab.service[i].Label);
        tft.drawString(buf,2,75+(i*8),1);
      }
    }

    tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
    tft.drawString("Memory:",182,60,2);
    tft.setTextColor(TFT_GREEN,TFT_BLACK);
    for (int i=0;i<10;i++){
      sprintf(buf,"%d = %s, %d,%d ",i, memories[i].isDab?"Dab":"FM", memories[i].isDab?memories[i].dabChannel:memories[i].fmFreq, memories[i].isDab?memories[i].dabServiceID:NULL);
      tft.drawString(buf,182,75+(i*8),1);
    }
  }

  if (button.name=="Service"){
    if (settings.isDab){
      if (settings.activeBtn==FindButtonIDByName("Service")){
        keyboardNumber = 0; //actualDabService;
        actualPage = BTN_NUMERIC;
        if (doDraw) DrawScreen();
      } else {
        settings.activeBtn=FindButtonIDByName("Service");
        if (doDraw) DrawButtons();
      }
    } 
  }

  if (button.name=="MEM"){
    if (settings.activeBtn==FindButtonIDByName("MEM")){
      keyboardNumber = 0; //settings.memoryChannel;
      actualPage = BTN_NUMERIC;
      if (doDraw) DrawScreen();
    } else {
      SetRadioFromMemory(settings.memoryChannel);
      settings.activeBtn=FindButtonIDByName("MEM");
      if (doDraw) DrawButtons();
    }
  }

  if (button.name=="Save"){
    if (settings.activeBtn!=FindButtonIDByName("MEM")){
      settings.activeBtn=FindButtonIDByName("Save");
      keyboardNumber = 0; //settings.memoryChannel;
      actualPage = BTN_NUMERIC;
      if (doDraw) DrawScreen();
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
    settings.activeBtn=FindButtonIDByName("Light");
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
    isOn = false;
    actualPage=1;
    DoTurnOff();
    delay(500);
  }

  if (String(button.name).substring(0,3) =="A00") {
    int i = String(button.name).substring(3).toInt();
    keyboardNumber = (keyboardNumber*10)+i;

    if (settings.activeBtn==FindButtonIDByName("Tune")){
      if (settings.isDab && keyboardNumber>=DAB_FREQS) keyboardNumber=0;
      if (!settings.isDab && keyboardNumber>107900) keyboardNumber=0;
    } 
    if (settings.activeBtn==FindButtonIDByName("Vol")){
      if (keyboardNumber>63) keyboardNumber=0;
    } 
    if (settings.activeBtn==FindButtonIDByName("Service")){
      if (keyboardNumber>Dab.numberofservices - 1) keyboardNumber=0;
    } 
    if (settings.activeBtn==FindButtonIDByName("MEM")){
      if (keyboardNumber>9) keyboardNumber=0;
    } 
    if (settings.activeBtn==FindButtonIDByName("Save")){
      if (keyboardNumber>9) keyboardNumber=0;
    }
    if (doDraw) DrawKeyboardNumber(false);
  }

  if (button.name=="Enter") {
    if (settings.activeBtn==FindButtonIDByName("Vol")){
      settings.volume=keyboardNumber;
      Dab.vol(settings.volume);
    }
    if (settings.activeBtn==FindButtonIDByName("Tune")){
      if (settings.isDab){
        DrawPatience();
        settings.dabChannel=keyboardNumber;
        actualDabService=0;
        Dab.tune(settings.dabChannel);
        Dab.set_service(actualDabService);
        settings.dabServiceID = Dab.service[actualDabService].ServiceID;
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
    if (settings.activeBtn==FindButtonIDByName("Service")){
      if (settings.isDab){
        actualDabService=keyboardNumber;
        Dab.set_service(actualDabService); 
        settings.dabServiceID = Dab.service[actualDabService].ServiceID;
      }
    } 
    if (settings.activeBtn==FindButtonIDByName("MEM")){
      settings.memoryChannel=keyboardNumber; 
      SetRadioFromMemory(settings.memoryChannel);
    } 
    if (settings.activeBtn==FindButtonIDByName("Save")){
      settings.memoryChannel=keyboardNumber;
      Serial.printf("SaveButton:%d\r\n",settings.memoryChannel);
      Memory myMemory = {settings.isDab, settings.isStereo, settings.dabChannel, settings.dabServiceID, settings.fmFreq};
      memories[settings.memoryChannel] = myMemory;
      SetRadioFromMemory(settings.memoryChannel);
      SaveMemories();
      settings.activeBtn=FindButtonIDByName("MEM");
    }
    actualPage = 1;
    if (doDraw) DrawScreen();
  }

  if (button.name=="Clear") {
    if (keyboardNumber>0){
      keyboardNumber = 0;
      if (doDraw) DrawKeyboardNumber(false);
    } else {
      actualPage = 1;
      if (doDraw) DrawScreen();
    }
  }
}

void SetRadioFromMemory(int channel){
  DrawPatience();
  settings.isDab = memories[channel].isDab;
  settings.isStereo = memories[channel].isStereo;
  settings.dabChannel = memories[channel].dabChannel;
  settings.dabServiceID = memories[channel].dabServiceID;
  settings.fmFreq = memories[channel].fmFreq;
  SetRadio();
  if (settings.isDab) actualDabService = findChannelOnDabServiceID(settings.dabServiceID);
  Serial.printf("Set from Memory: DABChannel=%d, DABService=%d %d, DABServiceID=%d, isDab=%d, Volume=%d, isStereo=%d\r\n",settings.dabChannel, actualDabService, findChannelOnDabServiceID(settings.dabServiceID), settings.dabServiceID, settings.isDab, settings.volume, settings.isStereo);
}

void SetRadio(){
  SetRadio(false);
}

void SetRadio(bool firstTime){
  Serial.printf("Set from Radio: DABChannel=%d, DABService=%d %d, DABServiceID=%d, isDab=%d, Volume=%d, isStereo=%d\r\n",settings.dabChannel, actualDabService, findChannelOnDabServiceID(settings.dabServiceID), settings.dabServiceID, settings.isDab, settings.volume, settings.isStereo);

  bool doTune = false;
  if (actualIsDab != settings.isDab || firstTime){
    actualIsDab = settings.isDab;
    doTune = true;
    Dab.begin(settings.isDab?0:1);
    Dab.vol(settings.volume);
  }

  if (actualDabVol != settings.volume || firstTime){
    actualDabVol = settings.volume;
    Dab.vol(settings.volume);
  }

  SetMute();
  if (actualIsStereo != settings.isStereo || firstTime){
    actualIsStereo = settings.isStereo;
    Dab.mono(!settings.isStereo);
  }

  
  if (settings.isDab){
    if (actualDabChannel != settings.dabChannel || firstTime || doTune){
      actualDabChannel = settings.dabChannel;
      Dab.tune(settings.dabChannel);
      Serial.printf("Dab channel set to %d\r\n",actualDabChannel);
    }
    if (firstTime) actualDabService = findChannelOnDabServiceID(settings.dabServiceID);
    Dab.set_service(findChannelOnDabServiceID(settings.dabServiceID));
  } else {
    if (actualFmFreq != settings.fmFreq || firstTime || doTune){
      actualFmFreq != settings.fmFreq;
      Dab.tune((uint16_t)(settings.fmFreq/10));
    }
  }
  if (!firstTime) DrawFrequency();
}

void SetMute(){
    if (settings.isMuted>3) settings.isMuted=0;
    if (settings.isMuted==0) Dab.mute(false,false);
    if (settings.isMuted==1) Dab.mute(true,false);
    if (settings.isMuted==2) Dab.mute(false,true);
    if (settings.isMuted==3) Dab.mute(true,true);
}

/***************************************************************************************
**            Turn off
***************************************************************************************/
void DoTurnOff(){
  ledcWrite(ledChannelforTFT, 255);
  tft.fillScreen(TFT_BLACK);
  Dab.mute(1,1);
}

void WaitForWakeUp(){
  while (!isOn){
    //esp_task_wdt_reset();
    uint16_t x = 0, y = 0;
    bool pressed = tft.getTouch(&x, &y);
    if (pressed) ESP.restart();
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

bool SaveMemories() {
  for (unsigned int t = 0; t < sizeof(memories); t++)
    EEPROM.write(offsetEEPROM + sizeof(settings) + 10 + t, *((char*)&memories + t));
  EEPROM.commit();
  Serial.println("Memories:saved");
  return true;
}

bool LoadMemories() {
  bool retVal = true;
  for (unsigned int t = 0; t < sizeof(memories); t++)
    *((char*)&memories + t) = EEPROM.read(offsetEEPROM + sizeof(settings) + 10 + t);
  Serial.println("Memories:" + retVal?"Loaded":"Not loaded");
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
