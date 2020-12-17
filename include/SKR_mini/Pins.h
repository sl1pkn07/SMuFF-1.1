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
 * Pins configuration file for SKR mini V1.1 board
 */
#pragma once

#define BOARD_INFO          "SKR mini V1.1"
// SELECTOR (X)
#define STEP_HIGH_X         digitalWrite(X_STEP_PIN, HIGH); 
#define STEP_LOW_X          digitalWrite(X_STEP_PIN, LOW);  
#define X_STEP_PIN          PC6
#define X_DIR_PIN           PC7
#define X_ENABLE_PIN        PB15
#define X_END_PIN           PC2
// REVOLVER (Y)
#define STEP_HIGH_Y         digitalWrite(Y_STEP_PIN, HIGH);
#define STEP_LOW_Y          digitalWrite(Y_STEP_PIN, LOW);
#define Y_STEP_PIN          PB13
#define Y_DIR_PIN           PB14
#define Y_ENABLE_PIN        PB12
#define Y_END_PIN           PC1     // Endstop Y-
// FEEDER (E)
// moved from Z to E because of the pins for 3rd Serial port,
// so don't get confused by the pin names
#define STEP_HIGH_Z         digitalWrite(Z_STEP_PIN, HIGH);
#define STEP_LOW_Z          digitalWrite(Z_STEP_PIN, LOW);
#define Z_STEP_PIN          PC5
#define Z_DIR_PIN           PB0
#define Z_ENABLE_PIN        PC4
#define Z_END_PIN           PC0     // Endstop Z-
#define Z_END2_PIN          PA2     // Endstop X+
#define Z_END_DUET_PIN      Z_END2_PIN     

#define BEEPER_PIN          PC10 

#define DEBUG_PIN           PA1     // Endstop Y+
#define RELAIS_PIN          PC1     // Endstop Y- (Relais for stepper motor switching)

#if !defined(SMUFF_V5)
#define SERVO1_PIN          PA1     // Endstop Y+
#define SERVO2_PIN          PC3     // Endstop Z+
#else
#define SERVO1_PIN          PB13     // Y STEP pin used because of 5V tolerance
#define SERVO2_PIN          PB14     // Y DIR pin 
#endif
#define FAN_PIN             PC8
#define HEATER0_PIN         PA8
#define HEATBED_PIN         PC9

#include "FastLED.h"
_DEFPIN_ARM(PC12, 12, C);           // needed to compensate "Invalid pin specified" while compiling
_DEFPIN_ARM(PB9, 9, B);

#if defined(USE_MINI12864_PANEL_V20)
#define RGB_LED_R_PIN       PB7
#define RGB_LED_G_PIN       PC14
#define RGB_LED_B_PIN       PC15
#elif defined(USE_MINI12864_PANEL_V21)
#define NEOPIXEL_PIN        PB7
#define NUM_LEDS            3       // number of Neopixel LEDS
#elif defined(USE_TWI_DISPLAY)
#define NEOPIXEL_PIN        PB9     // PC12
#define NUM_LEDS            5       // number of Neopixel LEDS
#else
#define NEOPIXEL_PIN        PB9
#define NUM_LEDS            5       // number of Neopixel LEDS
#endif
#define BRIGHTNESS          127
#define LED_TYPE            WS2812B
#define COLOR_ORDER         GRB

#define SDCS_PIN            -1      // use default

#define USB_CONNECT_PIN     -1      // not avail 
#define SD_DETECT_PIN       PA3

#define DSP_SCL             PB6     // By default we run the SMuFF controller display on TWI (I2C)
#define DSP_SDA             PB7

#if defined(USE_ANET_DISPLAY)

#define DSP_CS_PIN          PC14    // CS
#define DSP_DC_PIN          PB7     // CLK
#define ENCODER1_PIN        PC13
#define ENCODER2_PIN        PC15
#define ENCODER_BUTTON_PIN  PB6

#elif defined(USE_MINI12864_PANEL_V21) || defined(USE_MINI12864_PANEL_V20)
#define DSP_CS_PIN          PB6     // CS
#define DSP_DC_PIN          PC12    // CLK
#define DSP_RESET_PIN       PC13 
#define ENCODER1_PIN        PD2
#define ENCODER2_PIN        PB8
#define ENCODER_BUTTON_PIN  PC11
#else

#define DSP_CS_PIN          PB7     // These pins are only valid if a SPI display is being used
#define DSP_DC_PIN          PC15
#define DSP_RESET_PIN       -1 

#ifndef USE_TWI_DISPLAY
#define ENCODER1_PIN        PD2
#define ENCODER2_PIN        PB8
#else
#define ENCODER1_PIN        PC14    // moved over to EXP1 for a more convenient cabeling 
#define ENCODER2_PIN        PC15    // (only possible if TWI display is used)
#endif
#define ENCODER_BUTTON_PIN  PC11
#endif

#define STALL_X_PIN         -1      // 
#define STALL_Y_PIN         -1      // 
#define STALL_Z_PIN         -1      // 

#ifdef USE_TWI_DISPLAY
#define DEBUG_OFF_PIN       -1       // not needed on TWI display
#else
#define DEBUG_OFF_PIN       PC3      // (PC3) Z+ pin - set to GND to re-enable debugging via STLink
#endif

// SERIAL1
#define CAN_USE_SERIAL1     true

#define TX1_PIN             PA9     // TFT header TX
#define RX1_PIN             PA10    // TFT header RX

/* 
 Those pins cannot be used for serial data transfer because they're 
 already in use on the SKR Mini V1.1
*/
// SERIAL2
#define CAN_USE_SERIAL2     false

#define TX2_PIN             PA2     // on SKR Mini already used for X+ endstop (but might be reconfigured)
#define RX2_PIN             PA3     // on SKR Mini already used for SD-Card DATA2

/* 
 Those pins can be used for serial data transfer if the Z-Axis
 is not being used on the SKR Mini V1.1
*/
// SERIAL3
#define CAN_USE_SERIAL3     true

#define TX3_PIN             PB10    // on SKR Mini usually used for Z-Axis STEP
#define RX3_PIN             PB11    // on SKR Mini usually used for Z-Axis DIR