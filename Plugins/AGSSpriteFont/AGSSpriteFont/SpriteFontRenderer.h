#pragma once
#include "../../../Common/agsplugin.h"
#include "SpriteFont.h"
#include <vector>
class SpriteFontRenderer :
	public IAGSFontRenderer
{
public:
	SpriteFontRenderer(IAGSEngine *engine);
	~SpriteFontRenderer(void);
	bool LoadFromDisk(int fontNumber, int fontSize) { return true; }
	void FreeMemory(int fontNumber) { }
	bool SupportsExtendedCharacters(int fontNumber);
	int GetTextWidth(const char *text, int fontNumber);
	int GetTextHeight(const char *text, int fontNumber);
	void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour);
	void AdjustYCoordinateForFont(int *ycoord, int fontNumber) { }
	void EnsureTextValidForFont(char *text, int fontNumber);
	void SetSpriteFont(int fontNum, int sprite, int rows, int columns, int charWidth, int charHeight, int charMin, int charMax, bool use32bit);
	
	
private:
	SpriteFont *getFontFor(int fontNum);
	void Draw(BITMAP *src, BITMAP *dest, int destx, int desty, int srcx, int srcy, int width, int height);
	std::vector<SpriteFont * > _fonts;
	IAGSEngine *_engine;
};

