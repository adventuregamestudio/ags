
#include "ac/global_dynamicsprite.h"
#include "wgt2allg.h"
#include "ali3d.h"
#include "ac/dynamicsprite.h"
#include "ac/file.h"
#include "sprcache.h"

extern SpriteCache spriteset;
extern IGraphicsDriver *gfxDriver;

int LoadImageFile(const char *filename) {

    char loadFromPath[MAX_PATH];
    get_current_dir_path(loadFromPath, filename);

    block loadedFile = load_bitmap(loadFromPath, NULL);

    if (loadedFile == NULL)
        return 0;

    int gotSlot = spriteset.findFreeSlot();
    if (gotSlot <= 0)
        return 0;

    add_dynamic_sprite(gotSlot, gfxDriver->ConvertBitmapToSupportedColourDepth(loadedFile));

    return gotSlot;
}
