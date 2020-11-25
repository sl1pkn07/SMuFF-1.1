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

/*
 * Module containing helper functions
 */

#include <Arduino.h>
#include "SMuFF.h"
#include "SMuFFBitmaps.h"
#include "Config.h"
#include "InputDialogs.h"

#if defined(__ESP32__)
  extern BluetoothSerial SerialBT;
#endif

extern ZStepper       steppers[];
extern ZServo         servo;
extern ZServo         servoLid;
extern SdFs           SD;

SMuFFConfig           smuffConfig;
int8_t                toolSelected = -1;
bool                  feederJammed = false;
PositionMode          positionMode = RELATIVE;
bool                  displayingUserMessage = false;
bool                  isAbortRequested = false;
uint16_t              userMessageTime = 0;
uint8_t               swapTools[MAX_TOOLS];
bool                  isWarning;
bool                  lidOpen = true;
unsigned long         feederErrors = 0;
bool                  ignoreHoming = false;
unsigned long         stallDetectedCountSelector = 0;
unsigned long         stallDetectedCountFeeder = 0;
//char PROGMEM          tuneStartup[]  = { "F440D120P150.F523D120P80.F196D220P80.F196D120P80.F587D400P120.F349D80P240.F349D80P160." }; // the new tune
char PROGMEM          tuneStartup[100] = { "F1760D90.F1975D90.F2093D90.F1975D90.F1760D200P50." };   // the "old" tune
char PROGMEM          tuneUser[40]      = { "F1760D90P90.F440D90P90.F440D90P90." };
char PROGMEM          tuneBeep[20]      = { "F1760D90P200." };
char PROGMEM          tuneLongBeep[20]  = { "F1760D450P500." };
#if defined(USE_LEONERD_DISPLAY)
  char PROGMEM        tuneEncoder[20]   = { "F330D10P10." };
#else
  char PROGMEM        tuneEncoder[20]   = { "F1440D3." };
#endif

uint16_t              sequence[MAX_SEQUENCE][3];    // store for tune sequence for background playing
uint8_t               sequenceCnt = 0;
bool                  startSequence = false;        // trigger for background playing of tune sequence
bool                  isPlaying = false;


#define _F_           0
#define _D_           1
#define _P_           2

void setupDisplay() {
  // The next code line (display.setI2CAddress) changes the address for your TWI_DISPLAY if it's
  // configured differently.
  // Usually, thoses displays are pre-configured at I2C address 0x78, which equals to 0x3c
  // from the software side because of the 7-Bit address mode.
  // If it's configured at 0x7a, you need to change the I2C_DISPLAY_ADDRESS in Config.h to 0x3d.
  #if I2C_DISPLAY_ADDRESS != 0x3C
    display.setI2CAddress(I2C_DISPLAY_ADDRESS);
  #endif
  #if defined(USE_LEONERD_DISPLAY)
    //display.setBusClock(400000);
    display.begin();
  #else
    display.begin(/*Select=*/ ENCODER_BUTTON_PIN,  /* menu_next_pin= */ U8X8_PIN_NONE, /* menu_prev_pin= */ U8X8_PIN_NONE, /* menu_home_pin= */ U8X8_PIN_NONE);
  #endif
  display.enableUTF8Print();
  resetDisplay();
  display.setContrast(smuffConfig.lcdContrast);
}

void drawLogo() {
  //__debug(PSTR("drawLogo start..."));
  char brand[] = VERSION_STRING;

  display.setBitmapMode(1);
  display.drawXBMP(0, 0, logo_width, logo_height, logo_bits);
  display.setFont(LOGO_FONT);
  display.setFontMode(0);
  display.setFontDirection(0);
  display.setDrawColor(1);
  display.setCursor(display.getDisplayWidth() - display.getStrWidth(brand) - 1, display.getDisplayHeight() - display.getMaxCharHeight());
  display.print(brand);
  //__debug(PSTR("drawLogo end..."));
}

void drawStatus() {
  char _wait[128];
  char tmp[80];
  char pos[20];

  //__debug(PSTR("drawStatus start..."));
  display.setFont(STATUS_FONT);
  display.setFontMode(0);
  display.setDrawColor(1);
  sprintf_P(tmp, P_CurrentTool);
  display.drawStr(display.getDisplayWidth() - display.getStrWidth(tmp) - 10, 14, tmp);
  if((toolSelected >= 0 && toolSelected < smuffConfig.toolCount))
    sprintf_P(tmp, PSTR("%2d"), toolSelected);
  else
    sprintf_P(tmp, PSTR("--"));
  display.drawStr(display.getDisplayWidth() - display.getStrWidth(tmp) - 10, 14, tmp);
  sprintf_P(tmp, P_Feed);
  display.drawStr(display.getDisplayWidth() - display.getStrWidth(tmp) - 10, 34, tmp);
  display.setFontMode(1);
  display.setFont(SMALL_FONT);
  display.setDrawColor(2);
  display.drawBox(0, display.getDisplayHeight()-display.getMaxCharHeight()+2, display.getDisplayWidth(), display.getMaxCharHeight());
  sprintf_P(_wait, parserBusy ? P_Busy : (smuffConfig.prusaMMU2) ? P_Pemu : P_Ready);

  sprintf_P(pos, PSTR("%4.2f"), steppers[FEEDER].getStepsTakenMM());
  sprintf_P(tmp, PSTR("%-7s| %-4s | %-4s "), pos, smuffConfig.externalStepper ? P_External : P_Internal, _wait);
  display.drawStr(1, display.getDisplayHeight(), tmp);

  display.setFontMode(0);
  display.setDrawColor(1);
  if(steppers[FEEDER].getMovementDone()) {
    display.setFont(ICONIC_FONT);
    display.drawGlyph(110, 38, feederEndstop() ? 0x41 : 0x42);
    display.setFont(BASE_FONT);
  }
  else {
    drawFeed();
  }
  //__debug(PSTR("drawStatus end..."));
}

void drawSDRemoved(bool removed) {
  if(!removed)
    return;

  display.setFont(ICONIC_FONT);
  display.drawGlyph(2, 10, 0x47);
}

void drawFeed() {
  char tmp[80];
  sprintf_P(tmp, PSTR("%-7s"), String(steppers[FEEDER].getStepsTakenMM()).c_str());
  #if defined(__STM32F1__) || defined(__ESP32__)
    display.setFont(SMALL_FONT);
    display.setFontMode(0);
    display.setDrawColor(0);
    uint16_t x = 0;
    uint16_t y = display.getDisplayHeight()-display.getMaxCharHeight() + 2;
    uint16_t w = 40;
    uint16_t h = display.getMaxCharHeight();
    display.drawBox(x, y, w, h);
    display.drawStr(x+1, display.getDisplayHeight(), tmp);
    display.sendBuffer();
    //display.updateDisplayArea(x, y, w, h);

  #else
    display.setFont(STATUS_FONT);
    display.setFontMode(0);
    display.setDrawColor(0);
    display.drawBox(62, 24, display.getDisplayWidth(), display.getMaxCharHeight()-2);
    display.setDrawColor(1);
    display.drawStr(62, 34, tmp);
  #endif
}

uint8_t duetTurn = 0;
void drawSymbolCallback() {
  uint16_t symbols[] = { 0x25ef, 0x25d0, 0x25d3, 0x25d1, 0x25d2 };

  if(duetLS.isMoving()) {
    if(++duetTurn > 4)
      duetTurn = 1;
  }
  else {
    duetTurn = 0;
  }
  display.setFont(ICONIC_FONT);
  display.drawGlyph(118, 10, symbols[duetTurn]);
}

void showDuetLS() {
  char _msg[128];
  char _addData[15];
  char _dir[3];
  int16_t turn;
  uint8_t btn;
  bool isHeld, isClicked;

  bool extStepper = smuffConfig.externalStepper;
  switchFeederStepper(INTERNAL);
  debounceButton();
  encoder.setAccelerationEnabled(true);
  while(1) {
    getInput(&turn, &btn, &isHeld, &isClicked);
    if(isHeld || isClicked) {
      break;
    }
    // if the encoder knob is being turned, extrude / retract filament by the value defined
    switch(encoder.getValue()) {
        case -1: moveFeeder(-0.25); break;
        case  1: moveFeeder( 0.25); break;
        case -2: moveFeeder(-0.50); break;
        case  2: moveFeeder( 0.50); break;
        case -3: moveFeeder(-1.00); break;
        case  3: moveFeeder( 1.00); break;
        case -4: moveFeeder(-2.00); break;
        case  4: moveFeeder( 2.00); break;
        case -5: moveFeeder(-5.00); break;
        case  5: moveFeeder( 5.00); break;
        default: break;
    }
    uint8_t err = duetLS.getSensorError();
    if(err != 0)
      sprintf_P(_addData, PSTR("%4s/%04x"), err==E_SENSOR_INIT ? "INIT" : err==E_SENSOR_VCC ? "VCC" : String(err).c_str(), duetLS.getError());
    else {
      if(!duetLS.isValid())
        sprintf_P(_addData, PSTR("-invalid-"));
      else
        sprintf_P(_addData, PSTR("none/%04x"), duetLS.getError());
    }
    switch(duetLS.getDirection()) {
      case DIR_NONE: sprintf(_dir,"  "); break;
      case DIR_EXTRUDE: sprintf(_dir, "<<"); break;
      case DIR_RETRACT: sprintf(_dir, ">>"); break;
    }

    sprintf_P(_msg, P_DuetLSData, _dir, String(duetLS.getPositionMM()).c_str(),  duetLS.getQuality(), duetLS.getBrightness(), duetLS.getShutter(), duetLS.getSwitch() ? P_On : P_Off, _addData, duetLS.getVersion());
    drawUserMessage(_msg, true, false, drawSymbolCallback);
    delay(100);
  }
  if(extStepper)
    switchFeederStepper(EXTERNAL);
}

TMC2209Stepper* showDriver = nullptr;

void drawStallCallback() {
  uint16_t symbols[] = { 0x0020, 0x21af, 0x0020, 0x2607 };
  if(showDriver == nullptr)
    return;
  bool stat = showDriver->diag();
  display.setFont(ICONIC_FONT);
  display.drawGlyph(100, 11, symbols[stat]);
  stat = showDriver->SG_RESULT() < showDriver->SGTHRS()*2;
  display.drawGlyph(114, 11, symbols[stat+2]);
}

