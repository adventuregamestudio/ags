#include "VariableWidthSpriteFont.h"
#include <string>
#include <string.h>
#include <stdint.h>
#include "color.h"



VariableWidthSpriteFontRenderer::VariableWidthSpriteFontRenderer(IAGSEngine *engine)
{
	_engine = engine;
}


VariableWidthSpriteFontRenderer::~VariableWidthSpriteFontRenderer(void) = default;


void VariableWidthSpriteFontRenderer::FreeMemory(int fontNum)
{
	for(auto it = _fonts.begin(); it != _fonts.end() ; ++it)
	{
		VariableWidthFont *font = *it;
		if (font->FontReplaced == fontNum)
		{
			_fonts.erase(it);
			delete font;
			return;
		}
	}
}

bool VariableWidthSpriteFontRenderer::SupportsExtendedCharacters(int fontNumber) { return false; }

int VariableWidthSpriteFontRenderer::GetTextWidth(const char *text, int fontNumber)
{
	int total = 0;
	VariableWidthFont *font = getFontFor(fontNumber);
	for(size_t i = 0; i < strlen(text); i++)
	{
		if (font->characters.count(text[i]) > 0)
		{
			total += font->characters[text[i]].Width;
			if (text[i] != ' ') total += font->Spacing;
		}
	}
	return total;
}

int VariableWidthSpriteFontRenderer::GetTextHeight(const char *text, int fontNumber)
{
	VariableWidthFont *font = getFontFor(fontNumber);
	for(size_t i = 0; i < strlen(text); i++)
	{
		if (font->characters.count(text[i]) > 0)
		{
			return font->characters[text[i]].Height;
		}
	}
	return 0;
}

int VariableWidthSpriteFontRenderer::GetFontHeight(int fontNumber)
{
	VariableWidthFont *font = getFontFor(fontNumber);
	if (font->characters.size() > 0)
	{
		return font->characters.begin()->second.Height + font->LineHeightAdjust;
	}
	return 0;
}

int VariableWidthSpriteFontRenderer::GetLineSpacing(int fontNumber)
{
	VariableWidthFont *font = getFontFor(fontNumber);
	return font->LineSpacingOverride;
}

void VariableWidthSpriteFontRenderer::SetSpacing(int fontNum, int spacing)
{
	VariableWidthFont *font = getFontFor(fontNum);
	font->Spacing = spacing;
}

void VariableWidthSpriteFontRenderer::SetLineHeightAdjust(int fontNum, int lineHeight, int spacingHeight, int spacingOverride)
{
	VariableWidthFont *font = getFontFor(fontNum);
	font->LineHeightAdjust = lineHeight;
	font->LineSpacingAdjust = spacingHeight;
	font->LineSpacingOverride = spacingOverride;

	if (_engine->version >= 26)
		_engine->NotifyFontUpdated(fontNum);
}

void VariableWidthSpriteFontRenderer::EnsureTextValidForFont(char *text, int fontNumber)
{
	VariableWidthFont *font = getFontFor(fontNumber);
	std::string s(text);
	
	for(int i = (int)s.length() - 1; i >= 0 ; i--)
	{
		if (font->characters.count(s[i]) == 0)
		{
			s.erase(i,1);
		}
	}
	text = strcpy(text,s.c_str());
	
}

void VariableWidthSpriteFontRenderer::SetGlyph(int fontNum, int charNum, int x, int y, int width, int height)
{
	VariableWidthFont *font = getFontFor(fontNum);
	font->SetGlyph(charNum, x, y, width, height);

	// Only notify engine at the first engine glyph,
	// that should be enough for calculating font height metrics,
	// and will reduce work load (sadly there's no Begin/EndUpdate functions).
	if ((_engine->version >= 26) && (font->characters.size() == 1))
		_engine->NotifyFontUpdated(fontNum);
}

void VariableWidthSpriteFontRenderer::SetSprite(int fontNum, int spriteNum)
{
	VariableWidthFont *font = getFontFor(fontNum);
	font->SpriteNumber = spriteNum;
}

VariableWidthFont *VariableWidthSpriteFontRenderer::getFontFor(int fontNum){
	VariableWidthFont *font;
	for (size_t i = 0; i < _fonts.size(); i ++)
	{
		font = _fonts.at(i);
		if (font->FontReplaced == fontNum) return font;
	}
	//not found
	font = new VariableWidthFont;
	font->FontReplaced = fontNum;
	_fonts.push_back(font);
	return font;
}

void VariableWidthSpriteFontRenderer::RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour)
{
	VariableWidthFont *font = getFontFor(fontNumber);
	int totalWidth = 0;
	for(size_t i = 0; i < strlen(text); i++)
	{
		char c = text[i];
				
		BITMAP *src = _engine->GetSpriteGraphic(font->SpriteNumber);
		Draw(src, destination, x + totalWidth, y, font->characters[c].X, font->characters[c].Y, font->characters[c].Width, font->characters[c].Height); 
		totalWidth += font->characters[c].Width;
		if (text[i] != ' ') totalWidth += font->Spacing;
	}
	
}


void VariableWidthSpriteFontRenderer::Draw(BITMAP *src, BITMAP *dest, int destx, int desty, int srcx, int srcy, int width, int height)
{

	int32 srcWidth, srcHeight, destWidth, destHeight, srcColDepth, destColDepth;

	unsigned char **srccharbuffer = _engine->GetRawBitmapSurface (src); //8bit
	uint16_t **srcshortbuffer = (uint16_t**)srccharbuffer; //16bit;
	uint32_t **srclongbuffer = (uint32_t**)srccharbuffer; //32bit

	unsigned char **destcharbuffer = _engine->GetRawBitmapSurface (dest); //8bit
	uint16_t**destshortbuffer = (uint16_t**)destcharbuffer; //16bit;
	uint32_t **destlongbuffer = (uint32_t**)destcharbuffer; //32bit

	int transColor = _engine->GetBitmapTransparentColor(src);

	_engine->GetBitmapDimensions(src, &srcWidth, &srcHeight, &srcColDepth);
	_engine->GetBitmapDimensions(dest, &destWidth, &destHeight, &destColDepth);

	if (srcy + height > srcHeight || srcx + width > srcWidth || srcx < 0 || srcy < 0) return;

	if (width + destx > destWidth) width = destWidth - destx;
	if (height + desty > destHeight) height = destHeight - desty;

	int startx = MAX(0, (-1 * destx));
	int starty = MAX(0, (-1 * desty));

	int srca, srcr, srcg, srcb, desta, destr, destg, destb, finalr, finalg, finalb, finala, col;

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

					finalr = srcr;
					finalg = srcg;
					finalb = srcb;

					finala = 255-(255-srca)*(255-desta)/255;
					finalr = srca*finalr/finala + desta*destr*(255-srca)/finala/255;
					finalg = srca*finalg/finala + desta*destg*(255-srca)/finala/255;
					finalb = srca*finalb/finala + desta*destb*(255-srca)/finala/255;
					col = makeacol32(finalr, finalg, finalb, finala);
					destlongbuffer[destyy][destxx] = col;
				}
			}
		}
	}
	
	_engine->ReleaseBitmapSurface(src);
	_engine->ReleaseBitmapSurface(dest);
}

