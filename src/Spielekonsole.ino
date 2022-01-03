#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include "Colors.h" // eigerner header
#include "Input.h" // eigener header

// Pins für den SchiebeRegister
#define SR_DATA 2
#define SR_LATCH 3
#define SR_CLOCK 4

// Pins für den TFT Display
#define TFT_DC 9
#define TFT_CS 10

// Die größe des Displays
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Variable zur steuerung des TFT Displays
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

enum gameID
{
	NONE,
	SNAKE,
	PONG,
	MINESWEEPER,
	TICTACTOE
};

void setup() {
	pinMode(SR_LATCH, OUTPUT);
	pinMode(SR_DATA, INPUT);
	pinMode(SR_CLOCK, OUTPUT);

	// TFT Display vorbereiten
	tft.begin();
	tft.setRotation(3);
	tft.fillScreen(BLACK);

	// Begrüßungsnachricht
#define SKIPGREETING false // Debug variable um die Begrüßungsnachricht zu überspringen
#if !SKIPGREETING
	greeting();
#endif
}

bool inGame = false;
byte currentGame = NONE;
byte inputData = 0;
byte prevInputData = 0;

void loop() {
	handleInput();

	if (inGame) {
		playGame(currentGame);
	}
	else {
		currentGame = selectGame();
	}
}

// Begrüßungsnachricht
void greeting() {
	tft.setTextSize(2);
	tft.setTextWrap(false);

	printCentered(F("PAR PROJEKT 2021/22")); // eigene funktion, die den Text in die mitte des Displays schreibt
	tft.setTextSize(1);
	printCentered(F("Leon Gies"), 0, 20); // überladung mit offset

	delay(4000);
	tft.fillScreen(BLACK);
}

void handleInput() {
	prevInputData = inputData;
	inputData = 0;

	digitalWrite(SR_LATCH, LOW); // aktiviert parallele inputs
	digitalWrite(SR_CLOCK, LOW); // startet clock pin low
	digitalWrite(SR_CLOCK, HIGH); // setzt clock pin high, daten werden in SR geladen
	digitalWrite(SR_LATCH, HIGH); // deaktiviert parallele inputs and aktiviert serielle output

	// laufe durch jeden daten pin
	for (byte j = 0; j < 8; j++) {
		byte wert = digitalRead(SR_DATA); // wert der daten pins;

		if (wert) {
			// schiebe die eins so tief je nach dem welcher taster gedrückt wurde
			byte schiebeBit = (B0000001 << j);
			// Kombiniere die verschobene eins mit dataIn
			inputData |= schiebeBit;
		}

		// gehe zum nächsten pin
		digitalWrite(SR_CLOCK, LOW);
		digitalWrite(SR_CLOCK, HIGH);
	}
}

byte selectedGame = 0;
byte selectGame() {
	if (INPUT_UP_PRESSED && !PREV_INPUT_UP_PRESSED) {
		selectedGame--;
		selectedGame %= 4;
	}
	if (INPUT_DOWN_PRESSED && !PREV_INPUT_DOWN_PRESSED) {
		selectedGame = (selectedGame + 1) % 4;
	}

	const String spiele[] = { "Snake", "Pong", "Minesweeper", "Tic Tac Toe" };

	tft.setCursor(0, 0);
	tft.setTextSize(3);
	tft.setTextColor(YELLOW);

	tft.println(F("Spieleliste"));
	tft.setTextSize(2);
	setCursorRelative(0, 10);

	for (int i = 0; i < 4; i++) {
		// wenn i das ausgewählte Spiel ist, setze die Farbe zu weiß, sonst: setze die Farbe zu grau
		tft.setTextColor(selectedGame == i ? WHITE : LIGHT_GRAY);

		tft.print((char)0xF8);
		setCursorRelative(5, 0);
		tft.println(spiele[i]);
	}

	if (INPUT_A_PRESSED) {
		inGame = true;

		// reset 
		tft.fillScreen(BLACK);
		tft.setCursor(0, 0);

		return selectedGame + 1;
	}

	return NONE;
}

void playGame(const byte game) {
	switch (game)
	{
	case gameID::SNAKE:
		playSnake();
		break;
	case gameID::PONG:
		playPong();
		break;
	case gameID::MINESWEEPER:
		playMinesweeper();
		break;
	case gameID::TICTACTOE:
		playTicTacToe();
		break;
	default:
		break;
	}
}

void playSnake() {
}

void playPong() {
}

void playMinesweeper() {
}

void playTicTacToe() {
}

void printCentered(const __FlashStringHelper* str) {
	int16_t boundsX, boundsY;
	uint16_t boundsW, boundsH;
	tft.getTextBounds(str, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, &boundsX, &boundsY, &boundsW, &boundsH);
	tft.setCursor((SCREEN_WIDTH - boundsW) / 2, SCREEN_HEIGHT / 2);
	tft.print(str);
}

void printCentered(const __FlashStringHelper* str, const int16_t& offsetX, const int16_t& offsetY) {
	int16_t boundsX, boundsY;
	uint16_t boundsW, boundsH;
	tft.getTextBounds(str, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, &boundsX, &boundsY, &boundsW, &boundsH);
	tft.setCursor((SCREEN_WIDTH - boundsW) / 2 + offsetX, SCREEN_HEIGHT / 2 + offsetY);
	tft.print(str);
}

void setCursorRelative(const int16_t& x, const int16_t& y) {
	tft.setCursor(tft.getCursorX() + x, tft.getCursorY() + y);
}