void showTMCStatus(uint8_t axis) {
  char _msg[256];
  int16_t turn;
  uint8_t btn;
  bool isHeld, isClicked;

  debounceButton();
  encoder.setAccelerationEnabled(true);
  showDriver = drivers[axis];
  if(showDriver == nullptr) {
    debounceButton();
    drawUserMessage(P_StepperNotCfg);
    delay(3000);
    return;
  }
  steppers[axis].setEnabled(true);
  displayingUserMessage = true;
  uint8_t n=0;
  while(1) {
    getInput(&turn, &btn, &isHeld, &isClicked);
    if(isHeld || isClicked) {
      break;
    }

    const char* ot_stat = P_No;
    if(showDriver->ot()) {
      if(showDriver->t157())
        ot_stat = P_OT_157;
      else if(showDriver->t150())
        ot_stat = P_OT_150;
      else if(showDriver->t143())
        ot_stat = P_OT_143;
      else if(showDriver->t120())
        ot_stat = P_OT_120;
    }
    sprintf_P(_msg, P_TMC_Status,
                showDriver->stealth() ? P_Stealth : P_Spread,
                showDriver->rms_current(),
                showDriver->ola() ? P_Yes : P_No,
                showDriver->olb() ? P_Yes : P_No,
                showDriver->s2ga() ? P_Yes : P_No,
                showDriver->s2gb() ? P_Yes : P_No,
                ot_stat);

    drawUserMessage(_msg, true, false, showDriver->stealth() ? drawStallCallback : nullptr);
    delay(100);
    if(showDriver->diag()) {
      //if(n==0)
      //  __debug(PSTR("Stepper stall detected @ %ld"), showDriver->SG_RESULT());
      n++;
    }
    // reset stall flag after 3 secs.
    if(n % 30 == 0 && showDriver->diag()) {
      n = 0;
      //__debug(PSTR("Stall has been reset!"));
    }
  }
  displayingUserMessage = false;
}

void resetDisplay() {
  display.clearDisplay();
  display.setFont(BASE_FONT);
  display.setFontMode(0);
  display.setDrawColor(1);
}

void drawSelectingMessage(uint8_t tool) {
  char _sel[128];
  char _wait[128];
  char tmp[80];


  if(displayingUserMessage)     // don't show if something else is being displayed
    return;
  display.firstPage();
  do {
    resetDisplay();
    sprintf_P(_sel, P_Selecting);
    sprintf_P(_wait, P_Wait);
    if(*smuffConfig.materials[tool] != 0) {
      sprintf_P(tmp,PSTR("%s"), smuffConfig.materials[tool]);
    }
    else {
      sprintf_P(tmp, P_ToolMenu, tool);
    }
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(_sel))/2, (display.getDisplayHeight() - display.getMaxCharHeight())/2-10, _sel);
    display.setFont(BASE_FONT_BIG);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(tmp))/2, (display.getDisplayHeight() - display.getMaxCharHeight())/2+9, tmp);
    display.setFont(BASE_FONT);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(_wait))/2, (display.getDisplayHeight() - display.getMaxCharHeight())/2 + display.getMaxCharHeight()+10, _wait);
  } while(display.nextPage());
}

void drawTestrunMessage(unsigned long loop, char* msg) {
  char _sel[128];
  char _wait[128];
  display.firstPage();
  do {
    resetDisplay();
    sprintf_P(_sel, P_RunningCmd, loop);
    sprintf_P(_wait, P_ButtonToStop);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(_sel))/2, (display.getDisplayHeight() - display.getMaxCharHeight())/2-10, _sel);
    display.setFont(BASE_FONT_BIG);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg))/2, (display.getDisplayHeight() - display.getMaxCharHeight())/2+12, msg);
    display.setFont(BASE_FONT);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(_wait))/2, (display.getDisplayHeight() - display.getMaxCharHeight())/2 + display.getMaxCharHeight()+15, _wait);
  } while(display.nextPage());
}

/**
 * Parses a given buffer for newlines (delimiter) and fills the lines pointer
 * array with the lines found.
 *
 * @param lines     container for lines
 * @param maxLines  max. storage capacity of lines
 * @param message   the source buffer to be parsed for lines
 * @param delimiter the EOL character to parse for
 * @returns the number of lines found
 */
uint8_t splitStringLines(char* lines[], uint8_t maxLines, const char* message, const char* delimiter) {

  char* tok = strtok((char*)message, delimiter);
  char* lastTok = nullptr;
  int8_t cnt = -1;

  while(tok != nullptr) {
    lines[++cnt] = tok;
    lastTok = tok;
    //__debug(PSTR("Line: %s"), lines[cnt]);
    if(cnt >= maxLines-1)
      break;
    tok = strtok(nullptr, delimiter);
  }
  if(lastTok != nullptr && *lastTok != 0 && cnt <= maxLines-1) {
    lines[cnt] = lastTok;   // copy the last line as well
    cnt++;
  }

  return cnt;
}

void drawUserMessage(String message, bool smallFont /* = false */, bool center /* = true */, void (*drawCallbackFunc)() /* = nullptr */) {

  char* lines[8];
  uint8_t lineCnt = splitStringLines(lines, (int)ArraySize(lines), message.c_str());

  if(isPwrSave) {
    setPwrSave(0);
  }
  display.setDrawColor(0);
  display.drawBox(1, 1, display.getDisplayWidth()-2, display.getDisplayHeight()-2);
  display.setDrawColor(1);
  display.drawFrame(0, 0, display.getDisplayWidth(), display.getDisplayHeight());
  display.firstPage();
  do {
    display.setFont(smallFont ? BASE_FONT : BASE_FONT_BIG);
    uint16_t y = (display.getDisplayHeight()-(lineCnt-1)*display.getMaxCharHeight())/2;
    display.firstPage();
    do {
      for(uint8_t i=0; i< lineCnt; i++) {
        uint16_t x = center ? (display.getDisplayWidth() - display.getStrWidth(lines[i]))/2 : 0;
        display.drawStr(x, y, lines[i]);
        if(i==0) {
          if(lineCnt > 1 && strcmp(lines[1]," ")==0) {
            display.drawHLine(0, y+3, display.getDisplayWidth());
          }
        }
        y += display.getMaxCharHeight();
      }
    } while(display.nextPage());
    if(drawCallbackFunc != nullptr)
      drawCallbackFunc();
    display.setFont(BASE_FONT);
  } while(display.nextPage());
  displayingUserMessage  = true;
  userMessageTime = millis();
}


void drawSDStatus(int8_t stat) {
  char tmp[80];

  //resetDisplay();
  switch(stat) {
    case SD_ERR_INIT:
      sprintf_P(tmp, P_SD_InitError);
      longBeep(2);
      break;
    case SD_ERR_NOCONFIG:
      sprintf_P(tmp, P_SD_NoConfig);
      longBeep(1);
      break;
    case SD_READING_CONFIG:
      sprintf_P(tmp, P_SD_Reading, P_SD_ReadingConfig);
      break;
    case SD_READING_TMC:
      sprintf_P(tmp, P_SD_Reading, P_SD_ReadingTmc);
      break;
    case SD_READING_SERVOS:
      sprintf_P(tmp, P_SD_Reading, P_SD_ReadingServos);
      break;
    case SD_READING_MATERIALS:
      sprintf_P(tmp, P_SD_Reading, P_SD_ReadingMaterials);
      break;
  }
  display.firstPage();
  do {
    drawLogo();
    display.setCursor((display.getDisplayWidth() - display.getStrWidth(tmp))/2, display.getDisplayHeight());
    display.print(tmp);
  } while(display.nextPage());
}

bool selectorEndstop() {
  return steppers[SELECTOR].getEndstopHit();
}

bool revolverEndstop() {
  return steppers[REVOLVER].getEndstopHit();
}

bool feederEndstop(int8_t index) {
  return steppers[FEEDER].getEndstopHit(index);
}

void setAbortRequested(bool state) {
  steppers[FEEDER].setAbort(state);  // stop any ongoing stepper movements
}


bool moveHome(int8_t index, bool showMessage, bool checkFeeder) {

  if(!steppers[index].getEnabled())
    steppers[index].setEnabled(true);

  if(feederJammed) {
    beep(4);
    return false;
  }
  parserBusy = true;
  if (checkFeeder && feederEndstop()) {
    if (showMessage) {
      if (!showFeederLoadedMessage()) {
        parserBusy = false;
        return false;
      }
    }
    else {
      if (feederEndstop()) {
        unloadFilament();
      }
    }
  }
  if(smuffConfig.revolverIsServo) {
    //__debug(PSTR("Stepper home SERVO variant"));
    // don't release the servo when homing the Feeder but
    // release it when homing something else
    if(index != FEEDER)
      setServoLid(SERVO_OPEN);
    // Revolver isn't being used on a servo variant
    if(index != REVOLVER)
      steppers[index].home();
  }
  else {
    //__debug(PSTR("Stepper home non SERVO variant"));
    // not a servo variant, home stepper which ever it is
    steppers[index].home();
  }

  //__debug(PSTR("DONE Stepper home"));
  if (index == SELECTOR) {
    toolSelected = -1;
  }
  long pos = steppers[index].getStepPosition();
  if (index == SELECTOR || index == REVOLVER) {
    dataStore.tool = toolSelected;
  }
  dataStore.stepperPos[index] = pos;
  saveStore();
  //__debug(PSTR("DONE save store"));
  parserBusy = false;
  return true;
}

bool showFeederBlockedMessage() {
  bool state = false;
  lastEncoderButtonTime = millis();
  beep(1);
  uint8_t button = showDialog(P_TitleWarning, P_FeederLoaded, P_RemoveMaterial, P_CancelRetryButtons);
  if (button == 1) {
    drawStatus();
    unloadFilament();
    state = true;
  }
  display.clearDisplay();
  return state;
}

bool showFeederLoadedMessage() {
  bool state = false;
  lastEncoderButtonTime = millis();
  beep(1);
  uint8_t button = showDialog(P_TitleWarning, P_FeederLoaded, P_AskUnload, P_YesNoButtons);
  if (button == 1) {
    drawStatus();
    unloadFilament();
    state = true;
  }
  display.clearDisplay();
  return state;
}

bool showFeederLoadMessage() {
  bool state = false;
  lastEncoderButtonTime = millis();
  beep(1);
  uint8_t button = showDialog(P_TitleSelected, P_SelectedTool, P_AskLoad, P_YesNoButtons);
  if (button == 1) {
    drawStatus();
    if(smuffConfig.prusaMMU2)
      loadFilamentPMMU2();
    else
      loadFilament();
    state = true;
  }
  display.clearDisplay();
  return state;
}

bool showFeederFailedMessage(int8_t state) {
  lastEncoderButtonTime = millis();
  beep(3);
  uint8_t button = 99;
  isWarning = true;
  do {
    button = showDialog(P_TitleWarning, state == 1 ? P_CantLoad : P_CantUnload, P_CheckUnit, P_CancelRetryButtons);
  } while(button != 1 && button != 2);
  isWarning = false;
  display.clearDisplay();
  debounceButton();
  return button == 1 ? false : true;
}

