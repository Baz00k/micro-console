#pragma once

#include <Adafruit_PCD8544.h>
#include <Arduino.h>

#include "input.h"

struct GameContext {
  uint32_t deltaMs = 0;
  uint32_t totalMs = 0;
};

struct BundledGame {
  const char *title;
  void (*start)();
  void (*update)(const InputState &input, const GameContext &context);
  void (*draw)(Adafruit_PCD8544 &display);
  void (*stop)();
};
