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
#include "SMuFF.h"
#include "Config.h"
#include "ZTimerLib.h"
#include "ZStepperLib.h"
#include "ZServo.h"
#include "DuetLaserSensor.h"

#ifdef __BRD_I3_MINI
U8G2_ST7565_64128N_F_4W_HW_SPI  display(U8G2_R2, /* cs=*/ DSP_CS_PIN, /* dc=*/ DSP_DC_PIN, /* reset=*/ DSP_RESET_PIN);
#endif
#ifdef __BRD_SKR_MINI
  #ifdef USE_TWI_DISPLAY
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 
  #else
  U8G2_UC1701_MINI12864_1_2ND_4W_HW_SPI display(U8G2_R0, /* cs=*/ DSP_CS_PIN, /* dc=*/ DSP_DC_PIN, /* reset=*/ DSP_RESET_PIN);
  #endif
#endif
#ifdef __BRD_NUCLEO_F103RB
//U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
U8G2_SSD1327_WS_128X128_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#endif

ZStepper                steppers[NUM_STEPPERS];
ZTimer                  stepperTimer;
ZTimer                  encoderTimer;
ZServo                  servo;
ZServo                  servoRevolver;
DuetLaserSensor         duetLS;
ClickEncoder            encoder(ENCODER1_PIN, ENCODER2_PIN, ENCODER_BUTTON_PIN, 4);
//CRGB                    leds[NUM_LEDS];

volatile byte           nextStepperFlag = 0;
volatile byte           remainingSteppersFlag = 0;
volatile unsigned long  lastEncoderButtonTime = 0;
bool                    testMode = false;
int                     toolSelections[MAX_TOOLS]; 
volatile unsigned long  pwrSaveTime;
volatile bool           isPwrSave = false;
volatile bool           showMenu = false; 
volatile bool           lastZEndstopState = false;
volatile bool           lastZEndstop2State = false;
unsigned long           endstopZ2HitCnt = 0;
static unsigned long    lastDisplayRefresh = 0;
volatile unsigned long  generalCounter = 0;

String serialBuffer0, serialBuffer2, serialBuffer9; 
String traceSerial2;

extern int  swapTools[MAX_TOOLS];
extern void setToneTimerChannel(uint8_t ntimer, uint8_t channel);    // defined in tone library

void isrStepperHandler();       // forward declarations ... makes every compiler happy
void isrEncoderHandler();
void refreshStatus(bool withLogo);

#ifdef __STM32F1__
volatile uint32_t *stepper_reg_X = &((PIN_MAP[X_STEP_PIN].gpio_device)->regs->BSRR);
volatile uint32_t *stepper_reg_Y = &((PIN_MAP[Y_STEP_PIN].gpio_device)->regs->BSRR);
volatile uint32_t *stepper_reg_Z = &((PIN_MAP[Z_STEP_PIN].gpio_device)->regs->BSRR);
uint32_t pinMask_X = BIT(PIN_MAP[X_STEP_PIN].gpio_bit);
uint32_t pinMask_Y = BIT(PIN_MAP[Y_STEP_PIN].gpio_bit);
uint32_t pinMask_Z = BIT(PIN_MAP[Z_STEP_PIN].gpio_bit); 

#endif 

void overrideStepX() {
#ifdef __STM32F1__
  *stepper_reg_X = pinMask_X;
  if(smuffConfig.stepDelay_X > 0)
    delayMicroseconds(smuffConfig.stepDelay_X);
  *stepper_reg_X = pinMask_X << 16;
#else
  STEP_HIGH_X
  // if(smuffConfig.delayBetweenPulses) __asm__ volatile ("nop");
  STEP_LOW_X
#endif
}

void overrideStepY() {
#ifdef __STM32F1__
  *stepper_reg_Y = pinMask_Y;
  if(smuffConfig.stepDelay_Y > 0)
    delayMicroseconds(smuffConfig.stepDelay_Y);
  *stepper_reg_Y = pinMask_Y << 16;
#else
  STEP_HIGH_Y
  // if(smuffConfig.delayBetweenPulses) __asm__ volatile ("nop");
  STEP_LOW_Y
#endif
}

void overrideStepZ() {
#ifdef __STM32F1__
  *stepper_reg_Z = pinMask_Z;
  if(smuffConfig.stepDelay_Z > 0)
    delayMicroseconds(smuffConfig.stepDelay_Z);
  *stepper_reg_Z = pinMask_Z << 16;
#else
  STEP_HIGH_Z
  // if(smuffConfig.delayBetweenPulses) __asm__ volatile ("nop");
  STEP_LOW_Z
#endif
}

