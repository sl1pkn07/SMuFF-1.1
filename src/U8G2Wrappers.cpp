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
 * Wrapper functions for U8G2 library
 */

#include <Arduino.h>
#include "SMuFF.h"
#include "InputDialogs.h"

#define MY_BORDER_SIZE 1
#define SEPERATOR_MARGIN 3
#define SPACE_BETWEEN_BUTTONS_IN_PIXEL 6
#define SPACE_BETWEEN_TEXT_AND_BUTTONS_IN_PIXEL 3

extern "C" uint8_t u8g2_draw_button_line(u8g2_t *u8g2, u8g2_uint_t y, u8g2_uint_t w, uint8_t cursor, const char *s);

extern "C"
{

  uint8_t u8x8_GetMenuEvent(u8x8_t *u8x8)
  {
    uint8_t stat = 0;
    int16_t turn;
    uint8_t button;
    bool isHeld, isClicked;

    getInput(&turn, &button, &isHeld, &isClicked);
    if (button)
    {
      if (isHeld)
      {
        stat = U8X8_MSG_GPIO_MENU_HOME;
      }
      else
      {
        debounceButton();
        stat = U8X8_MSG_GPIO_MENU_SELECT;
      }
      if (remoteKey != REMOTE_HOME)
        remoteKey = REMOTE_NONE;
      resetAutoClose();
    }
    else
    {
      if (remoteKey != REMOTE_HOME)
        remoteKey = REMOTE_NONE;
      if (turn != 0)
      {
        resetAutoClose();
        switch (turn)
        {
        case 1:
          stat = U8X8_MSG_GPIO_MENU_NEXT;
          break;
        case -1:
          stat = U8X8_MSG_GPIO_MENU_PREV;
          break;
        }
      }
    }
    u8x8->debounce_state = button;
    if (!isWarning)
    {
      if (checkAutoClose())
      {
        stat = U8X8_MSG_GPIO_MENU_HOME;
      }
    }
    return stat;
  }

#if defined(__STM32F1__)
  #if !defined(USE_TWI_DISPLAY) && !defined(USE_LEONERD_DISPLAY) && !defined(__BRD_FYSETC_AIOII)
    SPIClass SPI_3(3);
    /* =========================================
    ATTENTION:
    The following section does a rewrite of the U8G2 library function

    extern "C" uint8_t u8x8_byte_arduino_2nd_hw_spi(...)

    This is a must for the SKR mini V1.1 in order to route
    all display handling to SPI3 instead of SPI2 since the
    controller is wired as such.

    If your NOT using an SKR mini V1.1 but any other controller
    board based on the STM32 AND this controller uses SPI2 for
    data exchange, you may comment out this whole section and the
    standard U8G2 library will do its work as expected.
    =========================================== */
      uint8_t __wrap_u8x8_byte_arduino_2nd_hw_spi(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr)
      {
        uint8_t *data;
        uint8_t internal_spi_mode;

        //__debug("x");
        switch (msg)
        {
        case U8X8_MSG_BYTE_SEND:

          // 1.6.5 offers a block transfer, but the problem is, that the
          // buffer is overwritten with the incoming data
          // so it can not be used...
          // SPI.transfer((uint8_t *)arg_ptr, arg_int);

          data = (uint8_t *)arg_ptr;
          while (arg_int > 0)
          {
            SPI_3.transfer((uint8_t)*data);
            data++;
            arg_int--;
          }

          break;
        case U8X8_MSG_BYTE_INIT:
          if (u8x8->bus_clock == 0) /* issue 769 */
            u8x8->bus_clock = u8x8->display_info->sck_clock_hz;
          /* disable chipselect */
          u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
          /* no wait required here */

          /* for SPI1: setup correct level of the clock signal */
          // removed, use SPI.begin() instead: pinMode(11, OUTPUT);
          // removed, use SPI.begin() instead: pinMode(13, OUTPUT);
          // removed, use SPI.begin() instead: digitalWrite(13, u8x8_GetSPIClockPhase(u8x8));

          /* setup hardware with SPI.begin() instead of previous digitalWrite() and pinMode() calls */
          SPI_3.begin();

          break;

        case U8X8_MSG_BYTE_SET_DC:
          u8x8_gpio_SetDC(u8x8, arg_int);
          break;

        case U8X8_MSG_BYTE_START_TRANSFER:
          /* SPI1 mode has to be mapped to the mode of the current controller, at least Uno, Due, 101 have different SPI_MODEx values */
          internal_spi_mode = 0;
          switch (u8x8->display_info->spi_mode)
          {
          case 0:
            internal_spi_mode = SPI_MODE0;
            break;
          case 1:
            internal_spi_mode = SPI_MODE1;
            break;
          case 2:
            internal_spi_mode = SPI_MODE2;
            break;
          case 3:
            internal_spi_mode = SPI_MODE3;
            break;
          }

          #if ARDUINO >= 10600
            SPI_3.beginTransaction(SPISettings(u8x8->bus_clock, MSBFIRST, internal_spi_mode));
          #else
            SPI_3.begin();

            if (u8x8->display_info->sck_pulse_width_ns < 70)
              SPI3.setClockDivider(SPI_CLOCK_DIV2);
            else if (u8x8->display_info->sck_pulse_width_ns < 140)
              SPI3.setClockDivider(SPI_CLOCK_DIV4);
            else
              SPI3.setClockDivider(SPI_CLOCK_DIV8);
              SPI3.setDataMode(internal_spi_mode);
              SPI3.setBitOrder(MSBFIRST);
          #endif

          u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);
          u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, nullptr);
          break;

        case U8X8_MSG_BYTE_END_TRANSFER:
          u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, nullptr);
          u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);

          #if ARDUINO >= 10600
            SPI_3.endTransaction();
          #else
            SPI_3.end();
          #endif

          break;
        default:
          return 0;
        }

        return 1;
      }
    #endif
  #endif

  void DrawUTF8Line(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, const char *s, uint8_t border_size, uint8_t is_invert)
  {
    u8g2_uint_t fx, fy, fw, fh;

    /* only horizontal strings are supported, so force this here */
    u8g2_SetFontDirection(u8g2, 0);

    /* revert y position back to baseline ref */
    y += u8g2->font_calc_vref(u8g2);

    /* caluclate text box */
    fx = x;
    fy = y - u8g2_GetAscent(u8g2);
    fw = w;
    fh = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2);

    //__debug(PSTR("X: %d  Y: %d  W: %d  H:%d  %s"), fx, fy, fw, fh, s);
    /* draw the box, if inverted */
    u8g2_SetDrawColor(u8g2, 1);
    if (is_invert)
    {
      u8g2_DrawBox(u8g2, fx, fy, fw, fh);
    }

    /* draw the frame */
    while (border_size > 0)
    {
      fx--;
      fy--;
      fw += 2;
      fh += 2;
      u8g2_DrawFrame(u8g2, fx, fy, fw, fh);
      border_size--;
    }

    if (is_invert)
    {
      u8g2_SetDrawColor(u8g2, 0);
    }
    else
    {
      u8g2_SetDrawColor(u8g2, 1);
    }

    /* draw the text */
    u8g2_DrawUTF8(u8g2, x, y, s);

    /* revert draw color */
    u8g2_SetDrawColor(u8g2, 1);
  }

  /*
    New function which takes care of separator characters (GS = 0x1d)
  */
  static u8g2_uint_t draw_selection_list_line(u8g2_t *u8g2, u8sl_t *u8sl, u8g2_uint_t y, uint8_t idx, const char *s)
  {
    u8g2_uint_t yy;
    uint8_t border_size = 0;
    uint8_t is_invert = 0;

    u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) + MY_BORDER_SIZE;

    /* calculate offset from display upper border */
    yy = idx;
    yy -= u8sl->first_pos;
    yy *= line_height;
    yy += y;

    /* check whether this is the current cursor line */
    if (idx == u8sl->current_pos)
    {
      border_size = MY_BORDER_SIZE;
      is_invert = 1;
    }

    /* get the line from the array */
    s = u8x8_GetStringLineStart(idx, s);

    /* first char is GS (Group Separator) */
    if (*s == 0x1d)
    {
      yy = y - (line_height / 2) + 1;
      u8g2_DrawHLine(u8g2, SEPERATOR_MARGIN, yy, u8g2_GetDisplayWidth(u8g2) - (SEPERATOR_MARGIN * 2));
      u8g2_DrawHLine(u8g2, SEPERATOR_MARGIN, yy + 1, u8g2_GetDisplayWidth(u8g2) - (SEPERATOR_MARGIN * 2));
      return line_height;
    }

    /* draw the line */
    if (s == nullptr)
      s = "";
    char s2[256];
    char *eol;
    // look out for end of line or end of string
    if ((eol = strchr(s, '\n')) != nullptr || (eol = strchr(s, '\0')) != nullptr)
    {
      int16_t l = eol - s;
      strncpy(s2, s, l);
      *(s2 + l) = 0;
    }
    char *tab = strrchr(s2, '\t');
    // if a delimiter (tab) is found, the string to the left is drawn left aligned
    // and the string to the right right aligned (to display dimensions)
    if (tab != nullptr)
    {
      *(tab) = 0;
      tab++;
      int16_t w = u8g2_GetUTF8Width(u8g2, tab);
      int16_t x = u8g2_GetDisplayWidth(u8g2) - w - MY_BORDER_SIZE;
      int16_t l = x - 1;
      DrawUTF8Line(u8g2, x, y, w, tab, border_size, is_invert);
      DrawUTF8Line(u8g2, MY_BORDER_SIZE, y, l - MY_BORDER_SIZE, s2, border_size, is_invert);
    }
    else
    {
      u8g2_DrawUTF8Line(u8g2, MY_BORDER_SIZE, y, u8g2_GetDisplayWidth(u8g2) - 2 * MY_BORDER_SIZE, s, border_size, is_invert);
    }
    return line_height;
  }

  void drawSelectionList(u8g2_t *u8g2, u8sl_t *u8sl, u8g2_uint_t y, const char *s)
  {
    uint8_t i;
    for (i = 0; i < u8sl->visible; i++)
    {
      y += draw_selection_list_line(u8g2, u8sl, y, i + u8sl->first_pos, s);
    }
  }

  /*
    Wrapper for U8G2 menus (interfaceSelectionList)
    Purpose is to draw seperator lines (if defined) and skip those on encoder/button inputs
  */
  uint8_t __wrap_u8g2_UserInterfaceSelectionList(u8g2_t *u8g2, const char *title, uint8_t start_pos, const char *sl)
  {
    u8sl_t u8sl;
    u8g2_uint_t yy;

    uint8_t event;

    u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) + MY_BORDER_SIZE;

    uint8_t title_lines = u8x8_GetStringLineCnt(title);
    uint8_t display_lines;

    //__debug(PSTR("__wrap_u8g2_UserInterfaceSelectionList"));

    if (start_pos > 0) /* issue 112 */
      start_pos--;   /* issue 112 */

    if (title_lines > 0)
    {
      display_lines = (u8g2_GetDisplayHeight(u8g2) - 3) / line_height;
      u8sl.visible = display_lines;
      u8sl.visible -= title_lines;
    }
    else
    {
      display_lines = u8g2_GetDisplayHeight(u8g2) / line_height;
      u8sl.visible = display_lines;
    }

    u8sl.total = u8x8_GetStringLineCnt(sl);
    u8sl.first_pos = 0;
    u8sl.current_pos = start_pos;

    if (u8sl.current_pos >= u8sl.total)
      u8sl.current_pos = u8sl.total - 1;
    if (u8sl.first_pos + u8sl.visible <= u8sl.current_pos)
      u8sl.first_pos = u8sl.current_pos - u8sl.visible + 1;

    u8g2_SetFontPosBaseline(u8g2);

    for (;;)
    {
      u8g2_FirstPage(u8g2);
      do
      {
        yy = u8g2_GetAscent(u8g2);
        if (title_lines > 0)
        {
          yy += u8g2_DrawUTF8Lines(u8g2, 0, yy, u8g2_GetDisplayWidth(u8g2), line_height, title);
          u8g2_DrawHLine(u8g2, 0, yy - line_height - u8g2_GetDescent(u8g2) + 1, u8g2_GetDisplayWidth(u8g2));
          yy += 3;
        }
        drawSelectionList(u8g2, &u8sl, yy, sl);
      } while (u8g2_NextPage(u8g2));

      #if defined(U8G2_REF_MAN_PIC)
        return 0;
      #endif

      for (;;)
      {
        event = u8x8_GetMenuEvent(u8g2_GetU8x8(u8g2));
        if (event == U8X8_MSG_GPIO_MENU_SELECT)
          return u8sl.current_pos + 1; /* +1, issue 112 */
        else if (event == U8X8_MSG_GPIO_MENU_HOME)
          return 0; /* issue 112: return 0 instead of start_pos */
        else if (event == U8X8_MSG_GPIO_MENU_NEXT || event == U8X8_MSG_GPIO_MENU_DOWN)
        {
          u8sl_Next(&u8sl);
          // if line is GS (Group Separator), skip line
          if (*u8x8_GetStringLineStart(u8sl.current_pos, sl) == 0x1d)
            u8sl_Next(&u8sl);
          break;
        }
        else if (event == U8X8_MSG_GPIO_MENU_PREV || event == U8X8_MSG_GPIO_MENU_UP)
        {
          u8sl_Prev(&u8sl);
          // if line is GS (Group Separator), skip line
          if (*u8x8_GetStringLineStart(u8sl.current_pos, sl) == 0x1d)
            u8sl_Prev(&u8sl);
          break;
        }
      }
    }
  }
}
