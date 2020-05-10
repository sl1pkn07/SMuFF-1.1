/**
 * SMuFF Firmware
 * Copyright (C) 2019 Technik Gegg
 * Copyright (C) 2020 sL1pKn07
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
 * Pins configuration file for NUCLEO-F103RB biard + CNC Shield board
 */
#pragma once

#define BOARD_INFO          "NUCLEO-F103RB + CNC Shield"
// SELECTOR (X)
#define STEP_HIGH_X         digitalWrite(X_STEP_PIN, HIGH);
#define STEP_LOW_X          digitalWrite(X_STEP_PIN, LOW);
#define X_STEP_PIN          PA10  // (D2)
#define X_DIR_PIN           PB4   // (D5)
#define X_ENABLE_PIN        PA9   // (D8)
#define X_END_PIN           PC7   // (D9) X+/-
// REVOLVER (Y)
#define STEP_HIGH_Y         digitalWrite(Y_STEP_PIN, HIGH);
#define STEP_LOW_Y          digitalWrite(Y_STEP_PIN, LOW);
#define Y_STEP_PIN          PB3   // (D3)
#define Y_DIR_PIN           PB10  // (D6)
#define Y_ENABLE_PIN        PA9   // (D8)
#define Y_END_PIN           PB6   // (D10) Y+/-
// FEEDER (Z)
#define STEP_HIGH_Z         digitalWrite(Z_STEP_PIN, HIGH);
#define STEP_LOW_Z          digitalWrite(Z_STEP_PIN, LOW);
#define Z_STEP_PIN          PB5   // (D4)
#define Z_DIR_PIN           PA8   // (D7)
#define Z_ENABLE_PIN        PA9   // (D8)
#define Z_END_PIN           PA6   // (D12) Z+/-
#define Z_END2_PIN          -1
#define Z_END_DUET_PIN      -1

#define BEEPER_PIN          PA7   // (D11) SPIN_DIR (SD)

#define SERVO1_PIN          PB0   // (A2) COOLANT (COOL)
#define SERVO2_PIN          PC3   // (A0) ABORT
#define FAN_PIN             PA5   // (D13) SPIN_EN (SE)
#define HEATER0_PIN         PC8   // CN10-2
#define HEATBED_PIN         PC9   // CN10-1

#define DSP_SCL             PB8   // (A5) SCL
#define DSP_SDA             PB9   // (A4) SDA

#define ENCODER1_PIN        PA13  // CN7-13
#define ENCODER2_PIN        PA14  // CN7-15
#define ENCODER_BUTTON_PIN  PA15  // CN7-17

#define DEBUG_OFF_PIN       -1    // not needed on TWI display

#define TX3_PIN             PA2   // (D1) TX
#define RX3_PIN             PA3   // (D0) RX

#define TX2_PIN             PC10  // CN7-1
#define RX2_PIN             PC11  // CN7-2

#define SDCS_PIN            PB1   // CN10-24
//#define SDCARD_MOSI_PIN     PB15  // CN10-26
//#define SDCARD_MISO_PIN     PB14  // CN10-28
//#define SDCARD_SCK_PIN      PB13  // CN10-30