uint8_t showDialog(PGM_P title, PGM_P message, PGM_P addMessage, PGM_P buttons) {
  //__debug(PSTR("showDialog: %S"), title);
  if(isPwrSave) {
    setPwrSave(0);
  }
  char _title[80];
  char msg1[256];
  char msg2[80];
  char btn[60];
  sprintf_P(_title, title);
  sprintf_P(msg1, message);
  sprintf_P(msg2, addMessage);
  sprintf_P(btn, buttons);
  return display.userInterfaceMessage(_title, msg1, msg2, btn);
}

void signalNoTool() {
  char _msg1[256];
  userBeep();
  sprintf_P(_msg1, P_NoTool);
  strcat_P(_msg1, P_Aborting);
  drawUserMessage(_msg1);
}

void switchFeederStepper(uint8_t stepper) {
  if(RELAY_PIN != -1) {
    if(stepper == EXTERNAL) {
      steppers[FEEDER].setEnabled(false);
      digitalWrite(RELAY_PIN, 0);
      smuffConfig.externalStepper = true;
    }
    else if(stepper == INTERNAL) {
      steppers[FEEDER].setEnabled(false);
      digitalWrite(RELAY_PIN, 1);
      smuffConfig.externalStepper = false;
    }
    // gain the relay some time to debounce
    delay(50);
  }
}

void moveFeeder(float distanceMM) {
  steppers[FEEDER].setEnabled(true);
  uint16_t curSpeed = steppers[FEEDER].getMaxSpeed();
  changeFeederInsertSpeed(smuffConfig.insertSpeed);
  prepSteppingRelMillimeter(FEEDER, distanceMM, true);
  runAndWait(FEEDER);
  steppers[FEEDER].setMaxSpeed(curSpeed);
}

void positionRevolver() {

  // disable Feeder temporarily
  steppers[FEEDER].setEnabled(false);
  if(smuffConfig.resetBeforeFeed && !ignoreHoming) {
    if(smuffConfig.revolverIsServo) {
      setServoLid(SERVO_OPEN);
    }
    else
      moveHome(REVOLVER, false, false);
  }
  if(smuffConfig.revolverIsServo) {
    setServoLid(SERVO_CLOSED);
    steppers[FEEDER].setEnabled(true);
    return;
  }

  long pos = steppers[REVOLVER].getStepPosition();
  long newPos = smuffConfig.firstRevolverOffset + (toolSelected *smuffConfig.revolverSpacing);
  // calculate the new position and decide whether to move forward or backard
  // i.e. which ever has the shorter distance
  long delta1 = newPos - (smuffConfig.stepsPerRevolution + pos);  // number of steps if moved backward
  long delta2 = newPos - pos;                                       // number of steps if moved forward
  if(abs(delta1) < abs(delta2))
    newPos = delta1;
  else
    newPos = delta2;

  // if the position hasn't changed, do nothing
  if(newPos != 0) {
    prepSteppingRel(REVOLVER, newPos, true); // go to position, don't mind the endstop
    remainingSteppersFlag |= _BV(REVOLVER);
    runAndWait(-1);
    if(smuffConfig.wiggleRevolver) {
      // wiggle the Revolver one position back and forth
      // just to adjust the gears a bit better
      delay(50);
      prepSteppingRel(REVOLVER, smuffConfig.revolverSpacing, true);
      remainingSteppersFlag |= _BV(REVOLVER);
      runAndWait(-1);
      delay(50);
      prepSteppingRel(REVOLVER, -(smuffConfig.revolverSpacing), true);
      remainingSteppersFlag |= _BV(REVOLVER);
      runAndWait(-1);
    }
  }
  steppers[FEEDER].setEnabled(true);
  delay(150);
  //__debug(PSTR("PositionRevolver: pos: %d"), steppers[REVOLVER].getStepPosition());
}

void changeFeederInsertSpeed(uint16_t speed) {
  unsigned long maxSpeed = translateSpeed(speed, FEEDER);
  //__debug(PSTR("Changing Feeder speed to %3d (%6ld ticks)"), speed, maxSpeed);
  steppers[FEEDER].setMaxSpeed(maxSpeed);
}

void repositionSelector(bool retractFilament) {
  int8_t tool = toolSelected;
  if(retractFilament && !smuffConfig.revolverIsServo) {
    char tmp[15];
    uint16_t curSpeed = steppers[FEEDER].getMaxSpeed();
    changeFeederInsertSpeed(smuffConfig.insertSpeed);
    ignoreHoming = true;
    // go through all tools available and retract some filament
    for(uint8_t i=0; i < smuffConfig.toolCount; i++) {
      if(i==tool)
        continue;
      sprintf(tmp, "Y%d", i);
      G0("G0", tmp, 0);                  // position Revolver on tool
      prepSteppingRelMillimeter(FEEDER, -smuffConfig.insertLength, true); // retract
      runAndWait(FEEDER);
    }
    ignoreHoming = false;
    sprintf(tmp, "Y%d", tool);
    G0("G0", tmp, 0);                  // position Revolver on tool selected
    steppers[FEEDER].setMaxSpeed(curSpeed);
  }
  moveHome(SELECTOR, false, false);   // home Revolver
  selectTool(tool, false);            // reposition Selector
}

bool feedToEndstop(bool showMessage) {
  // enable steppers if they were turned off
  if(!steppers[FEEDER].getEnabled())
    steppers[FEEDER].setEnabled(true);

  // don't allow "feed to endstop" being interrupted
  steppers[FEEDER].setIgnoreAbort(true);

  positionRevolver();

  uint16_t curSpeed = steppers[FEEDER].getMaxSpeed();
  //__debug(PSTR("InsertSpeed: %d"), smuffConfig.insertSpeed);
  uint16_t speed = smuffConfig.insertSpeed;
  changeFeederInsertSpeed(speed);
  if(smuffConfig.accelSpeed[FEEDER] > smuffConfig.insertSpeed)
    steppers[FEEDER].setAllowAccel(false);

  uint16_t max = (uint16_t)(smuffConfig.selectorDistance*2);  // calculate a maximum distance to avoid feeding endlesly
  uint8_t n = 0;
  int8_t retries = FEED_ERROR_RETRIES;  // max. retries for this operation

  feederJammed = false;

  // is the feeder endstop already being triggered?
  if(feederEndstop()) {
    // yes, filament is still fed, unload completelly and
    // abort this operation if that fails
    if(!unloadFromNozzle(showMessage))
      return false;
  }

  steppers[FEEDER].setStepPositionMM(0);
  // as long as Selector endstop doesn't trigger
  // feed the configured insertLength
  while (!feederEndstop()) {
    prepSteppingRelMillimeter(FEEDER, smuffConfig.insertLength, false);
    runAndWait(FEEDER);
    // has the endstop already triggered?
    if(feederEndstop()) {
      //__debug(PSTR("Position now: %s"), String(steppers[FEEDER].getStepPositionMM()).c_str());
      break;
    }
    n += smuffConfig.insertLength;  // increment the position of the filament
    // did the Feeder stall (TMC2209 only)?
    bool stallStat = handleFeederStall(&speed, &retries);
    // if endstop hasn't triggered yet, feed was not successful
    if (n >= max && !feederEndstop()) {
      delay(250);
      // retract half a insertLength and reset the Revolver
      prepSteppingRelMillimeter(FEEDER, -(smuffConfig.insertLength/2), true);
      runAndWait(FEEDER);
      resetRevolver();
      feederErrors++;   // global counter used for testing only
      if(stallStat)     // did not stall means no retries decrement, though, the endstop hasn't triggered yet
        retries--;
      // if only two retries are left, try repositioning the Selector
      if(retries == 1) {
        repositionSelector(false);
      }
      // if only one retry is left, rectract filaments a bit and try repositioning the Selector
      if(retries == 0) {
        repositionSelector(true);
      }
      // close lid servo in case it got openend by the reposition operation
      if(smuffConfig.revolverIsServo)
        setServoLid(SERVO_CLOSED);
      n = 0;
    }
    //__debug(PSTR("Max: %s  N: %s  Retries: %d  Endstop: %d"), String(max).c_str(), String(n).c_str(), retries, feederEndstop());
    if (!feederEndstop() && retries < 0) {
      // still got no endstop trigger, abort action
      if (showMessage) {
        moveHome(REVOLVER, false, false);   // home Revolver
        M18("M18", "", 0);                  // turn all motors off
        if(smuffConfig.revolverIsServo)     // release servo, if used
          setServoLid(SERVO_OPEN);
        // if user wants to retry...
        if(showFeederFailedMessage(1) == true) {
          // reset and start over again
          steppers[FEEDER].setEnabled(true);
          positionRevolver();
          n = 0;
          retries = FEED_ERROR_RETRIES;
          continue;
        }
      }
      // otherwise, assume the feeder is jammed
      feederJammed = true;
      break;
    }
  }
  steppers[FEEDER].setIgnoreAbort(false);
  steppers[FEEDER].setAllowAccel(true);
  steppers[FEEDER].setMaxSpeed(curSpeed);
  delay(300);
  return feederJammed ? false : true;
}

bool handleFeederStall(uint16_t* speed, int8_t* retries) {
  bool stat = true;
  // did the Feeder stall?
  if(smuffConfig.stepperStopOnStall[FEEDER] && steppers[FEEDER].getStallDetected()) {
    stat = false;
    int16_t newSpeed;
    // yes, turn the speed down by 25%
    if(smuffConfig.speedsInMMS) {
      // speeds in mm/s need to go down
      newSpeed = (int16_t)(*speed * 0.75);
      if(newSpeed > 0)
        *speed = newSpeed;
      else
        *speed = 1;     // set speed to absolute minimum
    }
    else {
      // whereas speeds in timer ticks need to go up
      newSpeed = (int16_t)(*speed * 1.25);
      if(newSpeed < 65500)
        *speed = newSpeed;
      else
        *speed = 65500;     // set speed to absolute minimum
    }
    *retries -= 1;
    changeFeederInsertSpeed(*speed);
    __debug(PSTR("Feeder has stalled, slowing down speed to %d"), *speed);
    // counter used in testRun
    stallDetectedCountFeeder++;   // for testrun only
  }
  return stat;
}

