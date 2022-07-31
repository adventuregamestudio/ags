#pragma once
#include "plugin/agsplugin.h"
#include "VariableWidthFont.h"
#include <vector>

class VariableWidthSpriteFontRenderer :
	public IAGSFontRenderer2
{
public:
	VariableWidthSpriteFontRenderer(IAGSEngine *engine);
	virtual ~VariableWidthSpriteFontRenderer(void);

	void SetGlyph(int fontNum, int charNum, int x, int y, int width, int height);
	void SetSprite(int fontNum, int spriteNum);
	void SetSpacing(int fontNum, int spacing);
	void SetLineHeightAdjust(int fontNum, int lineHeight, int spacingHeight, int spacingOverride);

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
	const char *GetRendererName() override { return "VariableWidthSpriteFontRenderer"; }
	const char *GetFontName(int fontNumber) override { return ""; /* not supported */ }
	int GetFontHeight(int fontNumber);
	int GetLineSpacing(int fontNumber);

protected:
	IAGSEngine *_engine;
	std::vector<VariableWidthFont * > _fonts;
	VariableWidthFont *getFontFor(int fontNum);
	void Draw(BITMAP *src, BITMAP *dest, int destx, int desty, int srcx, int srcy, int width, int height);
};
