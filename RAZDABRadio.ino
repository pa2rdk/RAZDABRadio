////////////////////////////////////////////////////////////
// V1.16 Touch and Screen rotatable in WiFi settings 
// V1.15 Touch turned
// V1.14 Hardware Mute on Pin 33
// V1.13 Better dialogs, Off button repaired
// V1.12 OTA
// V1.08 Aangepast aan standaard displayboard, een aantal pinnen verlegd zoek 'Sketch with standard board'
// V1.07 Moved website to core0 and changed Serial.print* to DebugPrint*
// V1.06 Rename HandleButton->HandleFunction and change SaveConfig
// V1.05 Code review
// V1.04 Better highlighted button
// V1.03 Gradient buttons
// V1.02 Bug with HTML characters solved
// V1.01 Includes even more webinterface
// V1.00 Includes full webinterface
// V0.95 Start of webinterface
// V0.94 Drastic interface changes
// V0.93 Info button acts as loop through all channels.
// V0.92 Sort and save channel list
// V0.91 Pin 12 changed to 35 on HSPI
// V0.90 DABShield on own SPI interface PAS OP.... Ik heb de library aangepast.
// V0.88 Debug on serial print
// V0.87 Cache
// V0.86 NoCache
// V0.85 Witte frequentie en clear van RDS info en aangepaste meter
// V0.84 Lichtkrant
// V0.83 Logo caching
// V0.82 Logo improvements
// V0.81 Much better logo's
// V0.80 The first logo's
// V0.72 Small interface corrections
// V0.71 Clear DAB Channels on Clear of keyboard
// V0.70 DAB Channels
// V0.63 SNR meter
// V0.62 Solved volume bug
// V0.61 Solved error in memory display
// V0.60 Select memory position
// V0.50 Layout changes, Memories, Meters
// V0.40 Layout changes, Memories, Extended info
// V0.30 Better tuning
// V0.20 BackLight
// V0.10 Initial
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
//  |   LED Coll.|     13.          |
//  |   SDO      |                  |
//  |   T_CLK    |     18           |
//  |   T_CS     |      5           |
//  |   T_DIN    |     23           |
//  |   T_DO     |     19           |
//  |   T_IRQ    |     34           |
//  |------------|------------------|
//
//  |------------|------------------|
//  | DAB Shield |     ESP32        |
//  |------------|------------------|
//  | INTERRUPT  |     26           |
//  | RESET      |     32           |
//  | PWREN      |     27           |
//  | MISO       |     35           |
//  | MOSI       |     13           |
//  | SCLK       |     14           |
//  | CS         |     15           |
//  |------------|------------------|
// Installing Libraries
//
// The Hybrid-Radio Project uses Libraries for various functionality. These can be installed from Library Manager  (Tools | Manage Libraries).
// DABShield - Version 1.5.3 or above
// ArduinoJson by Benoit Blanchon - Version 6.19.4
// PNGdec by Larry Bank - Version 1.0.1
// the following libraries require manual installation:
// N.V.T. DFRobot GDL - Require downloading the .zip Library  DFRobot_GDL Library and then added by Sketch |  Include Library |  Add .ZIP Library...
// TinyXML-2 - Source Files downloaded from here: https://github.com/leethomason/tinyxml2  and added to a Folder TinyXML2 in your Arduino libraires directory (<Arduino Project Directory>/ libraries/TinyXML2).
//
////////////////////////////////////////////////////////////
#include <SPIFFS.h>
#include "FS.h"
#include <SPI.h>
#include <WiFi.h>
#include <WifiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DABShield.h>
#include <EEPROM.h>
#include <TFT_eSPI.h>  // https://github.com/Bodmer/TFT_eSPI
#include <ArduinoJson.h>
#include <tinyxml2.h>
#include <PNGdec.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <RDKOTA.h>

#include <pthread.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_pthread.h"

#define offsetEEPROM 32
#define EEPROM_SIZE 4096

#define OTAHOST      "https://www.rjdekok.nl/Updates/RAZDABRadio"
#define OTAVERSION   "v1.16"

#define DebugEnabled
#ifdef DebugEnabled
#define DebugPrint(x)         Serial.print(x)
#define DebugPrintln(x)       Serial.println(x)
#define DebugPrintf(x, ...)   Serial.printf(x, __VA_ARGS__)
#else
#define DebugPrint(x)  
#define DebugPrintln(x)
#define DebugPrintf(x, ...)
#endif

const char mode_0[] = "Dual";
const char mode_1[] = "Mono";
const char mode_2[] = "Stereo";
const char mode_3[] = "Joint Stereo";
const char *const audiomode[] = { mode_0, mode_1, mode_2, mode_3 };

#define BTN_NAV 32768
#define BTN_NEXT 16384
#define BTN_PREV 8192
#define BTN_CLOSE 4096
#define BTN_ARROW 2048
#define BTN_NUMERIC 1024

#define DISPLAYLEDPIN 14  //33 Sketch with standard board

#define TFT_GREY 0x5AEB
#define TFT_LIGTHYELLOW 0xFF10
#define TFT_DARKBLUE 0x016F
#define TFT_SHADOW 0xE71C
#define TFT_BUTTONTOPCOLOR 0xB5FE

#define DAB_BAND 0
#define DAB_INTERRUPT 26
#define DAB_RESET 21    //32 Sketch with standard board
#define DAB_PWREN 27

#define HSPI_MISO 35
#define HSPI_MOSI 13
#define HSPI_SCLK 12    //14 Sketch with standard board
#define HSPI_SS 15
#define MUTE_PIN  33

typedef struct {  // WiFi Access
  const char *SSID;
  const char *PASSWORD;
} wlanSSID;

typedef struct {        // Buttons
  const char *name;     // Buttonname
  const char *caption;  // Buttoncaption
  char waarde[12];      // Buttontext
  uint16_t pageNo;
  uint16_t xPos;
  uint16_t yPos;
  uint16_t width;
  uint16_t height;
  uint16_t bottomColor;
  uint16_t topColor;
} Button;

typedef struct {
  bool isDab;
  bool isStereo;
  uint8_t dabChannel;
  uint16_t dabServiceID;
  uint32_t fmFreq;
} Memory;

typedef struct {
  uint8_t dabChannel;
  uint16_t dabServiceID;
  char dabName[40];
  bool noLogo;
} DABChannel;

typedef struct {
  uint16_t stationID;
  const char *location;
} DabLogoLocation;

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
  uint8_t dabChannelsCount;
  uint8_t dabChannelSelected;
  bool showOnlyCachedLogos;
  bool rotateScreen;
  bool rotateTouch;
  bool isDebug;
} Settings;

