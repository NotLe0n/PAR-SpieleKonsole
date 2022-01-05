#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_SPITFT.h>
#include <SPI.h>

// Pins für den SchiebeRegister
#define SR_DATA 2
#define SR_LATCH 3
#define SR_CLOCK 4

// Pins für den TFT Display
#define TFT_DC 8 // SPI Data
#define TFT_CS 10 // Chip select
#define TFT_BL 9 // Backlight

// LDR Pin
#define LDR_PIN A0

#pragma region InputHeaders
#define INPUT_UP_PRESSED (inputData & B10000000)
#define INPUT_DOWN_PRESSED (inputData & B01000000)
#define INPUT_LEFT_PRESSED (inputData & B00100000)
#define INPUT_RIGHT_PRESSED (inputData & B00010000)
#define INPUT_A_PRESSED (inputData & B00001000)
#define INPUT_B_PRESSED (inputData & B00000100)

#define PREV_INPUT_UP_PRESSED (prevInputData & B10000000)
#define PREV_INPUT_DOWN_PRESSED (prevInputData & B01000000)
#define PREV_INPUT_LEFT_PRESSED (prevInputData & B00100000)
#define PREV_INPUT_RIGHT_PRESSED (prevInputData & B00010000)
#define PREV_INPUT_A_PRESSED (prevInputData & B00001000)
#define PREV_INPUT_B_PRESSED (prevInputData & B00000100)
#pragma endregion

// Variable zur steuerung des TFT Displays
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Eine Strukur zum speichern von Positionen mit einer X und Y koordinate
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

	delay(4000); // warte 4s
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
		selectedGame = uint8_t(--selectedGame) % 4; // verrigere den wert um 1. Halte den wert zwischen 0 und 4
	}
	if (INPUT_DOWN_PRESSED && !PREV_INPUT_DOWN_PRESSED) {
		selectedGame = ++selectedGame % 4; // erhöhe den wert um 1. Halte den wert zwischen 0 und 4. Wenn der wert 5 ist wird er zu 0 gesetzt
	}

	// Alle Spiele
	const String spiele[] = { "Snake", "Pong", "Minesweeper", "Tic Tac Toe" };

	// schreibe "Spieleliste"
	tft.setCursor(0, 0);
	tft.setTextSize(3);
	tft.setTextColor(ILI9341_YELLOW);
	tft.println(F("Spieleliste"));

	// schreibe alle Spiele
	tft.setTextSize(2);
	setCursorRelative(0, 10);

	for (int i = 0; i < 4; i++) {
		// wenn i das ausgewählte Spiel ist, setze die Farbe zu weiß, sonst: setze die Farbe zu grau
		tft.setTextColor(selectedGame == i ? ILI9341_WHITE : ILI9341_DARKGREY);

		tft.print((char)0xF8);
		setCursorRelative(5, 0);
		tft.println(spiele[i]);
	}

	// Wenn 'A' gedrückt wurde
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
		// generiere neues essen an einer Zufälligen stelle
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

#pragma region Snake

#define MAX_SNAKE_LENGTH 100
#define SNAKE_SPEED 180
position head = position(0, 20);
position body[MAX_SNAKE_LENGTH];
uint8_t snakeLength;

enum snakeDirection : uint8_t 
{
	UP, DOWN, LEFT, RIGHT
};
uint8_t direction = 3;

