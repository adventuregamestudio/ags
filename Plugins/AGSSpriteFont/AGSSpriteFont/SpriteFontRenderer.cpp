#include "SpriteFontRenderer.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "color.h"



SpriteFontRenderer::SpriteFontRenderer(IAGSEngine *engine)
{
	_engine = engine;
}


SpriteFontRenderer::~SpriteFontRenderer(void)
{
	for(size_t i = 0; i < _fonts.size(); i++)
	{
		if(_fonts[i] != NULL)
			delete _fonts[i];
	}

	_fonts.clear();
}

void SpriteFontRenderer::SetSpriteFont(int fontNum, int sprite, int rows, int columns, int charWidth, int charHeight, int charMin, int charMax, bool use32bit)
{
	SpriteFont *font = getFontFor(fontNum);
	font->SpriteNumber = sprite;
	font->Rows = rows;
	font->Columns = columns;
	font->MinChar = charMin;
	font->MaxChar = charMax;
	font->Use32bit = use32bit;
	font->CharHeight = charHeight;
	font->CharWidth = charWidth;

}

void SpriteFontRenderer::EnsureTextValidForFont(char *text, int fontNumber)
{
	SpriteFont *font = getFontFor(fontNumber);
	for(size_t i = 0; i < strlen(text); i++)
	{
		if(text[i] < font->MinChar || text[i] > font->MaxChar)
		{
			if (font->MinChar < 63 || font->MaxChar > 63) text[i] = 63;
			else text[i] = font->MinChar;

		}

	}
}

bool SpriteFontRenderer::SupportsExtendedCharacters(int fontNumber)
{
	return true;
}

int SpriteFontRenderer::GetTextWidth(const char *text, int fontNumber)
{
	SpriteFont *font = getFontFor(fontNumber);
	size_t len = strlen(text);
	return (int) (font->CharWidth * len);

}

int SpriteFontRenderer::GetTextHeight(const char *text, int fontNumber)
{
	SpriteFont *font = getFontFor(fontNumber);
	return font->CharHeight;
}

SpriteFont *SpriteFontRenderer::getFontFor(int fontNum)
{
	SpriteFont *font;
	for (size_t i = 0; i < _fonts.size(); i ++)
	{
		font = _fonts.at(i);
		if (font->FontReplaced == fontNum) return font;
	}
	//not found
	font = new SpriteFont();
	font->FontReplaced = fontNum;
	_fonts.push_back(font);
	return font;
}



void SpriteFontRenderer::RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour)
{

	SpriteFont *font = getFontFor(fontNumber);

	for(size_t i = 0; i < strlen(text); i++)
	{
		char c = text[i];
		c -= font->MinChar;
		int row = c / font->Columns;
		int column = c % font->Columns;
		BITMAP *src = _engine->GetSpriteGraphic(font->SpriteNumber);
		Draw(src, destination, x + (i * font->CharWidth), y, column * font->CharWidth, row * font->CharHeight, font->CharWidth, font->CharHeight, colour);
	}

}




void SpriteFontRenderer::Draw(BITMAP *src, BITMAP *dest, int destx, int desty, int srcx, int srcy, int width, int height, int colour)
{

	int32 srcWidth = 0, srcHeight = 0, destWidth = 0, destHeight = 0, srcColDepth = 0, destColDepth = 0;

	unsigned char **srccharbuffer = _engine->GetRawBitmapSurface (src); //8bit
	// this is risky: may lead to crashes or bad performance if data is unaligned
	uint16_t **srcshortbuffer = (uint16_t**)srccharbuffer; //16bit;
	uint32_t **srclongbuffer = (uint32_t**)srccharbuffer; //32bit

	unsigned char **destcharbuffer = _engine->GetRawBitmapSurface (dest); //8bit
	// more unaligned data access risk
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


	int srca, srcr, srcg, srcb, desta, destr, destg, destb, finalr, finalg, finalb, finala, col, col_r,col_g,col_b;

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
				if (srccharbuffer[srcyy][srcxx] != transColor) destcharbuffer[destyy][destxx] = srccharbuffer[srcyy][srcxx];
			}
			else if (destColDepth == 16)
			{
				if (srcshortbuffer[srcyy][srcxx] != transColor) destshortbuffer[destyy][destxx] = srcshortbuffer[srcyy][srcxx];
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

					col_r = getr32(colour);
					col_g = getg32(colour);
					col_b = getb32(colour);

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
