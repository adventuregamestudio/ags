
#include "ac/global_mouse.h"
#include "ac/gamestate.h"

extern GameState play;

void HideMouseCursor () {
    play.mouse_cursor_hidden = 1;
}

void ShowMouseCursor () {
    play.mouse_cursor_hidden = 0;
}