bool feedToNozzle(bool showMessage) {

  bool stat = true;
  uint16_t speed = smuffConfig.maxSpeed[FEEDER];
  int8_t retries = FEED_ERROR_RETRIES;

  if(smuffConfig.prusaMMU2 && smuffConfig.enableChunks) {
    // prepare to feed full speed in chunks
    float bLen = smuffConfig.bowdenLength;
    float len = bLen/smuffConfig.feedChunks;
    for(uint8_t i=0; i<smuffConfig.feedChunks; i++) {
      prepSteppingRelMillimeter(FEEDER, len, true);
      runAndWait(FEEDER);
    }
  }
  else {
    float len = smuffConfig.bowdenLength * .95;
    float remains = 0;
    steppers[FEEDER].setStepPositionMM(0);
    // prepare 95% to feed full speed
    do {
      prepSteppingRelMillimeter(FEEDER, len-remains, true);
      runAndWait(FEEDER);
      // did the Feeder stall?
      stat = handleFeederStall(&speed, &retries);
      if(!stat) {
        remains = steppers[FEEDER].getStepPositionMM();
        __debug(PSTR("Len: %s  Remain: %s  To go: %s"), String(len).c_str(), String(remains).c_str(), String(len-remains).c_str());
      }
    } while(!stat && retries > 0);
    if(stat) {
      retries = FEED_ERROR_RETRIES;
      speed = smuffConfig.insertSpeed;
      float len = smuffConfig.bowdenLength * .05;
      float remains = 0;
      steppers[FEEDER].setStepPositionMM(0);
      // rest of it feed slowly
      do {
        changeFeederInsertSpeed(speed);
        prepSteppingRelMillimeter(FEEDER, len-remains, true);
        runAndWait(FEEDER);
        // did the Feeder stall again?
        stat = handleFeederStall(&speed, &retries);
        if(!stat) {
          remains = steppers[FEEDER].getStepPositionMM();
          __debug(PSTR("Len: %s  Remain: %s  To go: %s"), String(len).c_str(), String(remains).c_str(), String(len-remains).c_str());
        }
      }while(!stat && retries > 0);
    }
  }
  return stat;
}

bool loadFilament(bool showMessage) {
  if (toolSelected == -1) {
    signalNoTool();
    return false;
  }
  if(smuffConfig.extControlFeeder) {
    if(!smuffConfig.isSharedStepper) {
      positionRevolver();
      signalLoadFilament();
      return true;
    }
    else {
      switchFeederStepper(INTERNAL);
    }
  }
  parserBusy = true;
  uint16_t curSpeed = steppers[FEEDER].getMaxSpeed();
    // move filament until it hits the feeder endstop
  if(!feedToEndstop(showMessage))
    return false;

  steppers[FEEDER].setStepsTaken(0);
  // move filament until it gets to the nozzle
  if(!feedToNozzle(showMessage))
    return false;

  if(smuffConfig.reinforceLength > 0 && !steppers[FEEDER].getAbort()) {
    resetRevolver();
    prepSteppingRelMillimeter(FEEDER, smuffConfig.reinforceLength, true);
    runAndWait(FEEDER);
  }

  steppers[FEEDER].setMaxSpeed(curSpeed);
  dataStore.stepperPos[FEEDER] = steppers[FEEDER].getStepPosition();
  saveStore();

  if(smuffConfig.homeAfterFeed) {
    if(smuffConfig.revolverIsServo) {
      setServoLid(SERVO_OPEN);
    }
    else
      steppers[REVOLVER].home();
  }
  bool wasAborted = steppers[FEEDER].getAbort();
  steppers[FEEDER].setAbort(false);

  if(smuffConfig.extControlFeeder && smuffConfig.isSharedStepper) {
    switchFeederStepper(EXTERNAL);
  }

  parserBusy = false;
  return true; //aborted ? false : true;
}

/*
  This method is used to feed the filament Prusa style (L command on MMU2).
  If first feeds the filament until the endstop is hit, then
  it pulls it back again.
*/
bool loadFilamentPMMU2(bool showMessage) {
  if (toolSelected == -1) {
    signalNoTool();
    return false;
  }
  if(smuffConfig.extControlFeeder && !smuffConfig.isSharedStepper) {
    positionRevolver();
    signalLoadFilament();
    return true;
  }
  if(smuffConfig.extControlFeeder && smuffConfig.isSharedStepper) {
    switchFeederStepper(INTERNAL);
  }

  parserBusy = true;
  uint16_t curSpeed = steppers[FEEDER].getMaxSpeed();
  // move filament until it hits the feeder endstop
  if(!feedToEndstop(showMessage))
    return false;

  steppers[FEEDER].setStepsTaken(0);
  // inhibit interrupts at this step
  steppers[FEEDER].setIgnoreAbort(true);
  // now pull it back again
  changeFeederInsertSpeed(smuffConfig.insertSpeed);
  prepSteppingRelMillimeter(FEEDER, -smuffConfig.selectorDistance, true);
  runAndWait(FEEDER);

  if(smuffConfig.reinforceLength > 0 && !steppers[FEEDER].getAbort()) {
    resetRevolver();
    prepSteppingRelMillimeter(FEEDER, smuffConfig.reinforceLength, true);
    runAndWait(FEEDER);
  }
  steppers[FEEDER].setIgnoreAbort(false);

  steppers[FEEDER].setMaxSpeed(curSpeed);
  dataStore.stepperPos[FEEDER] = steppers[FEEDER].getStepPosition();
  saveStore();

  if(smuffConfig.homeAfterFeed) {
    if(smuffConfig.revolverIsServo) {
      setServoLid(SERVO_OPEN);
    }
    else
      steppers[REVOLVER].home();
  }
  steppers[FEEDER].setAbort(false);

  if(smuffConfig.extControlFeeder && smuffConfig.isSharedStepper) {
    switchFeederStepper(EXTERNAL);
  }

  parserBusy = false;
  return true;
}

bool unloadFromNozzle(bool showMessage) {
  bool stat = true;
  uint16_t speed = smuffConfig.maxSpeed[FEEDER];
  int8_t retries = FEED_ERROR_RETRIES;

  if(smuffConfig.prusaMMU2 && smuffConfig.enableChunks) {
    __debug(PSTR("Unloading in %d chunks "), smuffConfig.feedChunks);
    // prepare to unfeed 3 times the bowden length full speed in chunks
    float bLen = -smuffConfig.bowdenLength*3;
    float len = bLen/smuffConfig.feedChunks;
    for(uint8_t i=0; i<smuffConfig.feedChunks; i++) {
      prepSteppingRelMillimeter(FEEDER, len);
      runAndWait(FEEDER);
    }
  }
  else {
    do {
      // prepare 110% to retract with full speed
      prepSteppingRelMillimeter(FEEDER, -(smuffConfig.bowdenLength*1.1));
      runAndWait(FEEDER);
      // did the Feeder stall?
      stat = handleFeederStall(&speed, &retries);
    } while(!stat && retries > 0);
  }
  //__debug(PSTR("Feeder endstop now %d (%d)"), feederEndstop(), steppers[FEEDER].getEndstopState());
  // reset feeder to max. speed
  changeFeederInsertSpeed(smuffConfig.maxSpeed[FEEDER]);
  delay(500);
  return stat;
}

bool unloadFromSelector() {
  bool stat = true;
  // only if the unload hasn't been aborted yet, unload from Selector
  if(steppers[FEEDER].getAbort() == false) {
      int8_t retries = FEED_ERROR_RETRIES;
      uint16_t speed = smuffConfig.insertSpeed;
      do {
        // retract the selector distance
        changeFeederInsertSpeed(speed);
        prepSteppingRelMillimeter(FEEDER, -smuffConfig.selectorDistance, true);
        runAndWait(FEEDER);
        // did the Feeder stall?
        stat = handleFeederStall(&speed, &retries);
      } while(!stat && retries > 0);
  }
  return stat;
}

bool unloadFilament() {
  if (toolSelected == -1) {
    signalNoTool();
    return false;
  }
  if(smuffConfig.extControlFeeder && !smuffConfig.isSharedStepper) {
    positionRevolver();
    signalUnloadFilament();
    return true;
  }
  else if(smuffConfig.extControlFeeder && smuffConfig.isSharedStepper) {
    switchFeederStepper(INTERNAL);
  }
  steppers[FEEDER].setStepsTaken(0);
  parserBusy = true;
  if(!steppers[FEEDER].getEnabled())
    steppers[FEEDER].setEnabled(true);

  positionRevolver();

  uint16_t curSpeed = steppers[FEEDER].getMaxSpeed();

  if(smuffConfig.unloadRetract != 0) {
    prepSteppingRelMillimeter(FEEDER, smuffConfig.unloadRetract);
    runAndWait(FEEDER);
    if(smuffConfig.unloadPushback != 0) {
      changeFeederInsertSpeed(smuffConfig.insertSpeed);
      prepSteppingRelMillimeter(FEEDER, smuffConfig.unloadPushback);
      runAndWait(FEEDER);
      delay(smuffConfig.pushbackDelay*1000);
      steppers[FEEDER].setMaxSpeed(curSpeed);
    }
  }
  // invert endstop trigger state for unloading
  bool endstopState = steppers[FEEDER].getEndstopState();
  steppers[FEEDER].setEndstopState(!endstopState);
  // unload until Selector endstop gets released
  int8_t retries = FEED_ERROR_RETRIES;
  do {
    unloadFromNozzle(false);
    if(steppers[FEEDER].getEndstopHit())
      break;
    else
      retries--;
  }while(retries > 0);
  // reset endstop state
  steppers[FEEDER].setEndstopState(endstopState);

  /*
  // just to make sure we've got the right position, feed until the endstop gets hit again
  // then unload from Selector
  float ofs = steppers[FEEDER].getStepPositionMM();
  // __debug(PSTR("Offset: %s"), String(ofs).c_str());
  changeFeederInsertSpeed(smuffConfig.insertSpeed);
  prepSteppingRelMillimeter(FEEDER, smuffConfig.insertLength);
  runAndWait(FEEDER);
  ofs = steppers[FEEDER].getStepPositionMM() - ofs;
  __debug(PSTR("Offset now: %s,  Endstop: %d"), String(ofs).c_str(), feederEndstop());
  */

  unloadFromSelector();

  feederJammed = false;
  steppers[FEEDER].setIgnoreAbort(false);
  steppers[FEEDER].setMaxSpeed(curSpeed);
  steppers[FEEDER].setStepPosition(0);
  steppers[FEEDER].setAbort(false);

  dataStore.stepperPos[FEEDER] = steppers[FEEDER].getStepPosition();
  saveStore();

  steppers[FEEDER].setAbort(false);
  if(smuffConfig.homeAfterFeed) {
    if(smuffConfig.revolverIsServo) {
      setServoLid(SERVO_OPEN);
    }
    else
      steppers[REVOLVER].home();
  }
  if(smuffConfig.extControlFeeder && smuffConfig.isSharedStepper) {
    switchFeederStepper(EXTERNAL);
  }
  parserBusy = false;
  return true;
}

