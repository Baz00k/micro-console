#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Arduino.h>

#include "bundled_games.h"
#include "console_shell.h"
#include "hardware.h"

Adafruit_PCD8544 display(LCD_CLK_PIN, LCD_DIN_PIN, LCD_DC_PIN, LCD_CS_PIN,
                         LCD_RESET_PIN);

ConsoleShell shell(display, BUNDLED_GAMES, BUNDLED_GAME_COUNT);

void setupDisplay() {
  pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH);

  display.begin();
  display.setContrast(60);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("micro-console boot");

  setupInput();
  setupDisplay();
  shell.begin();
}

void loop() { shell.loop(); }
