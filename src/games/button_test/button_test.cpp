#include "games/button_test/button_test.h"

namespace {
const char *lastButton = "none";
uint32_t frame = 0;
uint32_t elapsedMs = 0;

void notePressed(const ButtonInput &button, const char *label) {
  if (!button.pressed) {
    return;
  }

  lastButton = label;
  Serial.print("Button pressed: ");
  Serial.println(lastButton);
}

void start(const GameContext &context) {
  (void)context;
  lastButton = "none";
  frame = 0;
  elapsedMs = 0;
}

void update(const InputState &input, const GameContext &context) {
  frame++;
  elapsedMs = context.totalMs;
  notePressed(input.left, "LEFT");
  notePressed(input.right, "RIGHT");
  notePressed(input.a, "A");
  notePressed(input.b, "B");
}

void draw(Adafruit_PCD8544 &display) {
  display.setTextSize(1);
  display.setTextColor(BLACK);

  display.setCursor(0, 0);
  display.print("Button Test");

  display.setCursor(0, 12);
  display.print("Last: ");
  display.print(lastButton);

  display.setCursor(0, 24);
  display.print("Frame: ");
  display.print(frame);

  display.setCursor(0, 36);
  display.print("Time: ");
  display.print(elapsedMs / 1000);
  display.print("s");
}

void stop(const GameContext &context) { (void)context; }
} // namespace

const BundledGame BUTTON_TEST_GAME = {"Button Test", "button", start,
                                      update,        draw,     stop};
