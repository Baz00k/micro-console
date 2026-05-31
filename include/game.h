#pragma once

#include <Adafruit_PCD8544.h>
#include <Arduino.h>

#include "input.h"

struct GameContext {
  uint32_t deltaMs = 0;
  uint32_t totalMs = 0;
  bool (*loadSave)(const char *key, void *data, size_t size) = nullptr;
  bool (*save)(const char *key, const void *data, size_t size) = nullptr;
};

template <typename T> bool loadGameSave(const GameContext &context,
                                        const char *key, T &value) {
  return context.loadSave != nullptr && context.loadSave(key, &value, sizeof(T));
}

template <typename T> bool saveGameSave(const GameContext &context,
                                        const char *key, const T &value) {
  return context.save != nullptr && context.save(key, &value, sizeof(T));
}

struct BundledGame {
  const char *title;
  const char *saveId;
  void (*start)(const GameContext &context);
  void (*update)(const InputState &input, const GameContext &context);
  void (*draw)(Adafruit_PCD8544 &display);
  void (*stop)(const GameContext &context);
};