bool nudgeBackFilament() {
  if (toolSelected == -1) {
    return false;
  }
  if(smuffConfig.extControlFeeder && !smuffConfig.isSharedStepper) {
    positionRevolver();
    signalUnloadFilament();
    return true;
  }
  else if(smuffConfig.extControlFeeder && smuffConfig.isSharedStepper) {
    switchFeederStepper(INTERNAL);
  }
  steppers[FEEDER].setStepsTaken(0);
  if(!steppers[FEEDER].getEnabled())
    steppers[FEEDER].setEnabled(true);

  positionRevolver();

  uint16_t curSpeed = steppers[FEEDER].getMaxSpeed();
  changeFeederInsertSpeed(smuffConfig.insertSpeed);
  prepSteppingRelMillimeter(FEEDER, smuffConfig.insertLength);
  runAndWait(FEEDER);
  steppers[FEEDER].setMaxSpeed(curSpeed);

  dataStore.stepperPos[FEEDER] = steppers[FEEDER].getStepPosition();
  saveStore();

  steppers[FEEDER].setAbort(false);
  if(smuffConfig.homeAfterFeed) {
    if(smuffConfig.revolverIsServo) {
      setServoLid(SERVO_OPEN);
    }
    else
      steppers[REVOLVER].home();
  }
  if(smuffConfig.extControlFeeder && smuffConfig.isSharedStepper) {
    switchFeederStepper(EXTERNAL);
  }
  return true;
}

void handleStall(int8_t axis) {
  const char P_StallHandler[] PROGMEM = { "Stall handler: %s" };
  __debug(P_StallHandler,"Triggered on %c-axis", 'X'+axis);
  // check if stall must be handled
  if(steppers[axis].getStopOnStallDetected()) {
    __debug(P_StallHandler, "Stopped on stall");
    // save speed/acceleration settings
    uint16_t maxSpeed = steppers[axis].getMaxSpeed();
    uint16_t accel = steppers[axis].getAcceleration();
    // slow down speed/acceleration for next moves
    steppers[axis].setMaxSpeed(accel*2);
    steppers[axis].setAcceleration(accel*2);
    steppers[axis].setEnabled(false);

    // in order to determine where the stall happens...
    // try to move 5mm to the left
    delay(1000);
    steppers[axis].setEnabled(true);
    prepStepping(axis, 5, true, true);
    remainingSteppersFlag |= _BV(axis);
    runAndWait(-1);
    bool stallLeft = steppers[axis].getStallCount() > (uint32_t)steppers[axis].getStallThreshold();
    __debug(PSTR("Left: %d"), steppers[axis].getStallCount());
    steppers[axis].setEnabled(false);
    delay(1000);
    steppers[axis].setEnabled(true);
    // try to move 5mm to the right
    prepStepping(axis, -5, true, true);
    remainingSteppersFlag |= _BV(axis);
    runAndWait(-1);
    bool stallRight = steppers[axis].getStallCount() > (uint32_t)steppers[axis].getStallThreshold();
    __debug(PSTR("Right: %d"), steppers[axis].getStallCount());

    if(stallLeft && stallRight)   __debug(P_StallHandler, "Stalled center");
    if(stallLeft && !stallRight)  __debug(P_StallHandler, "Stalled left");
    if(!stallLeft && stallRight)  __debug(P_StallHandler, "Stalled right");

    nudgeBackFilament();
    __debug(P_StallHandler,"Feeder nudged back");
    delay(1000);
    if(axis != FEEDER) {    // Feeder can't be homed
      moveHome(axis, false, false);
      __debug(P_StallHandler, "%c-Axis Homed", 'X'+axis);
    }
    else {
      // TODO: add stall handling for Feeder
    }
    // reset speed/acceleration
    steppers[axis].setMaxSpeed(maxSpeed);
    steppers[axis].setAcceleration(accel);
    delay(1000);
  }
}

bool selectTool(int8_t ndx, bool showMessage) {

  char _msg1[256];
  char _tmp[40];

  if(ndx < 0 || ndx >= MAX_TOOLS) {
    if(showMessage) {
      userBeep();
      sprintf_P(_msg1, P_WrongTool, ndx);
      drawUserMessage(_msg1);
    }
    return false;
  }

  ndx = swapTools[ndx];
  if(feederJammed) {
    beep(4);
    sprintf_P(_msg1, P_FeederJammed);
    strcat_P(_msg1, P_Aborting);
    drawUserMessage(_msg1);
    feederJammed = false;
    return false;
  }
  signalSelectorBusy();

  if(toolSelected == ndx) { // tool is the one we already have selected, do nothing
    if(!smuffConfig.extControlFeeder) {
      userBeep();
      sprintf_P(_msg1, P_ToolAlreadySet);
      drawUserMessage(_msg1);
    }
    if(smuffConfig.extControlFeeder) {
      signalSelectorReady();
    }
    return true;
  }
  if(!steppers[SELECTOR].getEnabled())
    steppers[SELECTOR].setEnabled(true);

  if (showMessage) {
    while(feederEndstop()) {
      if (!showFeederLoadedMessage())
        return false;
    }
  }
  else {
    if (!smuffConfig.extControlFeeder && feederEndstop()) {
      unloadFilament();
    }
    else if (smuffConfig.extControlFeeder && feederEndstop()) {
      beep(4);
      if(smuffConfig.sendActionCmds) {
        // send action command to indicate a jam has happend and
        // the controller shall wait
        sprintf_P(_tmp, P_Action, P_ActionWait);
        printResponse(_tmp, 0);
        printResponse(_tmp, 1);
        printResponse(_tmp, 2);
      }
      while(feederEndstop()) {
        moveHome(REVOLVER, false, false);   // home Revolver
        M18("M18", "", 0);   // motors off
        bool stat = showFeederFailedMessage(0);
        if(!stat)
          return false;
        if(smuffConfig.unloadCommand != nullptr && strlen(smuffConfig.unloadCommand) > 0) {
          if(CAN_USE_SERIAL2) {
            Serial2.print(smuffConfig.unloadCommand);
            Serial2.print("\n");
            //__debug(PSTR("Feeder jammed, sent unload command '%s'\n"), smuffConfig.unloadCommand);
          }
        }
      }
      if(smuffConfig.sendActionCmds) {
        // send action command to indicate jam cleared, continue printing
        sprintf_P(_tmp, P_Action, P_ActionCont);
        printResponse(_tmp, 0);
        printResponse(_tmp, 1);
        printResponse(_tmp, 2);
      }
    }
    if(smuffConfig.revolverIsServo) {
      // release servo prior moving the selector
      setServoLid(SERVO_OPEN);
    }
  }
  //__debug(PSTR("Selecting tool: %d"), ndx);
  parserBusy = true;
  drawSelectingMessage(ndx);
  //__debug(PSTR("Message shown"));
  uint16_t speed = steppers[SELECTOR].getMaxSpeed();

  uint8_t retry = 3;
  bool posOk = false;
  do {
    steppers[SELECTOR].resetStallDetected();
    prepSteppingAbsMillimeter(SELECTOR, smuffConfig.firstToolOffset + (ndx * smuffConfig.toolSpacing));
    remainingSteppersFlag |= _BV(SELECTOR);
    #if !defined(SMUFF_V5)
      if(!smuffConfig.resetBeforeFeed) {
        prepSteppingAbs(REVOLVER, smuffConfig.firstRevolverOffset + (ndx *smuffConfig.revolverSpacing), true);
        remainingSteppersFlag |= _BV(REVOLVER);
      }
      runAndWait(-1);
    #else
      runAndWait(SELECTOR);
    #endif
    //__debug(PSTR("Selector in position: %d"), ndx);
    if(smuffConfig.stepperStall[SELECTOR] > 0) {
      //__debug(PSTR("Selector stall count: %d"), steppers[SELECTOR].getStallCount());
    }
    if(steppers[SELECTOR].getStallDetected()) {
        posOk = false;
        handleStall(SELECTOR);
        stallDetectedCountSelector++;
    }
    else
      posOk = true;
    retry--;
    if(!retry)
      break;
  } while(!posOk);
  steppers[SELECTOR].setMaxSpeed(speed);
  if(posOk) {
    toolSelected = ndx;
    dataStore.tool = toolSelected;
    dataStore.stepperPos[SELECTOR] = steppers[SELECTOR].getStepPosition();
    dataStore.stepperPos[REVOLVER] = steppers[REVOLVER].getStepPosition();
    dataStore.stepperPos[FEEDER] = steppers[FEEDER].getStepPosition();
    saveStore();
    //__debug(PSTR("Data stored"));

    if (!smuffConfig.extControlFeeder && showMessage) {
      showFeederLoadMessage();
    }
    if(smuffConfig.extControlFeeder) {
      //__debug(PSTR("Resetting Revolver"));
      resetRevolver();
      signalSelectorReady();
      //__debug(PSTR("Revolver reset done"));
    }
  }
  parserBusy = false;
  //__debug(PSTR("Finished selecting tool"));
  return posOk;
}

void resetRevolver() {
  moveHome(REVOLVER, false, false);
  if (toolSelected >=0 && toolSelected <= smuffConfig.toolCount-1) {
    if(!smuffConfig.revolverIsServo) {
      prepSteppingAbs(REVOLVER, smuffConfig.firstRevolverOffset + (toolSelected*smuffConfig.revolverSpacing), true);
      runAndWait(REVOLVER);
    }
    else {
      //__debug(PSTR("Positioning servo to: %d (CLOSED)"), smuffConfig.revolverOnPos);
      setServoLid(SERVO_CLOSED);
    }
  }
}

void setStepperSteps(int8_t index, long steps, bool ignoreEndstop) {
  // make sure the servo is in off position before the Selector gets moved
  // ... just in case... you never know...
  if(smuffConfig.revolverIsServo && index == SELECTOR) {
    if(lidOpen) {
      //__debug(PSTR("Positioning servo to: %d (OPEN)"), smuffConfig.revolverOffPos);
      setServoLid(SERVO_OPEN);
    }
  }
  if (steps != 0)
    steppers[index].prepareMovement(steps, ignoreEndstop);
}

void prepSteppingAbs(int8_t index, long steps, bool ignoreEndstop) {
  long pos = steppers[index].getStepPosition();
  long _steps = steps - pos;
  //__debug(PSTR("Pos: %ld  New: %ld"), pos, _steps);
  setStepperSteps(index, _steps, ignoreEndstop);
}

void prepSteppingAbsMillimeter(int8_t index, float millimeter, bool ignoreEndstop) {
  uint16_t stepsPerMM = steppers[index].getStepsPerMM();
  long steps = (long)((float)millimeter * stepsPerMM);
  long pos = steppers[index].getStepPosition();
  setStepperSteps(index, steps - pos, ignoreEndstop);
}

void prepSteppingRel(int8_t index, long steps, bool ignoreEndstop) {
  setStepperSteps(index, steps, ignoreEndstop);
}

void prepSteppingRelMillimeter(int8_t index, float millimeter, bool ignoreEndstop) {
  uint16_t stepsPerMM = steppers[index].getStepsPerMM();
  long steps = (long)((float)millimeter * stepsPerMM);
  setStepperSteps(index, steps, ignoreEndstop);
}