void endstopYevent() {
  //__debug(PSTR("Endstop Revolver: %d"), steppers[REVOLVER].getStepPosition());
}

void endstopZevent() {
  //__debug(PSTR("Endstop Revolver: %d"), steppers[REVOLVER].getStepPosition());
}

void endstopZ2event() {
}

void duetLSHandler() {
  duetLS.service();
}

/*
  Handles the timer interrupt for the rotary encoder.
  Also serves as a general purpose timer dispatcher since the 
  interrupt occures every millisecond.
*/
unsigned long lastTick;
unsigned gcInterval;
void isrEncoderHandler() {
  encoder.service();
  generalCounter++;
  if(generalCounter % 20 == 0) { // every 20 ms
    // do the servos interrupt routines so we save one timer 
    if(!servo.hasTimer())
      servo.setServo();
    if(!servoRevolver.hasTimer())
      servoRevolver.setServo();
  }
  if(generalCounter % 100 == 0) { // every 100 ms
  }
  if(generalCounter % 250 == 0) { // every 250 ms
  }
  if(generalCounter % 500 == 0) { // every 500 ms
  }
  if(generalCounter % 1000 == 0) { // every 1000 ms
    unsigned long tmp = millis();
    gcInterval = tmp-lastTick;
    lastTick = tmp;
  }
  //duetLSHandler();
}

bool checkDuetEndstop() {
  if(smuffConfig.useDuetLaser) {
    return duetLS.getSwitch();
  }
  return false;
}

