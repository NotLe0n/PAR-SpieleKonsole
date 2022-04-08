#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_SPITFT.h>
#include <SPI.h>

// Pins für den SchiebeRegister
#define SR_LATCH 3
#define SR_DATA 4
#define SR_CLOCK 5

// Pins für den TFT Display
#define TFT_DC 8 // SPI Data
#define TFT_CS 10 // Chip select
#define TFT_BL 9 // Backlight

// LDR Pin
#define LDR_PIN A0

// Interrupt Pin
#define INT_PIN 2

#pragma region InputDefines
#define INPUT_CHANGED (prevInputData != inputData)
#define INPUT_UP_PRESSED (inputData & 0b00000100)
#define INPUT_DOWN_PRESSED (inputData & 0b00000010)
#define INPUT_LEFT_PRESSED (inputData & 0b00000001)
#define INPUT_RIGHT_PRESSED (inputData & 0b00001000)
#define INPUT_A_PRESSED (inputData & 0b00010000)
#define INPUT_B_PRESSED (inputData & 0b00100000)

#define PREV_INPUT_UP_PRESSED (prevInputData & 0b00000100)
#define PREV_INPUT_DOWN_PRESSED (prevInputData & 0b00000010)
#define PREV_INPUT_LEFT_PRESSED (prevInputData & 0b00000001)
#define PREV_INPUT_RIGHT_PRESSED (prevInputData & 0b00001000)
#define PREV_INPUT_A_PRESSED (prevInputData & 0b00010000)
#define PREV_INPUT_B_PRESSED (prevInputData & 0b00100000)
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
#define SKIPGREETING true // Debug variable um die Begrüßungsnachricht zu überspringen
#if !SKIPGREETING
	greeting();
#endif

	// Interrupt auf den Pin festlegen
	attachInterrupt(digitalPinToInterrupt(INT_PIN), zumHauptmenu, FALLING);
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
	tft.setTextWrap(false); // Text soll nicht auf die nächste zeile springen

	printCentered(F("PAR PROJEKT 2021/22")); // eigene funktion, die den Text in die mitte des Displays schreibt
	tft.setTextSize(1);
	printCentered(F("Leon Gies"), 0, 20); // überladung mit offset

	delay(4000); // warte 4s
	tft.fillScreen(ILI9341_BLACK); // Bildschirm schwarz machen
}

bool dynamicBacklight = true; // variable um die dynamische Hintergrundbeleuchtung auszuschalten
void backlightKontrollieren() {
	// wenn die dynamische Hintergrundbeleuchtung ausgeschaltet ist, wird die Backlight auf volle Helligkeit gestellt
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
			uint8_t schiebeBit = (B00000001 << j);
			// Kombiniere die verschobene eins mit dataIn
			inputData |= schiebeBit;
		}

		// gehe zum nächsten pin
		digitalWrite(SR_CLOCK, LOW);
		digitalWrite(SR_CLOCK, HIGH);
	}
}

uint8_t selectedGame = 0; // welches Spiel Ausgewählt wurde

// Wird jeden Loop() durchlauf aufgerufen wenn inGame false ist
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

		tft.print((char)0xF8); // Punkt zeichen
		setCursorRelative(5, 0); // Cursor um 5 nach unten verschieben
		tft.println(spiele[i]); // Schreibe das Spiel
	}

	// Wenn 'A' gedrückt wurde
	if (INPUT_A_PRESSED || INPUT_B_PRESSED) {
		inGame = true;

		tft.fillScreen(ILI9341_BLACK); // Bildschirm schwarz machen

		setupGame(selectedGame + 1); // setup game aufrufen (plus 1 weil arrays von 0 anfangen)

		return selectedGame + 1;
	}

	return NONE;
}

// wird aufgerufen wenn ein Spiel in selectGame() ausgewählt wurde
void setupGame(const uint8_t game) {
	switch (game)
	{
	case gameID::SNAKE:
		setupSnake();
		break;
	case gameID::PONG:
		setupPong();
		break;
	case gameID::MINESWEEPER:
		setupMinesweeper();
		break;
	case gameID::TICTACTOE:
		setupTTT();
		break;
	default:
		break;
	}
}

// Wird jeden Loop() durchlauf aufgerufen wenn inGame true ist
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

// "#pragma region" für Visual Studio IDE, ändert nichts an der funktionsweise
#pragma region Snake

#define MAX_SNAKE_LENGTH 100 // die Maximale Länge der Schlange
#define SNAKE_SPEED 180 // die Geschwindigkeit der Schlange
position snakeHead; // die Kopfposition der Schlange
position snakeBody[MAX_SNAKE_LENGTH]; // die Position aller Körperteile der Schlange
uint8_t snakeLength; // die aktuelle Länge der schlange

