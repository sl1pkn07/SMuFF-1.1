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

#define VERSION_STRING            "V2.14"
#define PMMU_VERSION              106               // Version number for Prusa MMU2 Emulation mode
#define PMMU_BUILD                372               // Build number for Prusa MMU2 Emulation mode
#define VERSION_DATE              "2020-11-17"
#define CONFIG_FILE               "SMUFF.CFG"
#define MATERIALS_FILE            "MATERIALS.CFG"
#define TMC_CONFIG_FILE           "TMCDRVR.CFG"
#define SERVOMAP_FILE             "SERVOMAP.CFG"
#define DATASTORE_FILE            "EEPROM.DAT"
#define TUNE_FILE                 "TUNE.DAT"
#define BEEP_FILE                 "BEEP.DAT"
#define LONGBEEP_FILE             "LBEEP.DAT"
#define USERBEEP_FILE             "UBEEP.DAT"
#define ENCBEEP_FILE              "EBEEP.DAT"

#define MAX_MATERIAL_LEN          12        // max. length of material names

#define NUM_STEPPERS              3
#define SELECTOR                  0
#define REVOLVER                  1
#define FEEDER                    2

#define MIN_TOOLS                 2
#define MAX_TOOLS                 12

#define DSP_CONTRAST              200
#define MIN_CONTRAST              60
#define MAX_CONTRAST              250

#define I2C_SLAVE_ADDRESS         0x88
#define I2C_DISPLAY_ADDRESS       0x3C      // supposed to be wired by default on OLED (alternative 0x3D)
#define I2C_ENCODER_ADDRESS       0x3D      // default address for the LeoNerd Encoder
#define I2C_SERVOCTL_ADDRESS      0x40      // default address for multi servo controller
#define I2C_SERVOBCAST_ADDRESS    0x70      // default address for multi servo controller (Broadcast Address)

#define SERVO_WIPER               0
#define SERVO_LID                 1

#define SERVO_CLOSED_OFS          35        // for Multiservo

#define FEED_ERROR_RETRIES  4

#define REMOTE_NONE         0
#define REMOTE_UP           1
#define REMOTE_DOWN         2
#define REMOTE_SELECT       3
#define REMOTE_ESCAPE       4
#define REMOTE_HOME         5
#define REMOTE_END          6
#define REMOTE_PGUP         7
#define REMOTE_PGDN         8

#if defined(__STM32F1__)
  //#define STEPPER_PSC           9         // 8MHz on STM32 (72MHz MCU)
  #define STEPPER_PSC             3         // 24MHz on STM32 (72MHz MCU)
#elif defined(__ESP32__)
  #define STEPPER_PSC             10        // 8MHz on ESP32 (80MHz MCU)
#else
  #define STEPPER_PSC             2         // 8MHz on AVR (16MHz MCU)
#endif
#define MAX_POWER                 2000      // maximum allowed power for rms_current()
#define MAX_STALL_COUNT           100       // maximum stall counter for stepper
#define MAX_MMS                   700       // maximum mm/s for in menus
#define MAX_TICKS                 32000     // maximum ticks in menus
#define INC_MMS                   5         // speed increment for mm/s
#define INC_TICKS                 50        // speed increment for ticks
#define MAX_MENU_ORDINALS         40

#include "Pins.h"                           // path is defined in build environment of platformio.ini (-I)

#define FIRST_TOOL_OFFSET         1.2       // value in millimeter
#define TOOL_SPACING              21.0      // value im millimeter
#define FIRST_REVOLVER_OFFSET     320       // value in steps
#define REVOLVER_SPACING          320       // value im steps
#define USER_MESSAGE_RESET        15        // value in seconds
#define MAX_LINES                 5
#define MAX_LINE_LENGTH           80
#define POWER_SAVE_TIMEOUT        15        // value in seconds

#if !defined(NUM_LEDS)
  #define NUM_LEDS                1         // number of Neopixel LEDS
  #define BRIGHTNESS              64
  #define LED_TYPE                WS2812B
  #define COLOR_ORDER             GRB
#endif
#define LED_BLACK_COLOR           0         // color codes for RGB LEDs
#define LED_RED_COLOR             1
#define LED_GREEN_COLOR           2
#define LED_BLUE_COLOR            3
#define LED_CYAN_COLOR            4
#define LED_MAGENTA_COLOR         5
#define LED_YELLOW_COLOR          6
#define LED_WHITE_COLOR           7

#define BASE_FONT                 u8g2_font_6x12_t_symbols
#define BASE_FONT_BIG             u8g2_font_7x14_tf
#define SMALL_FONT                u8g2_font_6x10_tr
#define STATUS_FONT               BASE_FONT_BIG
#define LOGO_FONT                 BASE_FONT
#define ICONIC_FONT               u8g2_font_open_iconic_check_2x_t
#define ICONIC_FONT2              u8g2_font_open_iconic_embedded_2x
//#define SYMBOL_FONT             u8g2_font_unifont_t_symbols