void setup() {

#ifdef __STM32F1__
  #ifndef USE_TWI_DISPLAY
  afio_remap(AFIO_REMAP_SPI1);  // remap SPI3 to SPI1 if a "normal" display is being used

  if(DEBUG_OFF_PIN != -1) {
    pinMode(DEBUG_OFF_PIN, INPUT_PULLUP);
    /* ============ WARNING ================
      If debug ports (JTAG) are disabled, you won't 
      be able to program the device via STLink anymore!
      Therefore, before programming the device,
      set Z+ input to ground and reset the device
      to reenable debug ports.
      =====================================*/
    if(digitalRead(DEBUG_OFF_PIN)==HIGH)
      disableDebugPorts();      // disable JTAG if Z+ is unconnected
  }
  #endif
#endif

  delay(1000);
  
  serialBuffer0.reserve(40);
  serialBuffer2.reserve(40);
  traceSerial2.reserve(40);
  
  /* This is a No-Go. Maybe at a later stage.
  FastLED.addLeds<LED_TYPE, NEOPIXEL_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  */

  Serial.begin(57600);        // set fixed baudrate until config file was read
  setupDisplay(); 
  readConfig();
  // special case: 
  // if the baudrate is set to 0, the board is running out of working memory
  if(smuffConfig.serial1Baudrate != 0) { 
    if(smuffConfig.serial1Baudrate != 57600) {
      Serial.end();
      Serial.begin(smuffConfig.serial1Baudrate);
    }
  }
  else {
    writeConfig(&Serial);
    longBeep(3);
    showDialog(P_TitleConfigError, P_ConfigFail1, P_ConfigFail4, P_OkButtonOnly);
  }
#ifdef __STM32F1__
  Serial1.begin(smuffConfig.serial2Baudrate);
  Serial3.begin(smuffConfig.serial2Baudrate);
#else
  Serial2.begin(smuffConfig.serial2Baudrate);
#endif
  //__debug(PSTR("DONE init SERIAL"));

  setupSteppers();
  setupTimers();
  
  servo.attach(SERVO1_PIN, true, 0);
  servo.setMaxCycles(smuffConfig.servoCycles);
  setServoPos(0, 90);
  // This one's quite experimental yet.
  // Trying to replace the Revolver stepper motor with a servo motor
  servoRevolver.attach(SERVO2_PIN, true, 1);  
  servoRevolver.setMaxCycles(smuffConfig.servoCycles);
  setServoPos(1, smuffConfig.revolverOffPos);

  // this call must happen after setupSteppers()
  getStoredData();

  // Duet Laser Sensor is not being used yet because the 
  // measurements are somewhat unreliable. Not sure if it's the 
  // sensor itself or it's just some mechanical issue.
  /*
  if(smuffConfig.useDuetLaser) {
    duetLS.attach(Z_END_PIN);
  }
  */
  
  // Please note: All the PWM pins on the SKR are not working as
  // expected. Maybe it's a common libmaple issue, maybe it's a 
  // STM32 timer related thing or maybe it's the
  // hardware design of the board itself. 
  // Can't tell for sure, need some more investigation.
  if(HEATER0_PIN != -1) {
    pinMode(HEATER0_PIN, OUTPUT);
  }
#ifdef __STM32F1__
  if(HEATBED_PIN != -1) {
    pinMode(HEATBED_PIN, PWM); 
  }
#endif
  
  if(FAN_PIN != -1){
#ifdef __STM32F1__

#define FAN_PWM_FREQ_MS -1  // set to 40 for about 25 kHz
    pinMode(FAN_PIN, PWM);
    // Set a different PWM frequency than default if needed.
    // Notice: The default frequency is good enough is a PWM Fan is being used. 
    // A normal Fan (not PWM driven) will either do full speed or no speed.
    if(FAN_PWM_FREQ_MS != -1) {
      timer_dev *dev = PIN_MAP[FAN_PIN].timer_device;     // determine timer via PIN mapping
      uint8 cc_channel = PIN_MAP[FAN_PIN].timer_channel;  // as well as the channel
      timer_pause(dev);
      uint32 period_cyc = FAN_PWM_FREQ_MS * CYCLES_PER_MICROSECOND;
      uint16 prescaler = (uint16)(period_cyc / ((1 << 16) - 1) + 1);
      uint16 overflow = (uint16)((period_cyc + (prescaler / 2)) / prescaler);
      timer_set_prescaler(dev, prescaler);
      timer_set_reload(dev, overflow);
      timer_generate_update(dev);
      timer_resume(dev);
    }
#else
    pinMode(FAN_PIN, OUTPUT);
#endif      
    if(smuffConfig.fanSpeed >= 0 && smuffConfig.fanSpeed <= 100) {
      analogWrite(FAN_PIN, map(smuffConfig.fanSpeed, 0, 100, 0, 255));    
    }
    //__debug(PSTR("DONE FAN init"));
  }

#ifndef __STM32F1__
  // We can't do Master and Slave on this device. 
  // Slave mode is used for the I2C OLE Display on SKR mini
  if(smuffConfig.i2cAddress != 0) {
    Wire.begin(smuffConfig.i2cAddress);
    Wire.onReceive(wireReceiveEvent);
  }
  //__debug(PSTR("DONE I2C init"));
 #endif
  if(smuffConfig.homeAfterFeed) {
    moveHome(REVOLVER, false, false);
  }
  else {
    resetRevolver();
  }
  //__debug(PSTR("DONE reset Revolver"));
  
  sendStartResponse(0);
  if(smuffConfig.prusaMMU2) {
#ifdef __STM32F1__    
    sendStartResponse(1);
    sendStartResponse(3);
#else
    sendStartResponse(2);
#endif
  }
  
  pwrSaveTime = millis();
  
  initBeep();
  
}

