#pragma once
#include "VariableWidthSpriteFont.h"

class VariableWidthSpriteFontRendererClifftopGames : public VariableWidthSpriteFontRenderer
{
public:
	VariableWidthSpriteFontRendererClifftopGames(IAGSEngine *engine);
	~VariableWidthSpriteFontRendererClifftopGames(void) override;

	void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour) override;

private:
	void Draw(BITMAP *src, BITMAP *dest, int destx, int desty, int srcx, int srcy, int width, int height, int colour);
};

