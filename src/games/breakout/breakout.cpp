#include "games/breakout/breakout.h"

namespace {
constexpr int16_t SCREEN_WIDTH = 84;
constexpr int16_t SCREEN_HEIGHT = 48;

constexpr int16_t HUD_BOTTOM = 7;
constexpr int16_t WALL_TOP = HUD_BOTTOM + 1;
constexpr int16_t PLAY_BOTTOM = SCREEN_HEIGHT - 1;

constexpr int16_t PADDLE_WIDTH = 18;
constexpr int16_t PADDLE_HEIGHT = 3;
constexpr int16_t PADDLE_Y = SCREEN_HEIGHT - 5;
constexpr float PADDLE_SPEED_PX_PER_MS = 0.052f;

constexpr int16_t BALL_SIZE = 3;
constexpr float BALL_SPEED_X_PX_PER_MS = 0.026f;
constexpr float BALL_SPEED_Y_PX_PER_MS = 0.031f;
constexpr float MAX_BALL_SPEED_X_PX_PER_MS = 0.044f;

constexpr uint8_t BRICK_COLUMNS = 7;
constexpr uint8_t BRICK_ROWS = 4;
constexpr uint8_t BRICK_COUNT = BRICK_COLUMNS * BRICK_ROWS;
constexpr char HIGH_SCORE_SAVE_KEY[] = "high";
constexpr int16_t BRICK_WIDTH = 10;
constexpr int16_t BRICK_HEIGHT = 4;
constexpr int16_t BRICK_GAP = 1;
constexpr int16_t BRICKS_LEFT = 4;
constexpr int16_t BRICKS_TOP = 11;

enum class State { Ready, Playing, Won, Lost };

bool bricks[BRICK_COUNT];
uint8_t bricksLeft = 0;
uint16_t score = 0;
uint16_t highScore = 0;
float paddleX = 0;
float ballX = 0;
float ballY = 0;
float ballVelocityX = 0;
float ballVelocityY = 0;
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

bool overlaps(float ax, float ay, int16_t aw, int16_t ah, int16_t bx,
              int16_t by, int16_t bw, int16_t bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

uint8_t brickIndex(uint8_t column, uint8_t row) {
  return row * BRICK_COLUMNS + column;
}

int16_t brickX(uint8_t column) {
  return BRICKS_LEFT + column * (BRICK_WIDTH + BRICK_GAP);
}

int16_t brickY(uint8_t row) {
  return BRICKS_TOP + row * (BRICK_HEIGHT + BRICK_GAP);
}

void resetBallOnPaddle() {
  ballX = paddleX + (PADDLE_WIDTH - BALL_SIZE) / 2.0f;
  ballY = PADDLE_Y - BALL_SIZE - 1;
  ballVelocityX = 0;
  ballVelocityY = 0;
}

void serveBall() {
  ballVelocityX = BALL_SPEED_X_PX_PER_MS * (random(2) == 0 ? -1 : 1);
  ballVelocityY = -BALL_SPEED_Y_PX_PER_MS;
  state = State::Playing;
}

void updateHighScore(const GameContext &context) {
  if (score > highScore) {
    highScore = score;
    saveGameSave(context, HIGH_SCORE_SAVE_KEY, highScore);
  }
}

void start(const GameContext &context) {
  loadGameSave(context, HIGH_SCORE_SAVE_KEY, highScore);
  for (uint8_t i = 0; i < BRICK_COUNT; i++) {
    bricks[i] = true;
  }
  bricksLeft = BRICK_COUNT;
  score = 0;
  paddleX = (SCREEN_WIDTH - PADDLE_WIDTH) / 2.0f;
  state = State::Ready;
  resetBallOnPaddle();
}

void updatePaddle(const InputState &input, uint32_t deltaMs) {
  float movement = 0;
  if (input.left.down) {
    movement -= PADDLE_SPEED_PX_PER_MS * deltaMs;
  }
  if (input.right.down) {
    movement += PADDLE_SPEED_PX_PER_MS * deltaMs;
  }
  paddleX = clampFloat(paddleX + movement, 0, SCREEN_WIDTH - PADDLE_WIDTH);
}

void reboundFromPaddle() {
  const float paddleCenter = paddleX + PADDLE_WIDTH / 2.0f;
  const float ballCenter = ballX + BALL_SIZE / 2.0f;
  const float offset = (ballCenter - paddleCenter) / (PADDLE_WIDTH / 2.0f);

  ballVelocityX = clampFloat(offset * MAX_BALL_SPEED_X_PX_PER_MS,
                             -MAX_BALL_SPEED_X_PX_PER_MS,
                             MAX_BALL_SPEED_X_PX_PER_MS);
  ballVelocityY = -BALL_SPEED_Y_PX_PER_MS;
}

void hitBrick(const GameContext &context, uint8_t index) {
  bricks[index] = false;
  bricksLeft--;
  score += 10;
  updateHighScore(context);
  ballVelocityY = -ballVelocityY;

  if (bricksLeft == 0) {
    state = State::Won;
  }
}

void updateBrickCollision(const GameContext &context) {
  for (uint8_t row = 0; row < BRICK_ROWS; row++) {
    for (uint8_t column = 0; column < BRICK_COLUMNS; column++) {
      const uint8_t index = brickIndex(column, row);
      if (!bricks[index]) {
        continue;
      }

      if (overlaps(ballX, ballY, BALL_SIZE, BALL_SIZE, brickX(column),
                   brickY(row), BRICK_WIDTH, BRICK_HEIGHT)) {
        hitBrick(context, index);
        return;
      }
    }
  }
}

void updateBall(const GameContext &context) {
  const uint32_t deltaMs = context.deltaMs;
  ballX += ballVelocityX * deltaMs;
  ballY += ballVelocityY * deltaMs;

  if (ballX <= 0) {
    ballX = 0;
    ballVelocityX = -ballVelocityX;
  } else if (ballX + BALL_SIZE >= SCREEN_WIDTH) {
    ballX = SCREEN_WIDTH - BALL_SIZE;
    ballVelocityX = -ballVelocityX;
  }

  if (ballY <= WALL_TOP) {
    ballY = WALL_TOP;
    ballVelocityY = -ballVelocityY;
  }

  if (ballVelocityY > 0 &&
      overlaps(ballX, ballY, BALL_SIZE, BALL_SIZE, roundToPixel(paddleX),
               PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT)) {
    ballY = PADDLE_Y - BALL_SIZE;
    reboundFromPaddle();
  }

  updateBrickCollision(context);

  if (ballY > PLAY_BOTTOM) {
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

  updatePaddle(input, context.deltaMs);

  if (state == State::Ready) {
    resetBallOnPaddle();
    if (input.a.pressed) {
      serveBall();
    }
    return;
  }

  updateBall(context);
}

void drawHud(Adafruit_PCD8544 &display) {
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.print("Break ");
  display.print(score);
  display.print("/");
  display.print(highScore);
  display.drawFastHLine(0, HUD_BOTTOM, SCREEN_WIDTH, BLACK);
}

void drawBricks(Adafruit_PCD8544 &display) {
  for (uint8_t row = 0; row < BRICK_ROWS; row++) {
    for (uint8_t column = 0; column < BRICK_COLUMNS; column++) {
      if (bricks[brickIndex(column, row)]) {
        display.fillRect(brickX(column), brickY(row), BRICK_WIDTH,
                         BRICK_HEIGHT, BLACK);
      }
    }
  }
}

void drawEndMessage(Adafruit_PCD8544 &display) {
  display.setCursor(24, 18);
  display.print(state == State::Won ? "You Win" : "Missed");
  display.setCursor(18, 30);
  display.print("A:Retry");
}

void draw(Adafruit_PCD8544 &display) {
  drawHud(display);

  if (state == State::Won || state == State::Lost) {
    drawEndMessage(display);
    return;
  }

  drawBricks(display);
  display.fillRect(roundToPixel(paddleX), PADDLE_Y, PADDLE_WIDTH,
                   PADDLE_HEIGHT, BLACK);
  display.fillRect(roundToPixel(ballX), roundToPixel(ballY), BALL_SIZE,
                   BALL_SIZE, BLACK);

  if (state == State::Ready) {
    display.setCursor(24, 36);
    display.print("A:Serve");
  }
}

void stop(const GameContext &context) { updateHighScore(context); }
} // namespace

const BundledGame BREAKOUT_GAME = {"Breakout", "breakout", start, update,
                                   draw,       stop};
