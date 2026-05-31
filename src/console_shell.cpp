#include "console_shell.h"

namespace {
constexpr uint16_t TARGET_FRAME_MS = 50;
constexpr uint16_t MAX_DELTA_MS = 100;
constexpr uint16_t RETURN_TO_SHELL_HOLD_MS = 1000;

Preferences *activeSaveStore = nullptr;

bool loadActiveGameSave(const char *key, void *data, size_t size) {
  if (activeSaveStore == nullptr || key == nullptr || data == nullptr) {
    return false;
  }

  return activeSaveStore->getBytesLength(key) == size &&
         activeSaveStore->getBytes(key, data, size) == size;
}

bool saveActiveGameSave(const char *key, const void *data, size_t size) {
  if (activeSaveStore == nullptr || key == nullptr || data == nullptr) {
    return false;
  }

  return activeSaveStore->putBytes(key, data, size) == size;
}
} // namespace

ConsoleShell::ConsoleShell(Adafruit_PCD8544 &display,
                           const BundledGame *const *games, uint8_t gameCount)
    : display(display), games(games), gameCount(gameCount) {}

void ConsoleShell::begin() {
  lastFrameMs = millis();
  display.clearDisplay();
  drawSelector();
  display.display();
}

void ConsoleShell::loop() {
  const uint32_t nowMs = millis();
  pollInput();

  if ((nowMs - lastFrameMs) < TARGET_FRAME_MS) {
    delay(1);
    return;
  }

  runFrame(nowMs);
}

void ConsoleShell::runFrame(uint32_t nowMs) {
  const uint32_t elapsedMs = nowMs - lastFrameMs;
  lastFrameMs = nowMs;

  readInput(input);

  display.clearDisplay();
  if (mode == Mode::Selector) {
    updateSelector();
    if (mode == Mode::Selector) {
      drawSelector();
    } else {
      drawGame();
    }
  } else {
    const GameContext context = buildGameContext(elapsedMs, nowMs);
    updateGame(context);
    if (mode == Mode::Game) {
      drawGame();
    } else {
      drawSelector();
    }
  }
  display.display();
}

GameContext ConsoleShell::buildGameContext(uint32_t deltaMs,
                                           uint32_t nowMs) const {
  return GameContext{min(deltaMs, static_cast<uint32_t>(MAX_DELTA_MS)),
                     nowMs - gameStartMs, loadActiveGameSave,
                     saveActiveGameSave};
}

void ConsoleShell::updateSelector() {
  if (gameCount == 0) {
    return;
  }

  if (input.left.pressed) {
    selectedGame = selectedGame == 0 ? gameCount - 1 : selectedGame - 1;
  }

  if (input.right.pressed) {
    selectedGame = (selectedGame + 1) % gameCount;
  }

  if (input.a.pressed) {
    launchSelectedGame(millis());
  }
}

void ConsoleShell::updateGame(const GameContext &context) {
  if (input.b.heldMs >= RETURN_TO_SHELL_HOLD_MS) {
    returnToSelector();
    return;
  }

  games[activeGame]->update(input, context);
}

void ConsoleShell::drawSelector() {
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.print("Micro Arcade");

  if (gameCount == 0) {
    display.setCursor(0, 18);
    display.print("No games");
    return;
  }

  display.setCursor(0, 16);
  display.print(games[selectedGame]->title);

  display.setCursor(0, 32);
  display.print(selectedGame + 1);
  display.print("/");
  display.print(gameCount);
  display.print("  A:Start");
}

void ConsoleShell::drawGame() { games[activeGame]->draw(display); }

void ConsoleShell::launchSelectedGame(uint32_t nowMs) {
  activeGame = selectedGame;
  mode = Mode::Game;
  gameStartMs = nowMs;
  Serial.print("launch ");
  Serial.println(games[activeGame]->title);
  saveStore.begin(games[activeGame]->saveId, false);
  activeSaveStore = &saveStore;
  games[activeGame]->start(buildGameContext(0, nowMs));
}

void ConsoleShell::returnToSelector() {
  Serial.println("return to shell");
  games[activeGame]->stop(buildGameContext(0, millis()));
  activeSaveStore = nullptr;
  saveStore.end();
  mode = Mode::Selector;
}
