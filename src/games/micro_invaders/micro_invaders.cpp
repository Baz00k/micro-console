#include "games/micro_invaders/micro_invaders.h"

namespace {
constexpr int16_t SCREEN_WIDTH = 84;
constexpr int16_t SCREEN_HEIGHT = 48;

constexpr int16_t HUD_BOTTOM = 7;
constexpr int16_t PLAY_TOP = HUD_BOTTOM + 1;
constexpr int16_t PLAYER_WIDTH = 9;
constexpr int16_t PLAYER_HEIGHT = 4;
constexpr int16_t PLAYER_Y = SCREEN_HEIGHT - PLAYER_HEIGHT;
constexpr float PLAYER_SPEED_PX_PER_MS = 0.055f;

constexpr uint8_t INVADER_COLUMNS = 7;
constexpr uint8_t INVADER_ROWS = 3;
constexpr uint8_t INVADER_COUNT = INVADER_COLUMNS * INVADER_ROWS;
constexpr int16_t INVADER_WIDTH = 6;
constexpr int16_t INVADER_HEIGHT = 4;
constexpr int16_t INVADER_GAP_X = 4;
constexpr int16_t INVADER_GAP_Y = 3;
constexpr int16_t INVADER_START_X = 7;
constexpr int16_t INVADER_START_Y = 12;
constexpr float INVADER_START_SPEED_PX_PER_MS = 0.007f;
constexpr float INVADER_MAX_SPEED_PX_PER_MS = 0.023f;
constexpr int16_t INVADER_DROP = 3;

constexpr int16_t PLAYER_BULLET_WIDTH = 2;
constexpr int16_t PLAYER_BULLET_HEIGHT = 4;
constexpr float PLAYER_BULLET_SPEED_PX_PER_MS = 0.105f;
constexpr uint16_t PLAYER_SHOT_COOLDOWN_MS = 260;
constexpr int16_t ENEMY_BULLET_WIDTH = 2;
constexpr int16_t ENEMY_BULLET_HEIGHT = 3;
constexpr float ENEMY_BULLET_SPEED_PX_PER_MS = 0.034f;
constexpr uint16_t ENEMY_SHOT_INTERVAL_MS = 1300;
constexpr char HIGH_SCORE_SAVE_KEY[] = "high";

enum class State { Ready, Playing, Won, Lost };

bool invaders[INVADER_COUNT];
uint8_t invadersLeft = 0;
uint16_t score = 0;
uint16_t highScore = 0;
float playerX = 0;
float formationX = 0;
float formationY = 0;
float invaderVelocityX = INVADER_START_SPEED_PX_PER_MS;
float playerBulletX = 0;
float playerBulletY = 0;
bool playerBulletActive = false;
uint16_t playerShotWaitMs = 0;
float enemyBulletX = 0;
float enemyBulletY = 0;
bool enemyBulletActive = false;
uint16_t enemyShotWaitMs = ENEMY_SHOT_INTERVAL_MS;
State state = State::Ready;

float clampFloat(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

int16_t roundToPixel(float value) { return static_cast<int16_t>(value + 0.5f); }

uint8_t invaderIndex(uint8_t column, uint8_t row) {
  return row * INVADER_COLUMNS + column;
}

int16_t invaderLocalX(uint8_t column) {
  return column * (INVADER_WIDTH + INVADER_GAP_X);
}

int16_t invaderLocalY(uint8_t row) {
  return row * (INVADER_HEIGHT + INVADER_GAP_Y);
}

bool overlaps(float ax, float ay, int16_t aw, int16_t ah, float bx, float by,
              int16_t bw, int16_t bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

void updateHighScore(const GameContext &context) {
  if (score > highScore) {
    highScore = score;
    saveGameSave(context, HIGH_SCORE_SAVE_KEY, highScore);
  }
}

void resetWave() {
  for (uint8_t i = 0; i < INVADER_COUNT; i++) {
    invaders[i] = true;
  }
  invadersLeft = INVADER_COUNT;
  playerX = (SCREEN_WIDTH - PLAYER_WIDTH) / 2.0f;
  formationX = INVADER_START_X;
  formationY = INVADER_START_Y;
  invaderVelocityX = INVADER_START_SPEED_PX_PER_MS;
  playerBulletActive = false;
  playerShotWaitMs = 0;
  enemyBulletActive = false;
  enemyShotWaitMs = ENEMY_SHOT_INTERVAL_MS;
}

void start(const GameContext &context) {
  loadGameSave(context, HIGH_SCORE_SAVE_KEY, highScore);
  score = 0;
  state = State::Ready;
  resetWave();
}

void updatePlayer(const InputState &input, uint32_t deltaMs) {
  float movement = 0;
  if (input.left.down) {
    movement -= PLAYER_SPEED_PX_PER_MS * deltaMs;
  }
  if (input.right.down) {
    movement += PLAYER_SPEED_PX_PER_MS * deltaMs;
  }
  playerX = clampFloat(playerX + movement, 0, SCREEN_WIDTH - PLAYER_WIDTH);
}

void firePlayerBullet() {
  if (playerBulletActive || playerShotWaitMs > 0) {
    return;
  }
  playerBulletActive = true;
  playerShotWaitMs = PLAYER_SHOT_COOLDOWN_MS;
  playerBulletX = playerX + (PLAYER_WIDTH - PLAYER_BULLET_WIDTH) / 2.0f;
  playerBulletY = PLAYER_Y - PLAYER_BULLET_HEIGHT;
}

void updatePlayerShotCooldown(uint32_t deltaMs) {
  if (playerShotWaitMs > deltaMs) {
    playerShotWaitMs -= deltaMs;
  } else {
    playerShotWaitMs = 0;
  }
}

void updatePlayerBullet(const GameContext &context) {
  if (!playerBulletActive) {
    return;
  }

  playerBulletY -= PLAYER_BULLET_SPEED_PX_PER_MS * context.deltaMs;
  if (playerBulletY + PLAYER_BULLET_HEIGHT < PLAY_TOP) {
    playerBulletActive = false;
    return;
  }

  for (uint8_t row = 0; row < INVADER_ROWS; row++) {
    for (uint8_t column = 0; column < INVADER_COLUMNS; column++) {
      const uint8_t index = invaderIndex(column, row);
      if (!invaders[index]) {
        continue;
      }

      const int16_t invaderX = roundToPixel(formationX) + invaderLocalX(column);
      const int16_t invaderY = roundToPixel(formationY) + invaderLocalY(row);
      if (!overlaps(playerBulletX, playerBulletY, PLAYER_BULLET_WIDTH,
                    PLAYER_BULLET_HEIGHT, invaderX, invaderY, INVADER_WIDTH,
                    INVADER_HEIGHT)) {
        continue;
      }

      invaders[index] = false;
      invadersLeft--;
      score += 10;
      updateHighScore(context);
      playerBulletActive = false;
      if (invadersLeft == 0) {
        resetWave();
      }
      return;
    }
  }
}

void liveFormationBounds(int16_t &left, int16_t &right, int16_t &bottom) {
  left = SCREEN_WIDTH;
  right = 0;
  bottom = PLAY_TOP;

  for (uint8_t row = 0; row < INVADER_ROWS; row++) {
    for (uint8_t column = 0; column < INVADER_COLUMNS; column++) {
      if (!invaders[invaderIndex(column, row)]) {
        continue;
      }
      const int16_t x = roundToPixel(formationX) + invaderLocalX(column);
      const int16_t y = roundToPixel(formationY) + invaderLocalY(row);
      left = min(left, x);
      right = max(right, static_cast<int16_t>(x + INVADER_WIDTH));
      bottom = max(bottom, static_cast<int16_t>(y + INVADER_HEIGHT));
    }
  }
}

void updateInvaders(uint32_t deltaMs) {
  if (invadersLeft == 0) {
    return;
  }

  const float clearedRatio =
      static_cast<float>(INVADER_COUNT - invadersLeft) / INVADER_COUNT;
  const float speed = clampFloat(
      INVADER_START_SPEED_PX_PER_MS + clearedRatio * 0.016f,
      INVADER_START_SPEED_PX_PER_MS, INVADER_MAX_SPEED_PX_PER_MS);
  formationX += (invaderVelocityX < 0 ? -speed : speed) * deltaMs;

  int16_t left = 0;
  int16_t right = 0;
  int16_t bottom = 0;
  liveFormationBounds(left, right, bottom);
  if (left <= 0 || right >= SCREEN_WIDTH) {
    formationX = clampFloat(formationX, formationX - left,
                            formationX + SCREEN_WIDTH - right);
    formationY += INVADER_DROP;
    invaderVelocityX = -invaderVelocityX;
    liveFormationBounds(left, right, bottom);
  }

  if (bottom >= PLAYER_Y) {
    state = State::Lost;
  }
}

bool columnHasInvader(uint8_t column, uint8_t &bottomRow) {
  for (int8_t row = INVADER_ROWS - 1; row >= 0; row--) {
    if (invaders[invaderIndex(column, row)]) {
      bottomRow = row;
      return true;
    }
  }
  return false;
}

void fireEnemyBullet() {
  if (enemyBulletActive || invadersLeft == 0) {
    return;
  }

  const uint8_t startColumn = random(INVADER_COLUMNS);
  for (uint8_t i = 0; i < INVADER_COLUMNS; i++) {
    const uint8_t column = (startColumn + i) % INVADER_COLUMNS;
    uint8_t row = 0;
    if (!columnHasInvader(column, row)) {
      continue;
    }
    enemyBulletActive = true;
    enemyBulletX = formationX + invaderLocalX(column) + INVADER_WIDTH / 2.0f;
    enemyBulletY = formationY + invaderLocalY(row) + INVADER_HEIGHT;
    return;
  }
}

void updateEnemyBullet(uint32_t deltaMs) {
  if (!enemyBulletActive) {
    if (enemyShotWaitMs > deltaMs) {
      enemyShotWaitMs -= deltaMs;
    } else {
      enemyShotWaitMs = ENEMY_SHOT_INTERVAL_MS + random(0, 500);
      fireEnemyBullet();
    }
    return;
  }

  enemyBulletY += ENEMY_BULLET_SPEED_PX_PER_MS * deltaMs;
  if (enemyBulletY > SCREEN_HEIGHT) {
    enemyBulletActive = false;
    return;
  }

  if (overlaps(enemyBulletX, enemyBulletY, ENEMY_BULLET_WIDTH,
               ENEMY_BULLET_HEIGHT, playerX, PLAYER_Y, PLAYER_WIDTH,
               PLAYER_HEIGHT)) {
    enemyBulletActive = false;
    state = State::Lost;
  }
}

void update(const InputState &input, const GameContext &context) {
  if (state == State::Won || state == State::Lost) {
    if (input.a.pressed) {
      start(context);
    }
    return;
  }

  updatePlayer(input, context.deltaMs);
  updatePlayerShotCooldown(context.deltaMs);
  if (input.a.down) {
    state = State::Playing;
    firePlayerBullet();
  }

  if (state == State::Ready) {
    return;
  }

  updateInvaders(context.deltaMs);
  updatePlayerBullet(context);
  updateEnemyBullet(context.deltaMs);
}

void drawHud(Adafruit_PCD8544 &display) {
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.print("Inv ");
  display.print(score);
  display.print("/");
  display.print(highScore);
  display.drawFastHLine(0, HUD_BOTTOM, SCREEN_WIDTH, BLACK);
}

void drawInvader(Adafruit_PCD8544 &display, int16_t x, int16_t y) {
  display.drawFastHLine(x + 1, y, 4, BLACK);
  display.fillRect(x, y + 1, INVADER_WIDTH, 2, BLACK);
  display.drawPixel(x + 1, y + 3, BLACK);
  display.drawPixel(x + 4, y + 3, BLACK);
}

void drawInvaders(Adafruit_PCD8544 &display) {
  for (uint8_t row = 0; row < INVADER_ROWS; row++) {
    for (uint8_t column = 0; column < INVADER_COLUMNS; column++) {
      if (invaders[invaderIndex(column, row)]) {
        drawInvader(display, roundToPixel(formationX) + invaderLocalX(column),
                    roundToPixel(formationY) + invaderLocalY(row));
      }
    }
  }
}

void drawPlayer(Adafruit_PCD8544 &display) {
  const int16_t x = roundToPixel(playerX);
  display.fillRect(x + 3, PLAYER_Y, 3, 1, BLACK);
  display.fillRect(x + 1, PLAYER_Y + 1, 7, 2, BLACK);
  display.drawFastHLine(x, PLAYER_Y + 3, PLAYER_WIDTH, BLACK);
}

void drawEndMessage(Adafruit_PCD8544 &display) {
  display.setCursor(18, 18);
  display.print(state == State::Won ? "Wave Clear" : "Base Hit");
  display.setCursor(18, 30);
  display.print("A:Retry");
}

void draw(Adafruit_PCD8544 &display) {
  drawHud(display);

  if (state == State::Won || state == State::Lost) {
    drawEndMessage(display);
    return;
  }

  drawInvaders(display);
  drawPlayer(display);

  if (playerBulletActive) {
    display.fillRect(roundToPixel(playerBulletX), roundToPixel(playerBulletY),
                     PLAYER_BULLET_WIDTH, PLAYER_BULLET_HEIGHT, BLACK);
  }
  if (enemyBulletActive) {
    display.fillRect(roundToPixel(enemyBulletX), roundToPixel(enemyBulletY),
                     ENEMY_BULLET_WIDTH, ENEMY_BULLET_HEIGHT, BLACK);
  }

  if (state == State::Ready) {
    display.setCursor(24, 36);
    display.print("A:Fire");
  }
}

void stop(const GameContext &context) { updateHighScore(context); }
} // namespace

const BundledGame MICRO_INVADERS_GAME = {"Micro Invaders", "invaders", start,
                                         update,           draw,       stop};
