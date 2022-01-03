#pragma once
// Diese Header Datei enth�lt defines f�r den Input Schieberegister.
// Da die Werte der einzelnen Tasten in einem 8bit Integer gespeichert werden sind Bitmasks ben�tigt um auf einzelne Tastenzust�nde zuzugreifen.

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