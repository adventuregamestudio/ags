
#include "ac/global_parser.h"
#include "ac/common.h"
#include "ac/gamestate.h"
#include "ac/string.h"

extern GameState play;

int SaidUnknownWord (char*buffer) {
    VALIDATE_STRING(buffer);
    strcpy (buffer, play.bad_parsed_word);
    if (play.bad_parsed_word[0] == 0)
        return 0;
    return 1;
}
