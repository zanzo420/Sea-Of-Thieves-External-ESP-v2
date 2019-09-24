#pragma once

struct sColor
{
	int r = 255;
	int g = 255;
	int b = 255;
	int a = 255;

	sColor operator=(int color[4])
	{
		r = color[0];
		g = color[1];
		b = color[2];
		a = color[3];
		return *this;
	}
};

class vars {
public:

	bool GUI = true;

	struct ESP
	{
		int colorCommon[4], colorRare[4], colorMythical[4], colorLegendary[4], colorSpecial[4], colorOther[4];

		struct Animals
		{
			int colorWorld[4];
			bool bActive, bSnake, bChicken, bPig;
		}Animals;
		struct World
		{
			int colorWorld[4];
			bool bShipWreck, bMermaid, bFort, bIslands, bMapPins;
		}World;
		struct Ships
		{
			int colorEnemy[4], colorTeam[4];
			bool bActive;
		}Ships;
		struct Player
		{
			int colorEnemy[4], colorTeam[4];
			bool bName, bActive, bWeapon, bHealth, bTeam;
		}Player;
		struct Skeleton
		{
			int colorSkeleton[4];
			bool bActive, bWeapon;
		}Skeleton;
		struct Treasure
		{
			bool bActive;
		}Treasure;
	}ESP;

};

extern vars Vars;