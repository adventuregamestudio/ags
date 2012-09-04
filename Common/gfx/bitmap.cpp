
#include "gfx/bitmap.h"
#include "gfx/allegrobitmap.h"

namespace AGS
{
namespace Common
{

// TODO: revise this construction later
namespace Bitmap
{

IBitmap *CreateBitmap(int width, int height, int color_depth)
{
	return CAllegroBitmap::CreateBitmap(width, height, color_depth);
}

IBitmap *CreateSubBitmap(IBitmap *src, int x, int y, int width, int height)
{
	return CAllegroBitmap::CreateSubBitmap(src, x, y, width, height);
}

};

} // namespace Common
} // namespace AGS
