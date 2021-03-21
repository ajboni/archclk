#include <Arduino.h>
#include <TM1637TinyDisplay.h>
#include <BobaBlox.h>
#include "gameMode.h"
#include "chessTimer.h"
#include <arduino-timer.h>

/* Define Digital Pins */
#define BLACK_LCD_CLK 2
#define BLACK_LCD_DIO 3
#define WHITE_LCD_CLK 4
#define WHITE_LCD_DIO 5
#define BLACK_LED 6
#define WHITE_LED 7
#define BLACK_BTN 8
#define WHITE_BTN 9
#define SELECT_BTN 10

/*  Initialize TM1637TinyDisplay */
TM1637TinyDisplay whiteDisplay(WHITE_LCD_CLK, WHITE_LCD_DIO);
TM1637TinyDisplay blackDisplay(BLACK_LCD_CLK, BLACK_LCD_DIO);

/* Buttons */
Button whiteButton(WHITE_BTN);
Button blackButton(BLACK_BTN);
Button selectButton(SELECT_BTN);

/*  Game State */
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

/* Game Modes */
GameMode gameModes[] = {
	{"0100", "BULL", 60, 0},
	{"02:01", "BULL", 120, 1},
	{"03:00", "BLIT", 180, 0},
	{"03:02", "BLIT", 180, 2},
	{"05:00", "BLIT", 300, 0},
	{"05:03", "BLIT", 300, 3},
	{"10:00", "RAPI", 600, 0},
	{"10:05", "RAPI", 600, 5},
	{"15:10", "RAPI", 900, 10},
	{"30:00", "CLAS", 1800, 0},
	{"30:20", "CLAS", 1800, 20},
	{"60:00", "CLAS", 3600, 0}};

int currentGameModeIndex = 6;
GameMode currentGameMode = gameModes[currentGameModeIndex];

/* Timers */
ChessTimer whiteTimer;
ChessTimer blackTimer;
auto masterTimer = timer_create_default(); // create a timer with default set

/* Global Variables */
int selectButtonHoldStartTime = 0;
int selectButtonHoldTime = 0;

/* Reset the device. */
void (*resetFunc)(void) = 0;

void setup()
{

	Serial.begin(115200);

	/* Pin Modes */
	pinMode(WHITE_LED, OUTPUT);
	pinMode(BLACK_LED, OUTPUT);

	/* Displays */
	whiteDisplay.setBrightness(2);
	// whiteDisplay.setScrolldelay(500);

	blackDisplay.setBrightness(2);
	// blackDisplay.setScrolldelay(500);

	whiteDisplay.showString("CHESS");
	blackDisplay.showString("CHESS");

	// call the toggle_led function every 1000 millis (1 second)
	masterTimer.every(1000, updateTimers);
}

bool updateTimers(void *)
{
	if (gameState == GameState::paused)
	{
		return true;
	}

	// Susbtract time acording to whose turn is.
	//temp

	whiteTimer.CurrentTime--;
	return true;
}

void loop()
{

	handleSelectionLongPress();

	switch (gameState)
	{
	case GameState::boot:
		boot_loop();
		break;
	case GameState::selection:
		selection_loop();
		break;
	case GameState::created:
		gameCreatedLoop();
		break;
	case GameState::started:
		gameStartedLoop();
		break;
	default:
		boot_loop();
		break;
	}
}

/* In any time of the state machine, the user can press and hold the selection button to reset the sequence. */
void handleSelectionLongPress()
{
	if (selectButton.isDown())
	{
		/* First time we pressed buttons */
		if (selectButtonHoldStartTime == 0)
		{
			selectButtonHoldStartTime = millis();
		}
		/* We are holding the button */
		else
		{
			int idleTime = millis() - selectButtonHoldStartTime;
			if (idleTime >= 2000)
			{
				whiteDisplay.clear();
				blackDisplay.clear();
				resetFunc();
			}
		}
	}
	if (selectButton.isUp())
	{
		selectButtonHoldStartTime = 0;
	}
}

void boot_loop()
{

	whiteDisplay.showString("WHTE");
	blackDisplay.showString("BLCK");

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

	/* Convert game mode to int */
	whiteDisplay.showNumberDec(currentGameMode.MaxTime / 60, 0b01000000, true, 2, 0);
	whiteDisplay.showNumberDec(currentGameMode.BonusTime, 0b01000000, true, 2, 2);
	blackDisplay.showString(currentGameMode.Type);

	int newIndex = 0;
	if (whiteButton.wasReleased())
	{
		setGameModeByIndex(currentGameModeIndex + 1);
	}
	if (blackButton.wasReleased())
	{
		setGameModeByIndex(currentGameModeIndex - 1);
	}
	if (selectButton.wasReleased())
	{
		gameCreatedEnter();
	}
}

/* Game has been created. Set timers and await black clock. */
void gameCreatedEnter()
{
	Serial.write("Enter game created state.");

	/* Set up timers */
	whiteTimer.MaxTime = currentGameMode.MaxTime;
	whiteTimer.CurrentTime = currentGameMode.MaxTime;
	whiteTimer.BonusTime = currentGameMode.BonusTime;
	blackTimer.CurrentTime = currentGameMode.MaxTime;
	blackTimer.MaxTime = currentGameMode.MaxTime;
	blackTimer.BonusTime = currentGameMode.BonusTime;

	changeTurn(1);
	gameState = GameState::created;
}

void gameCreatedLoop()
{

	/* Set Up displays */
	whiteDisplay.showNumberDec(whiteTimer.CurrentTime / 60, 0b01000000, true, 2, 0);
	whiteDisplay.showNumberDec(whiteTimer.CurrentTime % 60, 0b01000000, true, 2, 2);
	blackDisplay.showNumberDec(blackTimer.CurrentTime, 0b01000000, true, 4, 0);
	masterTimer.tick();
}

/* Game has started. process movements and countdown. */
void gameStartedEnter()
{
}

void gameStartedLoop()
{
}

void gameLoop() {}

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

	// Do no teprocess timers if we are in the setup pahse.
	if (gameState == GameState::created)
		return;
}

void setGameModeByIndex(int index)
{

	if (index < 0)
		index = (sizeof(gameModes) / sizeof(gameModes[0])) - 1;
	if (index > (sizeof(gameModes) / sizeof(gameModes[0]) - 1))
		index = 0;

	currentGameMode = gameModes[index];
	currentGameModeIndex = index;
}