void setupSteppers() {

  steppers[SELECTOR] = ZStepper(SELECTOR, (char*)"Selector", X_STEP_PIN, X_DIR_PIN, X_ENABLE_PIN, smuffConfig.acceleration_X, smuffConfig.maxSpeed_X);
  steppers[SELECTOR].setEndstop(X_END_PIN, smuffConfig.endstopTrigger_X, ZStepper::MIN);
  steppers[SELECTOR].stepFunc = overrideStepX;
  steppers[SELECTOR].setMaxStepCount(smuffConfig.maxSteps_X);
  steppers[SELECTOR].setStepsPerMM(smuffConfig.stepsPerMM_X);
  steppers[SELECTOR].setInvertDir(smuffConfig.invertDir_X);
  steppers[SELECTOR].setMaxHSpeed(smuffConfig.maxSpeedHS_X);
  steppers[SELECTOR].setAccelDistance(smuffConfig.accelDistance_X);
  
  steppers[REVOLVER] = ZStepper(REVOLVER, (char*)"Revolver", Y_STEP_PIN, Y_DIR_PIN, Y_ENABLE_PIN, smuffConfig.acceleration_Y, smuffConfig.maxSpeed_Y);
  steppers[REVOLVER].setEndstop(Y_END_PIN, smuffConfig.endstopTrigger_Y, ZStepper::ORBITAL);
  steppers[REVOLVER].stepFunc = overrideStepY;
  steppers[REVOLVER].setMaxStepCount(smuffConfig.stepsPerRevolution_Y);
  steppers[REVOLVER].setStepsPerDegree(smuffConfig.stepsPerRevolution_Y/360);
  steppers[REVOLVER].endstopFunc = endstopYevent;
  steppers[REVOLVER].setInvertDir(smuffConfig.invertDir_Y);
  steppers[REVOLVER].setMaxHSpeed(smuffConfig.maxSpeedHS_Y);
  steppers[REVOLVER].setAccelDistance(smuffConfig.accelDistance_Y);
  
  steppers[FEEDER] = ZStepper(FEEDER, (char*)"Feeder", Z_STEP_PIN, Z_DIR_PIN, Z_ENABLE_PIN, smuffConfig.acceleration_Z, smuffConfig.maxSpeed_Z);
  /*
  if(smuffConfig.useDuetLaser) {
    steppers[FEEDER].setEndstop(-1, smuffConfig.endstopTrigger_Z, ZStepper::MIN);
    steppers[FEEDER].endstopCheck = checkDuetEndstop;
  }
  else
  */
  steppers[FEEDER].setEndstop(Z_END_PIN, smuffConfig.endstopTrigger_Z, ZStepper::MIN);
  if(Z_END2_PIN != -1)
    steppers[FEEDER].setEndstop(Z_END2_PIN, smuffConfig.endstopTrigger_Z, ZStepper::MIN, 2); // optional; used for testing only
  steppers[FEEDER].stepFunc = overrideStepZ;
  steppers[FEEDER].setStepsPerMM(smuffConfig.stepsPerMM_Z);
  steppers[FEEDER].endstopFunc = endstopZevent;
  steppers[FEEDER].endstop2Func = endstopZ2event;
  steppers[FEEDER].setInvertDir(smuffConfig.invertDir_Z);
  steppers[FEEDER].setMaxHSpeed(smuffConfig.maxSpeedHS_Z);
  steppers[FEEDER].setAccelDistance(smuffConfig.accelDistance_Z);

  for(int i=0; i < NUM_STEPPERS; i++) {
      steppers[i].runAndWaitFunc = runAndWait;
      steppers[i].runNoWaitFunc = runNoWait;
      steppers[i].setEnabled(true);
  }
  //__debug(PSTR("DONE enabling steppers"));

  for(int i=0; i < MAX_TOOLS; i++) {
    swapTools[i] = i;
  }
  //__debug(PSTR("DONE initializing swaps"));
}

void setupTimers() {
#ifdef __BRD_I3_MINI
  // *****
  // Attn: Servo uses TIMER5 if setup to create its own timer 
  // *****
  stepperTimer.setupTimer(ZTimer::ZTIMER4, ZTimer::PRESCALER1);
  encoderTimer.setupTimer(ZTimer::ZTIMER3, ZTimer::PRESCALER256); // round about 1ms on 16MHz CPU
#else
  // *****
  // Attn: 
  //    Servo uses TIMER5 CH1 if it's set up to create its own timer 
  //    Fan uses TIMER8 CH3
  //    Beeper uses TIMER4 CH3
  //    PC9 (Heatbed) uses TIMER1 CH1
  // *****
  stepperTimer.setupTimer(ZTimer::ZTIMER2, 3, 1);
  encoderTimer.setupTimer(ZTimer::ZTIMER1, 1, 1460);  // equals to 1ms on 72MHz CPU (supposed to be 1800 but measured around 1760)
  setToneTimerChannel(4, 3);                          // force TIMER4 / CH3 on STM32F1x for tone library
#endif

  stepperTimer.setupTimerHook(isrStepperHandler);
  encoderTimer.setupTimerHook(isrEncoderHandler);
  encoder.setDoubleClickEnabled(true);
#ifdef __BRD_I3_MINI
  encoderTimer.setNextInterruptInterval(63);    // run encoder timer
#else
  encoderTimer.setNextInterruptInterval(40);    // run encoder timer
#endif
  //__debug(PSTR("DONE setup timers"));
}

void startStepperInterval() {
  unsigned int minDuration = 65535;
  for(int i = 0; i < NUM_STEPPERS; i++) {
    if((_BV(i) & remainingSteppersFlag) && steppers[i].getDuration() < minDuration ) {
      minDuration = steppers[i].getDuration();
    }
  }

  nextStepperFlag = 0;
  for(int i = 0; i < NUM_STEPPERS; i++) {
    if ( (_BV(i) & remainingSteppersFlag) && steppers[i].getDuration() == minDuration )
      nextStepperFlag |= _BV(i);
  }

  if(remainingSteppersFlag == 0) {
    stepperTimer.stopTimer();
    stepperTimer.setOverflow(65534);
  }
  else {
    //__debug(PSTR("minDuration: %d"), minDuration);
    stepperTimer.setNextInterruptInterval(minDuration);
  }
}

