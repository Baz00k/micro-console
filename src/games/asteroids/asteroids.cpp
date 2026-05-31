#include "games/asteroids/asteroids.h"

#include <math.h>

namespace {
constexpr int16_t SCREEN_WIDTH = 84;
constexpr int16_t SCREEN_HEIGHT = 48;
constexpr int16_t HUD_BOTTOM = 7;
constexpr int16_t PLAY_TOP = HUD_BOTTOM + 1;
constexpr float PI_F = 3.14159265f;

constexpr uint8_t MAX_ASTEROIDS = 5;
constexpr uint8_t MAX_SHOTS = 3;
constexpr uint8_t STARTING_LIVES = 3;
constexpr char HIGH_SCORE_SAVE_KEY[] = "high";

constexpr float ROTATION_RAD_PER_MS = 0.0048f;
constexpr float THRUST_PX_PER_MS2 = 0.000065f;
constexpr float FRICTION_PER_MS = 0.00062f;
constexpr float MAX_SHIP_SPEED_PX_PER_MS = 0.045f;
constexpr float SHOT_SPEED_PX_PER_MS = 0.08f;
constexpr uint16_t SHOT_TTL_MS = 900;
constexpr uint16_t RESPAWN_SAFE_MS = 1300;
constexpr float SAFE_SPAWN_RADIUS = 24.0f;

enum class State { Playing, GameOver };

struct Asteroid {
  bool active = false;
  float x = 0;
  float y = 0;
  float vx = 0;
  float vy = 0;
  uint8_t size = 0;
};

struct Shot {
  bool active = false;
  float x = 0;
  float y = 0;
  float vx = 0;
  float vy = 0;
  uint16_t ttlMs = 0;
};

Asteroid asteroids[MAX_ASTEROIDS];
Shot shots[MAX_SHOTS];
float shipX = 0;
float shipY = 0;
float shipVx = 0;
float shipVy = 0;
float shipAngle = -PI_F / 2.0f;
uint16_t score = 0;
uint16_t highScore = 0;
uint8_t lives = STARTING_LIVES;
uint8_t wave = 1;
uint16_t respawnSafeMs = 0;
bool newHighScore = false;
State state = State::Playing;

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

float randomFloat(float minValue, float maxValue) {
  return minValue + (maxValue - minValue) * (random(0, 1000) / 1000.0f);
}

uint8_t asteroidRadius(uint8_t size) {
  if (size >= 3) {
    return 7;
  }
  if (size == 2) {
    return 5;
  }
  return 3;
}

void updateHighScore(const GameContext &context) {
  if (score > highScore) {
    highScore = score;
    newHighScore = true;
    saveGameSave(context, HIGH_SCORE_SAVE_KEY, highScore);
  }
}

void wrapPosition(float &x, float &y) {
  if (x < 0) {
    x += SCREEN_WIDTH;
  } else if (x >= SCREEN_WIDTH) {
    x -= SCREEN_WIDTH;
  }

  if (y < PLAY_TOP) {
    y += SCREEN_HEIGHT - PLAY_TOP;
  } else if (y >= SCREEN_HEIGHT) {
    y -= SCREEN_HEIGHT - PLAY_TOP;
  }
}

bool nearShipStart(float x, float y) {
  const float dx = x - shipX;
  const float dy = y - shipY;
  return dx * dx + dy * dy < SAFE_SPAWN_RADIUS * SAFE_SPAWN_RADIUS;
}

int8_t findInactiveAsteroid() {
  for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
    if (!asteroids[i].active) {
      return i;
    }
  }
  return -1;
}

void spawnAsteroid(uint8_t size, float x, float y) {
  const int8_t index = findInactiveAsteroid();
  if (index < 0) {
    return;
  }

  const float angle = randomFloat(0, 2.0f * PI_F);
  const float speed = randomFloat(0.006f, 0.014f) + wave * 0.0007f;
  asteroids[index] = Asteroid{true, x, y, cosf(angle) * speed,
                              sinf(angle) * speed, size};
}

void spawnWave() {
  for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
    asteroids[i].active = false;
  }
  for (uint8_t i = 0; i < MAX_SHOTS; i++) {
    shots[i].active = false;
  }

  const uint8_t count = static_cast<uint8_t>(clampFloat(1 + wave, 2, 3));
  for (uint8_t i = 0; i < count; i++) {
    float x = 0;
    float y = 0;
    do {
      x = random(0, SCREEN_WIDTH);
      y = random(PLAY_TOP, SCREEN_HEIGHT);
    } while (nearShipStart(x, y));
    spawnAsteroid(3, x, y);
  }
}