void printPeriodicalState(int8_t serial) {
  char tmp[80];

  const char* _triggered = "on";
  const char* _open      = "off";
  sprintf_P(tmp, PSTR("echo: states: T: T%d\tS: %s\tR: %s\tF: %s\tF2: %s\n"),
          toolSelected,
          selectorEndstop()  ? _triggered : _open,
          revolverEndstop()  ? _triggered : _open,
          feederEndstop(1)   ? _triggered : _open,
          feederEndstop(2)   ? _triggered : _open);
  printResponse(tmp, serial);
}

void printDriverMode(int8_t serial) {
  char tmp[128];

  sprintf_P(tmp, P_TMC_StatusAll,
          drivers[SELECTOR] == nullptr ? P_Unknown : drivers[SELECTOR]->stealth() ? P_Stealth : P_Spread,
          drivers[REVOLVER] == nullptr ? P_Unknown : drivers[REVOLVER]->stealth() ? P_Stealth : P_Spread,
          drivers[FEEDER] == nullptr ? P_Unknown : drivers[FEEDER]->stealth() ? P_Stealth : P_Spread);
  printResponse(tmp, serial);
}

void printDriverRms(int8_t serial) {
  char tmp[128];

  sprintf_P(tmp, P_TMC_StatusAll,
          drivers[SELECTOR] == nullptr ? P_Unknown : (String(drivers[SELECTOR]->rms_current()) + String(P_MilliAmp)).c_str(),
          drivers[REVOLVER] == nullptr ? P_Unknown : (String(drivers[REVOLVER]->rms_current()) + String(P_MilliAmp)).c_str(),
          drivers[FEEDER] == nullptr ? P_Unknown : (String(drivers[FEEDER]->rms_current()) + String(P_MilliAmp)).c_str());
  printResponse(tmp, serial);
}

void printDriverStallThrs(int8_t serial) {
  char tmp[128];
  sprintf_P(tmp, P_TMC_StatusAll,
          drivers[SELECTOR] == nullptr ? P_Unknown : String(drivers[SELECTOR]->SGTHRS()).c_str(),
          drivers[REVOLVER] == nullptr ? P_Unknown : String(drivers[REVOLVER]->SGTHRS()).c_str(),
          drivers[FEEDER] == nullptr ? P_Unknown : String(drivers[FEEDER]->SGTHRS()).c_str());
  printResponse(tmp, serial);
}

void printEndstopState(int8_t serial) {
  char tmp[128];
  const char* _triggered = "triggered";
  const char* _open      = "open";
  sprintf_P(tmp, P_TMC_StatusAll,
          selectorEndstop()  ? _triggered : _open,
          revolverEndstop()  ? _triggered : _open,
          feederEndstop()    ? _triggered : _open);
  printResponse(tmp, serial);
  if(Z_END2_PIN != -1) {
    sprintf_P(tmp, PSTR("Z2 (Feeder2): %s\n"), feederEndstop(2) ? _triggered : _open);
    printResponse(tmp, serial);
  }
}

void printSpeeds(int8_t serial) {
  char tmp[150];

  sprintf_P(tmp, !smuffConfig.speedsInMMS ? P_AccelSpeedTicks : P_AccelSpeedMms,
          smuffConfig.maxSpeed[SELECTOR],
          smuffConfig.stepDelay[SELECTOR],
          smuffConfig.maxSpeed[REVOLVER],
          smuffConfig.stepDelay[REVOLVER],
          smuffConfig.maxSpeed[FEEDER],
          smuffConfig.extControlFeeder ? " (Ext.)" : " (Int.)",
          smuffConfig.stepDelay[FEEDER],
          smuffConfig.insertSpeed);
  printResponse(tmp, serial);
}

void printAcceleration(int8_t serial) {
  char tmp[150];

  sprintf_P(tmp, !smuffConfig.speedsInMMS ? P_AccelSpeedTicks : P_AccelSpeedMms,
          smuffConfig.accelSpeed[SELECTOR],
          smuffConfig.stepDelay[SELECTOR],
          smuffConfig.accelSpeed[REVOLVER],
          smuffConfig.stepDelay[REVOLVER],
          smuffConfig.accelSpeed[FEEDER],
          smuffConfig.extControlFeeder ? " (Ext.)" : " (Int.)",
          smuffConfig.stepDelay[FEEDER],
          smuffConfig.insertSpeed);
  printResponse(tmp, serial);
}

void printSpeedAdjust(int8_t serial) {
  char tmp[150];

  sprintf_P(tmp, P_SpeedAdjust,
          String(smuffConfig.speedAdjust[SELECTOR]).c_str(),
          String(smuffConfig.speedAdjust[SELECTOR]).c_str(),
          String(smuffConfig.speedAdjust[REVOLVER]).c_str()
  );
  printResponse(tmp, serial);
}

void printOffsets(int8_t serial) {
  char tmp[128];
  sprintf_P(tmp, P_TMC_StatusAll,
          String((int)(smuffConfig.firstToolOffset*10)).c_str(),
          String(smuffConfig.firstRevolverOffset).c_str(),
          "--",
          "");
  printResponse(tmp, serial);
}

void printPos(int8_t index, int8_t serial) {
  char buf[128];
  sprintf_P(buf, PSTR("Pos. '%s': %ld\n"), steppers[index].getDescriptor(), steppers[index].getStepPosition());
  printResponseP(buf, serial);
}


#if defined(__STM32F1__)
  /*
    Simple wrapper for tone()
  */
  void playTone(int8_t pin, int16_t freq, int16_t duration) {
  #if defined(USE_LEONERD_DISPLAY)
    encoder.playFrequency(freq, duration);
  #else
    if(pin != -1)
      tone(pin, freq, duration);
  #endif
  }

  void muteTone(int8_t pin) {
  #if defined(USE_LEONERD_DISPLAY)
    encoder.muteTone();
  #else
    if(pin != -1)
      pinMode(pin, INPUT);
  #endif
}
#endif

void beep(uint8_t count) {
  //showLed(1, count);
  prepareSequence(tuneBeep, false);
  for (uint8_t i = 0; i < count; i++) {
    playSequence(true);
  }
}

void longBeep(uint8_t count) {
  #if defined(USE_LEONERD_DISPLAY)
    encoder.setLED(LED_RED, true);
  #endif
  prepareSequence(tuneLongBeep, false);
  for (uint8_t i = 0; i < count; i++) {
    playSequence(true);
  }
}

void userBeep() {
  #if defined(USE_LEONERD_DISPLAY)
    encoder.setLED(LED_RED, true);
  #endif
  prepareSequence(tuneUser, false);
  playSequence(true);
}

void encoderBeep(uint8_t count) {
  prepareSequence(tuneEncoder, false);
  playSequence(true);
}

void startupBeep() {
  #if defined(USE_LEONERD_DISPLAY)
    showLed(4, 1);
  #endif
  prepareSequence(tuneStartup, true);
}


/*
  Prepares a tune sequence from string to an array of notes to be played in background.
  The format is: F{frequency} D{duration} [P{pause}].F{frequency}D{duration}[P{pause}]. ...
  Example: "F440D120P80." plays an A (440Hz) with a duration of 120mS and pauses 80mS
           after the tone has played.
           The '.' at the end of a tone is needed to play that tone and must not be omitted.
*/
void prepareSequence(const char* seq, bool autoPlay) {
  #if !defined(USE_LEONERD_DISPLAY)
    if(BEEPER_PIN == -1)
      return;
  #endif
  uint16_t f=0, d=0, p=0;
  uint8_t n=0;
  startSequence = false;
  if(seq == nullptr || *seq == 0)
    return;
  while(*seq) {
    if(*seq=='"' || *seq==' ')
      seq++;
    if(toupper(*seq)=='F') {
      f = atoi(++seq);
    }
    if(toupper(*seq)=='D') {
      d = atoi(++seq);
    }
    if(toupper(*seq)=='P') {
      p = atoi(++seq);
    }
    if(*seq=='.') {
      if(f && d) {
        sequence[n][_F_] = (uint16_t)f;
        sequence[n][_D_] = (uint16_t)d;
        sequence[n][_P_] = (uint16_t)0;
      }
      if(p) {
        sequence[n][_P_] = (uint16_t)p;
      }
      f = d = p = 0;
      if(n < MAX_SEQUENCE-1)
        n++;
      else
        break;
    }
    seq++;
  }
  // mark end-of-sequence
  sequence[n][_F_] = 0;
  sequence[n][_D_] = 0;
  sequence[n][_P_] = 0;
  if(autoPlay)
    playSequence(true);
}

/*
  Start playing in background via timer interrupt
*/
void playSequence(bool inForeground) {
  if(!timerRunning || inForeground) {
    // timers not initialized by now, play in foreground
    for(uint8_t i=0; i < MAX_SEQUENCE; i++) {
      if(sequence[i][_F_] == 0)
        return;
      _tone(sequence[i][_F_], sequence[i][_D_]);
      delay(sequence[i][_D_]);
      if(sequence[i][_P_] > 0)
        delay(sequence[i][_P_]);
    }
    return;
  }
  while(isPlaying)
    delay(10);
  startSequence = true;
}

/*
  Gets called every 1ms from within the general timer to play the
  next tone in sequence if startSequence is set
*/
void playSequenceBackgnd() {
  if(!startSequence) {
    isPlaying = false;
    sequenceCnt = 0;
    return;
  }

  // stop condition: Frequency and Duration == 0
  if(sequence[sequenceCnt][_F_] == 0 && sequence[sequenceCnt][_D_] == 0) {
    startSequence = false;
    isPlaying = false;
    sequenceCnt = 0;
    _noTone();
    return;
  }

  if(isPlaying && sequence[sequenceCnt][_D_] > 0) {
    sequence[sequenceCnt][_D_]  -= 1;
  }
  else {
    if(isPlaying && sequence[sequenceCnt][_P_] > 0) {
      sequence[sequenceCnt][_P_] -= 1;
    }
    else if(isPlaying) {
      _noTone();
      isPlaying = false;
      sequenceCnt++;
    }
    else {
      _tone(sequence[sequenceCnt][_F_], 0);
      isPlaying = true;
    }
  }
}

void setServoMinPwm(int8_t servoNum, uint16_t pwm) {
  if(servoNum == 0) {
    servo.setPulseWidthMin(pwm);
  }
  else if(servoNum == 1) {
    servoLid.setPulseWidthMin(pwm);
  }
}

void setServoMaxPwm(int8_t servoNum, uint16_t pwm) {
  if(servoNum == 0) {
    servo.setPulseWidthMax(pwm);
  }
  else if(servoNum == 1) {
    servoLid.setPulseWidthMax(pwm);
  }
}

