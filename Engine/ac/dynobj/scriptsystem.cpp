#include "ac/dynobj/scriptsystem.h"
#include "script/cc_error.h"

int32_t ScriptSystem::ReadInt32(const char *address, intptr_t offset)
{
    int index = offset / sizeof(int32_t);
    switch (index)
    {
    case 0: return width;
    case 1: return height;
    case 2: return coldepth;
    case 3: return os;
    case 4: return windowed;
    case 5: return vsync;
    case 6: return viewport_width;
    case 7: return viewport_height;
    }
    cc_error("ScriptSystem: unsupported variable offset %d", offset);
    return 0;
}

void ScriptSystem::WriteInt32(const char *address, intptr_t offset, int32_t val)
{
    int index = offset / sizeof(int32_t);
    switch (index)
    {
    case 5: vsync = val; break;
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
        cc_error("ScriptSystem: attempt to write readonly variable at offset %d", offset);
        break;
    default:
        cc_error("ScriptSystem: unsupported variable offset %d", offset);
    }
}
