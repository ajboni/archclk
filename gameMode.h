
#ifndef GAMEMODE_H
#define GAMEMODE_H

struct GameMode
{
	const char *Name;
	const char *Type;
	int MaxTime;
	int BonusTime;
	GameMode(const char *name, const char *type, int maxTime, int bonusTime);
};

#endif