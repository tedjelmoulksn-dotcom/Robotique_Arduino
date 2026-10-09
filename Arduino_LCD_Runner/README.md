# Arduino LCD Runner — Interrupts and Character Graphics

A small obstacle-avoidance game on a 16×2 character LCD. A button requests a jump while the terrain scrolls; collision ends the run and travelled distance drives the score.

## Embedded implementation

| Function | Role |
|---|---|
| `initializeGraphics()` | Loads custom character patterns |
| `advanceTerrain()` | Shifts upper/lower terrain buffers |
| `drawHero()` | Draws the character and checks collisions |
| `loop()` | Advances the game state and display |

The implementation uses the HD44780-style controller's custom-character memory, limited to eight slots. Two line buffers represent terrain, and discrete hero states represent running/jumping phases. Button input uses an external interrupt.

## Hardware and setup

The original project uses an Arduino Uno-compatible board, a 16×2 LCD and a push-button. Review [`arduino_lcd_runner.ino`](arduino_lcd_runner.ino) for the exact library constructor and pin definitions; match those definitions to the wiring before upload.

Open the sketch in an Arduino-compatible IDE, select the board/port and compile before uploading.

## Timing and extension

The main loop uses `delay()`, so animation timing is blocking. This is an interactive embedded example rather than a measured hard real-time application. A continuation could introduce `millis()` scheduling, explicit button debouncing and a review of ISR-shared variables.

To inspect interaction timing, follow the button interrupt, the shared jump request and the next display update. This separates input latency from the animation cadence.

## Source attribution

This project adapts a third-party LCD-game example. The project work concerns game behaviour and embedded interaction; the original tutorial code remains third-party material, and its redistribution terms govern reuse.

## Licence

No project-wide licence has been defined.
