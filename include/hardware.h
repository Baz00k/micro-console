#pragma once

#include <Arduino.h>

// Nokia 5110 / PCD8544 display pins.
// Display pin names vary by module:
// RST -> LCD_RESET_PIN
// CE/CS/SCE -> LCD_CS_PIN
// DC/D-C -> LCD_DC_PIN
// DIN/DN/MOSI -> LCD_DIN_PIN
// CLK/SCLK -> LCD_CLK_PIN
// LIGHT/BL -> LCD_BACKLIGHT_PIN
constexpr uint8_t LCD_CLK_PIN = 4;
constexpr uint8_t LCD_DIN_PIN = 6;
constexpr uint8_t LCD_DC_PIN = 7;
constexpr uint8_t LCD_CS_PIN = 21;
constexpr uint8_t LCD_RESET_PIN = 3;
constexpr uint8_t LCD_BACKLIGHT_PIN = 5;

// Four tactile buttons wired from pin to GND. Internal pullups keep idle HIGH.
constexpr uint8_t BUTTON_LEFT_PIN = 20;
constexpr uint8_t BUTTON_RIGHT_PIN = 8;
constexpr uint8_t BUTTON_A_PIN = 10;
constexpr uint8_t BUTTON_B_PIN = 9;

constexpr uint8_t BUTTON_COUNT = 4;
constexpr uint16_t BUTTON_DEBOUNCE_MS = 10;

struct ButtonConfig {
  const char *label;
  uint8_t pin;
};

constexpr ButtonConfig BUTTONS[BUTTON_COUNT] = {
    {"LEFT", BUTTON_LEFT_PIN},
    {"RIGHT", BUTTON_RIGHT_PIN},
    {"A", BUTTON_A_PIN},
    {"B", BUTTON_B_PIN},
};
