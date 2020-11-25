/**
 * SMuFF Firmware
 * Copyright (C) 2019 Technik Gegg
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#if defined(__AVR__)
  #include <avr/pgmspace.h>
#elif defined(__ESP32__)
  #include <pgmspace.h>
#endif
#include <Arduino.h>
#include "Config.h"
#include "Strings.h"
#include "GCodes.h"
#include "Menus.h"
#if defined(USE_LEONERD_DISPLAY)
  #include "LeoNerdEncoder.h"
#else
  #include "ClickEncoder.h"
#endif
#include <SPI.h>
#include <Wire.h>
#include <SdFs.h>
#include "U8g2lib.h"
#include "MemoryFree.h"
#include "DataStore.h"
#if defined(USE_FASTLED_BACKLIGHT)
  #include "FastLED.h"
#endif
#include "ZTimerLib.h"
#include "ZStepperLib.h"
#include "ZServo.h"
#include "ZPortExpander.h"
#include "ZFan.h"
#include "DuetLaserSensor.h"
#include "SoftwareSerial.h"
#include <TMCStepper.h>
#if defined(MULTISERVO)
  #include <Adafruit_PWMServoDriver.h>
#endif

#if !defined(USE_FASTLED_BACKLIGHT)
  #define CRGB  uint32_t
#endif

#if defined(__STM32F1__)
  #include <wirish.h>
  #include <libmaple/gpio.h>
  #include <USBComposite.h>

#undef  sprintf_P
  #define sprintf_P(s, f, ...)  sprintf(s, f, ##__VA_ARGS__)
  #define vsnprintf_P           vsnprintf
  extern USBMassStorage         MassStorage;
  extern USBCompositeSerial     CompositeSerial;

#elif defined(__ESP32__)
  #include <WiFi.h>
  #include <BluetoothSerial.h>
#endif


#define FEEDER_SIGNAL         1
#define SELECTOR_SIGNAL       2
#define REVOLVER_SIGNAL       3
#define LED_SIGNAL            4

#define PORT_EXPANDER_ADDRESS 0x3F

#define INTERNAL          1
#define EXTERNAL          0

#define MAX_SEQUENCE      60
#define MAX_TUNE1         100
#define MAX_TUNE2         50

#define SERVO_OPEN        0
#define SERVO_CLOSED      1

#define LAST_INTERVAL     9


#if defined(__HW_DEBUG__) && defined(DEBUG_PIN)
  // used for internal hardware debugging only - will produce a 500Hz signal on the output pin
  #define FLIPDBG         if(DEBUG_PIN != -1) digitalWrite(DEBUG_PIN, !digitalRead(DEBUG_PIN));
#endif

#define ArraySize(arr)    (sizeof(arr)/sizeof(arr[0]))

typedef enum {
  ABSOLUTE,
  RELATIVE
} PositionMode;

typedef struct {
  unsigned  long    serialBaudrates[4]      = { 57600, 57600, 57600, 57600 };
  uint8_t           toolCount               = 5;
  float             bowdenLength            = 400.0f;
  float             selectorDistance        = 23.0f;
  uint8_t           i2cAddress              = 0x58;
  uint8_t           lcdContrast             = DSP_CONTRAST;
  uint8_t           menuAutoClose           = 20;
  uint8_t           fanSpeed                = 0;
  uint16_t          powerSaveTimeout        = 300;
  bool              sendActionCmds          = false;
  bool              prusaMMU2               = true;
  char              unloadCommand[50]       = { 0 };
  uint16_t          servoMinPwm             = 800;
  uint16_t          servoMaxPwm             = 2400;
  char              wipeSequence[30]        = { 0 };
  uint8_t           backlightColor          = 0x4;    // Cyan by default
  uint8_t           hasPanelDue             = 0;      // Serial Port for PanelDue (0=None)
  bool              encoderTickSound        = false;
  bool              sendPeriodicalStats     = true;
  char              lButtonDown[20]         = { 0 };
  char              lButtonHold[20]         = { 0 };
  char              rButtonDown[20]         = { 0 };
  char              rButtonHold[20]         = { 0 };
  bool              speedsInMMS             = true;
  bool              runoutDetection         = false;

  // ALL STEPPERS
  uint16_t  stepsPerMM[NUM_STEPPERS]        = { 80, 0, 410 };
  long      maxSteps[NUM_STEPPERS]          = { 68000, 9600, 0 };
  uint16_t  maxSpeed[NUM_STEPPERS]          = { 10, 10, 10 };
  uint16_t  accelSpeed[NUM_STEPPERS]        = { 10, 10, 10 };
  bool      invertDir[NUM_STEPPERS]         = { false, false, false };
  uint8_t   endstopTrg[NUM_STEPPERS+1]      = { HIGH, HIGH, HIGH, LOW };
  uint8_t   stepDelay[NUM_STEPPERS]         = { 3, 3, 3 };
  uint8_t   accelDist[NUM_STEPPERS]         = { 21, 5, 5 };
  int8_t    ms3config[NUM_STEPPERS]         = { -1, -1, -1 };
  float     speedAdjust[NUM_STEPPERS]       = { 1, 1, 1 };
  // TMC drivers via UART or SPI
  uint16_t  stepperPower[NUM_STEPPERS+1]      = { 700, 700, 700, 700 };
  uint8_t   stepperMode[NUM_STEPPERS+1]       = { 0, 0, 0, 0 };                   // 0 = NONE, 1 = UART, 2 = SPI
  bool      stepperStealth[NUM_STEPPERS+1]    = { false, false, false, false };   // true = StealthChop, false = SpreadCycle mode (for Stall Detection)
  float     stepperRSense[NUM_STEPPERS+1]     = { 0.11, 0.11, 0.11, 0.11 };
  uint16_t  stepperMicrosteps[NUM_STEPPERS+1] = { 16, 16, 16,16 };
  int8_t    stepperStall[NUM_STEPPERS+1]      = { 0, 0, 0, 0 };
  int8_t    stepperCSmin[NUM_STEPPERS+1]      = { 0, 0, 0, 0 };
  int8_t    stepperCSmax[NUM_STEPPERS+1]      = { 0, 0, 0, 0 };
  int8_t    stepperCSdown[NUM_STEPPERS+1]     = { 0, 0, 0, 0 };
  int8_t    stepperAddr[NUM_STEPPERS+1]       = { 0, 0, 0, 0 };
  int8_t    stepperToff[NUM_STEPPERS+1]       = { -1, -1, -1, -1 };
  bool      stepperStopOnStall[NUM_STEPPERS+1]= { false, false, false, false };
  int8_t    stepperMaxStallCnt[NUM_STEPPERS+1]= { 5, 5, 5, 5};
  // REVOLVER specific settings
  uint16_t  firstRevolverOffset             = FIRST_REVOLVER_OFFSET;
  uint16_t  revolverSpacing                 = REVOLVER_SPACING;
  long      stepsPerRevolution              = 9600;
  bool      resetBeforeFeed                 = true;
  bool      homeAfterFeed                   = true;
  bool      wiggleRevolver                  = false;
  bool      revolverIsServo                 = false;
  uint8_t   revolverOffPos                  = 0;
  uint8_t   revolverOnPos                   = 90;
  uint8_t   servoCycles1                    = 0;
  uint8_t   servoCycles2                    = 0;
  // SELECTOR specific settings
  float     firstToolOffset                 = FIRST_TOOL_OFFSET;
  float     toolSpacing                     = TOOL_SPACING;
  // FEEDER specific settings
  bool      extControlFeeder                = false;
  uint16_t  insertSpeed                     = 1000;
  uint8_t   feedChunks                      = 20;
  bool      enableChunks                    = false;
  float     insertLength                    = 5.0;
  float     unloadRetract                   = -20.0f;
  float     unloadPushback                  = 5.0f;
  float     pushbackDelay                   = 1.5f;
  float     reinforceLength                 = 3.0f;
  bool      isSharedStepper                 = false;
  bool      externalStepper                 = false;
  bool      useDuetLaser                    = false;

  uint16_t  motorOnDelay                    = 30;
  char      materials[MAX_TOOLS][MAX_MATERIAL_LEN];
} SMuFFConfig;

#if defined(__BRD_I3_MINI)
extern U8G2_ST7565_64128N_F_4W_HW_SPI       display;
#elif defined(__BRD_SKR_MINI) || defined(__BRD_SKR_MINI_E3) || defined(__BRD_SKR_MINI_E3DIP)
  extern "C" uint8_t __wrap_u8x8_byte_arduino_2nd_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
  #if defined(USE_TWI_DISPLAY)
    extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C  display;
  #elif defined(USE_LEONERD_DISPLAY)
    extern U8G2_SH1106_128X64_NONAME_F_HW_I2C  display;
  #elif defined(USE_ANET_DISPLAY)
    extern U8G2_ST7920_128X64_F_2ND_HW_SPI display;
  // extern U8G2_ST7920_128X64_F_SW_SPI display;
  #elif defined(USE_FYSETC_1_2_DISPLAY) || defined(USE_FYSETC_2_1_DISPLAY)
    extern U8G2_ST7567_JLX12864_F_2ND_4W_HW_SPI display;
  #elif defined(USE_MKS_2_0_DISPLAY)
    extern U8G2_ST7567_ENH_DG128064_F_2ND_4W_HW_SPI display;
  #elif defined(USE_MKS_2_1_DISPLAY)
    U8G2_ST7565_NHD_C12864_F_2ND_4W_HW_SPI display;
  #elif USE_CREALITY_DISPLAY
    extern U8G2_ST7920_128X64_F_SW_SPI display;
  #endif
#elif defined(__BRD_ESP32)
  #if defined(USE_TWI_DISPLAY)
    extern U8G2_SH1306_128X64_NONAME_F_HW_I2C  display;
  #elif defined(USE_LEONERD_DISPLAY)
    extern U8G2_SSD1106_128X64_NONAME_F_HW_I2C  display;
  #else
    extern U8G2_ST7567_ENH_DG128064_F_4W_HW_SPI display;
  #endif
  extern HardwareSerial Serial3;
#elif defined(__BRD_FYSETC_AIOII)
  extern U8G2_UC1701_MINI12864_F_4W_HW_SPI display;
#endif

#if defined(__STM32F1__)
  extern void playTone(int8_t pin, int16_t frequency, int16_t duration);
  extern void muteTone(int8_t pin);
  #define _tone(freq, duration)     playTone(BEEPER_PIN, freq, duration)
  #define _noTone()                 muteTone(BEEPER_PIN)
#elif defined (__ESP32__)
  #include "Tone32.h"
  #define _tone(freq, duration)     tone(BEEPER_PIN, freq, duration, BEEPER_CHANNEL)
  #define _noTone()                 noTone(BEEPER_PIN, BEEPER_CHANNEL)
#else
  #define _tone(freq, duration)     tone(BEEPER_PIN, freq, duration)
  #define _noTone()                 noTone(BEEPER_PIN)
#endif

extern ZStepper         steppers[];
extern ZTimer           stepperTimer;
extern ZTimer           gpTimer;
extern ZServo           servo;
extern ZServo           servoLid;
extern ZFan             fan;
#if defined(USE_LEONERD_DISPLAY)
  extern LeoNerdEncoder encoder;
#else
  extern ClickEncoder   encoder;
#endif
#if defined(USE_FASTLED_BACKLIGHT)
  extern CRGB           leds[];
#else
#endif
#if defined(MULTISERVO)
  extern Adafruit_PWMServoDriver servoPwm;
  extern int8_t servoMapping[];
#endif
extern uint8_t servoPosClosed[];

extern SMuFFConfig    smuffConfig;
extern GCodeFunctions gCodeFuncsM[];
extern GCodeFunctions gCodeFuncsG[];

typedef struct {
  uint16_t period;
  uint16_t val;
  void (*func)();
} IntervalHandler;



extern const char     brand[];
extern uint8_t        swapTools[];
extern volatile byte  nextStepperFlag;
extern volatile byte  remainingSteppersFlag;
extern volatile unsigned long lastEncoderButtonTime;
extern int8_t         toolSelected;
extern PositionMode   positionMode;
extern String         serialBuffer0, serialBuffer2, serialBuffer9, traceSerial2;
extern char           tuneStartup[100];
extern char           tuneUser[40];
extern char           tuneBeep[20];
extern char           tuneLongBeep[20];
extern char           tuneEncoder[20];
extern bool           displayingUserMessage;
extern uint16_t       userMessageTime;
extern bool           testMode;
extern bool           feederJammed;
extern volatile bool  parserBusy;
extern volatile bool  isPwrSave;
extern volatile bool  actionOk;
extern volatile bool  sendingResponse;
extern volatile bool  showMenu;
extern bool           maintainingMode;
extern volatile double lastDuetPos;
extern DuetLaserSensor duetLS;
extern String         wirelessHostname;
extern volatile bool  initDone;
extern bool           LeoNerdBlinkGreen;
extern bool           LeoNerdBlinkRed;
extern bool           forceStopMenu;
extern bool           startSequence;
extern uint16_t       sequence[][3];
extern bool           timerRunning;
extern uint8_t        remoteKey;
extern bool           settingsChanged;
extern Stream*        logSerial;
extern Stream*        debugSerial;
extern bool           isWarning;
extern bool           lidOpen;
extern uint16_t       mmsMin, mmsMax;
extern uint16_t       speedIncrement;

extern TMC2209Stepper* drivers[];

extern void setupSerial();
extern void setupSwSerial0();
extern void setupDisplay();
extern void setupTimers();
extern void setupSteppers();
extern void setupTMCDrivers();
extern void setupServos();
extern void setupHeaterBed();
extern void setupFan();
extern void setupPortExpander();
extern void setupRelay();
extern void setupI2C();
extern void setupDeviceName();
extern void setupSerialBT();
extern void setupBuzzer();
extern void setupEncoder();
extern void setupBacklight();
extern void setupDuetLaserSensor();
extern void setupHBridge();
extern void initHwDebug();
extern void initFastLED();
extern void initUSB();
extern void drawLogo();
extern void drawStatus();
extern void drawSelectingMessage();
extern void drawUserMessage(String message, bool smallFont = false, bool center = true, void (*drawCallbackFunc)() = nullptr);
extern void drawSDStatus(int8_t stat);
extern void drawFeed();
extern void drawSDRemoved(bool removed);
extern void resetDisplay();
extern bool selectorEndstop();
extern bool revolverEndstop();
extern bool feederEndstop(int8_t index = 1);
extern bool showFeederLoadedMessage();
extern bool showFeederLoadMessage();
extern bool showFeederFailedMessage(int8_t state);
extern uint8_t  showDialog(PGM_P title, PGM_P message, PGM_P addMessage, PGM_P buttons);
extern bool moveHome(int8_t index, bool showMessage = true, bool checkFeeder = true);
extern bool loadFilament(bool showMessage = true);
extern bool loadFilamentPMMU2(bool showMessage = true);
extern bool unloadFilament();
extern bool unloadFromSelector();
extern bool handleFeederStall(uint16_t* speed, int8_t* retries);
extern bool nudgeBackFilament();
extern void handleStall(int8_t axis);
extern void runAndWait(int8_t index);
extern void runNoWait(int8_t index);
extern bool selectTool(int8_t ndx, bool showMessage = true);
extern void setStepperSteps(int8_t index, long steps, bool ignoreEndstop);
extern void prepSteppingAbs(int8_t index, long steps, bool ignoreEndstop = false);
extern void prepSteppingAbsMillimeter(int8_t index, float millimeter, bool ignoreEndstop = false);
extern void prepSteppingRel(int8_t index, long steps, bool ignoreEndstop = false);
extern void prepSteppingRelMillimeter(int8_t index, float millimeter, bool ignoreEndstop = false);
extern void resetRevolver();
extern void serialEvent();
extern void serialEvent2();
#if defined(__AVR__)
  extern void wireReceiveEvent(int numBytes);
#else
  extern void serialEvent1();
  extern void serialEvent3();
#endif
extern void beep(uint8_t count);
extern void longBeep(uint8_t count);
extern void userBeep();
extern void encoderBeep(uint8_t count);
extern void startupBeep();
extern void setSignalPort(uint8_t port, bool state);
extern void signalNoTool();
extern void signalLoadFilament();
extern void signalUnloadFilament();
extern void signalSelectorBusy();
extern void signalSelectorReady();
extern bool setServoPos(int8_t servoNum, uint8_t degree);
extern bool setServoMS(int8_t servoNum, uint16_t microseconds);
extern void setServoLid(uint8_t pos);
extern void setServoMinPwm(int8_t servoNum, uint16_t pwm);
extern void setServoMaxPwm(int8_t servoNum, uint16_t pwm);
extern void getStoredData();
extern const char* readTune(const char* filename);
extern void readSequences();
extern bool readConfig();
extern bool readTmcConfig();
extern bool readMaterials();
extern bool readServoMapping();
extern bool writeConfig(Print* dumpTo = nullptr);
extern bool writeTmcConfig(Print* dumpTo = nullptr);
extern bool writeServoMapping(Print* dumpTo = nullptr);
extern bool checkAutoClose();
extern void resetAutoClose();
extern bool checkUserMessage();
extern void listDir(File root, int8_t numTabs, int8_t serial);
extern void setPwrSave(int8_t state);
extern void __debug(const char* fmt, ...);
extern void __log(const char* fmt, ...);
extern void setAbortRequested(bool state);
extern void resetSerialBuffer(int8_t serial);
extern void checkSerialPending();
extern void drawSwapTool(uint8_t from, uint8_t with);
extern uint8_t swapTool(uint8_t index);
extern void positionRevolver();
extern bool feedToEndstop(bool showMessage);
extern bool feedToNozzle(bool showMessage);
extern bool unloadFromNozzle(bool showMessage);
extern uint8_t  splitStringLines(char* lines[], uint8_t maxLines, const char* message, const char* token="\n");
extern void debounceButton();
extern bool checkStopMenu(unsigned startTime);
extern void drawTestrunMessage(unsigned long loop, char* msg);
extern bool getFiles(const char* rootFolder PROGMEM, const char* pattern PROGMEM, uint8_t maxFiles, bool cutExtension, char* files);
extern void testRun(const char* fname);
extern void moveFeeder(float distanceMM);
extern void overrideStepX();
extern void overrideStepY();
extern void overrideStepZ();
extern void endstopEventY();
extern void endstopEventZ();
extern void endstopEventZ2();
extern bool checkDuetEndstop();
extern void setToneTimerChannel(uint8_t ntimer, uint8_t channel);
extern void isrStepperHandler();
extern void isrGPTimerHandler();
extern void isrStallDetectedX();
extern void isrStallDetectedY();
extern void isrStallDetectedZ();
extern void refreshStatus(bool withLogo, bool feedOnly);
extern void every10ms();
extern void every20ms();
extern void every50ms();
extern void every100ms();
extern void every250ms();
extern void every500ms();
extern void every1s();
extern void every2s();
extern void every5s();
extern void blinkLED();
extern void setDriverSpreadCycle(TMC2209Stepper* driver, bool spread, uint8_t stallThrs, uint8_t csmin=0, uint8_t csmax=0, uint8_t csdown=0, uint8_t toff=3);

extern void printEndstopState(int8_t serial);
extern void printPos(int8_t index, int8_t serial);
extern void printAcceleration(int8_t serial);
extern void printSpeedAdjust(int8_t serial);
extern void printSpeeds(int8_t serial);
extern void sendGList(int8_t serial);
extern void sendMList(int8_t serial);
extern void sendM205List(int8_t serial);
extern void sendToolResponse(int8_t serial);
extern void sendStartResponse(int8_t serial);
extern void sendOkResponse(int8_t serial);
extern void sendErrorResponse(int8_t serial, const char* msg = nullptr);
extern void sendErrorResponseP(int8_t serial, const char* msg = nullptr);
extern void parseGcode(const String& serialBuffer, int8_t serial);
extern bool parse_G(const String& buf, int8_t serial);
extern bool parse_M(const String& buf, int8_t serial);
extern bool parse_T(const String& buf, int8_t serial);
extern bool parse_PMMU2(char cmd, const String& buf, int8_t serial);
extern bool parse_Action(const String& buf, int8_t serial);
extern int  getParam(String buf, const char* token);
extern long getParamL(String buf, const char* token);
extern float getParamF(String buf, const char* token);
extern bool hasParam(String buf, const char* token);
extern bool getParamString(String buf, const char* token, char* dest, int16_t bufLen);
extern void prepStepping(int8_t index, long param, bool Millimeter = true, bool ignoreEndstop = false);
extern void saveSettings(int8_t serial);
extern void reportSettings(int8_t serial);
extern void printResponse(const char* response, int8_t serial);
extern void printResponseP(const char* response, int8_t serial);
extern void printOffsets(int8_t serial);
extern void printDriverMode(int8_t serial);
extern void printDriverRms(int8_t serial);
extern void printDriverStallThrs(int8_t serial);
extern void maintainTool();
extern void printPeriodicalState(int8_t serial);
extern void prepareSequence(const char* sequence, bool autoPlay = true);
extern void playSequence(bool inForeground = false);
extern void playSequenceBackgnd();

extern void showLed(uint8_t mode, uint8_t count);
extern void setBacklightIndex(int color);
extern void setFastLED(uint8_t index, CRGB color);
extern void setFastLEDIndex(uint8_t index, uint8_t color);
extern void setFastLEDIntensity(uint8_t intensity);
extern void testFastLED();

extern void showDuetLS();
extern void switchFeederStepper(uint8_t stepper);
extern void removeFirmwareBin();
extern void showMemInfo(int8_t serial);
extern uint8_t  scanI2CDevices(uint8_t * devices);
extern bool initSD(bool showStatus = true);

extern unsigned long translateSpeed(uint16_t speed, uint8_t axis);
extern bool getEncoderButton(bool encoderOnly);
extern void getEncoderButton(int16_t* turn, uint8_t* button, bool* isHeld, bool* isClicked);

extern void listTextFile(const char* filename PROGMEM, int8_t serial);
extern const char* loadMenu(const char* filename PROGMEM, uint8_t ordinals[]);
extern const char* loadOptions(const char* filename PROGMEM);
extern bool loadReport(const char* filename PROGMEM, char* buffer, uint16_t maxLen);
