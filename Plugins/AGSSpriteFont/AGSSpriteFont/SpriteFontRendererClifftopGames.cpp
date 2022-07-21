#include "SpriteFontRendererClifftopGames.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "color.h"


SpriteFontRendererClifftopGames::SpriteFontRendererClifftopGames(IAGSEngine *engine)
	: SpriteFontRenderer(engine)
{
}

SpriteFontRendererClifftopGames::~SpriteFontRendererClifftopGames(void) = default;

bool SpriteFontRendererClifftopGames::SupportsExtendedCharacters(int fontNumber)
{
	return true;
}

void SpriteFontRendererClifftopGames::RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour)
{
	SpriteFont *font = getFontFor(fontNumber);
	//BITMAP *vScreen = _engine->GetVirtualScreen();

	//_engine->SetVirtualScreen(destination);

	for (int i = 0; i < (int)strlen(text); i++)
	{
		char c = text[i];
		c -= font->MinChar;
		int row = c / font->Columns;
		int column = c % font->Columns;
		BITMAP *src = _engine->GetSpriteGraphic(font->SpriteNumber);
		Draw(src, destination, x + (i * font->CharWidth), y, column * font->CharWidth, row * font->CharHeight, font->CharWidth, font->CharHeight, colour);
	}

	//_engine->SetVirtualScreen(vScreen);
}

void SpriteFontRendererClifftopGames::Draw(BITMAP *src, BITMAP *dest, int destx, int desty, int srcx, int srcy, int width, int height, int colour)
{
	int32 srcWidth = 0, srcHeight = 0, destWidth = 0, destHeight = 0, srcColDepth = 0, destColDepth = 0;

	unsigned char **srccharbuffer = _engine->GetRawBitmapSurface (src); //8bit
	uint16_t **srcshortbuffer = (uint16_t**)srccharbuffer; //16bit;
	uint32_t **srclongbuffer = (uint32_t**)srccharbuffer; //32bit

	unsigned char **destcharbuffer = _engine->GetRawBitmapSurface (dest); //8bit
	uint16_t **destshortbuffer = (uint16_t**)destcharbuffer; //16bit;
    uint32_t **destlongbuffer = (uint32_t**)destcharbuffer; //32bit

	int transColor = _engine->GetBitmapTransparentColor(src);

	_engine->GetBitmapDimensions(src, &srcWidth, &srcHeight, &srcColDepth);
	_engine->GetBitmapDimensions(dest, &destWidth, &destHeight, &destColDepth);

	if (srcy + height > srcHeight || srcx + width > srcWidth || srcx < 0 || srcy < 0) return;

	if (width + destx > destWidth) width = destWidth - destx;
	if (height + desty > destHeight) height = destHeight - desty;

	int startx = MAX(0, (-1 * destx));
	int starty = MAX(0, (-1 * desty));

	int srca, srcr, srcg, srcb, desta, destr, destg, destb, finalr, finalg, finalb, finala, col, col_r, col_g, col_b;
	col_r = getr32(colour);
	col_g = getg32(colour);
	col_b = getb32(colour);

	for(int x = startx; x < width; x ++)
	{

		for(int y = starty; y <  height; y ++)
		{
			int srcyy = y + srcy;
			int srcxx = x + srcx;
			int destyy = y + desty;
			int destxx = x + destx;
			if (destColDepth == 8)
			{
				if (srccharbuffer[srcyy][srcxx] != transColor)
					destcharbuffer[destyy][destxx] = srccharbuffer[srcyy][srcxx];
			}
			else if (destColDepth == 16)
			{
				if (srcshortbuffer[srcyy][srcxx] != transColor)
					destshortbuffer[destyy][destxx] = srcshortbuffer[srcyy][srcxx];
			}
			else if (destColDepth == 32)
			{
				//if (srclongbuffer[srcyy][srcxx] != transColor) destlongbuffer[destyy][destxx] = srclongbuffer[srcyy][srcxx];

				srca =  (geta32(srclongbuffer[srcyy][srcxx]));

				if (srca != 0) {
					srcr =  getr32(srclongbuffer[srcyy][srcxx]);
					srcg =  getg32(srclongbuffer[srcyy][srcxx]);
					srcb =  getb32(srclongbuffer[srcyy][srcxx]);

					destr =  getr32(destlongbuffer[destyy][destxx]);
					destg =  getg32(destlongbuffer[destyy][destxx]);
					destb =  getb32(destlongbuffer[destyy][destxx]);
					desta =  geta32(destlongbuffer[destyy][destxx]);

					// The original source code from Clifftop Games has the code below, but since it is useless it is commented out
					/*
					finalr = (col_r * srcr) / 255;
					finalg = (col_g * srcg) / 255;
					finalb = (col_b * srcb) / 255;
						*/

					finala = 255-(255-srca)*(255-desta)/255;
					finalr = srca*col_r/finala + desta*destr*(255-srca)/finala/255;
					finalg = srca*col_g/finala + desta*destg*(255-srca)/finala/255;
					finalb = srca*col_b/finala + desta*destb*(255-srca)/finala/255;
					col = makeacol32(finalr, finalg, finalb, finala);
					destlongbuffer[destyy][destxx] = col;
				}
			}
		}
	}

	_engine->ReleaseBitmapSurface(src);
	_engine->ReleaseBitmapSurface(dest);
}
