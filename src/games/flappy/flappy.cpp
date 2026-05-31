#include "games/flappy/flappy.h"

namespace {
constexpr int16_t SCREEN_WIDTH = 84;
constexpr int16_t SCREEN_HEIGHT = 48;
constexpr int16_t HUD_BOTTOM = 7;
constexpr int16_t PLAY_TOP = HUD_BOTTOM + 1;
constexpr int16_t PLAY_BOTTOM = SCREEN_HEIGHT - 1;

constexpr int16_t BIRD_SIZE = 5;
constexpr float BIRD_X = 18.0f;
constexpr float START_BIRD_Y = 22.0f;
constexpr float GRAVITY_PX_PER_MS2 = 0.00030f;
constexpr float FLAP_VELOCITY_PX_PER_MS = -0.075f;
constexpr float MAX_FALL_VELOCITY_PX_PER_MS = 0.145f;

constexpr int16_t PIPE_WIDTH = 8;
constexpr uint8_t PIPE_COUNT = 3;
constexpr int16_t PIPE_SPACING = 36;
constexpr int16_t START_PIPE_X = SCREEN_WIDTH + 14;
constexpr int16_t MIN_GAP_TOP = PLAY_TOP + 3;
constexpr int16_t INITIAL_GAP_HEIGHT = 23;
constexpr int16_t MIN_GAP_HEIGHT = 15;
constexpr float INITIAL_PIPE_SPEED_PX_PER_MS = 0.022f;
constexpr float MAX_PIPE_SPEED_PX_PER_MS = 0.043f;
constexpr uint8_t HARDENING_LIMIT_SCORE = 50;
constexpr char HIGH_SCORE_SAVE_KEY[] = "high";

enum class State { Ready, Playing, GameOver };

struct Pipe {
  float x = 0;
  int16_t gapY = 0;
  bool scored = false;
};

Pipe pipes[PIPE_COUNT];
float birdY = START_BIRD_Y;
float birdVelocityY = 0;
uint16_t score = 0;
uint16_t highScore = 0;
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

bool overlaps(float ax, float ay, int16_t aw, int16_t ah, float bx, int16_t by,
              int16_t bw, int16_t bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

uint8_t difficultyStep() {
  return score > HARDENING_LIMIT_SCORE ? HARDENING_LIMIT_SCORE : score;
}

int16_t gapHeight() {
  return INITIAL_GAP_HEIGHT - (INITIAL_GAP_HEIGHT - MIN_GAP_HEIGHT) *
                                  difficultyStep() / HARDENING_LIMIT_SCORE;
}

float pipeSpeed() {
  return INITIAL_PIPE_SPEED_PX_PER_MS +
         (MAX_PIPE_SPEED_PX_PER_MS - INITIAL_PIPE_SPEED_PX_PER_MS) *
             difficultyStep() / HARDENING_LIMIT_SCORE;
}

int16_t randomGapY() {
  const int16_t gap = gapHeight();
  const int16_t maxGapTop = PLAY_BOTTOM - gap - 3;
  return random(MIN_GAP_TOP, maxGapTop + 1);
}

void updateHighScore(const GameContext &context) {
  if (score > highScore) {
    highScore = score;
    saveGameSave(context, HIGH_SCORE_SAVE_KEY, highScore);
  }
}

void resetPipe(uint8_t index, float x) {
  pipes[index].x = x;
  pipes[index].gapY = randomGapY();
  pipes[index].scored = false;
}

void resetRun() {
  birdY = START_BIRD_Y;
  birdVelocityY = 0;
  score = 0;
  state = State::Ready;

  for (uint8_t i = 0; i < PIPE_COUNT; i++) {
    resetPipe(i, START_PIPE_X + i * PIPE_SPACING);
  }
}

void start(const GameContext &context) {
  loadGameSave(context, HIGH_SCORE_SAVE_KEY, highScore);
  resetRun();
}

void flap() {
  birdVelocityY = FLAP_VELOCITY_PX_PER_MS;
  state = State::Playing;
}

void recyclePipe(uint8_t index) {
  float farthestX = pipes[0].x;
  for (uint8_t i = 1; i < PIPE_COUNT; i++) {
    if (pipes[i].x > farthestX) {
      farthestX = pipes[i].x;
    }
  }
  resetPipe(index, farthestX + PIPE_SPACING);
}

void endRun(const GameContext &context) {
  state = State::GameOver;
  updateHighScore(context);
}

void updatePipes(const GameContext &context) {
  const float movement = pipeSpeed() * context.deltaMs;
  for (uint8_t i = 0; i < PIPE_COUNT; i++) {
    Pipe &pipe = pipes[i];
    pipe.x -= movement;

    if (!pipe.scored && pipe.x + PIPE_WIDTH < BIRD_X) {
      pipe.scored = true;
      score++;
      updateHighScore(context);
    }

    if (pipe.x + PIPE_WIDTH < 0) {
      recyclePipe(i);
    }
  }
}

bool hitPipe(const Pipe &pipe) {
  const int16_t gap = gapHeight();
  return overlaps(BIRD_X, birdY, BIRD_SIZE, BIRD_SIZE, pipe.x, PLAY_TOP,
                  PIPE_WIDTH, pipe.gapY - PLAY_TOP) ||
         overlaps(BIRD_X, birdY, BIRD_SIZE, BIRD_SIZE, pipe.x, pipe.gapY + gap,
                  PIPE_WIDTH, PLAY_BOTTOM - pipe.gapY - gap);
}

void updateCollisions(const GameContext &context) {
  if (birdY < PLAY_TOP || birdY + BIRD_SIZE > PLAY_BOTTOM) {
    endRun(context);
    return;
  }

  for (uint8_t i = 0; i < PIPE_COUNT; i++) {
    if (hitPipe(pipes[i])) {
      endRun(context);
      return;
    }
  }
}

void update(const InputState &input, const GameContext &context) {
  if (state == State::GameOver) {
    if (input.a.pressed) {
      resetRun();
    }
    return;
  }

  if (input.a.pressed) {
    flap();
  }

  if (state == State::Ready) {
    return;
  }

  birdVelocityY =
      clampFloat(birdVelocityY + GRAVITY_PX_PER_MS2 * context.deltaMs,
                 FLAP_VELOCITY_PX_PER_MS, MAX_FALL_VELOCITY_PX_PER_MS);
  birdY += birdVelocityY * context.deltaMs;

  updatePipes(context);
  updateCollisions(context);
}

void drawHud(Adafruit_PCD8544 &display) {
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.print("Flappy ");
  display.print(score);
  display.print("/");
  display.print(highScore);
  display.drawFastHLine(0, HUD_BOTTOM, SCREEN_WIDTH, BLACK);
}

void drawPipes(Adafruit_PCD8544 &display) {
  const int16_t gap = gapHeight();
  for (uint8_t i = 0; i < PIPE_COUNT; i++) {
    const Pipe &pipe = pipes[i];
    const int16_t x = roundToPixel(pipe.x);
    display.fillRect(x, PLAY_TOP, PIPE_WIDTH, pipe.gapY - PLAY_TOP, BLACK);
    display.fillRect(x, pipe.gapY + gap, PIPE_WIDTH,
                     PLAY_BOTTOM - pipe.gapY - gap, BLACK);
  }
}

void draw(Adafruit_PCD8544 &display) {
  drawHud(display);
  drawPipes(display);
  display.fillRect(roundToPixel(BIRD_X), roundToPixel(birdY), BIRD_SIZE,
                   BIRD_SIZE, BLACK);

  if (state == State::Ready) {
    display.setCursor(18, 28);
    display.print("A:Flap");
  } else if (state == State::GameOver) {
    display.setCursor(18, 18);
    display.print("Game Over");
    display.setCursor(18, 30);
    display.print("A:Retry");
  }
}

void stop(const GameContext &context) { updateHighScore(context); }
} // namespace

const BundledGame FLAPPY_GAME = {"Flappy bird", "flappy", start,
                                 update,        draw,     stop};
