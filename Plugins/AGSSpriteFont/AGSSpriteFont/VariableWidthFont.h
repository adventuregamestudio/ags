#pragma once
#include <map>
#include "CharacterEntry.h"
#include "plugin/agsplugin.h"
class VariableWidthFont
{
public:
	VariableWidthFont(void);
	~VariableWidthFont(void);
	void SetGlyph(int character, int x, int y, int width, int height);
	void SetLineHeightAdjust(int LineHeight, int SpacingHeight, int SpacingOverride);
	int SpriteNumber;
	int FontReplaced;
	int Spacing;
	int LineHeightAdjust;
	int LineSpacingAdjust;
	int LineSpacingOverride;
	std::map<char, CharacterEntry> characters;

private:
	
};

