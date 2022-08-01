#pragma once
#include "plugin/agsplugin.h"
#include "SpriteFont.h"
#include <vector>
class SpriteFontRenderer :
	public IAGSFontRenderer2
{
public:
	SpriteFontRenderer(IAGSEngine *engine);
	virtual ~SpriteFontRenderer();

	void SetSpriteFont(int fontNum, int sprite, int rows, int columns, int charWidth, int charHeight, int charMin, int charMax, bool use32bit);

	// IAGSFontRenderer implementation
	bool LoadFromDisk(int fontNumber, int fontSize) override { return true; }
	void FreeMemory(int fontNumber) override;
	bool SupportsExtendedCharacters(int fontNumber) override;
	int GetTextWidth(const char *text, int fontNumber) override;
	int GetTextHeight(const char *text, int fontNumber) override;
	void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour) override;
	void AdjustYCoordinateForFont(int *ycoord, int fontNumber) override { }
	void EnsureTextValidForFont(char *text, int fontNumber) override;
	// IAGSFontRenderer2 implementation
	int GetVersion() override { return 26; /* compatible engine API ver */ }
	const char *GetRendererName() override { return "SpriteFontRenderer"; }
	const char *GetFontName(int fontNumber) override { return ""; /* not supported */ }
	int GetFontHeight(int fontNumber);
	int GetLineSpacing(int fontNumber) { return 0; /* not specified */ }

protected:
	SpriteFont *getFontFor(int fontNum);
	void Draw(BITMAP *src, BITMAP *dest, int destx, int desty, int srcx, int srcy, int width, int height);
	std::vector<SpriteFont * > _fonts;
	IAGSEngine *_engine;
};

