#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_SPITFT.h>
#include <SPI.h>
#include "Input.h" // eigener header

// Pins für den SchiebeRegister
#define SR_DATA 2
#define SR_LATCH 3
#define SR_CLOCK 4

// Pins für den TFT Display
#define TFT_DC 8 // SPI Data
#define TFT_CS 10 // Chip select
#define TFT_BL 9 // Backlight

#define LDR_PIN A0

// Variable zur steuerung des TFT Displays
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

struct position {
	uint16_t X;
	uint8_t Y;

	// konstruktoren für position
	position() : X(0), Y(0) { } // standart konstruktor
	position(uint16_t x, uint8_t y) : X(x), Y(y) { }

	// vergleichs operatoren für position
	bool operator==(const position& pos2) const { return X == pos2.X && Y == pos2.Y; }
	bool operator!=(const position& pos2) const { return !(*this == pos2); }
};

enum gameID : uint8_t
{
	NONE,
	SNAKE,
	PONG,
	MINESWEEPER,
	TICTACTOE
};

void setup() {
	// zufallsgenerator Seed anhand unangeschlossenem Analogen Pin generieren
	randomSeed(analogRead(A1));

	// Schieberegister pin deklarieren
	pinMode(SR_LATCH, OUTPUT);
	pinMode(SR_DATA, INPUT);
	pinMode(SR_CLOCK, OUTPUT);

	// TFT Backlight PWM pin
	pinMode(TFT_BL, OUTPUT);

	// TFT Display vorbereiten
	tft.begin();
	tft.setRotation(3);
	tft.fillScreen(ILI9341_BLACK);
	analogWrite(TFT_BL, 255); // Bildschirm anschalten

	// Begrüßungsnachricht
#define SKIPGREETING false // Debug variable um die Begrüßungsnachricht zu überspringen
#if !SKIPGREETING
	greeting();
#endif
}

bool inGame = false;
uint8_t currentGame = NONE;

void loop() {
	// lese Tasten zustände aus dem Schieberegister und speichere sie in inputData
	handleInput();

	if (inGame) {
		playGame(currentGame);
	}
	else {
		currentGame = selectGame();
	}

	// kontrolliere die TFT backlight anhand des LDR pins
	backlightKontrollieren();
}

// Begrüßungsnachricht
void greeting() {
	tft.setTextSize(2);
	tft.setTextWrap(false);

	printCentered(F("PAR PROJEKT 2021/22")); // eigene funktion, die den Text in die mitte des Displays schreibt
	tft.setTextSize(1);
	printCentered(F("Leon Gies"), 0, 20); // überladung mit offset

	delay(4000);
	tft.fillScreen(ILI9341_BLACK);
}

bool dynamicBacklight = true;
void backlightKontrollieren() {
	if (!dynamicBacklight) {
		analogWrite(TFT_BL, 255);
		return;
	}

	int wert = analogRead(LDR_PIN) / 4; // analogWrite werte gehen nur von 0-255 während analogRead werte von 0-1023 gehen
	wert -= wert % 5; // erhöhe werte nur in fünfer schritte
	wert = min(max(wert, 20), 255); // kleinster wert ist 20, darunter passieren böse dinge; größer wert ist 255

	analogWrite(TFT_BL, wert);
}

uint8_t inputData = 0;
uint8_t prevInputData = 0;

void handleInput() {
	prevInputData = inputData;
	inputData = 0;

	// Lade Daten in SR_DATA
	digitalWrite(SR_LATCH, LOW); // aktiviert parallele inputs
	digitalWrite(SR_CLOCK, LOW); // startet clock pin low
	digitalWrite(SR_CLOCK, HIGH); // setzt clock pin high, daten werden in SR geladen
	digitalWrite(SR_LATCH, HIGH); // deaktiviert parallele inputs and aktiviert serielle output

	// laufe durch jeden Daten-Pin
	for (uint8_t j = 0; j < 8; j++) {
		uint8_t wert = digitalRead(SR_DATA); // wert der daten pins;

		if (wert) {
			// schiebe die eins so tief je nach dem welcher taster gedrückt wurde
			uint8_t schiebeBit = (B0000001 << j);
			// Kombiniere die verschobene eins mit dataIn
			inputData |= schiebeBit;
		}

		// gehe zum nächsten pin
		digitalWrite(SR_CLOCK, LOW);
		digitalWrite(SR_CLOCK, HIGH);
	}
}