void isrStepperHandler() {
  stepperTimer.stopTimer();
  unsigned int tmp = stepperTimer.getOverflow(); 
  stepperTimer.setOverflow(65534);

  for (int i = 0; i < NUM_STEPPERS; i++) {
    if(!(_BV(i) & remainingSteppersFlag))
      continue;

    if(!(nextStepperFlag & _BV(i))) {
      steppers[i].setDuration(steppers[i].getDuration() - tmp);
      continue;
    }
    
    steppers[i].handleISR();
    if(steppers[i].getMovementDone())
      remainingSteppersFlag &= ~_BV(i); 
  }
  //__debug(PSTR("ISR(): %d"), remainingSteppersFlag);
  startStepperInterval();
}

void runNoWait(volatile int index) {
  if(index != -1)
    remainingSteppersFlag |= _BV(index);
  startStepperInterval();
}

void runAndWait(volatile int index) {
  runNoWait(index);
  while(remainingSteppersFlag) {
    checkSerialPending(); // not a really nice solution but needed to check serials for "Abort" command in PMMU mode
#ifdef __STM32F1__
    if(!showMenu)
      refreshStatus(true);
#endif
  }
  //if(index==FEEDER) __debug(PSTR("Fed: %smm"), String(steppers[index].getStepsTakenMM()).c_str());
}

void refreshStatus(bool withLogo) {
  display.firstPage();
  do {
    if(withLogo) 
      drawLogo();
    drawStatus();
  } while(display.nextPage());
  lastDisplayRefresh = millis();
}

