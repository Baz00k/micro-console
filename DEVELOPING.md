# Developing

## Add A Bundled Game

1. Create `include/games/<name>/<name>.h` declaring `extern const BundledGame <NAME>_GAME;`.
2. Create `src/games/<name>/<name>.cpp` implementing the `BundledGame` callbacks.
3. Add the game header and `&<NAME>_GAME` to `src/bundled_games.cpp`.
4. Build with `mise exec -- python -m platformio run`.

## Game Contract

- The **Console Shell** owns input polling, timing, display clear, and display flush.
- A **Bundled Game** implements `start`, `update`, `draw`, and `stop`.
- `update` receives debounced input and elapsed time.
- `draw` may draw directly with `Adafruit_PCD8544`.
- `start`, `update`, and `stop` receive a `GameContext` that can load and save small byte values by key.
- Each **Bundled Game** declares a stable `saveId`; changing it starts a fresh save namespace for that game.
- Use `loadGameSave(context, key, value)` and `saveGameSave(context, key, value)` for simple typed values such as high scores.
- Holding `B` returns to the **Console Shell**; games do not handle that themselves.