uint8_t selectedGame = 0;
uint8_t selectGame() {
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
	tft.setTextColor(ILI9341_YELLOW);
	tft.println(F("Spieleliste"));

	tft.setTextSize(2);
	setCursorRelative(0, 10);

	for (int i = 0; i < 4; i++) {
		// wenn i das ausgewählte Spiel ist, setze die Farbe zu weiß, sonst: setze die Farbe zu grau
		tft.setTextColor(selectedGame == i ? ILI9341_WHITE : ILI9341_DARKGREY);

		tft.print((char)0xF8);
		setCursorRelative(5, 0);
		tft.println(spiele[i]);
	}

	if (INPUT_A_PRESSED) {
		inGame = true;

		tft.fillScreen(ILI9341_BLACK);

		setupGame(selectedGame + 1);

		return selectedGame + 1;
	}

	return NONE;
}

position foodPos;
#define FOODSIZE 10
#define HEAD_SIZE 20

void setupGame(const uint8_t game) {
	switch (game)
	{
	case gameID::SNAKE:
		foodPos.X = genRandPointCenteredOnGrid(HEAD_SIZE, 10, 310);
		foodPos.Y = genRandPointCenteredOnGrid(HEAD_SIZE, 20, 230);
		tft.fillRect(foodPos.X, foodPos.Y, FOODSIZE, FOODSIZE, ILI9341_RED); // zeiche essen

		// statische punke anzeige zeichnen
		tft.setTextSize(2);
		tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // Weiß mit schwarzem hintergrund
		tft.setCursor(0, 0);
		tft.print(F("PUNKTE: "));
		tft.setCursor(100, 0);
		tft.println(0);
		break;
	case gameID::PONG:
		
		break;
	case gameID::MINESWEEPER:
		
		break;
	case gameID::TICTACTOE:
		
		break;
	default:
		break;
	}
}

