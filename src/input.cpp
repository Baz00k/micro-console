#include "input.h"

namespace {
struct DebouncedButtonState {
  bool stablePressed = false;
  bool lastReadingPressed = false;
  bool pressedEvent = false;
  bool releasedEvent = false;
  uint32_t lastChangeMs = 0;
  uint32_t pressedSinceMs = 0;
};

DebouncedButtonState buttonStates[BUTTON_COUNT];

void pollButton(DebouncedButtonState &state, bool readingPressed,
                uint32_t nowMs) {
  if (readingPressed != state.lastReadingPressed) {
    state.lastReadingPressed = readingPressed;
    state.lastChangeMs = nowMs;
  }

  if ((nowMs - state.lastChangeMs) < BUTTON_DEBOUNCE_MS) {
    return;
  }

  if (readingPressed == state.stablePressed) {
    return;
  }

  state.stablePressed = readingPressed;
  if (state.stablePressed) {
    state.pressedEvent = true;
    state.pressedSinceMs = nowMs;
  } else {
    state.releasedEvent = true;
  }
}

ButtonInput readButton(DebouncedButtonState &state, uint32_t nowMs) {
  ButtonInput input;
  input.down = state.stablePressed;
  input.pressed = state.pressedEvent;
  input.released = state.releasedEvent;
  input.heldMs = state.stablePressed ? nowMs - state.pressedSinceMs : 0;

  state.pressedEvent = false;
  state.releasedEvent = false;
  return input;
}
} // namespace

void setupInput() {
  for (const ButtonConfig &button : BUTTONS) {
    pinMode(button.pin, INPUT_PULLUP);
  }
}

void pollInput() {
  const uint32_t nowMs = millis();
  pollButton(buttonStates[0], digitalRead(BUTTON_LEFT_PIN) == LOW, nowMs);
  pollButton(buttonStates[1], digitalRead(BUTTON_RIGHT_PIN) == LOW, nowMs);
  pollButton(buttonStates[2], digitalRead(BUTTON_A_PIN) == LOW, nowMs);
  pollButton(buttonStates[3], digitalRead(BUTTON_B_PIN) == LOW, nowMs);
}

void readInput(InputState &input) {
  const uint32_t nowMs = millis();
  input.left = readButton(buttonStates[0], nowMs);
  input.right = readButton(buttonStates[1], nowMs);
  input.a = readButton(buttonStates[2], nowMs);
  input.b = readButton(buttonStates[3], nowMs);
}
