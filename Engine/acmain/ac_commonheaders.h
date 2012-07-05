#ifndef __AC_COMMONHEADERS_H
#define __AC_COMMONHEADERS_H

// [IKM] 2012-06-17
// I know this should not be like that (slows down compilation quite a bit),
// but that was an easy way to not worry about headers during splitting that
// ac.cpp.
//
// Things should change when class system is developed or code is otherwise
// enhanced and optimized.

#include "ac/ac_common.h"
#include "ac/event.h"
#include "ac/character.h"
#include "ac/global_character.h"
#include "ac/object.h"
#include "ac/global_object.h"
#include "ac/global_region.h"
#include "ac/region.h"
#include "acrun/ac_runninggame.h"
#include "acmain/ac_background.h"
#include "acmain/ac_cutscene.h"
#include "ac/dynamicsprite.h"
#include "acmain/ac_draw.h"
#include "acmain/ac_game.h"
#include "ac/gui.h"
#include "acmain/ac_interaction.h"
#include "acmain/ac_inventory.h"
#include "acmain/ac_location.h"
#include "acmain/ac_message.h"
#include "acmain/ac_mouse.h"
#include "ac/overlay.h"
#include "ac/global_overlay.h"
#include "acmain/ac_record.h"
#include "acmain/ac_room.h"
#include "acmain/ac_screen.h"
#include "acmain/ac_speech.h"
#include "acmain/ac_string.h"
#include "acmain/ac_strings.h"
#include "acmain/ac_timer.h"
#include "acmain/ac_translation.h"
#include "acmain/ac_viewframe.h"
#include "acmain/ac_viewport.h"
#include "debug/debug.h"
#include "main/config.h"
#include "main/engine.h"
#include "main/game_run.h"
#include "main/game_start.h"
#include "main/graphics_mode.h"
#include "main/main.h"
#include "main/quit.h"
#include "main/update.h"
#include "platform/agsplatformdriver.h"
#include "script/script.h"

#endif // __AC_COMMONHEADERS_H