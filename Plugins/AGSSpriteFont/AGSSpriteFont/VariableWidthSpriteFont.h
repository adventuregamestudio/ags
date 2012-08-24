#pragma once
#include "../../../Common/agsplugin.h"
#include "VariableWidthFont.h"
#include <vector>
class VariableWidthSpriteFontRenderer :
	public IAGSFontRenderer
{
public:
	VariableWidthSpriteFontRenderer(IAGSEngine *engine);
	~VariableWidthSpriteFontRenderer(void);
	bool LoadFromDisk(int fontNumber, int fontSize) { return true; }
	void FreeMemory(int fontNumber) { }
	bool SupportsExtendedCharacters(int fontNumber);
	int GetTextWidth(const char *text, int fontNumber);
	int GetTextHeight(const char *text, int fontNumber);
	void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour);
	void AdjustYCoordinateForFont(int *ycoord, int fontNumber) { }
	void EnsureTextValidForFont(char *text, int fontNumber);
	void SetGlyph(int fontNum, int charNum, int x, int y, int width, int height);
	void SetSprite(int fontNum, int spriteNum);
	void SetSpacing(int fontNum, int spacing);

private:
	IAGSEngine *_engine;
	std::vector<VariableWidthFont * > _fonts;
	VariableWidthFont *getFontFor(int fontNum);
	void Draw(BITMAP *src, BITMAP *dest, int destx, int desty, int srcx, int srcy, int width, int height);
};

