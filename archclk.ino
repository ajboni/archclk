#include <arduino-timer.h>
#include <Arduino.h>
#include <TM1637TinyDisplay.h>
#include <BobaBlox.h>
#include "gameMode.h"
#include "chessTimer.h"

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
	{"15:00", "TEST", 15, 2},
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
int currentTurn = 0; // 0 = White , 1 = Black
int winner = -1;	 // 0 = White , 1 = Black

/* Reset the device. */
void (*resetFunc)(void) = 0;

void setup()
{

	/* Pin Modes */
	pinMode(WHITE_LED, OUTPUT);
	pinMode(BLACK_LED, OUTPUT);
	digitalWrite(WHITE_LED, LOW);
	digitalWrite(BLACK_LED, LOW);

	/* Displays */
	whiteDisplay.setBrightness(1);
	// whiteDisplay.setScrolldelay(500);

	blackDisplay.setBrightness(1);
	// blackDisplay.setScrolldelay(500);

	whiteDisplay.showString("CHESS");
	blackDisplay.showString("CHESS");

	// call the toggle_led function every 1000 millis (1 second)
	masterTimer.every(1000, updateTimers);
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
	case GameState::paused:
		gameStartedLoop();
		break;
	case GameState::ended:
		gameEndLoop();
		break;
	default:
		boot_loop();
		break;
	}
}

/* System booted*/
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
	gameState = GameState::selection;
}

/* Game mode selection state.*/
void selection_loop()
{

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

/* Pre game loop (Awaiting black to set up clock.*/
void gameCreatedLoop()
{

	/* Set Up displays */
	whiteDisplay.showNumberDec(whiteTimer.CurrentTime / 60, 0b01000000, true, 2, 0);
	whiteDisplay.showNumberDec(whiteTimer.CurrentTime % 60, 0b01000000, true, 2, 2);
	blackDisplay.showNumberDec(blackTimer.CurrentTime / 60, 0b01000000, true, 2, 0);
	blackDisplay.showNumberDec(blackTimer.CurrentTime % 60, 0b01000000, true, 2, 2);

	if (blackButton.wasReleased())
	{
		gameStartedEnter();
	}
}

/* Game has started. process movements and countdown. */
void gameStartedEnter()
{
	// white starts
	changeTurn(0);
	gameState = GameState::started;
}

unsigned long pauseTick;

/* Main game loop*/
void gameStartedLoop()
{

	masterTimer.tick();

	/* Set Up displays */
	whiteDisplay.showNumberDec(whiteTimer.CurrentTime / 60, 0b01000000, true, 2, 0);
	whiteDisplay.showNumberDec(whiteTimer.CurrentTime % 60, 0b01000000, true, 2, 2);
	blackDisplay.showNumberDec(blackTimer.CurrentTime / 60, 0b01000000, true, 2, 0);
	blackDisplay.showNumberDec(blackTimer.CurrentTime % 60, 0b01000000, true, 2, 2);

	if (gameState == GameState::paused)
	{

		// blink current turn

		if (millis() - pauseTick > 750)
		{
			if (currentTurn == 0)
				digitalWrite(WHITE_LED, !digitalRead(WHITE_LED));
			else
				digitalWrite(BLACK_LED, !digitalRead(BLACK_LED));
			pauseTick = millis();
		}
	}

	/* Pause game. */
	if (selectButton.wasReleased())
	{
		if (gameState == GameState::paused)
		{

			if (currentTurn == 0)
				digitalWrite(WHITE_LED, HIGH);
			else
				digitalWrite(BLACK_LED, HIGH);
			gameState = GameState::started;
		}
		else if (gameState == GameState::started)
		{
			pauseTick = millis();
			gameState = GameState::paused;
		}
	}

	if (blackButton.wasReleased())
	{
		changeTurn(0); // Change turn to white
	}
	if (whiteButton.wasReleased())
	{
		changeTurn(1); // Change turn to black
	}

	if (whiteTimer.CurrentTime <= 0)
	{
		winner = 1;
		whiteDisplay.showString("LOSE");
		blackDisplay.showString("WIN");
		gameEndEnter();
	}
	if (blackTimer.CurrentTime <= 0)
	{
		winner = 0;
		whiteDisplay.showString("WIN");
		blackDisplay.showString("LOSE");
		gameEndEnter();
	}
}

void gameEndEnter()
{
	gameState = GameState::ended;
}

void gameEndLoop()
{

	if (whiteButton.wasReleased() || blackButton.wasReleased() || selectButton.wasReleased())
	{
		selection_enter();
	}
}

/* Loop through all game modes by index.*/
void setGameModeByIndex(int index)
{

	if (index < 0)
		index = (sizeof(gameModes) / sizeof(gameModes[0])) - 1;
	if (index > (sizeof(gameModes) / sizeof(gameModes[0]) - 1))
		index = 0;

	currentGameMode = gameModes[index];
	currentGameModeIndex = index;
}

/* Changes turns: 0 = white => 1 = black */
void changeTurn(int color)
{

	// Do no teprocess timers if we are paused.
	if (gameState == GameState::paused)
	{
		return;
	}

	// Abort if no changes.
	if (color == currentTurn)
	{
		return;
	}

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
	{
		currentTurn = color;
		return;
	}

	// Add bonus Time if applicable to the current player, before changing turn.
	if (currentTurn == 0 && gameState == GameState::started)
		whiteTimer.CurrentTime += whiteTimer.BonusTime;
	if (currentTurn == 1 && gameState == GameState::started)
		blackTimer.CurrentTime += blackTimer.BonusTime;

	currentTurn = color;
}

/* Called on every master timer tick*/
bool updateTimers(void *)
{
	if (gameState == GameState::paused)
	{
		return true;
	}

	// Susbtract time acording to whose turn is.

	if (currentTurn == 0)
		whiteTimer.CurrentTime--;
	if (currentTurn == 1)
		blackTimer.CurrentTime--;

	return true;
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