enum snakeDirection : uint8_t 
{
	UP, DOWN, LEFT, RIGHT
};
uint8_t snakeDir; // die Richtung der Schlange
position foodPos; // Position vom Futter
#define FOODSIZE 10 // Größe vom Futter
#define HEAD_SIZE 20 // Größe vom Kopf

void setupSnake() {
	snakeHead = position(0, 20);
	snakeDir = 3;
	snakeLength = 0;
	for (uint8_t i = 0; i < snakeLength; i++)
	{
		snakeBody[i] = {};
	}

	// statische punke anzeige zeichnen
	tft.setTextSize(2);
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // Weiß mit schwarzem hintergrund
	tft.setCursor(0, 0);
	tft.print(F("PUNKTE: "));
	tft.setCursor(100, 0);
	tft.println(0);

	// generiere neues essen an einer Zufälligen stelle
	foodPos.X = genRandPointCenteredOnGrid(HEAD_SIZE, 10, 310);
	foodPos.Y = genRandPointCenteredOnGrid(HEAD_SIZE, 20, 230);
	tft.fillRect(foodPos.X, foodPos.Y, FOODSIZE, FOODSIZE, ILI9341_RED); // zeiche essen
}

void playSnake() {
	// eingabe
	// Die Schlange darf keine 180° Umdrehung machen
	if (INPUT_UP_PRESSED && snakeDir != DOWN) {
		snakeDir = UP;
	}
	if (INPUT_DOWN_PRESSED && snakeDir != UP) {
		snakeDir = DOWN;
	}
	if (INPUT_LEFT_PRESSED && snakeDir != RIGHT) {
		snakeDir = LEFT;
	}
	if (INPUT_RIGHT_PRESSED && snakeDir != LEFT) {
		snakeDir = RIGHT;
	}

	// takt abstand
	// kein delay() weil sonst Eingaben überspringen werden können
	if (millis() % SNAKE_SPEED != 0)
		return;

	// entferne das letzte Körpersegment vom Display
	if (snakeLength > 0) {
		tft.fillRect(snakeBody[snakeLength - 1].X, snakeBody[snakeLength - 1].Y, HEAD_SIZE, HEAD_SIZE, ILI9341_BLACK);
	}

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
			snakeBody[i] = snakeHead;
		}
		else {
			// Alle anderen segmente bewegen sich an die Stelle ihres vorderen
			snakeBody[i] = snakeBody[i - 1];
		}
	}

	position oldHead = { snakeHead.X, snakeHead.Y };
	// snake position ändern
	switch (snakeDir)
	{
		case UP:
			snakeHead.Y -= HEAD_SIZE;
			break;
		case DOWN:
			snakeHead.Y += HEAD_SIZE;
			break;
		case LEFT:
			snakeHead.X -= HEAD_SIZE;
			break;
		case RIGHT:
			snakeHead.X += HEAD_SIZE;
			break;
		default:
			break;
	}

	// screenwrap
	// 65536 = 16 bit maximaler wert; nicht snakeHead.X < 0 weil X ein unsigned wert ist und X zu dem maximalen wert underflowed
	if (snakeHead.X == 65536 - HEAD_SIZE) { // linker rand
		snakeHead.X = tft.width() - HEAD_SIZE;
	}
	if (snakeHead.X >= tft.width()) {  // rechter Rand
		snakeHead.X = 0;
	}
	if (snakeHead.Y < 20) { // oberer Rand
		snakeHead.Y = tft.height() - HEAD_SIZE; 
	}
	if (snakeHead.Y >= tft.height()) { // unterer Rand
		snakeHead.Y = 20;
	}

	// Kopf neu zeichnen
	tft.fillRect(oldHead.X, oldHead.Y, HEAD_SIZE, HEAD_SIZE, ILI9341_BLACK);
	tft.fillRect(snakeHead.X, snakeHead.Y, HEAD_SIZE, HEAD_SIZE, ILI9341_DARKGREEN);

	// Erste Körper stelle Zeichnen
	if (snakeLength > 0) {
		tft.fillRect(snakeBody[0].X, snakeBody[0].Y, HEAD_SIZE, HEAD_SIZE, ILI9341_GREEN);
	}

	// Man hat verloren wenn sich der Kopf mit dem Körper trifft
	if (snakeLength > 0 && posarr_contains(snakeBody, snakeLength, snakeHead)) {
		tft.setTextColor(ILI9341_WHITE);
		printCentered(F("Verloren :("));

		delay(2000);
		zumHauptmenu();
	}

	// Das Spiel endet wenn man 10000 punkte erreicht hat
	if (snakeLength == 100) {
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

// Überprüft ob essen innerhalb der Schlange ist
bool foodInsideSnake() {
	// ist das Essen im Kopf der Schlange?
	if (snakeHead.X == (foodPos.X - 5) && snakeHead.Y == (foodPos.Y - 5)) // (foodPos.X - 5) weil foodPos am gitter zentriert ist
		return true;

	// ist das Essen im Körper der Schlange?
	for (uint8_t i = 0; i < snakeLength; i++) {
		if (snakeBody[i].X == (foodPos.X - 5) && snakeBody[i].Y == (foodPos.Y - 5)) {
			return true;
		}
	}
	return false;
}

#pragma endregion

#pragma region Pong

position P_ballPos; // position vom Ball
int8_t P_ballVelocityX; // Horizontale geschwindigkeit/richtung vom Ball
int8_t P_ballVelocityY; // Vertikale geschwindigkeit/richtung vom Ball
uint8_t P_paddle1Y; // Y Koordinate vom 1. Schläger
uint8_t P_paddle2Y; // Y Koordinate vom 2. Schläger
uint8_t P_points; // Zehner stelle = spieler1, Einser Stelle = spieler2 :: Beispiel: 12 = 1 : 2
#define P_paddleHeight 40 // Länge der Schläger
#define P_paddleWidth 5 // Breite der Schläger
#define P_ballSize 10 // Größe vom Ball
#define P_maxPoints 3 // maximale anzahl an punken

void setupPong() {
	P_ballPos = { ILI9341_TFTHEIGHT / 2, ILI9341_TFTWIDTH / 2 };
	P_ballVelocityX = random(-1, 1) ? 1 : -1; // 50% chance dass es eine -1 ist
	P_ballVelocityY = random(-1, 1) ? 1 : -1; // 50% chance dass es eine -1 ist
	P_paddle1Y = 80;
	P_paddle2Y = 80;
	P_points = 00;

	// Zeichne Schläger und den Ball
	tft.drawRect(10, P_paddle1Y, P_paddleWidth, P_paddleHeight, ILI9341_RED);
	tft.drawRect(tft.width() - P_paddleWidth - 10, P_paddle2Y, P_paddleWidth, P_paddleHeight, ILI9341_BLUE);
	tft.fillRect(P_ballPos.X, P_ballPos.Y, P_ballSize, P_ballSize, ILI9341_WHITE);

	// punkestand einstellungen
	tft.setTextWrap(false);
	tft.setTextSize(4);
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
}

void playPong() {
	if (millis() % 10 != 0)
		return;

	// HOCH und UNTEN bewegen den 1. Schläger
	if (INPUT_UP_PRESSED) {
		P_movePaddle1(-1);
	}
	if (INPUT_DOWN_PRESSED) {
		P_movePaddle1(1);
	}

	// A und B bewegen den 2. Schläger
	if (INPUT_A_PRESSED) {
		P_movePaddle2(-1);
	}
	if (INPUT_B_PRESSED) {
		P_movePaddle2(1);
	}

	P_moveBall(); // bewege und zeichne den Ball

	// falls der schläger am rand ankommt, bekommt der Spieler auf der anderen seite einen punkt
	if (P_ballPos.X < 5) {
		P_points++;

		// bei drei punkten wird das Spiel beendet
		if (P_points % 10 == P_maxPoints) {
			tft.setTextSize(2);
			printCentered(F("Spieler 1 hat gewonnen!"));

			delay(1000);
			zumHauptmenu();
			return;
		}

		P_reset();
	}
	if (P_ballPos.X > tft.width() - 5 - P_ballSize) {
		P_points += 10;

		// bei drei punkten wird das Spiel beendet
		if (P_points / 10 == P_maxPoints) {
			tft.setTextSize(2);
			printCentered(F("Spieler 2 hat gewonnen!"));

			delay(1000);
			zumHauptmenu();
			return;
		}

		P_reset();
	}
}

// ändert die Y position des 1. Schlägers um den gegebenen Wert
void P_movePaddle1(uint8_t amount) {
	// falls schläger außerhalb des Bildschirmes geht, nicht bewegen
	if (P_paddle1Y + amount == 255 || P_paddle1Y + amount == tft.height() - P_paddleHeight) {
		return;
	}

	tft.drawRect(10, P_paddle1Y, P_paddleWidth, P_paddleHeight, ILI9341_BLACK); // schläger vom Display entfernen

	P_paddle1Y += amount; // schläger bewegen

	tft.drawRect(10, P_paddle1Y, P_paddleWidth, P_paddleHeight, ILI9341_RED); // schläger wieder neu zeichnen
}

// ändert die Y Position des 2. Schlägers um den gegebenen Wert
void P_movePaddle2(uint8_t amount) {
	// falls schläger außerhalb des Bildschirmes ist, nicht bewegen
	if (P_paddle2Y + amount == 255 || P_paddle2Y + amount == tft.height() - P_paddleHeight) {
		return;
	}

	tft.drawRect(tft.width() - P_paddleWidth - 10, P_paddle2Y, P_paddleWidth, P_paddleHeight, ILI9341_BLACK);

	P_paddle2Y += amount; // schläger bewegen

	tft.drawRect(tft.width() - P_paddleWidth - 10, P_paddle2Y, P_paddleWidth, P_paddleHeight, ILI9341_BLUE); // schläger wieder neu zeichnen
}

void P_moveBall() {
	tft.fillRect(P_ballPos.X, P_ballPos.Y, P_ballSize, P_ballSize, ILI9341_BLACK); // ball vom Display entfernen

	// wenn der ball den oberen oder den unteren rand trifft
	if (P_ballPos.Y == 0 || P_ballPos.Y == tft.height() - P_ballSize - 1) {
		P_ballVelocityY = -P_ballVelocityY; // spiegle die Y geschwindigkeit
	}

	// wenn der linke Rand des Balls hinter dem Schläger ist und der Ball zwischen dem oberen und unteren Rand des Schlägers ist, wurde der 2. Schläger getroffen
	const bool hitPaddle1 = 
		P_ballPos.X <= 10 + P_paddleWidth && 
		(P_ballPos.Y + P_ballSize >= P_paddle1Y && P_ballPos.Y <= P_paddle1Y + P_paddleHeight);

	// wenn der rechte Rand des Balls hinter dem Rechten schläger ist und der Ball zwischen dem oberen und unteren Rand des Schlägers ist, wurde der 2. Schläger getroffen
	const bool hitPaddle2 =
		(P_ballPos.X + P_ballSize >= tft.width() - P_paddleWidth - 10 - 1) &&
		(P_ballPos.Y + P_ballSize >= P_paddle2Y && P_ballPos.Y <= P_paddle2Y + P_paddleHeight);

	// wenn der ball einer der Schläger trifft
	if (hitPaddle1)
	{
		P_ballPos.X = 10 + P_paddleWidth; // setze die Position zum rechten rand des 1. Schlägers, sodass der Ball nicht stecken bleibt
		P_ballVelocityX = -P_ballVelocityX; // spiegle die X geschwindigkeit
	}
	if (hitPaddle2) {
		P_ballPos.X = tft.width() - P_paddleWidth - 10 - P_ballSize - 1; // setze die Position zum linken rand des 2. Schlägers, sodass der Ball nicht stecken bleibt
		P_ballVelocityX = -P_ballVelocityX; // spiegle die X geschwindigkeit
	}

	// ändere die Position des balls
	P_ballPos.X += P_ballVelocityX;
	P_ballPos.Y += P_ballVelocityY;

	tft.fillRect(P_ballPos.X, P_ballPos.Y, P_ballSize, P_ballSize, ILI9341_WHITE); // ball neu zeichnen
}

void P_reset() {
	tft.fillScreen(ILI9341_BLACK);

	P_ballPos = { ILI9341_TFTHEIGHT / 2, ILI9341_TFTWIDTH / 2 };
	P_ballVelocityX = random(-1, 1) ? 1 : -1; // 50% chance dass es eine -1 ist
	P_ballVelocityY = random(-1, 1) ? 1 : -1; // 50% chance dass es eine -1 ist
	P_paddle1Y = 80;
	P_paddle2Y = 80;

	tft.setCursor(100, 100);
	tft.setTextSize(4);
	tft.print(P_points / 10);
	tft.print( + " : ");
	tft.print(P_points % 10);

	delay(500);
	tft.fillScreen(ILI9341_BLACK);

	tft.drawRect(10, P_paddle1Y, P_paddleWidth, P_paddleHeight, ILI9341_RED);
	tft.drawRect(tft.width() - P_paddleWidth - 10, P_paddle2Y, P_paddleWidth, P_paddleHeight, ILI9341_BLUE);
	tft.fillRect(P_ballPos.X, P_ballPos.Y, P_ballSize, P_ballSize, ILI9341_WHITE);
}

#pragma endregion

#pragma region Minesweeper

uint16_t time;
#define MS_XOFFSET 40
#define MS_YOFFSET 20

typedef uint8_t cell; // 0000 00<F><M><R> // F=Flagged, M=Mine, R=Revealed
#define MS_CELLSIZE 20 // wie groß die Felder gezeichnet werden
#define MS_FIELD_SIZE 11 // wie viele Felder in beide richtungen
#define CELLSTATE_REVEALED 0b001
#define CELLSTATE_MINE 0b010
#define CELLSTATE_FLAGGED 0b100
#define MS_getIndex(x, y) ((x) + MS_FIELD_SIZE * (y)) // makro um den index zu bekommen. Keine funktion: für effizienz
cell cells[MS_FIELD_SIZE * MS_FIELD_SIZE]; // Minesweeper Zellen
bool firstClick;
uint8_t MS_cursorPosX; // X Position vom Minesweeper Cursor
uint8_t MS_cursorPosY; // Y Position vom Minesweeper Cursor
#define MS_MINECOUNT 16

void setupMinesweeper() {
	firstClick = true;
	MS_cursorPosX = 5;
	MS_cursorPosY = 5;

	tft.setCursor(0, 0);
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.setTextSize(2);
	tft.print(F("ZEIT: 00:00"));

	tft.setCursor(150, 0);
	tft.print(F("MIENEN: "));
	tft.print(MS_MINECOUNT);

	for (uint8_t i = 0; i < MS_FIELD_SIZE * MS_FIELD_SIZE; i++) {
		cells[i] = 0;
	}

	drawCells();
}

void playMinesweeper() {
	// sobalt das erste feld aufgedeckt wurde wird jede sekunde die Variable "time" ausgegeben und erhöht
	if (!firstClick && millis() % 1000 == 0) {
		tft.setCursor(70, 0);
		char str[16]; // text als zeichen array
		sprintf(str, "%02u:%02u", time / 60, time % 60); // formatiere Zeit als Mm:Ss
		tft.print(str);
		time++; // zeit erhöhen
	}

	if (INPUT_CHANGED) {
		// Zeichne den roten rand
		tft.drawRect(MS_XOFFSET, MS_YOFFSET, MS_FIELD_SIZE * MS_CELLSIZE, MS_FIELD_SIZE * MS_CELLSIZE, ILI9341_RED);
		// Entferne den Zeiger
		MineSweeper_drawCell(cells[MS_getIndex(MS_cursorPosX, MS_cursorPosY)], MS_cursorPosX, MS_cursorPosY);
	}

	// ändere Zeigerposition
	if (INPUT_UP_PRESSED && !PREV_INPUT_UP_PRESSED) {
		MS_cursorPosY--;
	}
	if (INPUT_DOWN_PRESSED && !PREV_INPUT_DOWN_PRESSED) {
		MS_cursorPosY++;
	}
	if (INPUT_LEFT_PRESSED && !PREV_INPUT_LEFT_PRESSED) {
		MS_cursorPosX--;
	}
	if (INPUT_RIGHT_PRESSED && !PREV_INPUT_RIGHT_PRESSED) {
		MS_cursorPosX++;
	}

	// screenwrap
	if (MS_cursorPosX == 255) { // linker rand
		MS_cursorPosX = MS_FIELD_SIZE - 1;
	}
	if (MS_cursorPosX >= MS_FIELD_SIZE) {  // rechter Rand
		MS_cursorPosX = 0;
	}
	if (MS_cursorPosY == 255) { // oberer Rand
		MS_cursorPosY = MS_FIELD_SIZE - 1;
	}
	if (MS_cursorPosY >= MS_FIELD_SIZE) { // unterer Rand
		MS_cursorPosY = 0;
	}

	if (INPUT_A_PRESSED) {
		// Erstelle ein feld wenn A zum ersten mal gedrückt wurde
		if (firstClick) {
			generateField();
		}

		// Das Spiel endet wenn auf einer Miene A gedrückt wurde
		if (cells[MS_getIndex(MS_cursorPosX, MS_cursorPosY)] & CELLSTATE_MINE) {
			tft.setTextColor(ILI9341_WHITE);
			printCentered(F("Verloren :("));

			delay(2000);
			zumHauptmenu();
			return;	
		}

		if (!(cells[MS_getIndex(MS_cursorPosX, MS_cursorPosY)] & CELLSTATE_REVEALED)) {
			cells[MS_getIndex(MS_cursorPosX, MS_cursorPosY)] |= CELLSTATE_REVEALED;
			revealNeighbors(MS_cursorPosX, MS_cursorPosY);
		}

		firstClick = false;
		drawCells();

		if (MS_checkWin()) {
			tft.setTextColor(ILI9341_WHITE);
			printCentered(F("GEWONNEN"));

			delay(2000);
			zumHauptmenu();
			return;
		}

	}

	if (INPUT_B_PRESSED && !PREV_INPUT_B_PRESSED) {
		cells[MS_getIndex(MS_cursorPosX, MS_cursorPosY)] ^= CELLSTATE_FLAGGED; // XOR = toggeln
	}

	// Zeichne den Zeiger an der neuen position wenn eine Taste losgelassen wurde
	if (INPUT_CHANGED) {
		tft.drawRect(MS_cursorPosX * MS_CELLSIZE + MS_XOFFSET, MS_cursorPosY * MS_CELLSIZE + MS_YOFFSET, MS_CELLSIZE, MS_CELLSIZE, ILI9341_WHITE);
	}
}

// generiert ein zufälliges mienenfeld
void generateField() {
	do {
		for (uint8_t i = 0; i < MS_FIELD_SIZE * MS_FIELD_SIZE; i++) {
			cells[i] = 0;
		}
		for (uint8_t k = 0; k < MS_MINECOUNT; k++) {
			uint8_t ri = random(0, MS_FIELD_SIZE * MS_FIELD_SIZE - 1);
			cells[ri] = CELLSTATE_MINE;
		}
	} while (countNearbyMines(MS_cursorPosX, MS_cursorPosY) != 0); // erstelle solange ein neues feld bis die Zelle an der position des Zeigers frei ist
}


// Zeigt Rekursiv alle Nachbarn einer Zelle
void revealNeighbors(const uint8_t& posX, const uint8_t& posY) {
	Serial.print(RAMEND - SP);
	Serial.print(", ");

	// zeige alle Zellen direkt neben der Zelle
	for (int8_t x = -1; x <= 1; x++) {
		for (int8_t y = -1; y <= 1; y++) {
			// überspringe wenn die Koordinate außerhalb des Feldes ist.
			if (posX + x < 0 || posY + y < 0 || posX + x > MS_FIELD_SIZE - 1 || posY + y > MS_FIELD_SIZE - 1)
				continue;

			// Zeige keine Mienen und die eigene Zelle
			if ((cells[MS_getIndex(posX + x, posY + y)] & CELLSTATE_MINE) != 0 || (cells[MS_getIndex(posX + x, posY + y)] & CELLSTATE_REVEALED) != 0 || (x == 0 && y == 0))
				continue;
			
			if (countNearbyMines(posX, posY) == 0)
			{
				cells[MS_getIndex(posX + x, posY + y)] |= CELLSTATE_REVEALED;;
				revealNeighbors(posX + x, posY + y); // rekursion
				
			}
		}
	}
}

// Zählt wie viele Mienen um eine Zelle sind
uint8_t countNearbyMines(const uint8_t& posX, const uint8_t& posY) {
	uint8_t count = 0;

	// überprüfe alle felder direkt neben der Zelle
	for (int8_t x = -1; x <= 1; x++) {
		for (int8_t y = -1; y <= 1; y++) {
			// überspringe wenn die Koordinate außerhalb des Feldes ist.
			if (posX + x < 0 || posY + y < 0 || posX + x > MS_FIELD_SIZE - 1 || posY + y > MS_FIELD_SIZE - 1)
				continue;

			// ist die Zelle an der Stelle eine miene?
			if (cells[MS_getIndex(posX + x, posY + y)] & CELLSTATE_MINE) {
				count++;
			}
		}
	}
	return count;
}

// zeichnet alle Zellen
void drawCells() {
	for (uint8_t y = 0; y < MS_FIELD_SIZE; y++) {
		for (uint8_t x = 0; x < MS_FIELD_SIZE; x++) {
			MineSweeper_drawCell(cells[MS_getIndex(x, y)], x, y);
		}
	}
}

// Zeichnet eine zelle
void MineSweeper_drawCell(const uint8_t& cl, const uint8_t& x, const uint8_t& y) {
	if (cl & CELLSTATE_REVEALED) {
		tft.fillRect(x * MS_CELLSIZE + MS_XOFFSET, y * MS_CELLSIZE + MS_YOFFSET, MS_CELLSIZE, MS_CELLSIZE, ILI9341_BLACK);

		if (!(cl & CELLSTATE_FLAGGED)) {
			// Zeichnet die Box der Zelle
			if (y - 1 != 255 && !(cells[MS_getIndex(x, y - 1)] & CELLSTATE_REVEALED))
				tft.drawFastHLine(x * MS_CELLSIZE + MS_XOFFSET, y * MS_CELLSIZE + MS_YOFFSET, MS_CELLSIZE, ILI9341_RED);
			if (y + 1 >= 0 && !(cells[MS_getIndex(x, y + 1)] & CELLSTATE_REVEALED))
				tft.drawFastHLine(x * MS_CELLSIZE + MS_XOFFSET, y * MS_CELLSIZE + MS_CELLSIZE - 1 + MS_YOFFSET, MS_CELLSIZE, ILI9341_RED);
			if (x - 1 != 255 && !(cells[MS_getIndex(x - 1, y)] & CELLSTATE_REVEALED))
				tft.drawFastVLine(x * MS_CELLSIZE + MS_XOFFSET, y * MS_CELLSIZE + MS_YOFFSET, MS_CELLSIZE, ILI9341_RED);
			if (x + 1 >= 0 && !(cells[MS_getIndex(x + 1, y)] & CELLSTATE_REVEALED))
				tft.drawFastVLine(x * MS_CELLSIZE + MS_XOFFSET + MS_CELLSIZE - 1, y * MS_CELLSIZE + MS_YOFFSET, MS_CELLSIZE, ILI9341_RED);
		}
	}
	else {
		tft.fillRect(x * MS_CELLSIZE + MS_XOFFSET, y * MS_CELLSIZE + MS_YOFFSET, MS_CELLSIZE, MS_CELLSIZE, 0x4000); // 0x4000 = Dunkelrot
	}

	// Zeichne die Anzahl an umgebenden Mienen wenn die Zelle aufgedeckt wurde
	if (cl & CELLSTATE_REVEALED && !(cl & CELLSTATE_MINE)) {
		uint8_t mines = countNearbyMines(x, y);
		if (mines != 0) {
			tft.setCursor(x * MS_CELLSIZE + 4 + MS_XOFFSET, y * MS_CELLSIZE + 3 + MS_YOFFSET);
			tft.print(mines);
		}
	}

	if (cl & CELLSTATE_FLAGGED) {
		tft.drawRect(x * MS_CELLSIZE + MS_XOFFSET, y * MS_CELLSIZE + MS_YOFFSET, MS_CELLSIZE, MS_CELLSIZE, ILI9341_YELLOW);
	}
}

// überpruft ob alle Zellen, die keine Mienen sind aufgedeckt wurden
bool MS_checkWin() {
	for (uint8_t i = 0; i < MS_FIELD_SIZE * MS_FIELD_SIZE; i++) {
		if (!(cells[i] & CELLSTATE_REVEALED) && !(cells[i] & CELLSTATE_MINE)) {
			return false;
		}
	}
	return true;
}
#pragma endregion

#pragma region TicTacToe

#define TTT_getIndex(x, y) ((x) + 3 * (y))
#define TTT_fieldSize 50
#define TTT_offsetX 80
#define TTT_offsetY 40
uint8_t TTT_cursorX;
uint8_t TTT_cursorY;
uint8_t tictactoeField[9] = { 0 }; // 0 = nicht angekreuzt, 1 = X, 2 = O

void setupTTT() {
	TTT_cursorX = 0;
	TTT_cursorY = 0;

	for (uint8_t i = 0; i < 9; i++) {
		tictactoeField[i] = 0;
	}

	drawTTTField();
}

void playTicTacToe() {
	if (INPUT_UP_PRESSED && !PREV_INPUT_UP_PRESSED) {
		TTT_cursorY--;
		if (TTT_cursorY > 2) {
			TTT_cursorY = 2;
		}
	}
	if (INPUT_DOWN_PRESSED && !PREV_INPUT_DOWN_PRESSED) {
		TTT_cursorY = ++TTT_cursorY % 3;
	}
	if (INPUT_LEFT_PRESSED && !PREV_INPUT_LEFT_PRESSED) {
		TTT_cursorX--;
		if (TTT_cursorX > 2) {
			TTT_cursorX = 2;
		}
	}
	if (INPUT_RIGHT_PRESSED && !PREV_INPUT_RIGHT_PRESSED) {
		TTT_cursorX = ++TTT_cursorX % 3; // bewege den cursor nach rechts
	}

	// A = 1, B = 2
	// setze einen "X" wenn A gedrückt wurde und auf dem Feld kein "O" ist
	if (INPUT_A_PRESSED && !PREV_INPUT_A_PRESSED && tictactoeField[TTT_getIndex(TTT_cursorX, TTT_cursorY)] != 2) {
		tictactoeField[TTT_getIndex(TTT_cursorX, TTT_cursorY)] = 1;
	}
	// setze einen "O" wenn B gedrückt wurde und auf dem Feld kein "X" ist
	if (INPUT_B_PRESSED && !PREV_INPUT_B_PRESSED && tictactoeField[TTT_getIndex(TTT_cursorX, TTT_cursorY)] != 1) {
		tictactoeField[TTT_getIndex(TTT_cursorX, TTT_cursorY)] = 2;
	}

	if (INPUT_CHANGED) {
		drawTTTField(); // zeichne das feld
		tft.drawRect(TTT_cursorX * TTT_fieldSize + TTT_offsetX, TTT_cursorY * TTT_fieldSize + TTT_offsetY, TTT_fieldSize, TTT_fieldSize, ILI9341_BLUE); // zeichne den cursor

		// überprüfe ob das Spiel beendet werden soll
		if (TTT_checkWin() == 1) { // wenn Spieler 1 (X) gewonnen hat
			tft.setTextWrap(false);
			tft.setTextSize(2);
			printCentered(F("SPIELER X HAT GEWONNEN!"));
			delay(1000);

			zumHauptmenu();
		}
		else if (TTT_checkWin() == 2) { // wenn Spieler 2 (O) gewonnen hat
			tft.setTextWrap(false);
			tft.setTextSize(2);

			printCentered(F("SPIELER O HAT GEWONNEN!"));
			delay(1000);
			zumHauptmenu();
		}
		else if (TTT_checkFilled()) { // wenn niemand gewonnen hat und alle felder ausgefüllt wurden
			tft.setTextWrap(false);
			tft.setTextSize(2);

			printCentered(F("UNENDSCHIEDEN"));
			delay(1000);
			zumHauptmenu();
		}
	}
}

void drawTTTField() {
	for (uint8_t y = 0; y < 3; y++)
	{
		for (uint8_t x = 0; x < 3; x++)
		{
			if (tictactoeField[TTT_getIndex(x, y)] == 1) {
				tft.setCursor(x * TTT_fieldSize + 15 + TTT_offsetX, y * TTT_fieldSize + 12 + TTT_offsetY);
				tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
				tft.setTextSize(4);
				tft.print('X');
			}
			else if (tictactoeField[TTT_getIndex(x, y)] == 2) {
				tft.setCursor(x * TTT_fieldSize + 15 + TTT_offsetX, y * TTT_fieldSize + 12 + TTT_offsetY);
				tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK);
				tft.setTextSize(4);
				tft.print('O');
			}

			tft.drawRect(TTT_fieldSize * x + TTT_offsetX, TTT_fieldSize * y + TTT_offsetY, TTT_fieldSize, TTT_fieldSize, ILI9341_WHITE);
		}
	}
}