void playSnake() {
	// eingabe
	// Die Schlange darf keine 180° Umdrehung machen
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

	// takt abstand
	// kein delay() weil sonst Eingaben überspringen werden können
	if (millis() % SNAKE_SPEED != 0)
		return;

	// entferne das letzte Körpersegment vom Display
	tft.fillRect(body[snakeLength - 1].X, body[snakeLength - 1].Y, HEAD_SIZE, HEAD_SIZE, ILI9341_BLACK);

	// überprüfe ob essen im Kopf ist
	if (foodInsideSnake()) {
		// verlängere die Schlange
		snakeLength++;

		// generiere neues essen an einer Zufälligen stelle, die nicht innerhalb der Schlange ist
		do {
			foodPos.X = genRandPointCenteredOnGrid(HEAD_SIZE, 10, 310);
			foodPos.Y = genRandPointCenteredOnGrid(HEAD_SIZE, 20, 230);
		} while (foodInsideSnake());

		tft.fillRect(foodPos.X, foodPos.Y, FOODSIZE, FOODSIZE, ILI9341_RED); // zeiche essen

		// zeichne den neuen Punktestand
		tft.setCursor(100, 0);
		tft.println(snakeLength * 100);
	}

	// körper position ändern
	for (int i = snakeLength - 1; i >= 0; i--)
	{
		if (i == 0) {
			// das erste Körper segment wird an die Stelle des Kopfes bewegt
			body[i] = head;
		}
		else {
			// Alle anderen segmente bewegen sich an die Stelle ihres vorderen
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
	// 65536 = 16 bit maximaler wert; nicht head.X < 0 weil X ein unsigned wert ist und X zu dem maximalen wert underflowed
	if (head.X == 65536 - HEAD_SIZE) { // linker rand
		head.X = tft.width() - HEAD_SIZE;
	}
	if (head.X >= tft.width()) {  // rechter Rand
		head.X = 0;
	}
	if (head.Y < 20) { // oberer Rand
		head.Y = tft.height() - HEAD_SIZE; 
	}
	if (head.Y >= tft.height()) { // unterer Rand
		head.Y = 20;
	}
	
	// Kopf neu zeichnen
	tft.fillRect(oldHead.X, oldHead.Y, HEAD_SIZE, HEAD_SIZE, ILI9341_BLACK);
	tft.fillRect(head.X, head.Y, HEAD_SIZE, HEAD_SIZE, ILI9341_DARKGREEN);

	// Erste Körper stelle Zeichnen
	if (snakeLength > 0) {
		tft.fillRect(body[0].X, body[0].Y, HEAD_SIZE, HEAD_SIZE, ILI9341_GREEN);
	}

	// Man hat verloren wenn sich der Kopf mit dem Körper trifft
	if (snakeLength > 0 && posarr_contains(body, snakeLength, head)) {
		resetSnake();

		tft.setTextColor(ILI9341_WHITE);
		printCentered(F("Verloren :("));

		delay(2000);
		zumHauptmenu();
	}

	// Das Spiel endet wenn man 10000 punkte erreicht hat
	if (snakeLength == 100) {
		resetSnake();

		tft.setTextColor(ILI9341_WHITE);
		printCentered(F("GEWONNEN"));

		delay(2000);
		zumHauptmenu();
	}
}

// Erzeugt einen Punkt an einer Zufälligen position in der mitte einer Zelle
uint16_t genRandPointCenteredOnGrid(const uint16_t& cellSize, const uint16_t& min, const uint16_t& max) {
	uint16_t pos = random(min, max); // zufällige position erzeugen
	return (pos / cellSize) * cellSize + cellSize / 4; // zentrieren und zurückgeben
}

// setzt alle Variablen zu ihrem Standartwert
void resetSnake() {
	foodPos = {};
	snakeLength = 0;
	head = { 0, 20 };
	direction = 3;
	for (uint8_t i = 0; i < snakeLength; i++)
	{
		body[i] = {};
	}
}

// Überprüft ob essen innerhalb der Schlange ist
bool foodInsideSnake() {
	// ist das Essen im Kopf der Schlange?
	if (head.X == (foodPos.X - 5) && head.Y == (foodPos.Y - 5)) // (foodPos.X - 5) weil foodPos am gitter zentriert ist
		return true;

	// ist das Essen im Körper der Schlange?
	for (uint8_t i = 0; i < snakeLength; i++) {
		if (body[i].X == (foodPos.X - 5) && body[i].Y == (foodPos.Y - 5)) {
			return true;
		}
	}
	return false;
}

#pragma endregion

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
	tft.setCursor((tft.width() - boundsW) / 2, (tft.height() - boundsH) / 2);
	tft.print(str);
}

// const <T>& um keine kopie von der variable machen
void printCentered(const __FlashStringHelper* str, const int16_t& offsetX, const int8_t& offsetY) {
	int16_t boundsX, boundsY;
	uint16_t boundsW, boundsH;
	tft.getTextBounds(str, tft.width() / 2, tft.height() / 2, &boundsX, &boundsY, &boundsW, &boundsH);
	tft.setCursor((tft.width() - boundsW) / 2 + offsetX, (tft.height() - boundsH) / 2 + offsetY);
	tft.print(str);
}

void setCursorRelative(const int16_t& x, const int8_t& y) {
	tft.setCursor(tft.getCursorX() + x, tft.getCursorY() + y);
}

// gibt true zurück wenn 'elm' in 'arr' vorhanden ist
bool posarr_contains(position arr[], const uint8_t size, const position& elm) {
	for (uint8_t i = 0; i < size; i++) {
		if (arr[i] == elm) {
			return true;
		}
	}
	return false;
}

// Springt zurück zum Spiel Auswahl menü
void zumHauptmenu() {
	tft.fillScreen(ILI9341_BLACK);
	inGame = false;
	currentGame = NONE;
}