void playGame(const uint8_t game) {
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

#define MAX_SNAKE_LENGTH 100
position head = position(0, 20);
position body[MAX_SNAKE_LENGTH];
uint8_t length;

enum snakeDirection : uint8_t 
{
	UP, DOWN, LEFT, RIGHT
};
uint8_t direction = 3;

void playSnake() {
	// eingabe
	if (INPUT_UP_PRESSED && direction != DOWN) {
		direction = UP;
	}
	if (INPUT_DOWN_PRESSED && direction != UP) {
		direction = DOWN;
	}
	if (INPUT_LEFT_PRESSED && direction != RIGHT) {
		direction = LEFT;
	}
	if (INPUT_RIGHT_PRESSED && direction != LEFT) {
		direction = RIGHT;
	}

	// entferne das letzte Körpersegment vom Display
	tft.fillRect(body[length - 1].X, body[length - 1].Y, HEAD_SIZE, HEAD_SIZE, ILI9341_BLACK);

	// überprüfe ob essen im Kopf ist
	if (aabb(head.X, head.Y, foodPos.X, foodPos.Y, HEAD_SIZE, HEAD_SIZE, FOODSIZE, FOODSIZE)) {
		// verlängere die Schlange
		length++;

		// generiere neues essen an einer Zufälligen stelle, die nicht innerhalb der Schlange ist
		do {
			foodPos.X = genRandPointCenteredOnGrid(HEAD_SIZE, 10, 310);
			foodPos.Y = genRandPointCenteredOnGrid(HEAD_SIZE, 20, 230);
		} while (foodInsideSnake());

		tft.fillRect(foodPos.X, foodPos.Y, FOODSIZE, FOODSIZE, ILI9341_RED); // zeiche essen

		// zeichne den neuen Punktestand
		tft.setCursor(100, 0);
		tft.println(length * 100);
	}

	// körper position ändern
	for (int i = length - 1; i >= 0; i--)
	{
		if (i == 0) {
			body[i] = head;
		}
		else {
			body[i] = body[i - 1];
		}
	}

	position oldHead = { head.X, head.Y };
	// snake position ändern
	switch (direction)
	{
		case UP:
			head.Y -= HEAD_SIZE;
			break;
		case DOWN:
			head.Y += HEAD_SIZE;
			break;
		case LEFT:
			head.X -= HEAD_SIZE;
			break;
		case RIGHT:
			head.X += HEAD_SIZE;
			break;
		default:
			break;
	}

	// screenwrap
	if (head.X == 65536 - HEAD_SIZE) { // 65536 = 16 bit maximaler wert; nicht head.X < 0 weil X ein unsigned wert ist
		head.X = tft.width() - HEAD_SIZE;
	}
	if (head.X >= tft.width()) {
		head.X = 0;
	}
	if (head.Y < 20) {
		head.Y = tft.height() - HEAD_SIZE;
	}
	if (head.Y >= tft.height()) {
		head.Y = 20;
	}
	
	tft.fillRect(oldHead.X, oldHead.Y, HEAD_SIZE, HEAD_SIZE, ILI9341_BLACK);
	tft.fillRect(head.X, head.Y, HEAD_SIZE, HEAD_SIZE, ILI9341_DARKGREEN);

	if (length > 0) {
		tft.fillRect(body[0].X, body[0].Y, HEAD_SIZE, HEAD_SIZE, ILI9341_GREEN);
	}

	// Man hat verloren wenn sich der Kopf mit dem Körper trifft
	if (length > 0 && posarr_contains(body, length, head)) {
		resetSnake();
		printCentered(F("Verloren :("));
		delay(2000);
		zumHauptmenu();
	}

	// Das Spiel endet wenn man 10000 punkte erreicht hat
	if (length == 100) {
		resetSnake();
		printCentered(F("GEWONNEN"));
		delay(2000);
		zumHauptmenu();
	}

	delay(100);
}

// Erzeugt einen Punkt an einer Zufälligen position in der mitte einer Zelle
uint16_t genRandPointCenteredOnGrid(const uint16_t& cellSize, const uint16_t& min, const uint16_t& max) {
	uint16_t pos = random(min, max);
	return (pos / cellSize) * cellSize + cellSize / 4;
}

void resetSnake() {
	foodPos = {};
	length = 0;
	head = { 0, 20 };
	direction = 3;
	for (uint8_t i = 0; i < length; i++)
	{
		body[i] = {};
	}
}

bool foodInsideSnake() {
	if (head == foodPos)
		return true;

	for (uint8_t i = 0; i < length; i++) {
		if (body[i].X == (foodPos.X - 5) && body[i].Y == (foodPos.Y - 5)) {
			return true;
		}
	}
	return false;
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
	tft.getTextBounds(str, tft.width() / 2, tft.height()  / 2, &boundsX, &boundsY, &boundsW, &boundsH);
	tft.setCursor((tft.width() - boundsW) / 2, tft.height() / 2);
	tft.print(str);
}

// const <T>& um keine kopie von der variable machen
void printCentered(const __FlashStringHelper* str, const int16_t& offsetX, const int8_t& offsetY) {
	int16_t boundsX, boundsY;
	uint16_t boundsW, boundsH;
	tft.getTextBounds(str, tft.width() / 2, tft.height() / 2, &boundsX, &boundsY, &boundsW, &boundsH);
	tft.setCursor((tft.width() - boundsW) / 2 + offsetX, tft.height() / 2 + offsetY);
	tft.print(str);
}

void setCursorRelative(const int16_t& x, const int8_t& y) {
	tft.setCursor(tft.getCursorX() + x, tft.getCursorY() + y);
}

// AABB recteck zu rechteck collisions erkennung
bool aabb(const uint16_t& x1, const uint8_t& y1, const uint16_t& x2, const uint8_t& y2,
	const uint16_t& w1, const uint8_t& h1, const uint16_t& w2, const uint8_t& h2) {
	return x1 < x2 + w2 &&
		x1 + w1 > x2 &&
		y1 < y2 + h2 &&
		y1 + h1 > y2;
}

bool posarr_contains(position arr[], const uint8_t size, const position& elm) {
	for (uint8_t i = 0; i < size; i++) {
		if (arr[i] == elm) {
			return true;
		}
	}
	return false;
}

void zumHauptmenu() {
	tft.fillScreen(ILI9341_BLACK);
	inGame = false;
	currentGame = NONE;
}