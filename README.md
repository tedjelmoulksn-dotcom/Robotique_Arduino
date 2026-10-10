# Arduino and LEGO Robotics Experiments

A collection of embedded experiments covering sensors, motor drivers, LCD interaction and LEGO EV3 programming.

## Project guide

| Folder | Contents |
| --- | --- |
| [projects/Arduino_LCD_Runner](projects/Arduino_LCD_Runner/) | Arduino LCD runner game |
| [projects/LEGO_EV3_Escape_the_Aliens](projects/LEGO_EV3_Escape_the_Aliens/) | Native EV3 project |
| [experiments/Essai_pont_H_LMD18200](experiments/Essai_pont_H_LMD18200/) | H-bridge motor-driver test |
| [experiments/Infra-rouge](experiments/Infra-rouge/) | Infrared sensor sketch |
| [experiments/Ultra-Son-test](experiments/Ultra-Son-test/) | Ultrasonic measurement sketch |
| [experiments/LEGO_EV3_Brick_Test](experiments/LEGO_EV3_Brick_Test/) | Original brick-test project recovered from Drive |
| [third_party/mfrc522](third_party/mfrc522/) | Bundled MFRC522 library, examples and original license |

## Use

Open an Arduino sketch in Arduino IDE and check the board, library dependencies and pin assignments in its source. These experiments use different hardware configurations and are intended to run individually.

Open `.ev3` files with the LEGO MINDSTORMS EV3 software. The recovered brick test is preserved in its native project format.

The MFRC522 folder is upstream dependency material; its examples are attributed to that library. Robot competition code is maintained separately in [Farming Mars CFR](https://github.com/tedjelmoulksn-dotcom/Farming_Mars_CFR).

Hardware tests have not been repeated during repository organization.
