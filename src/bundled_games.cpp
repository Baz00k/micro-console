#include "bundled_games.h"

#include "games/asteroids/asteroids.h"
#include "games/breakout/breakout.h"
#include "games/button_test/button_test.h"
#include "games/flappy/flappy.h"
#include "games/micro_invaders/micro_invaders.h"
#include "games/pong/pong.h"
#include "games/snake/snake.h"

const BundledGame *const BUNDLED_GAMES[] = {
    &SNAKE_GAME, &PONG_GAME, &BREAKOUT_GAME, &FLAPPY_GAME,
    &MICRO_INVADERS_GAME, &ASTEROIDS_GAME, &BUTTON_TEST_GAME,
};

const uint8_t BUNDLED_GAME_COUNT =
    sizeof(BUNDLED_GAMES) / sizeof(BUNDLED_GAMES[0]);
