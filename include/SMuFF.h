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

#ifndef _SMUFF_H
#define _SMUFF_H 1

#if defined (__AVR__)
#include <avr/pgmspace.h>
#endif
#if defined (__ESP32__)
#include <pgmspace.h>
#endif
#include <Arduino.h>
#include "Config.h"
#include "Strings.h"
#include "GCodes.h"
#include "Menus.h"
#include "ClickEncoder.h"
#include <SPI.h>
#include "SdFat.h"
#include <Wire.h>
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

#if !defined(USE_FASTLED_BACKLIGHT)
#define CRGB  uint32_t
#endif

#if defined (__STM32F1__)
#include <wirish.h>
#include <libmaple/gpio.h>
#include <USBComposite.h>

#undef  sprintf_P
#define sprintf_P(s, f, ...)  sprintf(s, f, ##__VA_ARGS__)
#define vsnprintf_P           vsnprintf
#define cli()                 nvic_globalirq_disable()
#define sei()                 nvic_globalirq_enable()
extern USBMassStorage         MassStorage;
extern USBCompositeSerial     CompositeSerial;
#endif

#if defined(__ESP32__)
#include <WiFi.h>
#include <BluetoothSerial.h>
#endif


#define FEEDER_SIGNAL     1
#define SELECTOR_SIGNAL   2
#define REVOLVER_SIGNAL   3
#define LED_SIGNAL        4

#define PORT_EXPANDER_ADDRESS 0x3F

#define INTERNAL          1
#define EXTERNAL          0

#define ArraySize(arr)    (sizeof(arr)/sizeof(arr[0]))

typedef enum {
  ABSOLUTE,
  RELATIVE
} PositionMode;

typedef struct {
  int   toolCount           = 5;
  float firstToolOffset     = FIRST_TOOL_OFFSET;
  float toolSpacing         = TOOL_SPACING;
  int   firstRevolverOffset = FIRST_REVOLVER_OFFSET;
  int   revolverSpacing     = REVOLVER_SPACING;
  long  stepsPerMM_X        = 800;
  long  maxSteps_X          = 68000;
  unsigned maxSpeed_X       = 10;
  unsigned acceleration_X   = 510;
  bool  invertDir_X         = false;
  int   endstopTrigger_X    = HIGH;
  int   stepDelay_X         = 10;
  unsigned accelDistance_X  = 21;          
  
  long  stepsPerRevolution_Y= 9600;
  long  maxSteps_Y          = 9600;
  unsigned maxSpeed_Y       = 800;
  unsigned acceleration_Y   = 2000;
  bool  resetBeforeFeed_Y   = true;
  bool  invertDir_Y         = false;
  int   endstopTrigger_Y    = HIGH;
  int   stepDelay_Y         = 10;
  bool  wiggleRevolver      = false;
  bool  revolverIsServo     = false;
  int   revolverOffPos      = 0;
  int   revolverOnPos       = 90;
  int   servoCycles         = 0;
  int   servoCycles1        = 0;
  int   servoCycles2        = 0;
  unsigned accelDistance_Y  = 20;          
  
  bool  externalControl_Z   = false;
  long  stepsPerMM_Z        = 136;
  unsigned maxSpeed_Z       = 10;
  unsigned insertSpeed_Z    = 1000;
  unsigned acceleration_Z   = 300;
  bool  invertDir_Z         = false;
  int   endstopTrigger_Z    = LOW;
  int   stepDelay_Z         = 10;
  int   feedChunks          = 20;
  bool  enableChunks        = false;
  float insertLength        = 5.0;
  unsigned accelDistance_Z  = 5;          
  // values for TMC drivers via UART or SPI
  int   stepperPower[NUM_STEPPERS+1]      = { 700, 700, 700, 700 };    
  int   stepperMode[NUM_STEPPERS+1]       = { 0, 0, 0, 0 };          // 0 = NONE, 1 = UART, 2 = SPI
  float stepperRSense[NUM_STEPPERS+1]     = { 0.11, 0.11, 0.11, 0.11 };
  int   stepperMicrosteps[NUM_STEPPERS+1] = { 16, 16, 16,16 };
  int   stepperStall[NUM_STEPPERS+1]      = { 0, 0, 0, 0 };
  int   stepperCSmin[NUM_STEPPERS+1]      = { 0, 0, 0, 0 };
  int   stepperCSmax[NUM_STEPPERS+1]      = { 0, 0, 0, 0 };
  int   stepperCSdown[NUM_STEPPERS+1]     = { 0, 0, 0, 0 };
  int   stepperAddr[NUM_STEPPERS+1]       = { 0, 0, 0, 0 };
  int   stepperToff[NUM_STEPPERS+1]       = { -1, -1, -1, -1 };
  bool  stepperStopOnStall[NUM_STEPPERS+1]= { false, false, false, false };
  int   stepperMaxStallCnt[NUM_STEPPERS+1]= { 5, 5, 5, 5};
 
  float unloadRetract       = -20.0f;
  float unloadPushback      = 5.0f;
  float pushbackDelay       = 1.5f;
  float reinforceLength     = 3.0f;
  bool  homeAfterFeed       = true;
  float bowdenLength        = 400.0f;
  float selectorDistance    = 23.0f;
  int   i2cAddress          = 0x58;
  int   lcdContrast         = DSP_CONTRAST;
  int   menuAutoClose       = 20;
  unsigned long serial0Baudrate = 57600;
  unsigned long serial1Baudrate = 57600;
  unsigned long serial2Baudrate = 57600;
  unsigned long serial3Baudrate = 57600;
  bool  sendActionCmds      = false;
  int   fanSpeed            = 0;
  char  materials[MAX_TOOLS][20];
  long  powerSaveTimeout    = 300;
  char  unloadCommand[80]   = { 0 };
  char  wipeSequence[30]    = { 0 };
  bool  prusaMMU2           = true;
  bool  useDuetLaser        = false;
  int   hasPanelDue         = 0;      // Serial Port for PanelDue (0=None)
  int   servoMinPwm         = 550;
  int   servoMaxPwm         = 2400;
  bool  sendPeriodicalStats = true;
  int   backlightColor      = 0x4;    // Cyan by default
  bool  runoutDetection     = false;
  bool  isSharedStepper     = false;
  bool  externalStepper     = false;
  bool  encoderTickSound    = false;
  bool  speedInMMS          = true;
} SMuFFConfig;