bool setServoPos(int8_t servoNum, uint8_t degree) {
  #if defined(MULTISERVO)
    uint16_t pulseLen = map(degree, 0, 180, smuffConfig.servoMinPwm, smuffConfig.servoMaxPwm);
  #endif

  if(servoNum == 0) {
    #if defined(MULTISERVO)
      if(servoMapping[16] != -1) {
        servoPwm.writeMicroseconds(servoMapping[16], pulseLen);
      }
    #else
      servo.write(degree);
      delay(250);
    #endif
    return true;
  }
  else if(servoNum == 1) {
    servoLid.write(degree);
    delay(250);
    return true;
  }
  else if(servoNum >= 10 && servoNum <= 26) {
    #if defined(MULTISERVO)
      uint8_t servo = servoMapping[servoNum-10];
      //__debug(PSTR("Servo mapping: %d -> %d (pulse len: %d ms: %d)"), servoNum-10, servo, pulseLen, ms);
      if(servo != -1)
        servoPwm.writeMicroseconds(servo, pulseLen);
      return true;
    #endif
  }
  return false;
}

bool setServoMS(int8_t servoNum, uint16_t microseconds) {
  if(servoNum == 0) {
    #if defined(MULTISERVO)
      if(servoMapping[16] != -1) {
        servoPwm.writeMicroseconds(servoMapping[16], microseconds);
      }
    #else
      servo.writeMicroseconds(microseconds);
      delay(250);
    #endif
    return true;
  }
  else if(servoNum == 1) {
    #if defined(MULTISERVO)
      uint8_t servo = servoMapping[servoNum-10];
      if(servo != -1)
        servoPwm.writeMicroseconds(servo, microseconds);
    #else
      servoLid.writeMicroseconds(microseconds);
      delay(250);
    #endif
    return true;
  }
  return false;
}

void getStoredData() {
  recoverStore();
  steppers[SELECTOR].setStepPosition(dataStore.stepperPos[SELECTOR]);
  steppers[REVOLVER].setStepPosition(dataStore.stepperPos[REVOLVER]);
  steppers[FEEDER].setStepPosition(dataStore.stepperPos[FEEDER]);
  toolSelected = dataStore.tool;
  //__debug(PSTR("Recovered tool: %d"), toolSelected);
}

void setSignalPort(uint8_t port, bool state) {
  char tmp[40];

  if(!smuffConfig.prusaMMU2 && !smuffConfig.sendPeriodicalStats) {
    sprintf(tmp,"%c%c%s", 0x1b, port, state ? "1" : "0");
    if(CAN_USE_SERIAL1)
      Serial1.write(tmp);
    if(CAN_USE_SERIAL2)
      Serial2.write(tmp);
  }
}

void signalSelectorReady() {
  setSignalPort(SELECTOR_SIGNAL, false);
  //__debug(PSTR("Signalling Selector ready"));
}

void signalSelectorBusy() {
  setSignalPort(SELECTOR_SIGNAL, true);
  //__debug(PSTR("Signalling Selector busy"));
}

void signalLoadFilament() {
  setSignalPort(FEEDER_SIGNAL, true);
  //__debug(PSTR("Signalling load filament"));
}

void signalUnloadFilament() {
  setSignalPort(FEEDER_SIGNAL, false);
  //__debug(PSTR("Signalling unload filament"));
}

void listDir(File root, int8_t numTabs, int8_t serial) {
  char tmp[80];
  while (true) {
    File entry =  root.openNextFile();
    if (!entry)
      break;

    for (int8_t i = 1; i < numTabs; i++) {
      printResponseP(PSTR("\t"), serial);
    }
    printResponse(entry.name(), serial);
    if (entry.isDirectory()) {
      printResponseP(PSTR("/\r\n"), serial);
      //listDir(entry, numTabs + 1, serial);
    }
    else {
      #if defined(__ESP32__)
        sprintf_P(tmp, PSTR("\t\t%u\r\n"), entry.size());
      #else
        sprintf_P(tmp, PSTR("\t\t%lu\r\n"), entry.size());
      #endif
      printResponse(tmp, serial);
    }
    entry.close();
  }
}

bool getFiles(const char* rootFolder PROGMEM, const char* pattern PROGMEM, uint8_t maxFiles, bool cutExtension, char* files) {
  char fname[40];
  char tmp[40];
  uint8_t cnt = 0;
  FsFile file;
  FsFile root;
  SdFat SD;

  if (initSD(false)) {
    root.open(rootFolder, O_READ);
    while (file.openNext(&root, O_READ)) {
      if (!file.isHidden()) {
        file.getName(fname, sizeof(fname));
        //__debug(PSTR("File: %s"), fname);
        String lfn = String(fname);
        if(pattern != nullptr && !lfn.endsWith(pattern)) {
          continue;
        }
        if(pattern != nullptr && cutExtension)
          lfn.replace(pattern,"");
        sprintf_P(tmp, PSTR("%-20s\n"), lfn.c_str());
        strcat(files, tmp);
      }
      file.close();
      if(cnt >= maxFiles)
        break;
    }
    root.close();
    files[strlen(files)-1] = '\0';
    return true;
  }
  return false;
}

/*
  Removes file firmware.bin from root directory in order
  to prevent re-flashing on each reset!
*/
void removeFirmwareBin() {
  if(initSD(false)) {
    SD.remove("firmware.bin");
  }
}

/*
  Reads the next line from the test script and filters unwanted
  characters.
*/
uint8_t getTestLine(FsFile* file, char* line, int maxLen) {
  uint8_t n = 0;
  bool isQuote = false;
  while(1) {
    int c = file->read();
    if(c == -1)
      break;
    if(c == ' ' || c == '\r')
      continue;
    if(c == '\n') {
      *(line+n) = 0;
      break;
    }
    if(c == '"') {
      isQuote = !isQuote;
    }
    *(line+n) = isQuote ? c : toupper(c);
    n++;
    if(n >= maxLen)
      break;
  }
  return n;
}

extern SdFs SD;

void printReport(const char* line, unsigned long loopCnt, unsigned long cmdCnt, long toolChanges, unsigned long secs, unsigned long endstop2Hit[], unsigned long endstop2Miss[]) {
  char report[700];
  char runtm[20], gco[60], err[10], lop[10], cmdc[10], tc[20], stallS[10], stallF[10];
  char etmp[15], ehit[smuffConfig.toolCount*10], emiss[smuffConfig.toolCount*10];

  sprintf_P(gco,    PSTR("%-25s"), line);
  sprintf_P(err,    PSTR("%4lu"), feederErrors);
  sprintf_P(lop,    PSTR("%4lu"), loopCnt);
  sprintf_P(cmdc,   PSTR("%5lu"), cmdCnt);
  sprintf_P(tc,     PSTR("%5ld"), toolChanges);
  sprintf_P(stallS, PSTR("%4lu"), stallDetectedCountSelector);
  sprintf_P(stallF, PSTR("%4lu"), stallDetectedCountFeeder);
  sprintf_P(runtm,  PSTR("%4d:%02d:%02d"), (int)(secs/3600), (int)(secs/60)%60, (int)(secs%60));
  memset(ehit, 0, ArraySize(ehit));
  memset(emiss, 0, ArraySize(emiss));
  for(uint8_t tcnt=0; tcnt< smuffConfig.toolCount; tcnt++) {
    sprintf_P(etmp,   PSTR("%5lu | "), endstop2Hit[tcnt]);
    strcat(ehit, etmp);
    sprintf_P(etmp,   PSTR("%5lu | "), endstop2Miss[tcnt]);
    strcat(emiss, etmp);
  }
  char* lines[10];
  // load report from SD-Card; Can contain 10 lines at max.
  loadReport(PSTR("report"), report, ArraySize(report));
  // format report to be sent to terminal
  uint8_t cnt = splitStringLines(lines, ArraySize(lines), report);
  for(uint8_t n=0; n < cnt; n++) {
    //__debug(PSTR("LN: %s"), lines[n]);
    String s = String(lines[n]);
    s.replace("{TIME}", runtm);
    s.replace("{GCO}",  gco);
    s.replace("{ERR}",  err);
    s.replace("{LOOP}", lop);
    s.replace("{CMDS}", cmdc);
    s.replace("{TC}",   tc);
    s.replace("{STALL}",stallS);
    s.replace("{STALLF}",stallF);
    s.replace("{HIT}", ehit);
    s.replace("{MISS}", emiss);
    s.replace("{ESC:",  "\033[");
    s.replace("f}", "f");
    s.replace("m}", "m");
    s.replace("J}", "J");
    s.replace("H}", "H");
    __log(s.c_str());
  }
}

void testRun(const char* fname) {
  char line[80];
  char msg[80];
  char filename[80];
  FsFile file;
  String gCode;
  unsigned long loopCnt = 1L, cmdCnt = 1L;
  uint8_t tool = 0, lastTool = 0;
  uint8_t mode = 1;
  long toolChanges = 0;
  unsigned long startTime = millis();
  unsigned long endstop2Miss[smuffConfig.toolCount], endstop2Hit[smuffConfig.toolCount];
  int16_t turn;
  uint8_t btn;
  bool isHeld, isClicked;

  debounceButton();

  if(initSD(false)) {
    steppers[REVOLVER].setEnabled(true);
    steppers[SELECTOR].setEnabled(true);
    steppers[FEEDER].setEnabled(true);
    randomSeed(millis());
    sprintf_P(msg, P_RunningTest, fname);
    drawUserMessage(msg);
    delay(1750);
    sprintf(filename, "test/%s.gcode", fname);
    feederErrors = 0;
    __log(PSTR("\033[2J"));   // clear screen on VT100
    for(uint8_t i=0; i<smuffConfig.toolCount; i++) {
      endstop2Hit[i] = 0L;
      endstop2Miss[i] = 0L;
    }

    if(file.open(filename, O_READ)) {
      file.rewind();

      stallDetectedCountFeeder = stallDetectedCountSelector = 0;

      #if defined(USE_LEONERD_DISPLAY)
        encoder.setLED(LED_GREEN, true);
      #endif

      while(1) {
        getInput(&turn, &btn, &isHeld, &isClicked, false);
        if(isHeld || isClicked) {
          break;
        }
        if(turn < 0) {
          if(--mode < 0)
            mode = 3;
        }
        else if(turn > 0) {
          if(++mode > 3)
            mode = 0;
        }
        unsigned long secs = (millis()-startTime)/1000;
        if(getTestLine(&file, line, ArraySize(line)) > 0) {
          if(*line == ';')
            continue;
          gCode = String(line);
          if(gCode.indexOf("{RNDT}") >-1) {
            lastTool = tool;
            uint8_t retry = 5;
            do {
              tool = random(0, smuffConfig.toolCount);
              if(--retry == 0)
                break;
            } while(tool == lastTool);
            gCode.replace("{RNDT}", String(tool));
          }
          parseGcode(gCode, -1);
          //__debug(PSTR("GCode: %s"), gCode.c_str());
          if(*line=='T') {
            tool = strtol(line+1, nullptr, 10);
            toolChanges++;
          }
          if(*line=='C') {
            if(!feederEndstop(2))
              endstop2Hit[tool]++;
            else
              endstop2Miss[tool]++;
          }
          cmdCnt++;
          if(cmdCnt % 10 == 0) {
            mode++;
            if(mode > 3)
              mode = 0;
          }
          printReport(line, loopCnt, cmdCnt, toolChanges, secs, endstop2Hit, endstop2Miss);
        }
        else {
          // restart from begin and increment loop count
          file.rewind();
          //__debug(PSTR("Rewinding"));
          loopCnt++;
        }
        switch(mode) {
          case 3:
            sprintf_P(msg, P_FeederErrors, feederErrors);
            break;
          case 2:
            sprintf_P(msg, P_ToolChanges, toolChanges);
            break;
          case 1:
            sprintf_P(msg, P_CmdLoop, cmdCnt, tool);
            break;
          case 0:
            sprintf_P(msg, P_TestTime, (int)(secs/3600), (int)(secs/60)%60, (int)(secs%60));
            break;
        }
        drawTestrunMessage(loopCnt, msg);
        getInput(&turn, &btn, &isHeld, &isClicked, false);
        if(isHeld || isClicked) {
          break;
        }
      }
      file.close();
      #if defined(USE_LEONERD_DISPLAY)
        encoder.setLED(LED_GREEN, false);
      #endif
    }
    else {
      sprintf_P(msg, P_TestFailed, fname);
      drawUserMessage(msg);
      delay(3000);
    }
  }
}

