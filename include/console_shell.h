#pragma once

#include <Adafruit_PCD8544.h>
#include <Arduino.h>
#include <Preferences.h>

#include "game.h"

class ConsoleShell {
public:
  ConsoleShell(Adafruit_PCD8544 &display,
               const BundledGame *const *games,
               uint8_t gameCount);

  void begin();
  void loop();

private:
  enum class Mode { Selector, Game };

  void runFrame(uint32_t nowMs);
  GameContext buildGameContext(uint32_t deltaMs, uint32_t nowMs) const;
  void updateSelector();
  void updateGame(const GameContext &context);
  void drawSelector();
  void drawGame();
  void launchSelectedGame(uint32_t nowMs);
  void returnToSelector();

  Adafruit_PCD8544 &display;
  Preferences saveStore;
  const BundledGame *const *games;
  uint8_t gameCount;
  InputState input;
  Mode mode = Mode::Selector;
  uint8_t selectedGame = 0;
  uint8_t activeGame = 0;
  uint32_t lastFrameMs = 0;
  uint32_t gameStartMs = 0;
};
