#include "games/pong/pong.h"

#include <math.h>

namespace {
constexpr int16_t SCREEN_WIDTH = 84;
constexpr int16_t SCREEN_HEIGHT = 48;

constexpr int16_t PLAY_TOP = 8;
constexpr int16_t PLAY_BOTTOM = SCREEN_HEIGHT - 1;
constexpr int16_t PADDLE_WIDTH = 2;
constexpr int16_t PADDLE_HEIGHT = 12;
constexpr int16_t PLAYER_X = 3;
constexpr int16_t CPU_X = SCREEN_WIDTH - PLAYER_X - PADDLE_WIDTH;
constexpr int16_t BALL_SIZE = 4;

constexpr float PLAYER_SPEED_PX_PER_MS = 0.05f;
constexpr float CPU_SPEED_PX_PER_MS = 0.025f;
constexpr float START_BALL_SPEED_PX_PER_MS = 0.03f;
constexpr float BALL_SPEEDUP_PX_PER_MS = 0.0025f;
constexpr float MAX_BALL_SPEED_PX_PER_MS = 0.05f;
constexpr float SERVE_ANGLE_Y = 0.45f;
constexpr float MAX_REBOUND_ANGLE_Y = 0.82f;
constexpr float CPU_DEAD_ZONE_PX = 4.0f;
constexpr float CPU_HOME_Y =
    PLAY_TOP + (PLAY_BOTTOM - PLAY_TOP - PADDLE_HEIGHT) / 2.0f;
constexpr uint16_t CPU_AIM_INTERVAL_MS = 220;
constexpr uint16_t SERVE_WAIT_MS = 900;
constexpr uint8_t WINNING_SCORE = 11;

float playerY = 0;
float cpuY = 0;
float ballX = 0;
float ballY = 0;
float ballVelocityX = 0;
float ballVelocityY = 0;
float ballSpeed = START_BALL_SPEED_PX_PER_MS;
uint8_t playerScore = 0;
uint8_t cpuScore = 0;
uint16_t serveWaitMs = 0;
uint16_t cpuAimWaitMs = 0;
float cpuTargetY = CPU_HOME_Y;
int8_t serveDirection = 1;
bool matchOver = false;

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

void setBallVelocity(float directionX, float directionY) {
  const float length = sqrtf(directionX * directionX + directionY * directionY);
  ballVelocityX = ballSpeed * directionX / length;
  ballVelocityY = ballSpeed * directionY / length;
}

void resetBall() {
  ballX = (SCREEN_WIDTH - BALL_SIZE) / 2.0f;
  ballY = PLAY_TOP + (PLAY_BOTTOM - PLAY_TOP - BALL_SIZE) / 2.0f;
  ballSpeed = START_BALL_SPEED_PX_PER_MS;
  setBallVelocity(serveDirection,
                  random(2) == 0 ? -SERVE_ANGLE_Y : SERVE_ANGLE_Y);
  serveWaitMs = SERVE_WAIT_MS;
  cpuAimWaitMs = 0;
  cpuTargetY = CPU_HOME_Y;
}

void start() {
  playerY = PLAY_TOP + (PLAY_BOTTOM - PLAY_TOP - PADDLE_HEIGHT) / 2.0f;
  cpuY = playerY;
  playerScore = 0;
  cpuScore = 0;
  serveDirection = random(2) == 0 ? -1 : 1;
  matchOver = false;
  resetBall();
}

bool overlaps(float ax, float ay, int16_t aw, int16_t ah, float bx, float by,
              int16_t bw, int16_t bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

void reboundFromPaddle(float paddleY, int8_t direction) {
  const float paddleCenter = paddleY + PADDLE_HEIGHT / 2.0f;
  const float ballCenter = ballY + BALL_SIZE / 2.0f;
  const float offset = (ballCenter - paddleCenter) / (PADDLE_HEIGHT / 2.0f);

  ballSpeed = clampFloat(ballSpeed + BALL_SPEEDUP_PX_PER_MS,
                         START_BALL_SPEED_PX_PER_MS, MAX_BALL_SPEED_PX_PER_MS);
  setBallVelocity(
      direction, clampFloat(offset, -MAX_REBOUND_ANGLE_Y, MAX_REBOUND_ANGLE_Y));
}

void scorePoint(bool playerScored) {
  if (playerScored) {
    playerScore++;
    serveDirection = -1;
  } else {
    cpuScore++;
    serveDirection = 1;
  }

  if (playerScore >= WINNING_SCORE || cpuScore >= WINNING_SCORE) {
    matchOver = true;
    return;
  }

  resetBall();
}

void updatePlayer(const InputState &input, uint32_t deltaMs) {
  float movement = 0;
  if (input.left.down) {
    movement -= PLAYER_SPEED_PX_PER_MS * deltaMs;
  }
  if (input.right.down) {
    movement += PLAYER_SPEED_PX_PER_MS * deltaMs;
  }
  playerY =
      clampFloat(playerY + movement, PLAY_TOP, PLAY_BOTTOM - PADDLE_HEIGHT + 1);
}

void updateCpu(uint32_t deltaMs) {
  const float cpuCenter = cpuY + PADDLE_HEIGHT / 2.0f;
  const float maxMove = CPU_SPEED_PX_PER_MS * deltaMs;

  if (ballVelocityX < 0) {
    const float homeCenter = CPU_HOME_Y + PADDLE_HEIGHT / 2.0f;
    if (homeCenter < cpuCenter - CPU_DEAD_ZONE_PX) {
      cpuY -= maxMove;
    } else if (homeCenter > cpuCenter + CPU_DEAD_ZONE_PX) {
      cpuY += maxMove;
    }
    cpuY = clampFloat(cpuY, PLAY_TOP, PLAY_BOTTOM - PADDLE_HEIGHT + 1);
    return;
  }

  if (ballX > SCREEN_WIDTH / 2 && cpuAimWaitMs == 0) {
    cpuTargetY = ballY + BALL_SIZE / 2.0f + random(-3, 4);
    cpuAimWaitMs = CPU_AIM_INTERVAL_MS;
  }

  if (cpuAimWaitMs > deltaMs) {
    cpuAimWaitMs -= deltaMs;
  } else {
    cpuAimWaitMs = 0;
  }

  if (cpuTargetY < cpuCenter - CPU_DEAD_ZONE_PX) {
    cpuY -= maxMove;
  } else if (cpuTargetY > cpuCenter + CPU_DEAD_ZONE_PX) {
    cpuY += maxMove;
  }

  cpuY = clampFloat(cpuY, PLAY_TOP, PLAY_BOTTOM - PADDLE_HEIGHT + 1);
}

void updateBall(uint32_t deltaMs) {
  if (serveWaitMs > 0) {
    if (deltaMs >= serveWaitMs) {
      serveWaitMs = 0;
    } else {
      serveWaitMs -= deltaMs;
      return;
    }
  }

  ballX += ballVelocityX * deltaMs;
  ballY += ballVelocityY * deltaMs;

  if (ballY <= PLAY_TOP) {
    ballY = PLAY_TOP;
    ballVelocityY = -ballVelocityY;
  } else if (ballY + BALL_SIZE - 1 >= PLAY_BOTTOM) {
    ballY = PLAY_BOTTOM - BALL_SIZE + 1;
    ballVelocityY = -ballVelocityY;
  }

  if (ballVelocityX < 0 &&
      overlaps(ballX, ballY, BALL_SIZE, BALL_SIZE, PLAYER_X, playerY,
               PADDLE_WIDTH, PADDLE_HEIGHT)) {
    ballX = PLAYER_X + PADDLE_WIDTH;
    reboundFromPaddle(playerY, 1);
  } else if (ballVelocityX > 0 &&
             overlaps(ballX, ballY, BALL_SIZE, BALL_SIZE, CPU_X, cpuY,
                      PADDLE_WIDTH, PADDLE_HEIGHT)) {
    ballX = CPU_X - BALL_SIZE;
    reboundFromPaddle(cpuY, -1);
  }

  if (ballX < -BALL_SIZE) {
    scorePoint(false);
  } else if (ballX > SCREEN_WIDTH) {
    scorePoint(true);
  }
}

void update(const InputState &input, const GameContext &context) {
  if (matchOver) {
    if (input.a.pressed) {
      start();
    }
    return;
  }

  updatePlayer(input, context.deltaMs);
  updateCpu(context.deltaMs);
  updateBall(context.deltaMs);
}

void drawNet(Adafruit_PCD8544 &display) {
  for (int16_t y = PLAY_TOP + 1; y < SCREEN_HEIGHT; y += 6) {
    display.drawFastVLine(SCREEN_WIDTH / 2, y, 3, BLACK);
  }
}

void drawScore(Adafruit_PCD8544 &display) {
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(26, 0);
  display.print(playerScore);
  display.setCursor(54, 0);
  display.print(cpuScore);
}

void draw(Adafruit_PCD8544 &display) {
  drawScore(display);
  display.drawFastHLine(0, PLAY_TOP - 1, SCREEN_WIDTH, BLACK);
  drawNet(display);

  if (matchOver) {
    display.setCursor(18, 18);
    display.print(playerScore > cpuScore ? "You Win" : "You Lose");
    display.setCursor(18, 30);
    display.print("A:Retry");
    return;
  }

  display.fillRect(PLAYER_X, roundToPixel(playerY), PADDLE_WIDTH, PADDLE_HEIGHT,
                   BLACK);
  display.fillRect(CPU_X, roundToPixel(cpuY), PADDLE_WIDTH, PADDLE_HEIGHT,
                   BLACK);
  display.fillRect(roundToPixel(ballX), roundToPixel(ballY), BALL_SIZE,
                   BALL_SIZE, BLACK);
}

void stop() {}
} // namespace

const BundledGame PONG_GAME = {"Pong", start, update, draw, stop};
