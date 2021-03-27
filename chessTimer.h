
#ifndef CHESSTIMER_H
#define CHESSTIMER_H

struct ChessTimer
{
	int MaxTime;
	int BonusTime;
	int CurrentTime;
	int LastTime;
	ChessTimer();
	ChessTimer(int maxTime, int BonusTime);
};

#endif