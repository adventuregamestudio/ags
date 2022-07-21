#pragma once
#include "SpriteFontRenderer.h"
class SpriteFontRendererClifftopGames : public SpriteFontRenderer
{
public:
	SpriteFontRendererClifftopGames(IAGSEngine *engine);
	~SpriteFontRendererClifftopGames(void) override;

	bool SupportsExtendedCharacters(int fontNumber) override;
	void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour) override;

private:
	void Draw(BITMAP *src, BITMAP *dest, int destx, int desty, int srcx, int srcy, int width, int height, int colour);
};

