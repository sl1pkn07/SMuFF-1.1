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
#define Y_END_PIN           PC1
// FEEDER (E)
// moved from Z to E because of the pins for 2nd Serial port
#define STEP_HIGH_Z         digitalWrite(Z_STEP_PIN, HIGH);
#define STEP_LOW_Z          digitalWrite(Z_STEP_PIN, LOW);
#define Z_STEP_PIN          PC5
#define Z_DIR_PIN           PB0
#define Z_ENABLE_PIN        PC4
#define Z_END_PIN           PC0
#define Z_END2_PIN          PA2
#define Z_END_DUET_PIN      PC3 // for testing only

#define BEEPER_PIN          PC10 

//#define SERVO1_PIN          PB1     // THB - Not usable. See SKR mini schematics
//#define SERVO2_PIN          PA0     // TH0 - Not usable. See SKR mini schematics

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

//#define NEOPIXEL_PIN        46      // equals to PB9 (see definitions _FL_DEFPIN(...) in fastpin_arm_stm32.h)

#if defined(USE_MINI12864_PANEL_V21)
#define NEOPIXEL_PIN        PB7
#define NUM_LEDS            3       // number of Neopixel LEDS
#elif defined(USE_TWI_DISPLAY)
#define NEOPIXEL_PIN        PC12    // if compiling fails stating "Invalid pin specified" add this line "_DEFPIN_ARM(PC12, 12, C);" in fastpin_arm_stm32.h
#define NUM_LEDS            1       // number of Neopixel LEDS
#else
#define NEOPIXEL_PIN        PB9
#define NUM_LEDS            5       // number of Neopixel LEDS
#endif
#define BRIGHTNESS          127
#define LED_TYPE            WS2812B
#define COLOR_ORDER         GRB

#define SDCS_PIN            -1      // use default

#define DSP_SCL             PB6     // By default we run the SMuFF controller display on TWI (I2C)
#define DSP_SDA             PB7

#if defined(USE_ANET_DISPLAY)

#define DSP_CS_PIN          PC14    // CS
#define DSP_DC_PIN          PB7     // CLK
#define ENCODER1_PIN        PC13
#define ENCODER2_PIN        PC15
#define ENCODER_BUTTON_PIN  PB6

#elif defined(USE_MINI12864_PANEL_V21)
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


#ifdef USE_TWI_DISPLAY
#define DEBUG_OFF_PIN       -1       // not needed on TWI display
#else
#define DEBUG_OFF_PIN       PC3      // (PC3) Z+ pin - set to GND to re-enable debugging via STLink
#endif

#define TX3_PIN             PB10    // on SKR Mini usually used for Z-Axis STEP
#define RX3_PIN             PB11    // on SKR Mini usually used for Z-Axis DIR

/* 
 Those pins cannot be used for serial data transfer because they're 
 already in use on the SKR Mini V1.1
*/
#define TX2_PIN             PA2     // on SKR Mini already used for X+ endstop (but might be reconfigured)
#define RX2_PIN             PA3     // on SKR Mini already used for SD-Card DATA2