void resetShip() {
  shipX = SCREEN_WIDTH / 2.0f;
  shipY = PLAY_TOP + (SCREEN_HEIGHT - PLAY_TOP) / 2.0f;
  shipVx = 0;
  shipVy = 0;
  shipAngle = -PI_F / 2.0f;
  respawnSafeMs = RESPAWN_SAFE_MS;
}

void start(const GameContext &context) {
  loadGameSave(context, HIGH_SCORE_SAVE_KEY, highScore);
  score = 0;
  lives = STARTING_LIVES;
  wave = 1;
  newHighScore = false;
  state = State::Playing;
  resetShip();
  spawnWave();
}

void fireShot() {
  for (uint8_t i = 0; i < MAX_SHOTS; i++) {
    if (shots[i].active) {
      continue;
    }

    const float noseX = cosf(shipAngle);
    const float noseY = sinf(shipAngle);
    shots[i] = Shot{true,
                    shipX + noseX * 4.0f,
                    shipY + noseY * 4.0f,
                    shipVx + noseX * SHOT_SPEED_PX_PER_MS,
                    shipVy + noseY * SHOT_SPEED_PX_PER_MS,
                    SHOT_TTL_MS};
    return;
  }
}

void updateShip(const InputState &input, uint32_t deltaMs) {
  if (input.left.down) {
    shipAngle -= ROTATION_RAD_PER_MS * deltaMs;
  }
  if (input.right.down) {
    shipAngle += ROTATION_RAD_PER_MS * deltaMs;
  }
  if (input.a.down) {
    shipVx += cosf(shipAngle) * THRUST_PX_PER_MS2 * deltaMs;
    shipVy += sinf(shipAngle) * THRUST_PX_PER_MS2 * deltaMs;
  }
  if (input.a.pressed) {
    fireShot();
  }

  const float speed = sqrtf(shipVx * shipVx + shipVy * shipVy);
  if (speed > MAX_SHIP_SPEED_PX_PER_MS) {
    shipVx = shipVx / speed * MAX_SHIP_SPEED_PX_PER_MS;
    shipVy = shipVy / speed * MAX_SHIP_SPEED_PX_PER_MS;
  }

  const float friction = clampFloat(1.0f - FRICTION_PER_MS * deltaMs, 0.0f,
                                    1.0f);
  shipVx *= friction;
  shipVy *= friction;
  shipX += shipVx * deltaMs;
  shipY += shipVy * deltaMs;
  wrapPosition(shipX, shipY);
}

void updateShots(uint32_t deltaMs) {
  for (uint8_t i = 0; i < MAX_SHOTS; i++) {
    if (!shots[i].active) {
      continue;
    }

    shots[i].x += shots[i].vx * deltaMs;
    shots[i].y += shots[i].vy * deltaMs;
    wrapPosition(shots[i].x, shots[i].y);
    if (shots[i].ttlMs <= deltaMs) {
      shots[i].active = false;
    } else {
      shots[i].ttlMs -= deltaMs;
    }
  }
}

void updateAsteroids(uint32_t deltaMs) {
  for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
    if (!asteroids[i].active) {
      continue;
    }
    asteroids[i].x += asteroids[i].vx * deltaMs;
    asteroids[i].y += asteroids[i].vy * deltaMs;
    wrapPosition(asteroids[i].x, asteroids[i].y);
  }
}

bool circlesOverlap(float ax, float ay, float ar, float bx, float by, float br) {
  const float dx = ax - bx;
  const float dy = ay - by;
  const float radius = ar + br;
  return dx * dx + dy * dy <= radius * radius;
}

void splitAsteroid(const Asteroid &asteroid) {
  if (asteroid.size <= 1) {
    return;
  }
  spawnAsteroid(asteroid.size - 1, asteroid.x, asteroid.y);
}

void hitAsteroid(const GameContext &context, uint8_t asteroidIndex) {
  const Asteroid hit = asteroids[asteroidIndex];
  asteroids[asteroidIndex].active = false;
  score += hit.size == 3 ? 20 : hit.size == 2 ? 50 : 100;
  updateHighScore(context);
  splitAsteroid(hit);
}

void updateShotCollisions(const GameContext &context) {
  for (uint8_t shotIndex = 0; shotIndex < MAX_SHOTS; shotIndex++) {
    if (!shots[shotIndex].active) {
      continue;
    }

    for (uint8_t asteroidIndex = 0; asteroidIndex < MAX_ASTEROIDS;
         asteroidIndex++) {
      if (!asteroids[asteroidIndex].active) {
        continue;
      }

      if (circlesOverlap(shots[shotIndex].x, shots[shotIndex].y, 2,
                         asteroids[asteroidIndex].x,
                         asteroids[asteroidIndex].y,
                         asteroidRadius(asteroids[asteroidIndex].size))) {
        shots[shotIndex].active = false;
        hitAsteroid(context, asteroidIndex);
        break;
      }
    }
  }
}

