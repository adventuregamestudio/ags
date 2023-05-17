
#include <string.h>
#include "ac/dynobj/scriptgame.h"
#include "ac/game.h"
#include "ac/gamestate.h"

StaticGame      GameStaticManager;


void StaticGame::WriteInt32(const char *address, intptr_t offset, int32_t val)
{
    if (offset == 4 * sizeof(int32_t))
    { // game.debug_mode
        set_debug_mode(val != 0);
    }
    else if (offset == 99 * sizeof(int32_t) || offset == 112 * sizeof(int32_t))
    { // game.text_align, game.speech_text_align
        *(int32_t*)(address + offset) = ReadScriptAlignment(val);
    }
    else
    {
        *(int32_t*)(address + offset) = val;
    }
}
