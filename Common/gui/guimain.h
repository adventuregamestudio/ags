//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// 
//
//=============================================================================
#ifndef __AGS_CN_GUI__GUIMAIN_H
#define __AGS_CN_GUI__GUIMAIN_H

#include "ac/common_defines.h"
#include "gfx/bitmap.h"
#include "gui/guidefines.h"
#include "util/array.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class GuiObject;

enum GuiControlType
{
    kGuiControlUndefined = -1,
    kGuiButton      = 1,
    kGuiLabel       = 2,
    kGuiInvWindow   = 3,
    kGuiSlider      = 4,
    kGuiTextBox     = 5,
    kGuiListBox     = 6
};

enum GuiMainFlags
{
    kGuiMain_NoClick    = 0x01,
    kGuiMain_TextWindow = 0x05
};

enum GuiPopupStyle
{
    kGuiPopupNone           = 0,
    kGuiPopupMouseY,
    kGuiPopupScript,
    // don't remove automatically during cutscene
    kGuiPopupNoAutoRemove,
    // normal GUI, initially off
    kGuiPopupNoneInitiallyOff
};

enum GuiDisabledStyle
{
    kGuiDisabled_GreyOut        = 0x01,
    kGuiDisabled_HideControls   = 0x02,
    kGuiDisabled_Unchanged      = 0x04,
    kGuiDisabled_Hide           = 0x08
};

class GuiMain
{
public:
    GuiMain();

    static String FixupGuiName(const String &name);
    static String MakeScriptName(const String &name);

    void       Init();

    int        FindControlUnderMouse() const;
    // this version allows some extra leeway in the Editor so that
    // the user can grab tiny controls
    int        FindControlUnderMouse(int leeway) const;
    int        FindControlUnderMouse(int leeway, bool must_be_clickable) const;
    GuiControlType GetControlType(int index) const;
    inline int GetX()      const { return Frame.Left; }
    inline int GetY()      const { return Frame.Top; }
    inline int GetWidth()  const { return Frame.GetWidth(); }
    inline int GetHeight() const { return Frame.GetHeight(); }
    bool       HasAlphaChannel() const;
    bool       IsMouseOnGui();
    bool       IsTextWindow() const;

    bool       BringControlToFront(int index);
    void       Draw(Bitmap *ds);
    void       DrawAt(Bitmap *ds, int x, int y);
    void       Poll();
    void       RebuildArray();
    void       ResortZOrder();
    bool       SendControlToBack(int index);
    void       SetX(int x);
    void       SetY(int y);
    void       SetWidth(int width);
    void       SetHeight(int height);
    void       SetTransparencyAsPercentage(int percent);

    void       OnControlPositionChanged();
    void       OnMouseButtonDown();
    void       OnMouseButtonUp();

    void       ReadFromFile(Stream *in, GuiVersion gui_version);
    void       WriteToFile(Stream *out);
    void       ReadFromSavedGame(Stream *in, RuntimeGuiVersion version);
    void       WriteToSavedGame(Stream *out);

private:
    void       DrawBlob(Bitmap *ds, int x, int y, color_t draw_color);

// TODO: all members are currently public; hide them later
public:
    int32_t         Id;
    String          Name;
    int32_t         Flags;
private:
    Rect            Frame;
public:
    color_t         BackgroundColor;
    int32_t         BackgroundImage;
    color_t         ForegroundColor;
    int32_t         Transparency;
    GuiPopupStyle   PopupStyle;
    int32_t         PopupAtMouseY; // popup when mousey < this
    String          OnClickHandler;

    Array<GuiObject*> Controls;
    Array<int32_t>    ControlRefs; // for re-building objs array
    Array<short>      ControlDrawOrder;

    bool            IsVisible;
    int32_t         ZOrder;
    int32_t         FocusedControl;
    int32_t         HighlightControl;
    int32_t         MouseOverControl;
    int32_t         MouseDownControl;
    Point           MouseWasAt;

    // TODO: remove these later
    int32_t         ControlCount;
};

namespace Gui
{
    bool ReadGui(ObjectArray<GuiMain> &guis, Stream *in);
    void WriteGui(ObjectArray<GuiMain> &guis, Stream *out);
    bool ReadGuiFromSavedGame(ObjectArray<GuiMain> &guis, Common::Stream *in, RuntimeGuiVersion version);
    void WriteGuiToSavedGame(ObjectArray<GuiMain> &guis, Common::Stream *out);
} // namespace Gui

} // namespace Common
} // namespace AGS

extern int guis_need_update;
extern int all_buttons_disabled, gui_inv_pic;
extern int gui_disabled_style;
extern char lines[MAXLINE][200];
extern int  numlines;

extern int mousex, mousey;

extern int get_adjusted_spritewidth(int spr);
extern int get_adjusted_spriteheight(int spr);
extern bool is_sprite_alpha(int spr);
extern int final_col_dep;

// This function has distinct implementations in Engine and Editor
extern void draw_sprite_compensate(AGS::Common::Bitmap *ds, int spr, int x, int y, int xray);

extern AGS_INLINE int divide_down_coordinate(int coord);
extern AGS_INLINE int multiply_up_coordinate(int coord);
extern AGS_INLINE void multiply_up_coordinates(int *x, int *y);
extern AGS_INLINE int get_fixed_pixel_size(int pixels);

// Those function have distinct implementations in Engine and Editor
extern void wouttext_outline(AGS::Common::Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, const char *texx);
extern int wgettextwidth_compensate(AGS::Common::Bitmap *ds, const char *tex, int font) ;
extern void check_font(int *fontnum);

extern void set_our_eip(int eip);
#define SET_EIP(x) set_our_eip(x);
extern void set_eip_guiobj(int eip);
extern int get_eip_guiobj();

extern bool outlineGuiObjects;

#endif // __AC_GUIMAIN_H