#ifdef __BRD_I3_MINI
extern U8G2_ST7565_64128N_F_4W_HW_SPI       display;
#endif
#if defined(__BRD_SKR_MINI) || defined(__BRD_SKR_MINI_E3) || defined(__BRD_SKR_MINI_E3DIP)
  extern "C" uint8_t __wrap_u8x8_byte_arduino_2nd_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
  #ifdef USE_TWI_DISPLAY
  extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C  display;
  #elif defined(USE_ANET_DISPLAY)
  extern U8G2_ST7920_128X64_F_2ND_HW_SPI display;
  // extern U8G2_ST7920_128X64_F_SW_SPI display;
  #elif defined(USE_FYSETC_1_2_DISPLAY) || defined(USE_FYSETC_2_1_DISPLAY)
  extern U8G2_ST7567_JLX12864_F_2ND_4W_HW_SPI display;
  #elif defined(USE_MKS_2_0_DISPLAY)
  extern U8G2_ST7567_ENH_DG128064_F_2ND_4W_HW_SPI display;
  #elif defined(USE_MKS_2_1_DISPLAY)
  extern U8G2_ST7565_NHD_C12864_F_2ND_4W_HW_SPI display;
  #elif defined(USE_CREALITY_DISPLAY)
  extern U8G2_ST7920_128X64_F_SW_SPI display;	
  #else
  #endif
#endif
#ifdef __BRD_ESP32
  #ifdef USE_TWI_DISPLAY
  extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C  display;
  #else
  extern U8G2_ST7567_ENH_DG128064_F_4W_HW_SPI display;
  #endif
