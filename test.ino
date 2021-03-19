#include <Arduino.h>
#include <TM1637TinyDisplay.h>
#include <BobaBlox.h>

// Define Digital Pins
#define WHITE_LCD_CLK 2
#define WHITE_LCD_DIO 3
#define BLACK_LCD_CLK 4
#define BLACK_LCD_DIO 5
#define WHITE_LED 6
#define BLACK_LED 7
#define WHITE_BTN 8
#define BLACK_BTN 9
#define SELECT_BTN 10

// Initialize TM1637TinyDisplay
TM1637TinyDisplay whiteDisplay(WHITE_LCD_CLK, WHITE_LCD_DIO);
TM1637TinyDisplay blackDisplay(BLACK_LCD_CLK, BLACK_LCD_DIO);

/* Buttons */
Button whiteButton(WHITE_BTN);
Button blackButton(BLACK_BTN);
Button selectButton(SELECT_BTN);

enum GameState
{
	boot,
	selection,
	created,
	started,
	paused,
	ended
};
GameState gameState = GameState::boot;

void setup()
{

	Serial.begin(115200);

	/* Pin Modes */
	pinMode(WHITE_LED, OUTPUT);
	pinMode(BLACK_LED, OUTPUT);
	// pinMode(WHITE_BTN, INPUT);
	// digitalWrite(WHITE_BTN, HIGH);

	// pinMode(BLACK_BTN, INPUT_PULLUP);
	// pinMode(SELECT_BTN, INPUT_PULLUP);

	/* Displays */
	whiteDisplay.setBrightness(2);
	blackDisplay.setBrightness(2);
}

// variables will change:
int buttonState = 0; // variable for reading the pushbutton status

void loop()
{

	switch (gameState)
	{
	case GameState::boot:
		boot_loop();
		break;
	case GameState::selection:
		selection_loop();
		break;

	default:
		boot_loop();
		break;
	}
}

void boot_loop()
{

	whiteDisplay.showString("BRUN");
	blackDisplay.showString("SARA");

	if (whiteButton.wasReleased() || blackButton.wasReleased() || selectButton.wasReleased())
	{
		selection_enter();
	}
}

void selection_enter()
{
	Serial.write("Enter selection state.");
	gameState = GameState::selection;
}
void selection_loop()
{
	whiteDisplay.showString("---");
	blackDisplay.showString("---");
}

/* Changes turns: 0 = white => 1 = black */
void changeTurn(int color)
{
	if (color == 0)
	{
		digitalWrite(WHITE_LED, HIGH);
		digitalWrite(BLACK_LED, LOW);
	}
	else
	{
		digitalWrite(WHITE_LED, LOW);
		digitalWrite(BLACK_LED, HIGH);
	}
}