bool anyAsteroidsActive() {
  for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
    if (asteroids[i].active) {
      return true;
    }
  }
  return false;
}

void loseLife(const GameContext &context) {
  updateHighScore(context);
  if (lives > 0) {
    lives--;
  }
  if (lives == 0) {
    state = State::GameOver;
    return;
  }
  resetShip();
}

void updateShipCollisions(const GameContext &context) {
  if (respawnSafeMs > 0) {
    return;
  }

  for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
    if (!asteroids[i].active) {
      continue;
    }

    if (circlesOverlap(shipX, shipY, 4, asteroids[i].x, asteroids[i].y,
                       asteroidRadius(asteroids[i].size))) {
      loseLife(context);
      return;
    }
  }
}

void update(const InputState &input, const GameContext &context) {
  if (state == State::GameOver) {
    if (input.a.pressed) {
      start(context);
    }
    return;
  }

  if (respawnSafeMs > context.deltaMs) {
    respawnSafeMs -= context.deltaMs;
  } else {
    respawnSafeMs = 0;
  }

  updateShip(input, context.deltaMs);
  updateShots(context.deltaMs);
  updateAsteroids(context.deltaMs);
  updateShotCollisions(context);
  updateShipCollisions(context);

  if (!anyAsteroidsActive()) {
    wave++;
    resetShip();
    spawnWave();
  }
}

void drawHud(Adafruit_PCD8544 &display) {
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.print(score);
  display.setCursor(44, 0);
  display.print("L");
  display.print(lives);
  display.drawFastHLine(0, HUD_BOTTOM, SCREEN_WIDTH, BLACK);
}

void drawShip(Adafruit_PCD8544 &display) {
  if (respawnSafeMs > 0 && (respawnSafeMs / 120) % 2 == 0) {
    return;
  }

  const float noseX = cosf(shipAngle);
  const float noseY = sinf(shipAngle);
  const float leftX = cosf(shipAngle + 2.45f);
  const float leftY = sinf(shipAngle + 2.45f);
  const float rightX = cosf(shipAngle - 2.45f);
  const float rightY = sinf(shipAngle - 2.45f);
  const int16_t x1 = roundToPixel(shipX + noseX * 5.0f);
  const int16_t y1 = roundToPixel(shipY + noseY * 5.0f);
  const int16_t x2 = roundToPixel(shipX + leftX * 4.0f);
  const int16_t y2 = roundToPixel(shipY + leftY * 4.0f);
  const int16_t x3 = roundToPixel(shipX + rightX * 4.0f);
  const int16_t y3 = roundToPixel(shipY + rightY * 4.0f);

  display.drawLine(x1, y1, x2, y2, BLACK);
  display.drawLine(x2, y2, x3, y3, BLACK);
  display.drawLine(x3, y3, x1, y1, BLACK);
}

void drawAsteroids(Adafruit_PCD8544 &display) {
  for (uint8_t i = 0; i < MAX_ASTEROIDS; i++) {
    if (!asteroids[i].active) {
      continue;
    }
    const uint8_t radius = asteroidRadius(asteroids[i].size);
    const int16_t x = roundToPixel(asteroids[i].x);
    const int16_t y = roundToPixel(asteroids[i].y);
    display.drawCircle(x, y, radius, BLACK);
    display.drawPixel(x - radius / 2, y - radius / 2, BLACK);
    display.drawPixel(x + radius / 2, y + radius / 3, BLACK);
  }
}

void drawShots(Adafruit_PCD8544 &display) {
  for (uint8_t i = 0; i < MAX_SHOTS; i++) {
    if (shots[i].active) {
      display.fillRect(roundToPixel(shots[i].x), roundToPixel(shots[i].y), 2, 2,
                       BLACK);
    }
  }
}

void drawGameOver(Adafruit_PCD8544 &display) {
  display.setCursor(18, 10);
  display.print("Game Over");
  display.setCursor(12, 20);
  display.print("Score ");
  display.print(score);
  display.setCursor(12, 30);
  if (newHighScore) {
    display.print("New High");
  } else {
    display.print("High ");
    display.print(highScore);
  }
  display.setCursor(18, 40);
  display.print("A:Retry");
}

void draw(Adafruit_PCD8544 &display) {
  drawHud(display);
  if (state == State::GameOver) {
    drawGameOver(display);
    return;
  }

  drawAsteroids(display);
  drawShots(display);
  drawShip(display);
}

void stop(const GameContext &context) { updateHighScore(context); }
} // namespace

const BundledGame ASTEROIDS_GAME = {"Asteroids", "asteroids", start, update,
                                    draw,        stop};