#endif
#ifdef __BRD_FYSETC_AIOII
  //extern U8G2_UC1701_MINI12864_F_4W_HW_SPI display;
  extern U8G2_ST7567_JLX12864_F_4W_HW_SPI display;
#endif

extern ZStepper       steppers[];
extern ZTimer         stepperTimer;
extern ZTimer         gpTimer;
extern ZServo         servo;
extern ZServo         servoRevolver;
extern ZFan           fan;
extern ClickEncoder   encoder;
#if defined(USE_FASTLED_BACKLIGHT)
extern CRGB           leds[];
#else
#endif

extern SMuFFConfig    smuffConfig;
extern GCodeFunctions gCodeFuncsM[];
extern GCodeFunctions gCodeFuncsG[];

extern const char     brand[];
extern int            swapTools[];
extern volatile byte  nextStepperFlag;
extern volatile byte  remainingSteppersFlag;
extern volatile unsigned long lastEncoderButtonTime;
extern byte           toolSelected;
extern PositionMode   positionMode;
extern String         serialBuffer0, serialBuffer2, serialBuffer9, traceSerial2;
extern String         tuneSequence; 
extern String         tuneBeep; 
extern String         tuneLongBeep; 
extern String         tuneUser; 
extern String         tuneEncoder; 
extern bool           displayingUserMessage;
extern unsigned int   userMessageTime;
extern bool           testMode;
extern bool           feederJammed;
extern volatile bool  parserBusy;
extern volatile bool  isPwrSave;
extern volatile bool  actionOk;
extern volatile bool  sendingResponse;
extern unsigned long  endstopZ2HitCnt;
extern volatile bool  showMenu;
extern bool           maintainingMode;
extern volatile double lastDuetPos;
extern DuetLaserSensor duetLS;
extern String         wirelessHostname;
extern volatile bool  enablePeriStat;
extern volatile bool  interval20ms;
extern volatile bool  interval100ms; 
extern volatile bool  interval250ms; 
extern volatile bool  interval500ms; 
extern volatile bool  interval1s; 
extern volatile bool  interval2s; 
extern volatile bool  interval5s; 

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
extern void initHwDebug();
extern void initFastLED();
extern void initUSB();
extern void drawLogo();
extern void drawStatus();
extern void drawSelectingMessage();
extern void drawUserMessage(String message, bool smallFont = false, bool center = true, void (*drawCallbackFunc)() = NULL);
extern void drawSDStatus(int stat);
extern void drawFeed();
extern void resetDisplay();
extern bool selectorEndstop();
extern bool revolverEndstop();
extern bool feederEndstop(int index = 1);
extern bool showFeederLoadedMessage();
extern bool showFeederLoadMessage();
extern bool showFeederFailedMessage(int state);
extern int  showDialog(PGM_P title, PGM_P message, PGM_P addMessage, PGM_P buttons);
extern bool moveHome(int index, bool showMessage = true, bool checkFeeder = true);
extern bool loadFilament(bool showMessage = true);
extern bool loadFilamentPMMU2(bool showMessage = true);
extern bool unloadFilament();
extern bool nudgeBackFilament();
extern void handleStall(int axis);
extern void runAndWait(int index);
extern void runNoWait(int index);
extern bool selectTool(int ndx, bool showMessage = true);
extern void setStepperSteps(int index, long steps, bool ignoreEndstop);
extern void prepSteppingAbs(int index, long steps, bool ignoreEndstop = false);
extern void prepSteppingAbsMillimeter(int index, float millimeter, bool ignoreEndstop = false);
extern void prepSteppingRel(int index, long steps, bool ignoreEndstop = false);
extern void prepSteppingRelMillimeter(int index, float millimeter, bool ignoreEndstop = false);
extern void resetRevolver();
extern void serialEvent();
extern void serialEvent2();
#ifndef __AVR__
extern void serialEvent1();
extern void serialEvent3();
#endif
#ifdef __AVR__
extern void wireReceiveEvent(int numBytes);
#endif
extern void beep(int count);
extern void longBeep(int count);
extern void userBeep();
extern void encoderBeep(int count);
extern void startupBeep();
extern void setSignalPort(int port, bool state);
extern void signalNoTool();
extern void signalLoadFilament();
extern void signalUnloadFilament();
extern void signalSelectorBusy();
extern void signalSelectorReady();
extern bool setServoPos(int servoNum, int degree);
extern bool setServoMS(int servoNum, int microseconds);
extern void setServoMinPwm(int servoNum, int pwm);
extern void setServoMaxPwm(int servoNum, int pwm);
extern void getStoredData();
extern const char* readTune(const char* filename);
extern void readSequences();
extern void readConfig();
extern bool writeConfig(Print* dumpTo = NULL);
extern bool checkAutoClose();
extern void resetAutoClose();
extern bool checkUserMessage();
extern void listDir(File root, int numTabs, int serial);
extern void setPwrSave(int state);
extern void __debugS(const char* fmt, ...);
extern void setAbortRequested(bool state);
extern void resetSerialBuffer(int serial);
extern void checkSerialPending();
extern void setPwrSave(int state);
extern void drawSwapTool(int from, int with);
extern uint8_t swapTool(uint8_t index);
extern void positionRevolver();
extern bool feedToEndstop(bool showMessage);
extern void feedToNozzle();
extern void unloadFromNozzle();
extern int splitStringLines(char* lines[], int maxLines, const char* message, const char* token="\n");
extern void debounceButton();
extern bool checkStopMenu(unsigned startTime);
extern void drawTestrunMessage(unsigned long loop, char* msg);
extern bool getFiles(const char* rootFolder, const char* pattern, int maxFiles, bool cutExtension, char* files);
extern void testRun(String fname);
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
extern void setDriverSpreadCycle(TMC2209Stepper* driver, bool spread, int stallThrs, int csmin=0, int csmax=0, int csdown=0);

