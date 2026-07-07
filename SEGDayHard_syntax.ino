#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // change 0x27 to your module's address if different

// --- Hardware Mapping ---
const byte LED_PINS[]    = {6, 5, 4, 3, 2};      // 5 game colour LEDs
const byte BUTTON_PINS[] = {13, 12, 11, 10, 9};  // 5 game buttons
const int  TONES[]       = {220, 262, 330, 392, 494}; // one tone per colour

#define PIN_CORRECT 8
#define PIN_WRONG   7
#define PIN_BUZZER  1   // free ONLY because we don't use Serial anymore

// --- Game Config ---
const int MAX_ROUNDS  = 10;    // game ends after clearing 10 rounds
const int FIXED_SPEED = 200;   // constant blink/tone speed (ms) -- slightly faster, never changes

// --- Game Logic Variables ---
byte sequence[50];
int gameStep   = 0;
int readStep   = 0;
int gameSpeed  = FIXED_SPEED;
int gameStatus = 0; // 0: Reset, 1: Playback, 2: Input, 3: Game Over, 4: Win

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Simon Says");
  lcd.setCursor(0, 1);
  lcd.print("Get ready!");
  delay(1000);

  randomSeed(analogRead(A0));

  for (int i = 0; i < 5; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }
  pinMode(PIN_CORRECT, OUTPUT);
  pinMode(PIN_WRONG, OUTPUT);
  // no pinMode needed for PIN_BUZZER -- tone() handles that
}

void loop() {
  if      (gameStatus == 0) prepareNewGame();
  else if (gameStatus == 1) playSequence();
  else if (gameStatus == 2) checkPlayerInput();
  else if (gameStatus == 3) handleGameOver();
  else if (gameStatus == 4) handleWin();
}

// --- Game State Functions ---
void prepareNewGame() {
  gameStep  = 0;
  readStep  = 0;
  gameSpeed = FIXED_SPEED; // speed is set once and never touched again

  for (int i = 0; i < 50; i++) {
    sequence[i] = random(0, 5); // 5 colours: 0-4
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("New game!");
  delay(500);
  gameStatus = 1;
}

void playSequence() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Level: ");
  lcd.print(gameStep + 1);
  delay(600);

  for (int i = 0; i <= gameStep; i++) {
    triggerSignal(sequence[i]);
  }
  gameStatus = 2;
}

void checkPlayerInput() {
  int pressed = getPressedButton();
  if (pressed == -1) return; // Wait for interaction

  if (pressed == sequence[readStep]) {
    digitalWrite(PIN_CORRECT, HIGH);
    triggerSignal(pressed);
    digitalWrite(PIN_CORRECT, LOW);

    if (readStep == gameStep) {
      lcd.setCursor(0, 1);
      lcd.print("Round clear!");

      // Check if the player just cleared the FINAL round
      if (gameStep + 1 >= MAX_ROUNDS) {
        gameStatus = 4; // go to win state
        return;
      }

      gameStep++;
      readStep = 0;
      // gameSpeed stays constant -- no decrementing, no changes

      playLevelUpEffect();
      gameStatus = 1;
    } else {
      readStep++;
    }
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WRONG! Game Over");
    gameStatus = 3;
  }
}

void handleGameOver() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Score: ");
  lcd.print(gameStep);
  lcd.setCursor(0, 1);
  lcd.print("Press to restart");

  digitalWrite(PIN_WRONG, HIGH);
  tone(PIN_BUZZER, 100, 600);

  while (getPressedButton() == -1);

  digitalWrite(PIN_WRONG, LOW);
  gameStatus = 0;
}

void handleWin() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("YOU WIN!");
  lcd.setCursor(0, 1);
  lcd.print("All 10 rounds!");

  playWinAnimation();

  lcd.setCursor(0, 1);
  lcd.print("Press to restart");
  while (getPressedButton() == -1);

  gameStatus = 0;
}

// --- Utility Functions ---
void triggerSignal(int id) {
  digitalWrite(LED_PINS[id], HIGH);
  tone(PIN_BUZZER, TONES[id], gameSpeed);
  delay(gameSpeed);
  digitalWrite(LED_PINS[id], LOW);
  delay(gameSpeed / 2);
}

void playLevelUpEffect() {
  for (int i = 0; i < 5; i++) digitalWrite(LED_PINS[i], HIGH);
  tone(PIN_BUZZER, 440, 150); // quick "level up" beep
  delay(200);
  for (int i = 0; i < 5; i++) digitalWrite(LED_PINS[i], LOW);
}

// Endpoint animation: all LEDs blink together + triumphant buzzer notes
void playWinAnimation() {
  int victoryTones[] = {262, 330, 392, 523, 659, 784}; // ascending triumphant riff

  for (int rep = 0; rep < 3; rep++) {
    for (int t = 0; t < 6; t++) {
      // all LEDs on together
      for (int i = 0; i < 5; i++) digitalWrite(LED_PINS[i], HIGH);
      tone(PIN_BUZZER, victoryTones[t], 150);
      delay(150);

      // all LEDs off together
      for (int i = 0; i < 5; i++) digitalWrite(LED_PINS[i], LOW);
      delay(80);
    }
    delay(200);
  }
}

int getPressedButton() {
  for (int i = 0; i < 5; i++) {
    if (digitalRead(BUTTON_PINS[i]) == LOW) {
      delay(50);
      while (digitalRead(BUTTON_PINS[i]) == LOW); // Wait for release
      return i;
    }
  }
  return -1;
}