void loop() {

  //__debug(PSTR("gcInterval: %ld"), gcInterval);
#ifdef __STM32F1__
  if(Serial1.available()) serialEvent1();
  if(Serial3.available()) serialEvent3();
#endif
  if(feederEndstop() != lastZEndstopState) {
    lastZEndstopState = feederEndstop();
    bool state = feederEndstop();
    setSignalPort(FEEDER_SIGNAL, state);
    delay(200);
    setSignalPort(FEEDER_SIGNAL, !state);
    delay(200);
    setSignalPort(FEEDER_SIGNAL, state);
  }
  if(feederEndstop(2) != lastZEndstop2State) {
    lastZEndstop2State = feederEndstop(2);
    endstopZ2HitCnt++;
    //__debug(PSTR("Endstop Z2: %d"), endstopZ2HitCnt);
  }
  //__debug(PSTR("Mem: %d"), freeMemory());

  checkUserMessage();
  if(!displayingUserMessage) {
    if(!isPwrSave && !showMenu) {
#ifdef __STM32F1__
      if(millis()-lastDisplayRefresh > 250) { // refresh display every 100ms
#else
      if(millis()-lastDisplayRefresh > 500) { // refresh display every 500ms
#endif  
        refreshStatus(true);
      }
    }
  }

  if(!showMenu) {
    int button = encoder.getButton();
    if(button == ClickEncoder::Pressed && isPwrSave) {
      setPwrSave(0);
    }
    else if(button == ClickEncoder::Held) {
      setPwrSave(0);
      showMenu = true;
      char title[] = {"Settings"};
      showSettingsMenu(title);
      showMenu = false;
      debounceButton();
#ifdef __STM32F1__
#endif
    }
    else {
      int turn = encoder.getValue();
      if(turn != 0) {
        if(isPwrSave) {
          setPwrSave(0);
        }
        else {
          displayingUserMessage = false;
          showMenu = true;
          if(turn == -1) {
            showMainMenu();
          }
          else {
            showToolsMenu();
          }
          showMenu = false;
        }
      }
    }
  }
  
  //delay(10);
  if((millis() - pwrSaveTime)/1000 >= (unsigned long)smuffConfig.powerSaveTimeout && !isPwrSave) {
    //PSTR("Power save mode after %d seconds (%d)"), (millis() - pwrSaveTime)/1000, smuffConfig.powerSaveTimeout);
    setPwrSave(1);
  }
  /*
  unsigned duetState = duetLS.getState();
  String s = duetLS.getBits();
  String s1 = duetLS.getStuffBits();
  if(s != "") {
    lastState = duetState;
    //__debug(PSTR("DuetLS S:%x E:%x %s"), duetState, duetLS.getError(), s.c_str()); 
    //__debug(PSTR("DuetLS S:%x E:%x %s"), duetState, duetLS.getError(), s1.c_str()); 
    duetLS.resetBits();
    __debug(PSTR("V: %d SW: %d P:%d E:%x (ST:%x) Q: %d B: %d S: %d"), 
      duetLS.getVersion(), 
      duetLS.getSwitch(), 
      duetLS.getPosition(), 
      duetLS.getError(), 
      duetState, 
      duetLS.getQuality(), 
      duetLS.getBrightness(),
      duetLS.getShutter());
    lastMsg = millis();
  }
  */
}

void setPwrSave(int state) {
  display.setPowerSave(state);
  isPwrSave = state == 1;
  if(!isPwrSave) {
    delay(1200);
    pwrSaveTime = millis();
  }
}

bool checkUserMessage() {
  int button = digitalRead(ENCODER_BUTTON_PIN);
  if(button == LOW && displayingUserMessage) {
    displayingUserMessage = false;
  }
  //__debug(PSTR("%ld"), (millis()-userMessageTime)/1000);
  if(millis()-userMessageTime > USER_MESSAGE_RESET*1000) {
    displayingUserMessage = false;
  }
  return displayingUserMessage;
}


#ifdef __STM32F1__
void serialEventRun() {
  if(Serial.available()) serialEvent();
  if(Serial1.available()) serialEvent1();
  if(Serial2.available()) serialEvent2();
  if(Serial3.available()) serialEvent3();
}
#endif

void checkSerialPending() {
  serialEventRun(); 
}

void resetSerialBuffer(int serial) {
  switch(serial) {
    case 0: serialBuffer0 = ""; break;
    case 1: serialBuffer0 = ""; break;
    case 2: serialBuffer2 = ""; break;
    case 3: serialBuffer2 = ""; break;
    case 9: serialBuffer9 = ""; break;
  }
}

bool isQuote = false;
void filterSerialInput(String& buffer, char in) {
  if(in >= 'a' && in <='z') {
    if(!isQuote)
      in = in - 0x20;
  }
  switch(in) {
    case '\b': 
      {
        buffer = buffer.substring(0, buffer.length()-1);
      }
      break;
    case '\r': 
      break;
    case '"':
      isQuote = !isQuote;
      buffer += in;
      break;
    case ' ':
      if(isQuote)
        buffer += in;
      break;
    default:
      buffer += in;
      break;
  }
}

void serialEvent() {
  while(Serial.available()) {
    char in = (char)Serial.read();
    if (in == '\n') {
      //__debug(PSTR("Received-0: %s"), serialBuffer0.c_str());
      parseGcode(serialBuffer0, 0);
      isQuote = false;
    }
    else {
      filterSerialInput(serialBuffer0, in);
    }
  }
}

void serialEvent2() {
  while(Serial2.available()) {
    char in = (char)Serial2.read();
    if (in == '\n') {
      //__debug(PSTR("Received-2: %s"), serialBuffer2.c_str());
      parseGcode(serialBuffer2, 2);
      isQuote = false;
    }
    else {
      filterSerialInput(serialBuffer2, in);
    }
  }
}

#ifdef __STM32F1__
void serialEvent1() {
  while(Serial1.available()) {
    char in = (char)Serial1.read();
    //Serial1.write(in);
    if (in == '\n') {
      //__debug(PSTR("Received-1: %s"), serialBuffer0.c_str());
      parseGcode(serialBuffer0, 1);
      isQuote = false;
    }
    else {
      filterSerialInput(serialBuffer0, in);
    }
  }
}

void serialEvent3() {
  while(Serial3.available()) {
    char in = (char)Serial3.read();
    //Serial3.write(in);
    if (in == '\n') {
      //__debug(PSTR("Received-3: %s"), serialBuffer2.c_str());
      parseGcode(serialBuffer2, 3);
      isQuote = false;
    }
    else {
      filterSerialInput(serialBuffer2, in);
    }
  }
}
#endif


#ifndef __STM32F1__
void wireReceiveEvent(int numBytes) {
  while (Wire.available()) {
    char in = (char)Wire.read();
    if (in == '\n') {
      parseGcode(serialBuffer9, 9);
    }
    else
        serialBuffer9 += in;
  }
}
#endif

