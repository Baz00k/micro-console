#include "games/snake/snake.h"

namespace {
constexpr int8_t GRID_WIDTH = 21;
constexpr int8_t GRID_HEIGHT = 10;
constexpr int8_t CELL_SIZE = 4;
constexpr int8_t GRID_TOP = 8;
constexpr uint8_t MAX_SNAKE_LENGTH = GRID_WIDTH * GRID_HEIGHT;
constexpr uint16_t STEP_MS = 220;
constexpr char HIGH_SCORE_SAVE_KEY[] = "high";

struct Cell {
  int8_t x = 0;
  int8_t y = 0;
};

enum class Direction { Up, Right, Down, Left };

Cell snake[MAX_SNAKE_LENGTH];
uint8_t snakeLength = 0;
Cell food;
Direction direction = Direction::Right;
Direction nextDirection = Direction::Right;
uint16_t stepAccumulatorMs = 0;
uint16_t score = 0;
uint16_t highScore = 0;
bool gameOver = false;

bool sameCell(Cell a, Cell b) { return a.x == b.x && a.y == b.y; }

bool isOpposite(Direction a, Direction b) {
  return (a == Direction::Up && b == Direction::Down) ||
         (a == Direction::Down && b == Direction::Up) ||
         (a == Direction::Left && b == Direction::Right) ||
         (a == Direction::Right && b == Direction::Left);
}

bool snakeOccupies(Cell cell) {
  for (uint8_t i = 0; i < snakeLength; i++) {
    if (sameCell(snake[i], cell)) {
      return true;
    }
  }
  return false;
}

void placeFood() {
  if (snakeLength >= MAX_SNAKE_LENGTH) {
    food = {-1, -1};
    return;
  }

  Cell candidate;
  do {
    candidate = {static_cast<int8_t>(random(GRID_WIDTH)),
                 static_cast<int8_t>(random(GRID_HEIGHT))};
  } while (snakeOccupies(candidate));
  food = candidate;
}

Cell nextHead() {
  Cell head = snake[0];
  switch (direction) {
  case Direction::Up:
    head.y--;
    break;
  case Direction::Right:
    head.x++;
    break;
  case Direction::Down:
    head.y++;
    break;
  case Direction::Left:
    head.x--;
    break;
  }
  if (head.x < 0) {
    head.x = GRID_WIDTH - 1;
  } else if (head.x >= GRID_WIDTH) {
    head.x = 0;
  }
  if (head.y < 0) {
    head.y = GRID_HEIGHT - 1;
  } else if (head.y >= GRID_HEIGHT) {
    head.y = 0;
  }
  return head;
}

bool hitsBody(Cell head, bool willGrow) {
  const uint8_t bodyLength = willGrow ? snakeLength : snakeLength - 1;
  for (uint8_t i = 0; i < bodyLength; i++) {
    if (sameCell(snake[i], head)) {
      return true;
    }
  }
  return false;
}

void updateHighScore(const GameContext &context) {
  if (score > highScore) {
    highScore = score;
    saveGameSave(context, HIGH_SCORE_SAVE_KEY, highScore);
  }
}

void stepSnake(const GameContext &context) {
  direction = nextDirection;
  const Cell head = nextHead();
  const bool willGrow = sameCell(head, food);

  if (hitsBody(head, willGrow)) {
    gameOver = true;
    return;
  }

  if (willGrow && snakeLength < MAX_SNAKE_LENGTH) {
    snakeLength++;
    score++;
    updateHighScore(context);
  }

  for (int16_t i = snakeLength - 1; i > 0; i--) {
    snake[i] = snake[i - 1];
  }
  snake[0] = head;

  if (willGrow) {
    placeFood();
  }
}

void setDirection(Direction requested) {
  if (!isOpposite(direction, requested)) {
    nextDirection = requested;
  }
}

void start(const GameContext &context) {
  loadGameSave(context, HIGH_SCORE_SAVE_KEY, highScore);
  snakeLength = 3;
  snake[0] = {10, 5};
  snake[1] = {9, 5};
  snake[2] = {8, 5};
  direction = Direction::Right;
  nextDirection = Direction::Right;
  stepAccumulatorMs = 0;
  score = 0;
  gameOver = false;
  placeFood();
}

void update(const InputState &input, const GameContext &context) {
  if (gameOver) {
    if (input.a.pressed) {
      start(context);
    }
    return;
  }

  if (input.left.pressed) {
    setDirection(Direction::Left);
  } else if (input.right.pressed) {
    setDirection(Direction::Right);
  } else if (input.a.pressed) {
    setDirection(Direction::Up);
  } else if (input.b.pressed) {
    setDirection(Direction::Down);
  }

  stepAccumulatorMs += context.deltaMs;
  while (stepAccumulatorMs >= STEP_MS && !gameOver) {
    stepAccumulatorMs -= STEP_MS;
    stepSnake(context);
  }
}

void drawCell(Adafruit_PCD8544 &display, Cell cell, uint16_t color) {
  display.fillRect(cell.x * CELL_SIZE, GRID_TOP + cell.y * CELL_SIZE, CELL_SIZE,
                   CELL_SIZE, color);
}

void draw(Adafruit_PCD8544 &display) {
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.print("Snake ");
  display.print(score);
  display.print("/");
  display.print(highScore);

  if (gameOver) {
    display.setCursor(18, 18);
    display.print("Game Over");
    display.setCursor(18, 30);
    display.print("A:Retry");
    return;
  }

  drawCell(display, food, BLACK);
  for (uint8_t i = 0; i < snakeLength; i++) {
    drawCell(display, snake[i], BLACK);
  }
}

void stop(const GameContext &context) { updateHighScore(context); }
} // namespace

const BundledGame SNAKE_GAME = {"Snake", "snake", start, update, draw, stop};