extern void printEndstopState(int serial);
extern void printPos(int index, int serial);
extern void printAcceleration(int serial);
extern void printSpeeds(int serial);
extern void sendGList(int serial);
extern void sendMList(int serial);
extern void sendToolResponse(int serial);
extern void sendStartResponse(int serial);
extern void sendOkResponse(int serial);
extern void sendErrorResponse(int serial, const char* msg = NULL);
extern void sendErrorResponseP(int serial, const char* msg = NULL);
extern void parseGcode(const String& serialBuffer, int serial);
extern bool parse_G(const String& buf, int serial);
extern bool parse_M(const String& buf, int serial);
extern bool parse_T(const String& buf, int serial);
extern bool parse_PMMU2(char cmd, const String& buf, int serial);
extern bool parse_Action(const String& buf, int serial);
extern int  getParam(String buf, char* token);
extern long getParamL(String buf, char* token);
extern float getParamF(String buf, char* token);
extern bool hasParam(String buf, char* token);
extern bool getParamString(String buf, char* token, char* dest, int bufLen);
extern void prepStepping(int index, long param, bool Millimeter = true, bool ignoreEndstop = false);
extern void saveSettings(int serial);
extern void reportSettings(int serial);
extern void printResponse(const char* response, int serial);
extern void printResponseP(const char* response, int serial);
extern void printOffsets(int serial);
extern void printDriverMode(int serial);
extern void printDriverRms(int serial);
extern void printDriverStallThrs(int serial);
extern void maintainTool();
extern void printPeriodicalState(int serial);
extern void playSequence(const char* sequence);

extern void showLed(int mode, int count);
extern void setBacklightIndex(int color);
extern void setBacklightRGB(int color);
extern void setBacklightRGB(byte R, byte G, byte B);
extern void setBacklightCRGB(CRGB color);
extern void setFastLED(int index, CRGB color);
extern void setFastLEDIndex(int index, int color);
extern void setFastLEDIntensity(int intensity);
extern void testFastLED();

extern void showDuetLS();
extern void switchFeederStepper(int stepper);
extern void removeFirmwareBin();
extern bool initSD(bool showStatus = true);

extern unsigned int translateTicks(unsigned long ticks, int stepsPerMM);
extern long translateSpeed(unsigned int speed, int stepsPerMM);

#endif
