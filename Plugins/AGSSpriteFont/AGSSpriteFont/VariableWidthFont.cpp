#include "VariableWidthFont.h"


VariableWidthFont::VariableWidthFont(void)
{
	Spacing = 0;
	FontReplaced = 0;
	SpriteNumber = 0;
	LineHeightAdjust = 0;
	LineSpacingAdjust = 0;
	LineSpacingOverride = 0;
}


VariableWidthFont::~VariableWidthFont(void) = default;

void VariableWidthFont::SetGlyph(int character, int x, int y, int width, int height)
{
	characters[character].X = x;
	characters[character].Y = y;
	characters[character].Width = width;
	characters[character].Height = height;
	characters[character].Character = character;
}

void VariableWidthFont::SetLineHeightAdjust(int lineHeight, int spacingHeight, int spacingOverride)
{
	LineHeightAdjust = lineHeight;
	LineSpacingAdjust = spacingHeight;
	LineSpacingOverride = spacingOverride;
}
