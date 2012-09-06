
#include "ac/global_dynamicsprite.h"
#include "util/wgt2allg.h"
#include "gfx/ali3d.h"
#include "ac/dynamicsprite.h"
#include "ac/file.h"
#include "ac/spritecache.h"
#include "ac/runtime_defines.h" //MAX_PATH
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"

using AGS::Common::IBitmap;
namespace Bitmap = AGS::Common::Bitmap;

extern SpriteCache spriteset;
extern IGraphicsDriver *gfxDriver;

int LoadImageFile(const char *filename) {

    char loadFromPath[MAX_PATH];
    get_current_dir_path(loadFromPath, filename);

	IBitmap *loadedFile = Bitmap::LoadFromFile(loadFromPath);

    if (loadedFile == NULL)
        return 0;

    int gotSlot = spriteset.findFreeSlot();
    if (gotSlot <= 0)
        return 0;

    add_dynamic_sprite(gotSlot, gfxDriver->ConvertBitmapToSupportedColourDepth(loadedFile));

    return gotSlot;
}