const Button buttons[] = {
  { "ToLeft", "<<", "", BTN_ARROW, 2, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "ToRight", ">>", "", BTN_ARROW, 242, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },

  { "Vol", "Vol", "", 1, 2, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Tune", "Tune", "", 1, 82, 136, 74, 30, TFT_BLACK, TFT_WHITE },
  { "LoadList", "Channels", "", 1, 162, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "MEM", "MEM", "", 1, 242, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },

  { "Mute", "Mute", "", 1, 2, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Mode", "Mode", "", 1, 82, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Info", "Info", "", 1, 162, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Save", "Save", "", 1, 242, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },

  { "Light", "Light", "", 1, 82, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Off", "Off", "", 1, 162, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },

  //{"Service","Select","",    1, 82,136, 74,30, TFT_BLACK, TFT_BUTTONTOPCOLOR},
  //{"Stereo","Stereo","",      1,242,172, 74,30, TFT_BLACK, TFT_BUTTONTOPCOLOR},

  { "A001", "1", "", BTN_NUMERIC, 42, 100, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A002", "2", "", BTN_NUMERIC, 122, 100, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A003", "3", "", BTN_NUMERIC, 202, 100, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A004", "4", "", BTN_NUMERIC, 42, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A005", "5", "", BTN_NUMERIC, 122, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A006", "6", "", BTN_NUMERIC, 202, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A007", "7", "", BTN_NUMERIC, 42, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A008", "8", "", BTN_NUMERIC, 124, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A009", "9", "", BTN_NUMERIC, 202, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Clear", "Clear", "", BTN_NUMERIC, 42, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A000", "0", "", BTN_NUMERIC, 122, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Enter", "Enter", "", BTN_NUMERIC, 202, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A00M", "-", "", BTN_NUMERIC, 2, 100, 35, 137, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A00P", "+", "", BTN_NUMERIC, 282, 100, 35, 137, TFT_BLACK, TFT_BUTTONTOPCOLOR },

  { "Close", "Close", "", BTN_CLOSE, 122, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
};

const int ledFreq = 5000;
const int ledResol = 8;
const int ledChannelforTFT = 0;

#include "config.h";  // Change to config.h

DAB Dab;
TFT_eSPI tft = TFT_eSPI();  // Invoke custom library
DABTime dabtime;
WiFiMulti wifiMulti;
AsyncWebServer server(80);
AsyncEventSource events("/events");
RDKOTA rdkOTA(OTAHOST);
PNG png;
SPIClass *hspi = NULL;

const byte slaveSelectPin = 22;   //25 Sketch with standard board; Was 12 ivm second SPI;
int actualPage = 1;
int lastPage = 2;
int32_t keyboardNumber = 0;
char buf[300] = "\0";
char logoBuf[100] = "\0";
long lastTime = millis();
long saveTime = millis();
long startTime = millis();
long pressTime = millis();
bool wifiAvailable = false;
bool wifiAPMode = false;
bool isOn = true;
char actualInfo[100] = "\0";
char lastInfo[100] = "\0";
char dispInfo[100] = "\0";
int namePos = 0;
int nameLen = 24;
int infoPos = 0;
int infoLen = 24;
long infoTime = millis();
int services[20] = {};
String webCommand = "None";
DABChannel *dabChannels = 0;

bool actualIsDab = false;
byte actualDabVol = 0;
bool actualIsStereo = true;
byte actualDabChannel = 0;
int actualDabService = 0;

uint32_t actualFmFreq = 87500;
Memory memories[10] = {};

StaticJsonDocument<128> filter;
StaticJsonDocument<256> doc;

char buff[1024];
char *servicexml;
char imageurl[128];
char mime[32];

#include "webpages.h";

void setup() {
  pinMode(DISPLAYLEDPIN, OUTPUT);
  digitalWrite(DISPLAYLEDPIN, 0);

  pinMode(MUTE_PIN, OUTPUT);
  digitalWrite(MUTE_PIN, 0);

#ifdef DebugEnabled
  Serial.begin(115200);
  while (!Serial)
    ;
#endif

  DebugPrint(F("PI4RAZ DAB\n\n"));
  DebugPrint(F("Initializing....."));

  if (!SPIFFS.begin(true)) {
    DebugPrintln("SPIFFS Mount Failed");
  }

  ledcSetup(ledChannelforTFT, ledFreq, ledResol);
  ledcAttachPin(DISPLAYLEDPIN, ledChannelforTFT);

  //Enable SPI
  pinMode(slaveSelectPin, OUTPUT);
  digitalWrite(slaveSelectPin, HIGH);
  SPI.begin();
  hspi = new SPIClass(HSPI);
  //hspi->begin();
  hspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);

  tft.init();
  tft.setRotation(screenRotation);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  //DAB Setup
  Dab.setCallback(DrawServiceData);
  Dab.begin(DAB_BAND, DAB_INTERRUPT, DAB_RESET, DAB_PWREN);

  if (Dab.error != 0) {
    sprintf(dispInfo, "DAB Error %02d", Dab.error);
    DrawButton(80, 120, 160, 30, dispInfo, "", TFT_BLACK, TFT_WHITE, "");
    DebugPrintf("failed to initialize DAB shield %d", Dab.error);
    while (1)
      ;
  }

  if (!EEPROM.begin(EEPROM_SIZE)) {
    DrawButton(80, 120, 160, 30, "EEPROM Failed", "", TFT_BLACK, TFT_WHITE, "");
    DebugPrintln("failed to initialise EEPROM");
    while (1)
      ;
  }

  if (!LoadConfig()) {
    DebugPrintln(F("Writing defaults"));
    SaveConfig();
    Memory myMemory = { 0, 1, 0, 0, 87500 };
    for (int x = 0; x < 10; x++) {
      memories[x] = myMemory;
    }
    SaveMemories();
  }

  LoadConfig();
  LoadMemories();
  LoadStationList();

  tft.setRotation(settings.rotateScreen?1:3);
  if (settings.rotateTouch) calData[4]=1;
  tft.setTouch(calData);

  // add Wi-Fi networks from All_Settings.h
  for (int i = 0; i < sizeof(wifiNetworks) / sizeof(wifiNetworks[0]); i++) {
    wifiMulti.addAP(wifiNetworks[i].SSID, wifiNetworks[i].PASSWORD);
    DebugPrintf("Wifi:%s, Pass:%s.", wifiNetworks[i].SSID, wifiNetworks[i].PASSWORD);
    DebugPrintln();
  }
  DrawButton(80, 80, 160, 30, "Connecting to WiFi", "", TFT_BLACK, TFT_WHITE, "");
  wifiMulti.addAP(settings.wifiSSID, settings.wifiPass);
  DebugPrintf("Wifi:%s, Pass:%s.", settings.wifiSSID, settings.wifiPass);
  DebugPrintln();
  if (Connect2WiFi()) {
    if (rdkOTA.checkForUpdate(OTAVERSION)){
      if (questionBox("Install update", TFT_WHITE, TFT_NAVY, 80, 80, 160, 40)){
        DrawButton(80, 80, 160, 40, "Installing update", "", TFT_BLACK, TFT_RED, "");
        rdkOTA.installUpdate();
      } 
    }

    wifiAvailable = true;
    DrawButton(80, 80, 160, 30, "Connected to WiFi", WiFi.SSID(), TFT_BLACK, TFT_WHITE, "");
    delay(1000);
  } else {
    DrawButton(75, 80, 170, 30, "Connect to RAZDABRadio", WiFi.SSID(), TFT_BLACK, TFT_WHITE, "");
    wifiAPMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP("RAZDABRadio", NULL);
  }

  xTaskCreatePinnedToCore(
    SecondTask
    ,  "SecondTask"    // A name just for humans
    ,  10000             // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  0                // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL
    ,  0 );             // Core 0


  settings.isMuted = 0;
  SetRadio(true);
  ledcWrite(ledChannelforTFT, 256 - (settings.currentBrightness * 2.56));
  DrawScreen();
}

void SecondTask(void *pvParameters)  // This is a task.
{
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/css", css_html);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", settings_html, processor);
  });

  server.on("/store", HTTP_GET, [](AsyncWebServerRequest *request) {
    SaveSettings(request);
    SaveConfig();
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/dablogo", HTTP_GET, [](AsyncWebServerRequest *request) {
    //"/43b4.png"
    if (request->hasParam("plaatje")) request->getParam("plaatje")->value();
    request->send(SPIFFS, request->getParam("plaatje")->value(), "image/png");
  });

  server.on("/tunedown", HTTP_GET, [](AsyncWebServerRequest *request) {
    webCommand = "TuneDown";
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/tuneup", HTTP_GET, [](AsyncWebServerRequest *request) {
    webCommand = "TuneUp";
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/memdown", HTTP_GET, [](AsyncWebServerRequest *request) {
    webCommand = "MemDown";
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/memup", HTTP_GET, [](AsyncWebServerRequest *request) {
    webCommand = "MemUp";
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/volumedown", HTTP_GET, [](AsyncWebServerRequest *request) {
    webCommand = "VolumeDown";
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/volumeup", HTTP_GET, [](AsyncWebServerRequest *request) {
    webCommand = "VolumeUp";
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/channeldown", HTTP_GET, [](AsyncWebServerRequest *request) {
    webCommand = "ChannelDown";
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/channelup", HTTP_GET, [](AsyncWebServerRequest *request) {
    webCommand = "ChannelUp";
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/gochannel", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("channel")) keyboardNumber = request->getParam("channel")->value().toInt();
    webCommand = "GoChannel";
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/mute", HTTP_GET, [](AsyncWebServerRequest *request) {
    webCommand = "Mute";
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/mode", HTTP_GET, [](AsyncWebServerRequest *request) {
    webCommand = "Mode";
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Rebooting");
    ESP.restart();
  });

  events.onConnect([](AsyncEventSourceClient *client) {
    DebugPrintln("Connect web");
    if (client->lastId()) {
      DebugPrintf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  server.begin();
  DebugPrintln("HTTP server started");

  for (;;) // A Task shall never return or exit.
  {

  }
}

void DisplaySettings(String displayText) {
  DebugPrintln();
  DebugPrintln(displayText);
  DebugPrintln();
  DebugPrintf("Active Button = %d\r\n", settings.activeBtn);
  DebugPrintf("Brighness = %d\r\n", settings.currentBrightness);
  DebugPrintf("Dab channel = %d\r\n", settings.dabChannel);
  DebugPrintf("Dab serviceID = %d\r\n", settings.dabServiceID);
  DebugPrintf("Dab service = %d\r\n", actualDabService);
  DebugPrintf("FM Freq = %d\r\n", settings.fmFreq);
  DebugPrintf("Is Dab = %s\r\n", settings.isDab ? "True" : "False");
  DebugPrintf("Is Stereo = %s\r\n", settings.isStereo ? "True" : "False");
  DebugPrintf("Is Muted = %d\r\n", settings.isMuted);
}

byte findChannelOnDabServiceID(uint16_t dabServiceID) {
  DebugPrintf("DabService:%d, QTY:%d", dabServiceID, Dab.numberofservices);
  DebugPrintln();
  for (int i = 0; i < Dab.numberofservices; i++) {
    if (Dab.service[i].ServiceID == dabServiceID) return i;
  }
  return 0;
}

bool Connect2WiFi() {
  startTime = millis();
  DebugPrint("Connect to Multi WiFi");
  while (wifiMulti.run() != WL_CONNECTED && millis() - startTime < 30000) {
    // esp_task_wdt_reset();
    delay(1000);
    DebugPrint(".");
  }
  DebugPrintln();
  return (WiFi.status() == WL_CONNECTED);
}

void loop() {
  HandleWebCommand();

  if (millis() - lastTime > 1000 && actualPage < lastPage) {
    DrawTime();
    DrawStatus();
    lastTime = millis();
  }

  if (millis() - saveTime > 2000) {
    saveTime = millis();
    SaveConfig();
  }

  if (actualPage == 1 && millis() - infoTime > 500) {
    infoTime = millis();
    if (infoPos >= strlen(actualInfo)) infoPos = 0;
    if (strlen(actualInfo) > infoLen) {
      for (int i = 0; i < strlen(actualInfo) && i < infoLen; i++) {
        int iPos = i + infoPos;
        if (iPos >= strlen(actualInfo)) iPos = iPos - strlen(actualInfo);
        dispInfo[i] = actualInfo[iPos];
      }
      dispInfo[strlen(actualInfo)] = '\0';
      infoPos++;
    } else strcpy(dispInfo, actualInfo);

    tft.setTextDatum(ML_DATUM);
    if (settings.isDab) {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setTextPadding(tft.textWidth(dispInfo));  // String width + margin
      tft.fillRect(2, 60, 188, 13, TFT_BLACK);
      tft.drawString(dispInfo, 2, 65, 2);
    } else {
      sprintf(dispInfo, "%s - %s", Dab.ps, Dab.ServiceData);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setTextPadding(tft.textWidth(dispInfo));  // String width + margin
      tft.fillRect(2, 11, 318, 6, TFT_BLACK);
      tft.drawString(dispInfo, 2, 14, 1);
    }
    events.send(dispInfo, "DABRDS", millis());
  }

  if (millis() - pressTime > 200) {
    uint16_t x = 0, y = 0;
    bool pressed = tft.getTouch(&x, &y);
    if (pressed) {
      int showVal = ShowControls();
      for (int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
        if ((buttons[i].pageNo & showVal) > 0) {
          if (x >= buttons[i].xPos && x <= buttons[i].xPos + buttons[i].width && y >= buttons[i].yPos && y <= buttons[i].yPos + buttons[i].height) {
            HandleFunction(buttons[i], x, y);
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
void DrawScreen() {
  DrawScreen(false);
  DisplaySettings("Draw Screen");
}

void DrawScreen(bool drawAll) {
  tft.fillScreen(TFT_BLACK);
  if (actualPage < lastPage || drawAll) DrawFrequency();
  if (actualPage == BTN_NUMERIC) DrawKeyboardNumber(false);
  DrawButtons();
}

void DrawTime() {
  if (settings.isDab) {
    Dab.time(&dabtime);
    sprintf(buf, "%02d/%02d/%02d %02d:%02d", dabtime.Days, dabtime.Months, dabtime.Year, dabtime.Hours, dabtime.Minutes);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextPadding(tft.textWidth(buf));  // String width + margin
    tft.drawString(buf, 2, 4, 1);
    events.send(buf, "DABTime", millis());
  }
}

void DrawServiceData() {
  if (actualPage == 1) {
    tft.setTextDatum(ML_DATUM);
    if (settings.isDab) {
      sprintf(actualInfo, "%s ", Dab.ServiceData);
    } else {
      sprintf(actualInfo, "%02d/%02d/%04d %02d:%02d", Dab.Days, Dab.Months, Dab.Year, Dab.Hours, Dab.Minutes);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setTextPadding(tft.textWidth(actualInfo));  // String width + margin
      tft.drawString(actualInfo, 2, 4, 1);
      events.send(actualInfo, "DABTime", millis());
      sprintf(actualInfo, "%s - %s ", Dab.ps, Dab.ServiceData);
    }
    if (strcmp(lastInfo, actualInfo) != 0) {
      DebugPrintln("Old and new differ");
      strcpy(lastInfo, actualInfo);
      DebugPrintln(actualInfo);
      infoPos = 0;
    }
  }
}

void DrawFrequency() {
  if (actualPage < lastPage) {
    tft.fillRect(0, 0, 320, 99, TFT_BLACK);
    if (settings.isDab && Dab.freq_index > 0) {
      DrawButton(80, 20, 160, 30, "Zoek logo", "", TFT_BLACK, TFT_BLUE, "");
      GetDABLogo(settings.dabServiceID, Dab.EnsembleID, Dab.ECC, settings.showOnlyCachedLogos);
      tft.fillRect(0, 0, 190, 99, TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextDatum(ML_DATUM);
      sprintf(buf, "%03d.%03d MHz", (uint16_t)(Dab.freq_khz(Dab.freq_index) / 1000), (uint16_t)(Dab.freq_khz(Dab.freq_index) % 1000));
      events.send(buf, "DABFreq", millis());
      tft.setTextPadding(tft.textWidth(buf));
      tft.drawString(buf, 2, 14, 1);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      DebugPrintf("Draw fequency:%d,%d,%s - %d\r\n", Dab.service[actualDabService].ServiceID, actualDabService, Dab.service[actualDabService].Label, Dab.service[actualDabService].CompID);
      if (Dab.numberofservices > 0) {
        sprintf(buf, "%s", Dab.service[actualDabService].Label);

        if (strlen(buf) > 12) buf[12] = '\0';

        tft.drawString(buf, 2, 45, 4);
        tft.fillRect(180, 30, 10, 30, TFT_BLACK);
      }
      events.send(Dab.service[actualDabService].Label, "DABName", millis());
      events.send(GetLogoName(settings.dabServiceID), "DABLogo", millis());
    }
    if (!settings.isDab) {
      tft.setTextColor(TFT_GOLD, TFT_BLACK);
      tft.setTextDatum(MR_DATUM);
      sprintf(buf, "%3d.%1d", (uint16_t)Dab.freq / 100, (uint16_t)(Dab.freq % 100) / 10);
      tft.setTextPadding(tft.textWidth(buf));
      tft.drawString(buf, 260, 48, 7);
      tft.setTextPadding(tft.textWidth("MHz"));
      tft.drawString("MHz", 315, 50, 4);
      strcat(buf, "MHz");
      events.send(buf, "DABName", millis());
      events.send("", "DABLogo", millis());
      events.send("", "DABFreq", millis());
    }
    DrawStatus();
    if (settings.isDab && settings.isDebug) DebugPrintf("\r\nDabzaken ECC:%d, PI:%d, Ensemble:%s, RomID:%d, PartNo:%d,  VerMajor:%d, PS:%s, DabServiceID:%d, NumberOfServices:%d, EnsembleID:%d\r\n\r\n", Dab.ECC, Dab.pi, Dab.Ensemble, Dab.RomID, Dab.PartNo, Dab.VerMajor, Dab.ps, settings.dabServiceID, Dab.numberofservices, Dab.EnsembleID);
  }
}

void DrawPatience() {
  DrawPatience(false, "Even geduld...");
}

void DrawPatience(bool onBottom, String message) {
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(TFT_RED, TFT_BLACK);

  if (!onBottom) {
    if (actualPage == 1)
      tft.fillRect(0, 0, 320, 134, TFT_BLACK);
    else
      tft.fillRect(0, 0, 320, 98, TFT_BLACK);
    //tft.drawString(message, 0, 30, 4);
    DrawButton(80, 20, 160, 30, message, "", TFT_BLACK, TFT_RED, "");
    message.toCharArray(buf, message.length() + 1);
    events.send(buf, "DABName", millis());
    events.send("", "DABRDS", millis());
    events.send("", "DABLogo", millis());
  } else {
    tft.fillRect(0, 136, 320, 240, TFT_BLACK);
    //tft.drawString(message, 0, 160, 4);
    DrawButton(80, 160, 160, 30, message, "", TFT_BLACK, TFT_RED, "");
  }
}

void DrawStatus() {
  if (!settings.isDab) {
    sprintf(buf, "   RSSI:%d, SNR:%d", Dab.signalstrength, Dab.snr);
  }

  if (settings.isDab) {
    Dab.status();
    sprintf(buf, "   RSSI:%d, SNR:%d, Quality:%d\%", Dab.signalstrength, Dab.snr, Dab.quality);
  }

  DrawBox(2, 96, 154, 33);
  DrawMeter(5, 99, 148, 12, Dab.signalstrength, 10, 65, 50, 75, "RSSI:");
  DrawMeter(5, 114, 148, 12, Dab.snr, 0, settings.isDab ? 20 : 40, 50, 75, "SNR:");
}

void DrawButtons() {
  for (int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
    int showVal = ShowControls();
    if ((buttons[i].pageNo & showVal) > 0) DrawButton(buttons[i].name);
  }
}

void DrawButton(String btnName) {
  int showVal = ShowControls();
  Button button = FindButtonByName(btnName);
  if ((button.pageNo & showVal) > 0) {
    DrawButton(button.xPos, button.yPos, button.width, button.height, button.caption, button.waarde, button.bottomColor, button.topColor, button.name);
  }
}

void DrawButton(int xPos, int yPos, int width, int height, String caption, String waarde, uint16_t bottomColor, uint16_t topColor, String Name) {
  tft.setTextDatum(MC_DATUM);
  DrawBox(xPos, yPos, width, height);

  uint16_t gradientStartColor = TFT_BLACK;
  if (Name == FindButtonNameByID(settings.activeBtn)) gradientStartColor = topColor;

  tft.fillRectVGradient(xPos + 2, yPos + 2, width - 4, (height / 2) + 1, gradientStartColor, topColor);
  tft.setTextPadding(tft.textWidth(caption));
  tft.setTextColor(TFT_WHITE);
  if (gradientStartColor == topColor) tft.setTextColor(TFT_BLACK);
  tft.drawString(caption, xPos + (width / 2), yPos + (height / 2) - 5, 2);

  if (Name == "Navigate") {
    tft.setTextDatum(ML_DATUM);
    tft.setTextPadding(tft.textWidth("     <<     <"));
    tft.setTextColor(TFT_BLACK);
    tft.drawString("     <<     <", 5, yPos + (height / 2) - 5, 2);
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth(">     >>     "));
    tft.setTextColor(TFT_BLACK);
    tft.drawString(">     >>     ", 309, yPos + (height / 2) - 5, 2);
  }

  //tft.fillRectVGradient(xPos + 2,yPos + 2 + (height/2), width-4, (height/2)-4, TFT_BLACK, bottomColor);
  tft.fillRoundRect(xPos + 2, yPos + 2 + (height / 2), width - 4, (height / 2) - 4, 3, bottomColor);

  if (waarde != "") {
    tft.setTextPadding(tft.textWidth(waarde));
    tft.setTextColor(TFT_YELLOW);
    if (bottomColor != TFT_BLACK) tft.setTextColor(TFT_BLACK);
    tft.drawString(waarde, xPos + (width / 2), yPos + (height / 2) + 9, 1);
  }
}

int ShowControls() {
  int retVal = actualPage;
  if (actualPage == 1) retVal = retVal + BTN_ARROW + BTN_NAV;
  // if (actualPage >1 && actualPage <lastPage) retVal = retVal + BTN_ARROW + BTN_NEXT + BTN_PREV;
  if (actualPage == lastPage) retVal = retVal + BTN_CLOSE;
  return retVal;
}

Button FindButtonByName(String Name) {
  for (int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
    if (String(buttons[i].name) == Name) return FindButtonInfo(buttons[i]);
  }
  return buttons[0];
}

int8_t FindButtonIDByName(String Name) {
  for (int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
    if (String(buttons[i].name) == Name) return i;
  }
  return -1;
}

String FindButtonNameByID(int8_t id) {
  return String(buttons[id].name);
}

Button FindButtonInfo(Button button) {
  char buttonBuf[10] = "\0";

  if (button.name == "Vol") {
    sprintf(buttonBuf, "%d", settings.volume);
    strcpy(button.waarde, buttonBuf);
  }

  if (button.name == "Mute") {
    if (settings.isMuted == 0) strcpy(button.waarde, "");
    if (settings.isMuted == 1) strcpy(button.waarde, "Right");
    if (settings.isMuted == 2) strcpy(button.waarde, "Left");
    if (settings.isMuted == 3) strcpy(button.waarde, "Muted");

    button.bottomColor = settings.isMuted ? TFT_RED : TFT_BLACK;
    sprintf(buf, "%s", WebMuteStatus());
    events.send(buf, "myMute", millis());
  }

  if (button.name == "Mode") {
    sprintf(buttonBuf, "%s", settings.isDab ? "DAB" : "FM");
    strcpy(button.waarde, buttonBuf);
    button.bottomColor = settings.isDab ? TFT_GREEN : TFT_RED;
    sprintf(buf, "%s", settings.isDab ? "DAB" : "FM");
    events.send(buf, "myMode", millis());
  }

  if (button.name == "Tune") {
    if (settings.isDab) {
      //sprintf(buttonBuf,"%d",settings.dabChannel);
      sprintf(buttonBuf, "%d (%d/%d)", settings.dabChannel, actualDabService, Dab.numberofservices - 1);
      strcpy(button.waarde, buttonBuf);
    }
  }

  if (button.name == "MEM") {
    sprintf(buttonBuf, "%d", settings.memoryChannel);
    strcpy(button.waarde, buttonBuf);
  }

  if (button.name == "Save") {
    button.bottomColor = settings.activeBtn == FindButtonIDByName("MEM") ? TFT_GREY : TFT_BLACK;
  }

  if (button.name == "LoadList") {
    sprintf(buttonBuf, "        ");
    if (settings.activeBtn == FindButtonIDByName("LoadList") && settings.dabChannelsCount > 0)
      sprintf(buttonBuf, "%d/%d", settings.dabChannelSelected, settings.dabChannelsCount - 1);
    else
      sprintf(buttonBuf, "%d", settings.dabChannelsCount - 1);
    strcpy(button.waarde, buttonBuf);
    sprintf(buf, "%d", settings.dabChannelSelected);
    events.send(buf, "selectedStation", millis());
  }

  if (button.name == "Stereo") {
    sprintf(buttonBuf, "%s", settings.isStereo ? "Stereo" : "Mono");
    strcpy(button.waarde, buttonBuf);
    button.bottomColor = settings.isStereo ? TFT_GREEN : TFT_BLACK;
  }

  if (button.name == "Light") {
    DebugPrintln(GetLogoName(settings.dabServiceID));
    sprintf(buttonBuf, "%d", settings.currentBrightness);
    strcpy(button.waarde, buttonBuf);
  }

  if (button.name == "Info") {
    sprintf(buttonBuf, "%s", (settings.activeBtn == FindButtonIDByName("LoadList") && settings.dabChannelsCount > 0) ? "Zoek logos" : "");
    strcpy(button.waarde, buttonBuf);
  }

  if (button.name == "Off") {
    sprintf(buttonBuf, "%s", (settings.activeBtn == FindButtonIDByName("LoadList") && settings.dabChannelsCount > 0) ? "5sec reset" : "");
    strcpy(button.waarde, buttonBuf);
  }

  if (button.name == "Navigate") {
    sprintf(buttonBuf, "%s", FindButtonNameByID(settings.activeBtn));
    button.caption = buttonBuf;
  }
  return button;
}

void DrawBox(int xPos, int yPos, int width, int height) {
  tft.drawRoundRect(xPos + 2, yPos + 2, width, height, 4, TFT_SHADOW);
  tft.drawRoundRect(xPos, yPos, width, height, 3, TFT_WHITE);
  tft.fillRoundRect(xPos + 1, yPos + 1, width - 2, height - 2, 3, TFT_BLACK);
}

void DrawKeyboardNumber(bool doReset) {
  tft.setTextDatum(MC_DATUM);
  sprintf(buf, "%s", FindButtonNameByID(settings.activeBtn));
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(buf, 162, 15, 4);

  tft.setTextDatum(MR_DATUM);
  if (doReset) keyboardNumber = 0;
  int f1 = floor(keyboardNumber / 10000);
  int f2 = keyboardNumber - (f1 * 10000);
  sprintf(buf, "%8d", keyboardNumber);
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(buf, 282, 55, 7);
  if (settings.activeBtn == FindButtonIDByName("LoadList")) {
    tft.fillRect(0, 82, 315, 12, TFT_BLACK);
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth(dabChannels[settings.dabChannelSelected].dabName));
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString(dabChannels[keyboardNumber].dabName, 282, 88, 2);
    DebugPrintln(dabChannels[keyboardNumber].dabName);
  }
}

/***************************************************************************************
**            Draw meter
***************************************************************************************/
void DrawMeter(int xPos, int yPos, int width, int height, int value, int minValue, int maxValue, int orangeColorPCT, int greenColorPCT, String dispText) {

  tft.setTextDatum(MC_DATUM);
  // DrawBox(xPos, yPos, width, height);
  tft.setTextColor(TFT_WHITE);

  int scale = (width * 10) / (maxValue - minValue);
  for (int i = 0; i < maxValue - minValue; i++) {
    float mPos = ((i * scale) / 10) + xPos;
    uint16_t signColor = TFT_RED;
    if (i <= value) {
      if (i > (((maxValue - minValue) * orangeColorPCT) / 100)) signColor = TFT_ORANGE;
      if (i > (((maxValue - minValue) * greenColorPCT) / 100)) signColor = TFT_GREEN;
      tft.fillRect(mPos, yPos, (scale / 10) + 1, height, signColor);
    }
  }

  sprintf(buf, "%s%d", dispText, value);
  tft.setTextDatum(ML_DATUM);
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_WHITE);
  tft.drawString(buf, xPos + 5, yPos + (height / 2), 1);
}

/***************************************************************************************
**            Handle button
***************************************************************************************/

void HandleWebCommand() {
  if (webCommand == "TuneUp") HandleWebButton("ToRight", "Tune");
  if (webCommand == "TuneDown") HandleWebButton("ToLeft", "Tune");
  if (webCommand == "MemUp") HandleWebButton("ToRight", "MEM");
  if (webCommand == "MemDown") HandleWebButton("ToLeft", "MEM");
  if (webCommand == "VolumeUp") HandleWebButton("ToRight", "Vol");
  if (webCommand == "VolumeDown") HandleWebButton("ToLeft", "Vol");
  if (webCommand == "ChannelUp") HandleWebButton("ToRight", "LoadList");
  if (webCommand == "ChannelDown") HandleWebButton("ToLeft", "LoadList");
  if (webCommand == "GoChannel") {
    DebugPrintf("Keyboard is %d\r\n", keyboardNumber);
    HandleWebButton("Enter", "LoadList");
  }
  if (webCommand == "Mute") HandleWebButton("Mute", "LoadList");
  if (webCommand == "Mode") HandleWebButton("Mode", "LoadList");
  webCommand = "None";
}

void HandleWebButton(String buttonName, String activeButtonName) {
  settings.activeBtn = FindButtonIDByName(activeButtonName);
  HandleFunction(buttonName, true);
}

void HandleFunction(String buttonName, bool doDraw) {
  Button button = FindButtonByName(buttonName);
  HandleFunction(button, 0, 0, doDraw);
}

void HandleFunction(String buttonName, int x, int y) {
  HandleFunction(buttonName, x, y, true);
}

void HandleFunction(String buttonName, int x, int y, bool doDraw) {
  Button button = FindButtonByName(buttonName);
  HandleFunction(button, x, y, doDraw);
}

void HandleFunction(Button button, int x, int y) {
  HandleFunction(button, x, y, true);
}

void HandleFunction(Button button, int x, int y, bool doDraw) {
  DebugPrintln(button.name);
  if (button.name == "ToRight") {
    if (settings.activeBtn == FindButtonIDByName("Tune")) {
      if (doDraw) DrawPatience();
      if (settings.isDab) {
        if (actualDabService < Dab.numberofservices - 1) {
          actualDabService++;
          settings.dabServiceID = Dab.service[actualDabService].ServiceID;
        } else {
          if (settings.dabChannel < DAB_FREQS - 1) {
            settings.dabChannel++;
            if (doDraw) DrawButton("Tune");
            Dab.tune(settings.dabChannel);
            while (Dab.numberofservices < 1 && settings.dabChannel < DAB_FREQS - 1) {
              settings.dabChannel++;
              if (doDraw) DrawButton("Tune");
              Dab.tune(settings.dabChannel);
            }
            if (Dab.numberofservices < 1 && settings.dabChannel == DAB_FREQS - 1) {
              while (Dab.numberofservices < 1 && settings.dabChannel > 0) {
                settings.dabChannel--;
                if (doDraw) DrawButton("Tune");
                Dab.tune(settings.dabChannel);
              }
            }
            actualDabChannel = settings.dabChannel;
            actualDabService = 0;
            settings.dabServiceID = Dab.service[actualDabService].ServiceID;
          }
        }
        SetRadio();
        DebugPrintf("Tune:%d, Service:%d, Channels:%d", settings.dabChannel, actualDabService, DAB_FREQS);
      }
      if (!settings.isDab) {
        Dab.seek(1, 1);
        settings.fmFreq = Dab.freq * 10;
        DebugPrintln(settings.fmFreq);
        if (doDraw) DrawFrequency();
      }
      if (doDraw) DrawButtons();
    } else if (settings.activeBtn == FindButtonIDByName("Vol")) {
      if (settings.volume < 63) settings.volume++;
      Dab.vol(settings.volume);
      settings.isMuted = 0;
      SetMute();
      if (doDraw) DrawButton("Vol");
      if (doDraw) DrawButton("Mute");
    } else if (settings.activeBtn == FindButtonIDByName("MEM")) {
      if (settings.memoryChannel < 9) {
        settings.memoryChannel++;
        SetRadioFromMemory(settings.memoryChannel);
        if (doDraw) DrawButton("MEM");
      }
    } else if (settings.activeBtn == FindButtonIDByName("LoadList")) {
      if (settings.dabChannelSelected < settings.dabChannelsCount - 2) {
        settings.dabChannelSelected++;
        SetRadioFromList(settings.dabChannelSelected);
        if (doDraw) DrawButtons();
      }
    } else if (settings.activeBtn == FindButtonIDByName("Light")) {
      if (settings.currentBrightness < 100) settings.currentBrightness += 5;
      if (settings.currentBrightness > 100) settings.currentBrightness = 100;
      if (doDraw) DrawButton("Light");
      ledcWrite(ledChannelforTFT, 256 - (settings.currentBrightness * 2.56));
    }
    if (doDraw) DrawButton("Navigate");
  }

  if (button.name == "ToLeft") {
    if (settings.activeBtn == FindButtonIDByName("Tune")) {
      if (doDraw) DrawPatience();
      if (settings.isDab) {
        if (actualDabService > 0) {
          actualDabService--;
          settings.dabServiceID = Dab.service[actualDabService].ServiceID;
        } else {
          if (settings.dabChannel > 0) {
            settings.dabChannel--;
            Dab.tune(settings.dabChannel);
            while (Dab.numberofservices < 1 && settings.dabChannel > 0) {
              settings.dabChannel--;
              if (doDraw) DrawButton("Tune");
              Dab.tune(settings.dabChannel);
            }
            if (Dab.numberofservices < 1 && settings.dabChannel == 0) {
              while (Dab.numberofservices < 1 && settings.dabChannel < DAB_FREQS - 1) {
                settings.dabChannel++;
                if (doDraw) DrawButton("Tune");
                Dab.tune(settings.dabChannel);
              }
            }
            actualDabChannel = settings.dabChannel;
            actualDabService = Dab.numberofservices - 1;
            settings.dabServiceID = Dab.service[actualDabService].ServiceID;
          }
        }
        SetRadio();
        DebugPrintf("Tune:%d, Service:%d, Channels:%d", settings.dabChannel, actualDabService, DAB_FREQS);
      }
      if (!settings.isDab) {
        Dab.seek(0, 1);
        settings.fmFreq = Dab.freq * 10;
        if (doDraw) DrawFrequency();
      }
      if (doDraw) DrawButtons();
    } else if (settings.activeBtn == FindButtonIDByName("Vol")) {
      if (settings.volume > 0) settings.volume--;
      Dab.vol(settings.volume);
      settings.isMuted = 0;
      SetMute();
      if (doDraw) DrawButton("Vol");
      if (doDraw) DrawButton("Mute");
    } else if (settings.activeBtn == FindButtonIDByName("MEM")) {
      if (settings.memoryChannel > 0) {
        settings.memoryChannel--;
        SetRadioFromMemory(settings.memoryChannel);
        if (doDraw) DrawButton("MEM");
      }
    } else if (settings.activeBtn == FindButtonIDByName("LoadList")) {
      if (settings.dabChannelSelected > 0) {
        settings.dabChannelSelected--;
        SetRadioFromList(settings.dabChannelSelected);
        if (doDraw) DrawButtons();
      }
    } else if (settings.activeBtn == FindButtonIDByName("Light")) {
      if (settings.currentBrightness > 5) settings.currentBrightness -= 5;
      if (settings.currentBrightness < 5) settings.currentBrightness = 5;
      if (doDraw) DrawButton("Light");
      ledcWrite(ledChannelforTFT, 256 - (settings.currentBrightness * 2.56));
    }
    if (doDraw) DrawButton("Navigate");
  }

  if (button.name == "Vol") {
    if (settings.activeBtn == FindButtonIDByName("Vol")) {
      keyboardNumber = settings.volume;
      actualPage = BTN_NUMERIC;
      if (doDraw) DrawScreen();
    } else {
      settings.activeBtn = FindButtonIDByName("Vol");
      if (doDraw) DrawButtons();
    }
  }

  if (button.name == "Mute") {
    settings.isMuted++;
    SetMute();
    if (doDraw) DrawButton("Mute");
  }

  if (button.name == "Mode") {
    DrawPatience();
    settings.isDab = !settings.isDab;
    SetRadio();
    if (settings.isDab)
      settings.activeBtn = FindButtonIDByName("Service");
    else
      settings.activeBtn = FindButtonIDByName("Tune");
    if (doDraw) DrawButtons();
  }

  if (button.name == "Tune") {
    if (settings.isDab) {
      if (settings.activeBtn == FindButtonIDByName("Tune")) {
        keyboardNumber = settings.dabChannel;
        actualPage = BTN_NUMERIC;
        if (doDraw) DrawScreen();
      } else {
        settings.activeBtn = FindButtonIDByName("Tune");
        if (doDraw) DrawButtons();
      }
    } else {
      if (settings.activeBtn == FindButtonIDByName("Tune")) {
        keyboardNumber = 0;
        actualPage = BTN_NUMERIC;
        if (doDraw) DrawScreen();
      } else {
        settings.activeBtn = FindButtonIDByName("Tune");
        if (doDraw) DrawButtons();
      }
    }
  }

  if (button.name == "Info") {
    if (settings.activeBtn == FindButtonIDByName("LoadList")) {
      tft.fillRect(0, 136, 320, 240, TFT_BLACK);
      settings.dabChannelSelected = 0;
      qsort(dabChannels, settings.dabChannelsCount, sizeof(DABChannel), SortOnDabChannel);
      for (int i = 0; i < settings.dabChannelsCount - 1; i++) {
        sprintf(buf, "Channel (%d/%d)", i, settings.dabChannelsCount - 1);
        DrawButton(80, 160, 160, 30, "Ophalen logo's...", buf, TFT_BLACK, TFT_RED, "");
        settings.dabChannelSelected = i;
        SetRadioFromList(settings.dabChannelSelected);
        delay(1000);
      }
      qsort(dabChannels, settings.dabChannelsCount, sizeof(DABChannel), SortOnDabName);
      settings.showOnlyCachedLogos = true;
      tft.fillRect(0, 136, 320, 240, TFT_BLACK);
      if (doDraw) DrawButtons();
    } else {
      actualPage = lastPage;
      DrawScreen();

      tft.fillRect(2, 2, 320, 200, TFT_BLACK);
      tft.setTextDatum(ML_DATUM);
      tft.setTextPadding(50);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);

      if (wifiAvailable || wifiAPMode) {
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString("WiFi:", 2, 10, 2);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("SSID        :" + String(WiFi.SSID()), 2, 25, 1);
        sprintf(buf, "IP Address  :%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
        tft.drawString(buf, 2, 33, 1);
        tft.drawString("RSSI        :" + String(WiFi.RSSI()), 2, 41, 1);
      }

      if (settings.isDab) {
        tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
        tft.drawString("Service:", 2, 60, 2);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        for (int i = 0; i < Dab.numberofservices; i++) {
          sprintf(buf, "%2d=(%d) %s", i, Dab.service[i].ServiceID, Dab.service[i].Label);
          tft.drawString(buf, 2, 75 + (i * 8), 1);
        }
      }

      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.drawString("Memory:", 182, 60, 2);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      for (int i = 0; i < 10; i++) {
        sprintf(buf, "%d = %s, %d,%d ", i, memories[i].isDab ? "Dab" : "FM", memories[i].isDab ? memories[i].dabChannel : memories[i].fmFreq, memories[i].isDab ? memories[i].dabServiceID : NULL);
        tft.drawString(buf, 182, 75 + (i * 8), 1);
      }
    }
  }

  if (button.name == "Service") {
    if (settings.isDab) {
      if (settings.activeBtn == FindButtonIDByName("Service")) {
        keyboardNumber = actualDabService;
        actualPage = BTN_NUMERIC;
        if (doDraw) DrawScreen();
      } else {
        settings.activeBtn = FindButtonIDByName("Service");
        if (doDraw) DrawButtons();
      }
    }
  }

  if (button.name == "MEM") {
    if (settings.activeBtn == FindButtonIDByName("MEM")) {
      keyboardNumber = settings.memoryChannel;
      actualPage = BTN_NUMERIC;
      if (doDraw) DrawScreen();
    } else {
      SetRadioFromMemory(settings.memoryChannel);
      settings.activeBtn = FindButtonIDByName("MEM");
      if (doDraw) DrawButtons();
    }
  }

  if (button.name == "Save") {
    if (settings.activeBtn != FindButtonIDByName("MEM")) {
      settings.activeBtn = FindButtonIDByName("Save");
      keyboardNumber = settings.memoryChannel;
      actualPage = BTN_NUMERIC;
      if (doDraw) DrawScreen();
    }
  }

  if (button.name == "Stereo") {
    settings.isStereo = !settings.isStereo;
    Dab.mono(!settings.isStereo);
    if (doDraw) DrawButton("Stereo");
  }

  if (button.name == "Calibrate") {
    TouchCalibrate();
    actualPage = 1;
    if (doDraw) DrawScreen();
  }

  if (button.name == "LoadList") {
    if (settings.dabChannelsCount == 0) {
      DrawPatience();
      LoadList();
      actualPage = 1;
      if (doDraw) DrawScreen();
    } else {
      if (settings.activeBtn == FindButtonIDByName("LoadList")) {
        keyboardNumber = settings.dabChannelSelected;
        actualPage = BTN_NUMERIC;
        if (doDraw) DrawScreen();
      } else {
        SetRadioFromList(settings.dabChannelSelected);
        settings.activeBtn = FindButtonIDByName("LoadList");
        if (doDraw) DrawButtons();
      }
    }
  }

  if (button.name == "Light") {
    settings.activeBtn = FindButtonIDByName("Light");
    if (doDraw) DrawButtons();
  }

  if (button.name == "Next") {
    actualPage = actualPage << 1;
    if (actualPage > lastPage) actualPage = 1;
    if (doDraw) DrawScreen();
  }

  if (button.name == "Prev") {
    if (actualPage > 1) actualPage = actualPage >> 1;
    if (doDraw) DrawScreen();
  }

  if (button.name == "Close") {
    actualPage = 1;
    if (doDraw) DrawScreen();
  }

  if (button.name == "Off") {
    long startOffPressed = millis();
    uint16_t x = 0, y = 0;
    bool pressed = tft.getTouch(&x, &y);

    if (settings.activeBtn == FindButtonIDByName("LoadList") && settings.dabChannelsCount > 0){
      while (pressed){
        pressed = tft.getTouch(&x, &y);
        DebugPrint(".");
        delay(100);
        if (((millis() - startOffPressed) > 5000)) {
          tft.fillRect(0, 0, 320, 98, TFT_BLACK);
          DrawButton(80, 20, 160, 30, "Release to reset", "", TFT_BLACK, TFT_RED, "");
        }
      } 
        
      if (((millis() - startOffPressed) > 5000)) {
        if ((settings.activeBtn == FindButtonIDByName("LoadList")) && (settings.dabChannelsCount > 0)) {
          DrawPatience(false, "Channels gereset");
          DebugPrintf("Off pressed for %d seconds\r\n", millis() / 1000 - startOffPressed);

          SPIFFS.format();
          settings.dabChannelSelected = 0;
          settings.dabChannelsCount = 0;
          settings.showOnlyCachedLogos = false;
          dabChannels = {};
          delay(1000);
          DrawScreen();
        }
      }
    } else {
      if (millis() - startOffPressed < 500) {
        isOn = false;
        actualPage = 1;
        DoTurnOff();
        delay(500);
      }
    }
  }

  if (String(button.name).substring(0, 3) == "A00") {
    bool reachedNull = false;
    if (String(button.name).substring(3) == "M") {
      if (keyboardNumber > 0)
        keyboardNumber--;
      else
        reachedNull = true;
    } else if (String(button.name).substring(3) == "P") {
      keyboardNumber++;
    } else {
      int i = String(button.name).substring(3).toInt();
      keyboardNumber = (keyboardNumber * 10) + i;
    }

    if (settings.activeBtn == FindButtonIDByName("Tune")) {
      if (settings.isDab && keyboardNumber >= DAB_FREQS) keyboardNumber = 0;
      if (settings.isDab && reachedNull) keyboardNumber = DAB_FREQS - 1;
      if (!settings.isDab && keyboardNumber > 107900) keyboardNumber = 88500;
    }
    if (settings.activeBtn == FindButtonIDByName("Vol")) {
      if (keyboardNumber > 63) keyboardNumber = 0;
    }
    if (settings.activeBtn == FindButtonIDByName("Service")) {
      if (keyboardNumber > Dab.numberofservices - 1) keyboardNumber = 0;
      if (reachedNull) keyboardNumber = Dab.numberofservices - 1;
    }
    if (settings.activeBtn == FindButtonIDByName("MEM")) {
      if (keyboardNumber > 9) keyboardNumber = 0;
      if (reachedNull) keyboardNumber = 9;
    }
    if (settings.activeBtn == FindButtonIDByName("Save")) {
      if (keyboardNumber > 9) keyboardNumber = 0;
    }
    if (settings.activeBtn == FindButtonIDByName("LoadList")) {
      if (keyboardNumber > settings.dabChannelsCount - 1) keyboardNumber = 0;
      if (reachedNull) keyboardNumber = settings.dabChannelsCount - 1;
    }
    if (doDraw) DrawKeyboardNumber(false);
  }

  if (button.name == "Enter") {
    if (settings.activeBtn == FindButtonIDByName("Vol")) {
      settings.volume = keyboardNumber;
      Dab.vol(settings.volume);
    }
    if (settings.activeBtn == FindButtonIDByName("Tune")) {
      if (settings.isDab) {
        DrawPatience();
        settings.dabChannel = keyboardNumber;
        actualDabService = 0;
        Dab.tune(settings.dabChannel);
        Dab.set_service(actualDabService);
        settings.dabServiceID = Dab.service[actualDabService].ServiceID;
      }
      if (!settings.isDab) {
        while (keyboardNumber < 87500) keyboardNumber *= 10;
        if (keyboardNumber >= 87500 && keyboardNumber <= 107900) {
          DrawPatience();
          settings.fmFreq = keyboardNumber;
          Dab.tune((uint16_t)(settings.fmFreq / 10));
        }
      }
    }
    if (settings.activeBtn == FindButtonIDByName("Service")) {
      if (settings.isDab) {
        actualDabService = keyboardNumber;
        Dab.set_service(actualDabService);
        settings.dabServiceID = Dab.service[actualDabService].ServiceID;
      }
    }
    if (settings.activeBtn == FindButtonIDByName("MEM")) {
      settings.memoryChannel = keyboardNumber;
      SetRadioFromMemory(settings.memoryChannel);
    }
    if (settings.activeBtn == FindButtonIDByName("LoadList")) {
      settings.dabChannelSelected = keyboardNumber;
      SetRadioFromList(settings.dabChannelSelected);
    }
    if (settings.activeBtn == FindButtonIDByName("Save")) {
      settings.memoryChannel = keyboardNumber;
      DebugPrintf("SaveButton:%d\r\n", settings.memoryChannel);
      Memory myMemory = { settings.isDab, settings.isStereo, settings.dabChannel, settings.dabServiceID, settings.fmFreq };
      memories[settings.memoryChannel] = myMemory;
      SetRadioFromMemory(settings.memoryChannel);
      SaveMemories();
      settings.activeBtn = FindButtonIDByName("MEM");
    }
    actualPage = 1;
    if (doDraw) DrawScreen();
  }

  if (button.name == "Clear") {
    if (keyboardNumber > 0) {
      keyboardNumber = 0;
      if (doDraw) DrawKeyboardNumber(false);
    } else {
      actualPage = 1;
      if (settings.activeBtn == FindButtonIDByName("LoadList")) {
        settings.dabChannelSelected = 0;
        settings.dabChannelsCount = 0;
        dabChannels = {};
        settings.showOnlyCachedLogos = false;
      }
      if (doDraw) DrawScreen();
    }
  }
}

void SetRadioFromMemory(int channel) {
  DrawPatience();
  settings.isDab = memories[channel].isDab;
  settings.isStereo = memories[channel].isStereo;
  settings.dabChannel = memories[channel].dabChannel;
  settings.dabServiceID = memories[channel].dabServiceID;
  settings.fmFreq = memories[channel].fmFreq;
  SetRadio(false, false);
  if (settings.isDab) actualDabService = findChannelOnDabServiceID(settings.dabServiceID);
  DebugPrintf("Set from Memory: DABChannel=%d, DABService=%d %d, DABServiceID=%d, isDab=%d, Volume=%d, isStereo=%d\r\n", settings.dabChannel, actualDabService, findChannelOnDabServiceID(settings.dabServiceID), settings.dabServiceID, settings.isDab, settings.volume, settings.isStereo);
  DrawFrequency();
}

void SetRadioFromList(int channel) {
  DrawPatience();
  settings.isDab = true;
  settings.isStereo = true;
  settings.dabChannel = dabChannels[channel].dabChannel;
  settings.dabServiceID = dabChannels[channel].dabServiceID;
  SetRadio(false, false);
  if (settings.isDab) actualDabService = findChannelOnDabServiceID(settings.dabServiceID);
  DebugPrintf("Set from List: DABChannel=%d, DABService=%d %d, DABServiceID=%d, isDab=%d, Volume=%d, isStereo=%d\r\n", settings.dabChannel, actualDabService, findChannelOnDabServiceID(settings.dabServiceID), settings.dabServiceID, settings.isDab, settings.volume, settings.isStereo);
  DrawFrequency();
}

void SetRadio() {
  SetRadio(false, true);
}

void SetRadio(bool firstTime) {
  SetRadio(firstTime, false);
}

void SetRadio(bool firstTime, bool drawFreq) {
  actualInfo[0] = '\0';
  DebugPrintf("Set from Radio: DABChannel=%d, DABService=%d %d, DABServiceID=%d, isDab=%d, Volume=%d, isStereo=%d\r\n", settings.dabChannel, actualDabService, findChannelOnDabServiceID(settings.dabServiceID), settings.dabServiceID, settings.isDab, settings.volume, settings.isStereo);

  bool doTune = false;
  if (actualIsDab != settings.isDab || firstTime) {
    actualIsDab = settings.isDab;
    doTune = true;
    Dab.begin(settings.isDab ? 0 : 1);
    Dab.vol(settings.volume);
  }

  if (actualDabVol != settings.volume || firstTime) {
    actualDabVol = settings.volume;
    Dab.vol(settings.volume);
  }

  SetMute();
  if (actualIsStereo != settings.isStereo || firstTime) {
    actualIsStereo = settings.isStereo;
    Dab.mono(!settings.isStereo);
  }

  if (settings.isDab) {
    if (actualDabChannel != settings.dabChannel || firstTime || doTune) {
      actualDabChannel = settings.dabChannel;
      Dab.tune(settings.dabChannel);
      DebugPrintf("Dab channel set to %d\r\n", actualDabChannel);
    }
    if (firstTime) actualDabService = findChannelOnDabServiceID(settings.dabServiceID);
    Dab.set_service(findChannelOnDabServiceID(settings.dabServiceID));
  } else {
    if (actualFmFreq != settings.fmFreq || firstTime || doTune) {
      actualFmFreq != settings.fmFreq;
      Dab.tune((uint16_t)(settings.fmFreq / 10));
    }
  }
  if (drawFreq) DrawFrequency();
}

void addRec(int size) {
  if (dabChannels != 0) {
    dabChannels = (DABChannel *)realloc(dabChannels, size * sizeof(DABChannel));
  } else {
    dabChannels = (DABChannel *)malloc(size * sizeof(DABChannel));
  }
}

void LoadList() {
  dabChannels = 0;
  settings.dabChannelsCount = 0;
  for (uint8_t i = 0; i < DAB_FREQS; i++) {
    sprintf(buf, "Scan %d/%d (%d)", i, DAB_FREQS - 1, settings.dabChannelsCount);
    DrawButton(60, 20, 200, 30, "Even geduld...", buf, TFT_BLACK, TFT_RED, "");
    DebugPrintf("ID:%d\r\n", i);
    Dab.tune(i);
    actualDabChannel = i;
    //if (Dab.numberofservices>0){
    if (Dab.servicevalid()) {
      for (int j = 0; j < Dab.numberofservices; j++) {
        actualDabService = j;
        DebugPrintf("ID:%d, Service:%d\r\n", i, j);
        addRec(settings.dabChannelsCount + 1);
        dabChannels[settings.dabChannelsCount].dabChannel = i;
        dabChannels[settings.dabChannelsCount].dabServiceID = Dab.service[j].ServiceID;
        sprintf(dabChannels[settings.dabChannelsCount].dabName, "%s", Dab.service[j].Label);
        settings.dabChannelsCount++;
      }
    }
  }

  DebugPrintln("Show list");
  if (settings.dabChannelsCount > 0) {
    for (int i = 0; i < settings.dabChannelsCount; i++) {
      DebugPrintf("ID %d = Channel:%d, ServiceID:%d, Name:%s\r\n", i, dabChannels[i].dabChannel, dabChannels[i].dabServiceID, dabChannels[i].dabName);
    }
    settings.dabChannelSelected = 0;
    SaveStationList();
    SetRadioFromList(0);
  }
}

void SetMute() {
  if (settings.isMuted > 3) settings.isMuted = 0;
  if (settings.isMuted == 0) Dab.mute(false, false);
  if (settings.isMuted == 1) Dab.mute(true, false);
  if (settings.isMuted == 2) Dab.mute(false, true);
  if (settings.isMuted == 3) Dab.mute(true, true);
  digitalWrite(MUTE_PIN, settings.isMuted == 3?1:0);
}

/***************************************************************************************
**            Turn off
***************************************************************************************/
void DoTurnOff() {
  ledcWrite(ledChannelforTFT, 255);
  tft.fillScreen(TFT_BLACK);
  Dab.mute(1, 1);
  digitalWrite(MUTE_PIN, 1);
}

void WaitForWakeUp() {
  while (!isOn) {
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
  bool commitEeprom = false;
  for (unsigned int t = 0; t < sizeof(settings); t++){
    if (*((char *)&settings + t) != EEPROM.read(offsetEEPROM + t)){
      EEPROM.write(offsetEEPROM + t, *((char *)&settings + t));
      commitEeprom = true;
    } 
  }
  if (commitEeprom) EEPROM.commit();
  return true;
}

bool LoadConfig() {
  bool retVal = true;
  if (EEPROM.read(offsetEEPROM + 0) == settings.chkDigit) {
    for (unsigned int t = 0; t < sizeof(settings); t++)
      *((char *)&settings + t) = EEPROM.read(offsetEEPROM + t);
  } else retVal = false;
  DebugPrintln("Configuration:" + retVal ? "Loaded" : "Not loaded");
  return retVal;
}

bool SaveMemories() {
  for (unsigned int t = 0; t < sizeof(memories); t++)
    EEPROM.write(offsetEEPROM + sizeof(settings) + 10 + t, *((char *)&memories + t));
  EEPROM.commit();
  DebugPrintln("Memories:saved");
  return true;
}

bool LoadMemories() {
  for (unsigned int t = 0; t < sizeof(memories); t++)
    *((char *)&memories + t) = EEPROM.read(offsetEEPROM + sizeof(settings) + 10 + t);
  DebugPrintln("Memories loaded");
  return true;
}

int SortOnDabName(const void *v1, const void *v2) {
  DABChannel *ia = (DABChannel *)v1;
  DABChannel *ib = (DABChannel *)v2;
  return strcmp(ia->dabName, ib->dabName);
}

int SortOnDabChannel(const void *v1, const void *v2) {
  DABChannel *ia = (DABChannel *)v1;
  DABChannel *ib = (DABChannel *)v2;
  return ia->dabChannel - ib->dabChannel;
}

bool SaveStationList() {
  qsort(dabChannels, settings.dabChannelsCount, sizeof(DABChannel), SortOnDabName);
  for (int i = 0; i < settings.dabChannelsCount; i++)
    for (unsigned int t = 0; t < sizeof(DABChannel); t++)
      EEPROM.write(offsetEEPROM + sizeof(settings) + sizeof(memories) + (i * sizeof(DABChannel)) + 10 + t, *((char *)&dabChannels[i] + t));
  EEPROM.commit();
  return true;
}

bool LoadStationList() {
  DebugPrintf("Loading Stationlist: %d stations of %d bytes", settings.dabChannelsCount, sizeof(DABChannel));
  for (int i = 0; i < settings.dabChannelsCount; i++) {
    addRec(i + 1);
    for (unsigned int t = 0; t < sizeof(DABChannel); t++)
      *((char *)&dabChannels[i] + t) = EEPROM.read(offsetEEPROM + sizeof(settings) + sizeof(memories) + (i * sizeof(DABChannel)) + 10 + t);
  }
  DebugPrintln("Stationlist loaded");
  return true;
}
/***************************************************************************************
**            De rest
***************************************************************************************/
void DABSpiMsg(unsigned char *data, uint32_t len) {
  hspi->beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));  //2MHz for starters...
  digitalWrite(slaveSelectPin, LOW);
  hspi->transfer(data, len);
  digitalWrite(slaveSelectPin, HIGH);
  hspi->endTransaction();
}
/***************************************************************************************
**            Logo's
***************************************************************************************/
uint16_t GetDABLogo(uint16_t ServiceID, uint16_t EnsembleID, uint16_t ECC, bool onlyCached) {
  uint16_t GCC = ECC + ((ServiceID >> 4) & 0xF00);
  char radioDNS[128];
  char bearer[128];

  sprintf(radioDNS, "0.%04x.%04x.%x.dab.radiodns.org", ServiceID, EnsembleID, GCC);
  sprintf(bearer, "dab:%x.%04x.%04x.0", GCC, EnsembleID, ServiceID);

  char pngfilename[16];
  char jpgfilename[16];

  sprintf(pngfilename, "/%04x.png", ServiceID);
  sprintf(jpgfilename, "/%04x.jpg", ServiceID);
  if (SPIFFS.exists(pngfilename) || SPIFFS.exists(jpgfilename)) {
    DrawLogo(ServiceID);
    return ServiceID;
  }
  if (onlyCached) return ServiceID;
  return GetLogo(radioDNS, bearer, ServiceID);
}

uint16_t GetLogo(char *service, char *bearer, uint32_t id) {
  uint16_t newlogo = 0;
  char cname[128];
  char srv[128];
  char filename[32];

  DebugPrintf("radioDNS = %s\n", service);
  DebugPrintf("bearer = %s\n", bearer);

  if (GetCNAME(cname, service) == 0) {
    DebugPrintln("No CNAME");
    return 0;
  }

  if (GetSRV(srv, cname) == 0)
    return 0;

  servicexml = (char *)malloc(16 * 1024);
  if (GetServiceInfo(servicexml, (16 * 1024), srv, bearer) == 0) {
    free(servicexml);
    return 0;
  }

  if (ParseXMLImageURL(imageurl, mime, servicexml) == 0) {
    free(servicexml);
    return 0;
  }

  free(servicexml);

  if (strlen(imageurl)) {
    //find the extension...
    char *ext = strrchr(imageurl, '.');
    if (!ext || ext == imageurl)
      return 0;

    if (strcmp(ext, ".png") || strcmp(ext, ".jpg")) {
      ext[4] = '\0';
      sprintf(filename, "/%04x%s", id, ext);
      DebugPrintf("filename = %s\n", filename);
      if (GetImage(filename, imageurl) == 1)
        DrawLogo(id);
      return id;
    }
  }
  return 0;
}

int GetCNAME(char *cname, const char *service) {
  char dns_string[128];

  WiFiClientSecure *client = new WiFiClientSecure;

  if (client) {
    client->setInsecure();
    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
    HTTPClient https;

    //DebugPrint("[HTTPS] begin...\n");
    sprintf(dns_string, "https://dns.google/resolve?name=%s&type=CNAME", service);
    DebugPrintf("[HTTPS] begin... %s\n", dns_string);

    if (https.begin(*client, dns_string)) {  // HTTPS
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();


          filter["Authority"][0]["data"] = true;
          filter["Answer"][0]["data"] = true;
          DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

          if (error) {
            DebugPrint("deserializeJson() failed: ");
            DebugPrintln(error.c_str());
            delete client;
            return 0;
          } else {
            serializeJsonPretty(doc, Serial);
            if (doc["Answer"][0]["data"] == nullptr && doc["Authority"][0]["data"] == nullptr) {
              DebugPrintln("no data\n");
              https.end();
              delete client;
              return 0;
            }

            if (doc["Authority"][0]["data"] == nullptr) {
              DebugPrintf("%s", (const char *)doc["Answer"][0]["data"]);
              strcpy(cname, (const char *)doc["Answer"][0]["data"]);
            } else {
              DebugPrintf("%s", (const char *)doc["Authority"][0]["data"]);
              strcpy(cname, (const char *)doc["Authority"][0]["data"]);
              for (int i = 0; i < strlen(cname); i++) {
                if (cname[i] == ' ') {
                  cname[i] = '\0';
                  break;
                }
              }
            }
            DebugPrintf("cname:%s\n", cname);

            if (strlen(cname) > 0) {
              if (cname[strlen(cname) - 1] == '.') {
                cname[strlen(cname) - 1] = '\0';
              }
              DebugPrintf("Nett cname:%s\n", cname);
            } else {
              DebugPrintln("No CNAME\n");
              https.end();
              delete client;
              return 0;
            }
          }
        }
      } else {
        DebugPrintf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        https.end();
        client->flush();
        client->stop();
        delete client;
        return 0;
      }
      https.end();
    } else {
      DebugPrintln("[HTTPS] Unable to connect");
    }
    // End extra scoping block

    delete client;
  } else {
    DebugPrintln("Unable to create client");
  }
  return 1;
}

int GetSRV(char *srv, const char *cname) {
  char dns_string[128];

  WiFiClientSecure *client = new WiFiClientSecure;

  if (client) {
    client->setInsecure();
    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
    HTTPClient https;

    sprintf(dns_string, "https://dns.google/resolve?name=_radioepg._tcp.%s&type=SRV", cname);
    DebugPrintf("[HTTPS] begin... %s\n", dns_string);

    if (https.begin(*client, dns_string)) {  // HTTPS
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      DebugPrintf("[HTTPS] Code:%d\n", httpCode);
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        //DebugPrintf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          DebugPrintf("[HTTPS] Payload:%s\n", payload);
          filter["Answer"][0]["data"] = true;
          filter["Authority"][0]["data"] = true;
          DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

          if (error) {
            DebugPrint("deserializeJson() failed: ");
            DebugPrintln(error.c_str());
            https.end();
            delete client;
            return 0;
          } else {
            serializeJsonPretty(doc, Serial);
            if (doc["Answer"][0]["data"] == nullptr && doc["Authority"][0]["data"] == nullptr) {
              DebugPrintln("no data\n");
              https.end();
              delete client;
              return 0;
            }

            int service_priority;
            int service_weight;
            int service_port;

            if (doc["Authority"][0]["data"] == nullptr) {
              sscanf((const char *)doc["Answer"][0]["data"], "%d %d %d %s", &service_priority, &service_weight, &service_port, srv);
            } else {
              sscanf((const char *)doc["Authority"][0]["data"], "%d %d %d %s", &service_priority, &service_weight, &service_port, srv);
              for (int i = 0; i < strlen(srv); i++) {
                if (srv[i] == ' ') {
                  srv[i] = '\0';
                  break;
                }
              }
            }

            if (srv[strlen(srv) - 1] == '.') {
              srv[strlen(srv) - 1] = '\0';
            }
            DebugPrintln(srv);
          }
        }
      } else {
        DebugPrintf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      DebugPrintln("[HTTPS] Unable to connect");
      delete client;
      return 0;
    }
    // End extra scoping block
    delete client;
  } else {
    DebugPrintln("Unable to create client");
    return 0;
  }
  return 1;
}

int GetServiceInfo(char *serviceinfo, uint16_t maxsize, const char *srv, const char *bearer) {

  char xmlurl[128];
  imageurl[0] = '\0';
  int found = 0;

  WiFiClientSecure *client = NULL;
  HTTPClient https;

  sprintf(xmlurl, "http://%s/radiodns/spi/3.1/SI.xml", srv);
  DebugPrintf("[HTTPS] begin... %s\n", xmlurl);

  if (https.begin(xmlurl)) {  // HTTPS
    int i;
    const char *headerKeys[] = { "Content-Type", "Location" };
    const size_t numberOfHeaders = 2;
    https.collectHeaders(headerKeys, numberOfHeaders);
    int httpCode = https.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      //DebugPrintf("[HTTP] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String NewLocation = https.header("Location");
        DebugPrintf("Code 301 (moved permanently): %s", NewLocation.c_str());
        https.end();
        client = new WiFiClientSecure;
        client->setInsecure();
        https.begin(*client, NewLocation);
        https.setTimeout(16000);
        httpCode = https.GET();
      }

      // file found at server
      if (httpCode == HTTP_CODE_OK)  // || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        int len = https.getSize();

        DebugPrintln(https.header("Content-Type"));
        DebugPrintf("FileSize %d\n", len);

        WiFiClient *stream = https.getStreamPtr();

        while (https.connected() && (len > 0 || len == -1)) {
          size_t size = stream->available();
          if (size) {
            int c = stream->readBytes(buff, (size > (sizeof(buff) - 1)) ? (sizeof(buff) - 1) : size);
            if (len == -1)
              if (dechunk(buff, &c, buff, c))
                len = 0;

            buff[c] = '\0';
            if (!found) {
              if (ParseService(serviceinfo, maxsize, buff, bearer)) {
                //Perhaps we could end the stream...
                found = 1;
              }
            }
            if (len > 0)
              len -= c;
          }
        }
      } else {
        DebugPrintf("[HTTP] GET2... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
    } else {
      DebugPrintf("[HTTP] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }

    https.end();

    if (client)
      delete client;
  } else {
    DebugPrintln("[HTTP] Unable to connect");
  }
  return found;
}

int ParseXMLImageURL(char *url, char *mime, const char *xml) {
  using namespace tinyxml2;
  XMLDocument xmlDocument;
  if (xmlDocument.Parse(xml) != tinyxml2::XML_SUCCESS) {
    DebugPrintln("Error parsing");
    return 0;
  }

  XMLNode *service = xmlDocument.FirstChild();
  XMLElement *mediaDescription = service->FirstChildElement("mediaDescription");
  bool found = true;
  while (found) {
    if (mediaDescription) {
      XMLElement *multimedia = mediaDescription->FirstChildElement("multimedia");
      if (multimedia) {
        int height;
        int width;
        multimedia->QueryAttribute("height", &height);
        multimedia->QueryAttribute("width", &width);
        if ((height == 128) && (width == 128)) {
          url[0] = '\0';
          mime[0] = '\0';
          if (multimedia->Attribute("url"))
            strcpy(url, multimedia->Attribute("url"));
          if (multimedia->Attribute("mimeValue"))
            strcpy(mime, multimedia->Attribute("mimeValue"));
          DebugPrintf("Found (%dx%d), %s type:%s\n", height, width, url, mime);
          return 1;
        }
      }
      mediaDescription = mediaDescription->NextSiblingElement("mediaDescription");
    } else {
      found = false;
    }
  }
  return 0;
}

int GetImage(const char *filename, const char *url) {

  int success = 0;

  WiFiClientSecure *client = NULL;
  HTTPClient https;

  if (strlen(url)) {
    //download image
    File f = SPIFFS.open(filename, "w");
    DebugPrintf("[HTTPS] begin... %s\n", url);

    bool http_rc;
    if (strncmp(url, "https://", 8) == 0) {
      client = new WiFiClientSecure;
      client->setInsecure();
      http_rc = https.begin(*client, url);
    } else {
      http_rc = https.begin(url);
    }

    if (http_rc) {
      int i;

      // DebugPrint("[HTTP] GET...\n");
      // start connection and send HTTP header
      const char *headerKeys[] = { "Content-Type" };
      const size_t numberOfHeaders = 1;
      https.collectHeaders(headerKeys, numberOfHeaders);
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // file found at server
        if (httpCode == HTTP_CODE_OK) {
          int len = https.getSize();
          DebugPrintln(https.header("Content-Type"));
          DebugPrintf("FileSize %d\n", len);

          WiFiClient *stream = https.getStreamPtr();
          int total = 0;

          while (https.connected() && (len > 0 || len == -1)) {
            size_t size = stream->available();
            if (size) {
              int c = stream->readBytes(buff, (size > sizeof(buff)) ? sizeof(buff) : size);
              if (len == -1)
                if (dechunk(buff, &c, buff, c))
                  len = 0;

              f.write((const uint8_t *)buff, c);
              total += c;
              if (len > 0)
                len -= c;
            }
          }
          if (total > 0)
            success = 1;
          DebugPrintf("total %d\n", total);
        }
      }
      https.end();
    }
    f.close();
  }

  if (client)
    delete client;

  DebugPrintf("GetImage %s\n", success ? "success" : "fail");
  return success;
}

bool dechunk(char *outbuff, int *outsize, const char *inbuff, int insize) {
  static int chunklen = 0;
  static int chunkparse = 0;

  bool finished = false;
  int outindex = 0;
  int nibble;
  int i;
  char hex[2];

  for (i = 0; i < insize; i++) {
    //printf("%c", buff[i]);
    switch (chunkparse) {
      case 0:  //len
        hex[0] = inbuff[i];
        hex[1] = '\0';
        if (sscanf(hex, "%x", &nibble) == 1) {
          chunklen <<= 4;
          chunklen |= nibble;
        } else {
          if (inbuff[i] == '\r')
            chunkparse = 2;
        }
        break;
      case 1:  //0x0d
      case 2:  //0x0a
        if (inbuff[i] == '\n') {
          chunkparse = 3;
          if (chunklen == 0) {
            chunkparse = 0;
            finished = true;
          }
        }
        break;
      case 3:  //data
        outbuff[outindex] = inbuff[i];
        outindex++;

        chunklen--;
        if (chunklen == 0)
          chunkparse = 4;
        break;

      case 4:
        if (inbuff[i] == '\r')
          chunkparse = 5;
        break;
      case 5:
        if (inbuff[i] == '\n')
          chunkparse = 0;
        break;
    }
  }
  *outsize = outindex;
  outbuff[outindex] = '\0';
  return finished;
}

bool ParseService(char *serviceinfo, uint16_t maxsize, const char *text, const char *bearer) {
  const char servicestart[] = "<service>";
  const char serviceend[] = "</service>";
  static int stringindex = 0;
  static int servicestate = 0;
  static int serviceindex = 0;

  int i;
  for (i = 0; i < strlen(text); i++) {
    switch (servicestate) {
      case 0:
        //find "<service>"" or "<service " but not <serviceInfo etc..
        if ((text[i] == servicestart[stringindex]) || ((servicestart[stringindex] == '>') && (text[i] == ' '))) {
          serviceinfo[serviceindex] = text[i];
          serviceindex++;
          stringindex++;
          if (stringindex == strlen(servicestart)) {
            //DebugPrintf("stringindex = %d, %d, %c", stringindex, strlen(servicestart), text[i]);
            stringindex = 0;
            servicestate = 1;
          }
        } else {
          stringindex = 0;
          serviceindex = 0;
        }
        break;
      case 1:
        serviceinfo[serviceindex] = text[i];
        if (serviceindex < (maxsize - 1))
          serviceindex++;

        if (text[i] == serviceend[stringindex]) {
          stringindex++;
          if (stringindex == strlen(serviceend)) {
            serviceinfo[serviceindex] = '\0';
            stringindex = 0;
            serviceindex = 0;
            servicestate = 0;

            if (FindOurService(serviceinfo, bearer))
              return true;
          }
        } else {
          stringindex = 0;
        }
        break;
    }
  }
  return false;
}

bool FindOurService(const char *text, const char *bearer) {
  if (strstr(text, bearer) != 0) {
    //this serivce xml contains the station we are looking for
    DebugPrintln("Found it");
    return true;
  }
  return false;
}

char *GetLogoName(uint32_t id) {
  char pngfilename[16];
  char jpgfilename[16];
  sprintf(pngfilename, "/%04x.png", id);
  sprintf(jpgfilename, "/%04x.jpg", id);
  //<img width="128" height="128" src="/dablogo?plaatje=%DABLogo%">
  String s;
  if (SPIFFS.exists(pngfilename)) sprintf(logoBuf, "<img width=\"128\" height=\"128\" src=\"/dablogo?plaatje=%s\">", pngfilename);
  else if (SPIFFS.exists(jpgfilename)) sprintf(logoBuf, "<img width=\"128\" height=\"128\" src=\"/dablogo?plaatje=%s\">", jpgfilename);
  else{
    strcpy(logoBuf, "<h4>Dablogo<br>not<br>available</h4>");
    tft.fillRect(190, 0, 128, 128, TFT_BLACK);
  } 
  DebugPrintln(logoBuf);
  return logoBuf;
}

void DrawLogo(uint32_t id) {
  char pngfilename[16];
  char jpgfilename[16];

  sprintf(pngfilename, "/%04x.png", id);
  sprintf(jpgfilename, "/%04x.jpg", id);
  //UpdateLogo = 0;
  if (SPIFFS.exists(pngfilename)) {
    int rc = png.open((const char *)pngfilename, myOpen, myClose, myRead, mySeek, PNGDraw);
    if (rc == PNG_SUCCESS) {
      DebugPrintf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
      DebugPrintf("PNG bufferSize %d\n", png.getBufferSize());
      rc = png.decode(NULL, 0);
      png.close();
    }
  } else if (SPIFFS.exists(jpgfilename)) {
    //.jpg might not be a JPEG file - try openning it in pngdec
    int rc = png.open((const char *)jpgfilename, myOpen, myClose, myRead, mySeek, PNGDraw);
    if (rc == PNG_SUCCESS) {
      DebugPrintf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
      DebugPrintf("PNG bufferSize %d\n", png.getBufferSize());
      rc = png.decode(NULL, 0);
      png.close();
    } else {
      //Couldn't Find the logo, lets go to RadioDNS...
      //screen.drawRGBBitmap(/*x=*/20, /*y=*/20, /*bitmap gImage_Bitmap=*/(const unsigned uint16_t *)icon_DAB, /*w=*/128, /*h=*/128);
      //tft.drawBitmap(0,192,png.getBuffer(),png.getWidth(),png.getHeight(),TFT_WHITE);
      //x, y, logo, logoWidth, logoHeight, TFT_WHITE);
    }
  } else {
    //Couldn't Find the logo, lets go to RadioDNS...
    //screen.drawRGBBitmap(/*x=*/20, /*y=*/20, /*bitmap gImage_Bitmap=*/(const unsigned uint16_t *)icon_DAB, /*w=*/128, /*h=*/128);
    //tft.drawBitmap(0,192,png.getBuffer(),png.getWidth(),png.getHeight(),TFT_WHITE);
    //GetLogoSignal = true;
  }
}

void PNGDraw(PNGDRAW *pDraw) {
  PNGDraw(pDraw, false);
}

void PNGDraw(PNGDRAW *pDraw, bool halfSize) {
  if (halfSize) {
    if ((pDraw->y % 2) == 0) {
      uint16_t usPixels[128];
      png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
      for (int i = 0; i < 128; i++)
        if ((i % 2) == 0) tft.drawPixel(254 + floor(i / 2), 6 + floor(pDraw->y / 2), usPixels[i]);
    }
  } else {
    uint16_t usPixels[128];
    png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
    for (int i = 0; i < 128; i++)
      tft.drawPixel(190 + i, 2 + pDraw->y, usPixels[i]);
  }
}

/***************************************************************************************
**            File handle routines
***************************************************************************************/
File myfile;
void *myOpen(const char *filename, int32_t *size) {
  DebugPrintf("Attempting to open %s\n", filename);
  myfile = SPIFFS.open(filename, "r");
  *size = myfile.size();
  return &myfile;
}

void myClose(void *handle) {
  if (myfile) myfile.close();
}

int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!myfile) return 0;
  return myfile.read(buffer, length);
}

int32_t mySeek(PNGFILE *handle, int32_t position) {
  if (!myfile) return 0;
  return myfile.seek(position);
}

/***************************************************************************************
**            Web setup page 
***************************************************************************************/
String processor(const String &var) {
  if (var == "wifiSSID") return settings.wifiSSID;
  if (var == "wifiPass") return settings.wifiPass;
  if (var == "DABName") {
    String s = String(Dab.service[actualDabService].Label);
    s.replace("%", "&#37");
    return s;
  }
  if (var == "DABRDS") {
    String s = String(actualInfo);
    s.replace("%", "&#37");
    return s;
  }
  if (var == "style") return css_html;
  if (var == "DABLogo") return GetLogoName(settings.dabServiceID);
  if (var == "rotateScreen") return settings.rotateScreen ? "checked" : "";  
  if (var == "rotateTouch") return settings.rotateTouch ? "checked" : "";    
  if (var == "isDebug") return settings.isDebug ? "checked" : "";
  if (var == "stationList") return WebStationList();
  if (var == "myMute") return WebMuteStatus();
  if (var == "myMode") return settings.isDab ? "DAB" : "FM";

  return var;
}

String WebStationList() {
  String s = "<select name=\"stationsList\" id=\"stationsList\" onchange=\"changeActualChannel()\">";
  for (int i = 0; i < settings.dabChannelsCount; i++) {
    sprintf(buf, "<option value=\"%d\"%s>%02d: %s</option>", i, (settings.dabChannelSelected == i) ? " selected " : "", i, dabChannels[i].dabName);
    s += String(buf);
  }
  s += "</select>";
  s.replace("%", "&#37");  //
  return s;
}

String WebMuteStatus() {
  String retVal = "";
  if (settings.isMuted == 1) retVal = "Right";
  if (settings.isMuted == 2) retVal = "Left";
  if (settings.isMuted == 3) retVal = "Muted";
  return retVal;
}

void SaveSettings(AsyncWebServerRequest *request) {
  if (request->hasParam("wifiSSID")) request->getParam("wifiSSID")->value().toCharArray(settings.wifiSSID, 25);
  if (request->hasParam("wifiPass")) request->getParam("wifiPass")->value().toCharArray(settings.wifiPass, 25);
  settings.rotateScreen = request->hasParam("rotateScreen");
  settings.rotateTouch = request->hasParam("rotateTouch");    
  settings.isDebug = request->hasParam("isDebug");
}

/***************************************************************************************
**                          Draw messagebox with message
***************************************************************************************/
bool questionBox(const char *msg, uint16_t fgcolor, uint16_t bgcolor, int x, int y, int w, int h) {
  uint16_t current_textcolor = tft.textcolor;
  uint16_t current_textbgcolor = tft.textbgcolor;

  //tft.loadFont(AA_FONT_SMALL);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(fgcolor, bgcolor);
  tft.fillRoundRect(x, y, w, h, 5, fgcolor);
  tft.fillRoundRect(x + 2, y + 2, w - 4, h - 4, 5, bgcolor);
  tft.setTextPadding(tft.textWidth(msg));
  tft.drawString(msg, x + 4 + w/2, y + (h / 4));

  tft.fillRoundRect(x + 4, y + (h/2) - 2, (w - 12)/2, (h - 4)/2, 5, TFT_GREEN);
  tft.setTextColor(fgcolor, TFT_GREEN);
  tft.setTextPadding(tft.textWidth("Yes"));
  tft.drawString("Yes", x + 4 + ((w - 12)/4),y + (h/2) - 2 + (h/4));
  tft.fillRoundRect(x + (w/2) + 2, y + (h/2) - 2, (w - 12)/2, (h - 4)/2, 5, TFT_RED);
  tft.setTextColor(fgcolor, TFT_RED);
  tft.setTextPadding(tft.textWidth("No"));
  tft.drawString("No", x + (w/2) + 2 + ((w - 12)/4),y + (h/2) - 2 + (h/4));
  Serial.printf("Yes = x:%d,y:%d,w:%d,h:%d\r\n",x + 4, y + (h/2) - 2, (w - 12)/2, (h - 4)/2);
  Serial.printf("No  = x:%d,y:%d,w:%d,h:%d\r\n",x + (w/2) + 2, y + (h/2) - 2, (w - 12)/2, (h - 4)/2);
  tft.setTextColor(current_textcolor, current_textbgcolor);
  tft.unloadFont();

  uint16_t touchX = 0, touchY = 0;

  long startWhile = millis();
  while (millis()-startWhile<30000) {
    bool pressed = tft.getTouch(&touchX, &touchY);
    if (pressed){
      Serial.printf("Pressed = x:%d,y:%d\r\n",touchX,touchY);
      if (touchY>=y + (h/2) - 2 && touchY<=y + (h/2) - 2 + ((h - 4)/2)){
        if (touchX>=x + 4 && touchX<=x + 4 + ((w - 12)/2)) return true;
        if (touchX>=x + (w/2) + 2 && touchX<=x + (w/2) + 2 + ((w - 12)/2)) return false;
      }
    }
  }
  return false;
}

/***************************************************************************************
**            Calibrate touch
***************************************************************************************/
void TouchCalibrate() {
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

  if (settings.isDebug) {
    DebugPrintln();
    DebugPrintln();
    DebugPrintln("// Use this calibration code in setup():");
    DebugPrint("  uint16_t calData[5] = ");
    DebugPrint("{ ");
  }

  for (uint8_t i = 0; i < 5; i++) {
    DebugPrint(calData[i]);
    if (settings.isDebug && i < 4) DebugPrint(", ");
  }

  if (settings.isDebug) {
    DebugPrintln(" };");
    DebugPrint("  tft.setTouch(calData);");
    DebugPrintln();
    DebugPrintln();
  }

  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Calibration complete!");
  tft.println("Calibration code sent to Serial port.");

  delay(4000);
}