void listTextFile(const char* filename PROGMEM, int8_t serial) {
  FsFile file;
  char line[80];
  char fname[80];
  char delimiter[] = { "\n" };

  sprintf_P(fname, PSTR("help/%s.txt"), filename);
  if(file.open(fname, O_READ)) {
    while(file.fgets(line, sizeof(line)-1, delimiter) > 0) {
      printResponse(line, serial);
    }
    file.close();
  }
  else {
    sprintf_P(line, P_FileNotFound, filename);
    printResponse(line, serial);
  }
  printResponse(delimiter, serial);
}

/*
void dumpString(String &s) {
  for(uint16_t i=0; i < s.length(); i++) {
    __log(PSTR("%02x "), s.charAt(i));
  }
}
*/

/**
 * Loads a menu from SD-Card.
 *
 * @param filename  the name of the menu file to load
 * @param ordinals  the menu entry ordinals
 * @returns the contents of that file
 */
const char* loadMenu(const char* filename PROGMEM, uint8_t ordinals[]) {
  FsFile file;
  static char menu[700];
  char fname[80];
  char ordinal[10];

  memset(menu, 0, ArraySize(menu));
  memset(ordinals, -1, MAX_MENU_ORDINALS*sizeof(int));
  sprintf_P(fname, PSTR("menus/%s.mnu"), filename);
  if(file.open(fname, O_READ)) {
    uint16_t n = 0;
    uint8_t ln = 1;
    int c;
    while ((c=file.read()) != -1) {
      if(c == '\r')
        continue;
      // check for an separator indicating an ordinal number is following
      if(c == '|') {
        uint8_t on = 0;
        memset(ordinal, 0, ArraySize(ordinal));
        do {
          c = file.read();
          if(isdigit(c)) {
            ordinal[on++] = c;
          }
          if(on >= (int)ArraySize(ordinal))
            break;
        } while (isdigit(c));
        ordinals[ln] = atoi(ordinal);
        //__debug(PSTR("Ordinal found: %s = %d"), ordinal, ordinals[n]);
        continue;
      }
      menu[n] = c;
      if(c == '\n')
        ln++;
      // convert \n-\n to separator char (GS = Group Separator = \035)
      if(n > 2 && menu[n] == '\n' && menu[n-1]=='-' && menu[n-2]=='\n') {
        menu[n-1] = '\035';
      }
      n++;
    }
    file.close();

    if(n == 0) {
      __debug(PSTR("Failed to load menu '%s'"), filename);
    }
    //__debug(PSTR("Menu: '%s' %d lines  %d bytes"), filename, ln, n);
    return menu;
  }
  else {
    __debug(P_FileNotFound, filename);
  }
  return nullptr;
}

const char* loadOptions(const char* filename PROGMEM) {
  FsFile file;
  static char opts[300];
  char fname[80];

  memset(opts, 0, ArraySize(opts));
  sprintf_P(fname, PSTR("options/%s.opt"), filename);
  if(file.open(fname, O_READ)) {
    int16_t n = 0;
    int c;
    while ((c=file.read()) != -1) {
      if(c == '\r')
        continue;
      opts[n] = c;
      n++;
    }
    file.close();
    if(n == 0) {
      __debug(PSTR("Failed to load options '%s'"), filename);
    }
    //__debug(PSTR("Opts: '%s' %d bytes"), filename, n);
    return opts;
  }
  else {
    __debug(P_FileNotFound, filename);
  }
  return nullptr;
}

bool loadReport(const char* filename PROGMEM, char* buffer, uint16_t maxLen) {
  FsFile file;
  char fname[80];

  memset(buffer, 0, maxLen);
  sprintf_P(fname, PSTR("test/%s.txt"), filename);
  if(file.open(fname, O_READ)) {
    int n = file.read(buffer, maxLen-1);
    file.close();
    if(n == 0) {
      __debug(PSTR("Failed to load report '%s'"), filename);
    }
    return true;
  }
  else {
    __debug(P_FileNotFound, filename);
  }
  return false;
}

bool    maintainingMode = false;
int8_t  maintainingTool = -1;

void maintainTool() {
  int8_t newTool;

  while(feederEndstop()) {
    if (!showFeederBlockedMessage())
      return;
  }

  if(maintainingMode) {
    maintainingMode = false;
    if(maintainingTool != -1) {
      selectTool(maintainingTool, false);
    }
    maintainingTool = -1;
  }
  else {
    maintainingMode = true;
    maintainingTool = toolSelected;

    if(toolSelected <= (smuffConfig.toolCount/2)) {
      newTool = toolSelected+2;
    }
    else {
      newTool = toolSelected-2;
    }
    if(newTool >=0 && newTool < smuffConfig.toolCount) {
      selectTool(newTool, false);
    }
  }
}

void blinkLED() {
#if defined(LED_PIN)
  if(LED_PIN != -1)
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
#endif
}

/*
  Translates speeds from mm/s into MCU timer ticks.
  Please notice: This method runs the stepper far slower since it's got to ensure that the
  distance/second has to match.
*/
unsigned long translateSpeed(uint16_t speed, uint8_t axis) {
  if(!smuffConfig.speedsInMMS) {
    return speed;
  }
  uint16_t stepsPerMM = (axis == REVOLVER) ? smuffConfig.stepsPerRevolution/360 : smuffConfig.stepsPerMM[axis];
  uint8_t delay = smuffConfig.stepDelay[axis];
  float correction = smuffConfig.speedAdjust[axis];
  unsigned long freq = F_CPU/STEPPER_PSC;
  unsigned long pulses = speed * stepsPerMM;
  double delayTot = pulses * (delay * 0.000001);
  double timeTot = ((double) pulses / freq) + delayTot;
  // if delay total is more than 1 sec. its impossible to meet speed/s, so set a save value of 40mm/s.
  unsigned long ticks = (unsigned long)((timeTot <= 1) ? (1/(timeTot*correction)) : 500);   // 500 equals round about 40 mm/s
  /*
  char ttl[30], dtl[30];
  dtostrf(timeTot, 12, 6, ttl);
  dtostrf(delayTot, 12, 6, dtl);
  __debug(PSTR("FREQ: %lu  SPEED: %4d  StepsMM: %4d  TICKS: %12lu  TOTAL: %s  PULSES: %6lu   DLY: %s"), freq, speed, stepsPerMM, ticks, ttl, pulses, dtl);
  */
  if(timeTot > 1) {
    __debug(PSTR("Too fast! Slow down speed in mm/s."));
  }
  return ticks;
}

void setMotor(uint8_t pos) {
  #if defined(MOTOR_IN1_PIN) && defined(MOTOR_IN2_PIN)
    if(pos == SERVO_OPEN) {
      // move forward
      digitalWrite(MOTOR_IN1_PIN, HIGH);
      digitalWrite(MOTOR_IN2_PIN, LOW);
    }
    else {
      // move backward
      digitalWrite(MOTOR_IN1_PIN, LOW);
      digitalWrite(MOTOR_IN2_PIN, HIGH);
    }
    delay(smuffConfig.motorOnDelay);
    // stop moving
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, LOW);
  #endif
}

void setServoLid(uint8_t pos) {
  #if !defined(MULTISERVO)
    uint8_t posForTool = servoPosClosed[toolSelected];
    uint8_t p;
    if(posForTool == 0)
      p = (pos == SERVO_OPEN) ? smuffConfig.revolverOffPos : smuffConfig.revolverOnPos;
    else
      p = (pos == SERVO_OPEN) ? smuffConfig.revolverOffPos : posForTool;
    setServoPos(SERVO_LID, p);
  #else
    uint8_t p = (pos == SERVO_OPEN) ? servoPosClosed[toolSelected]-SERVO_CLOSED_OFS : servoPosClosed[toolSelected];
    //__debug(PSTR("Tool%d = %d"), toolSelected, p);
    setServoPos(toolSelected+10, p);
  #endif
  lidOpen = pos == SERVO_OPEN;

  // experimental
  #if defined(MOTOR_IN1_PIN) && defined(MOTOR_IN2_PIN)
    setMotor(pos);
  #endif
}

uint8_t scanI2CDevices(uint8_t *devices) {
  uint8_t cnt = 0;
  Wire.begin();
  memset(devices, 0, ArraySize(devices));
  for(uint8_t address = 1; address < 127; address++)
  {
    Wire.beginTransmission(address);
    uint8 stat = Wire.endTransmission();
    if (stat == SUCCESS) {
      *(devices+cnt) = address;
      cnt++;
      //__debug(PSTR("I2C device found at address 0x%2x"), address);
    }
    delay(3);
  }
  return cnt;
}

extern Stream* debugSerial;
extern Stream* logSerial;

void __debug(const char* fmt, ...) {
  if(debugSerial == nullptr)
    return;
  #if defined(DEBUG)
    char _dbg[512];
    va_list arguments;
    va_start(arguments, fmt);
    vsnprintf_P(_dbg, ArraySize(_dbg)-1, fmt, arguments);
    va_end (arguments);
    debugSerial->print(F("echo: dbg: "));
    debugSerial->println(_dbg);
  #endif
}

void __log(const char* fmt, ...) {
  if(logSerial == nullptr)
    return;
  char _log[1024];
  va_list arguments;
  va_start(arguments, fmt);
  vsnprintf_P(_log, ArraySize(_log)-1, fmt, arguments);
  va_end (arguments);
  logSerial->print(_log);
}
