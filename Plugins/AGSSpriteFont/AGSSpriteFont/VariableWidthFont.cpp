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


VariableWidthFont::~VariableWidthFont(void)
{
}

void VariableWidthFont::SetLineHeightAdjust(int LineHeight, int SpacingHeight, int SpacingOverride)
{
	LineHeightAdjust = LineHeight;
	LineSpacingAdjust = SpacingHeight;
	LineSpacingOverride = SpacingOverride;
}

void VariableWidthFont::SetGlyph(int character, int x, int y, int width, int height)
{
	characters[character].X = x;
	characters[character].Y = y;
	characters[character].Width = width;
	characters[character].Height = height;
	characters[character].Character = character;
}