#define	TTT_checkFeld(index, spieler) (tictactoeField[index] == spieler)
uint8_t TTT_checkWin() {
	// Horizontal, Spieler 1 (X)
	if ((TTT_checkFeld(0, 1) && TTT_checkFeld(1, 1) && TTT_checkFeld(2, 1)) ||
		(TTT_checkFeld(3, 1) && TTT_checkFeld(4, 1) && TTT_checkFeld(5, 1)) ||
		(TTT_checkFeld(6, 1) && TTT_checkFeld(7, 1) && TTT_checkFeld(8, 1))) {
		return 1; // Spieler 1 hat gewonnen
	}
	
	// Vertikal, Spieler 1 (X)
	if ((TTT_checkFeld(0, 1) && TTT_checkFeld(3, 1) && TTT_checkFeld(6, 1)) ||
		(TTT_checkFeld(1, 1) && TTT_checkFeld(4, 1) && TTT_checkFeld(7, 1)) ||
		(TTT_checkFeld(2, 1) && TTT_checkFeld(5, 1) && TTT_checkFeld(8, 1))) {
		return 1; // Spieler 1 hat gewonnen
	}

	// Diagonal, Spieler 1 (X)
	if ((TTT_checkFeld(0, 1) && TTT_checkFeld(4, 1) && TTT_checkFeld(8, 1)) ||
		(TTT_checkFeld(2, 1) && TTT_checkFeld(4, 1) && TTT_checkFeld(6, 1))) {
		return 1; // Spieler 1 hat gewonnen
	}

	// Horizontal, Spieler 2 (O)
	if ((TTT_checkFeld(0, 2) && TTT_checkFeld(1, 2) && TTT_checkFeld(2, 2)) ||
		(TTT_checkFeld(3, 2) && TTT_checkFeld(4, 2) && TTT_checkFeld(5, 2)) ||
		(TTT_checkFeld(6, 2) && TTT_checkFeld(7, 2) && TTT_checkFeld(8, 2))) {
		return 2; // Spieler 2 hat gewonnen
	}

	// Vertikal, Spieler 2 (O)
	if ((TTT_checkFeld(0, 2) && TTT_checkFeld(3, 2) && TTT_checkFeld(6, 2)) ||
		(TTT_checkFeld(1, 2) && TTT_checkFeld(4, 2) && TTT_checkFeld(7, 2)) ||
		(TTT_checkFeld(2, 2) && TTT_checkFeld(5, 2) && TTT_checkFeld(8, 2))) {
		return 2; // Spieler 2 hat gewonnen
	}

	// Diagonal, Spieler 2 (O)
	if ((TTT_checkFeld(0, 2) && TTT_checkFeld(4, 2) && TTT_checkFeld(8, 2)) ||
		(TTT_checkFeld(2, 2) && TTT_checkFeld(4, 2) && TTT_checkFeld(6, 2))) {
		return 2; // Spieler 2 hat gewonnen
	}

	return 0;
}

// überprüft ob alle felder angekreuzt/eingekreist wurden
bool TTT_checkFilled() {
	for (uint8_t i = 0; i < 9; i++) {
		if (tictactoeField[i] == 0) {
			return false;
		}
	}
	return true;
}

#pragma endregion

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
