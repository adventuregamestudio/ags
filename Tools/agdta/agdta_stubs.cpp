// Stubs for functions in Common that expect Engine implementations.
// Link stubs for agdta (Common/test/common_stubs.cpp pattern).

#include "ac/game_version.h"
#include "gui/guiobject.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guitextbox.h"
#include "script/cc_common.h"
#include "util/stream.h"
#include "util/string.h"
#include "util/version.h"

using namespace AGS::Common;

GameDataVersion loaded_game_file_version = kGameVersion_Current;
Version game_compiled_version;

namespace AGS {
namespace Common {

void GUIObject::MarkChanged() {}
void GUIObject::MarkParentChanged() {}
void GUIObject::MarkPositionChanged(bool) {}
void GUIObject::MarkStateChanged(bool, bool) {}

void GUIButton::PrepareTextToDraw() {}

bool GUIInvWindow::HasAlphaChannel() const { return false; }

void GUIInvWindow::Draw(Bitmap *, int, int) {}

int GUILabel::PrepareTextToDraw() { return 0; }

void GUIListBox::PrepareTextToDraw(const String &) {}

void GUITextBox::DrawTextBoxContents(Bitmap *, int, int) {}

} // namespace Common
} // namespace AGS

String cc_format_error(const String &message) {
    if (currentline > 0) {
        return String::FromFormat("Error (line %d): %s", currentline, message.GetCStr());
    }
    return String::FromFormat("Error (line unknown): %s", message.GetCStr());
}

String cc_get_callstack(int /*max_lines*/) { return ""; }

void quit(const char *) {}

int get_adjusted_spritewidth(int spr) { (void)spr; return 0; }
int get_adjusted_spriteheight(int spr) { (void)spr; return 0; }
bool is_sprite_alpha(int spr) { (void)spr; return false; }

void draw_gui_sprite(Bitmap *ds, int spr, int x, int y, bool use_alpha, BlendMode blend_mode) {
    (void)ds; (void)spr; (void)x; (void)y; (void)use_alpha; (void)blend_mode;
}

void draw_gui_sprite(Bitmap *ds, bool use_alpha, int x, int y, Bitmap *image, bool src_has_alpha,
                     BlendMode blend_mode, int alpha) {
    (void)ds; (void)use_alpha; (void)x; (void)y; (void)image; (void)src_has_alpha;
    (void)blend_mode; (void)alpha;
}

int game_to_data_coord(int coord) { return coord; }
int data_to_game_coord(int coord) { return coord; }
void data_to_game_coords(int *, int *) {}

int get_fixed_pixel_size(int pixels) { return pixels; }

void wouttext_outline(Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, const char *texx) {
    (void)ds; (void)xxp; (void)yyp; (void)usingfont; (void)text_color; (void)texx;
}

void wouttext_outline(Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color,
                      color_t outline_color, const char *texx) {
    (void)ds; (void)xxp; (void)yyp; (void)usingfont; (void)text_color; (void)outline_color; (void)texx;
}

void set_eip_guiobj(int eip) { (void)eip; }
int get_eip_guiobj() { return 0; }

bool ShouldAntiAliasText() { return false; }

int my_setcolor(int col, int, bool) { return col; }
