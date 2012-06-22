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
#include "acchars/ac_character.h"
#include "acrun/ac_platformdriver.h"
#include "acrun/ac_runninggame.h"
#include "acmain/ac_background.h"
#include "acmain/ac_collision.h"
#include "acmain/ac_cutscene.h"
#include "acmain/ac_dynamicsprite.h"
#include "acmain/ac_draw.h"
#include "acmain/ac_event.h"
#include "acmain/ac_game.h"
#include "acmain/ac_gui.h"
#include "acmain/ac_interaction.h"
#include "acmain/ac_inventory.h"
#include "acmain/ac_location.h"
#include "acmain/ac_main.h"
#include "acmain/ac_message.h"
#include "acmain/ac_mouse.h"
#include "acmain/ac_object.h"
#include "acmain/ac_overlay.h"
#include "acmain/ac_record.h"
#include "acmain/ac_region.h"
#include "acmain/ac_room.h"
#include "acmain/ac_screen.h"
#include "acmain/ac_script.h"
#include "acmain/ac_speech.h"
#include "acmain/ac_string.h"
#include "acmain/ac_strings.h"
#include "acmain/ac_timer.h"
#include "acmain/ac_translation.h"
#include "acmain/ac_viewframe.h"
#include "acmain/ac_viewport.h"
#include "acmain/ac_walkablearea.h"
#include "acmain/ac_walkbehind.h"
#include "debug/debug.h"

#endif // __AC_COMMONHEADERS_H