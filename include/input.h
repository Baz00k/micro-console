#pragma once

#include <Arduino.h>

#include "hardware.h"

struct ButtonInput {
  bool down = false;
  bool pressed = false;
  bool released = false;
  uint32_t heldMs = 0;
};

struct InputState {
  ButtonInput left;
  ButtonInput right;
  ButtonInput a;
  ButtonInput b;
};

void setupInput();
void pollInput();
void readInput(InputState &input);
