#include "ac/dynobj/scriptmouse.h"
#include "script/cc_error.h"

int32_t ScriptMouse::ReadInt32(const char *address, intptr_t offset)
{
    switch (offset)
    {
    case 0: return x;
    case 4: return y;
    }
    cc_error("ScriptMouse: unsupported variable offset %d", offset);
    return 0;
}

void ScriptMouse::WriteInt32(const char *address, intptr_t offset, int32_t val)
{
    switch (offset)
    {
    case 0:
    case 4:
        cc_error("ScriptMouse: attempt to write readonly variable at offset %d", offset);
        break;
    default:
        cc_error("ScriptMouse: unsupported variable offset %d", offset);
    }
}
