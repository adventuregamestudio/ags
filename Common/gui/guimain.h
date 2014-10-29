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

#ifndef __AC_GUIMAIN_H
#define __AC_GUIMAIN_H

#include <vector>
#include "gui/guiobject.h"
#include "ac/common_defines.h"       // AGS_INLINE
#include "util/geometry.h"
#include "util/string.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

// There were issues when including header caused conflicts
struct GameSetupStruct;

#define LEGACY_MAX_OBJS_ON_GUI      30

#define GUIMAIN_RESERVED_INTS       5
#define GUIMAIN_NAME_LENGTH         16
#define GUIMAIN_EVENTHANDLER_LENGTH 20
#define GUIMAIN_LEGACY_TW_FLAGS_SIZE 4

#define TEXTWINDOW_PADDING_DEFAULT  3

namespace AGS
{
namespace Common
{

enum GUIControlType
{
    kGUIControlUndefined = -1,
    kGUIButton      = 1,
    kGUILabel       = 2,
    kGUIInvWindow   = 3,
    kGUISlider      = 4,
    kGUITextBox     = 5,
    kGUIListBox     = 6
};

enum GUIMainFlags
{
    kGUIMain_NoClick    = 0x01,
    kGUIMain_TextWindow = 0x02
};

enum GUIMainLegacyFlags
{
    kGUIMain_LegacyTextWindow = 5
};

enum GUIPopupStyle
{
    // normal GUI, initally on
    kGUIPopupNone             = 0,
    // show when mouse moves to top of screen
    kGUIPopupMouseY           = 1,
    // pauses game when shown
    kGUIPopupModal            = 2,
    // initially on and not removed when interface is off
    kGUIPopupNoAutoRemove     = 3,
    // normal GUI, initially off
    kGUIPopupNoneInitiallyOff = 4
};

enum GUIVisibilityState
{
    kGUIVisibility_Concealed = -1, // gui is hidden by command
    kGUIVisibility_Off       =  0, // gui is disabled (won't show up by command)
    kGUIVisibility_On        =  1  // gui is shown by command
};


class GUIMain
{
public:
    static String FixupGUIName(const String &name);

public:
    GUIMain();

    void Init();

    // Tells if the gui background supports alpha channel
    bool        HasAlphaChannel() const;
    // Tells if gui is allowed to be displayed, but is currently hidden off-screen
    inline bool IsConcealed() const { return _visibility == kGUIVisibility_Concealed; }
    // Tells if mouse cursor is currently over gui
    bool        IsMouseOnGUI() const;
    // Tells if gui visibility is disabled
    inline bool IsOff() const { return _visibility == kGUIVisibility_Off; }
    // Tells if gui is a text window
    bool        IsTextWindow() const;
    // Tells if gui is allowed to be displayed on screen
    inline bool IsVisible() const { return _visibility == kGUIVisibility_On; }

    int32_t FindControlUnderMouse() const;
    // this version allows some extra leeway in the Editor so that
    // the user can grab tiny controls
    int32_t FindControlUnderMouse(int leeway) const;
    int32_t FindControlUnderMouse(int leeway, bool must_be_clickable) const;
    GUIControlType GetControlType(int index) const;

    // Operations
    bool    BringControlToFront(int index);
    void    Draw(Bitmap *ds);
    void    DrawAt(Bitmap *ds, int x, int y);
    void    Poll();
    void    RebuildArray();
    void    ResortZOrder();
    bool    SendControlToBack(int index);
    // attempts to change control's zorder; returns if zorder changed
    bool    SetControlZOrder(int index, int zorder);
    void    SetTransparencyAsPercentage(int percent);
    void    SetVisibility(GUIVisibilityState visibility);

    // Events
    void    OnMouseButtonDown();
    void    OnMouseButtonUp();
    void    OnControlPositionChanged();
  
    // Serialization
    void    ReadFromFile(Common::Stream *in, GuiVersion gui_version);
    void    WriteToFile(Common::Stream *out, GuiVersion gui_version) const;

private:
    void    DrawBlob(Bitmap *ds, int x, int y, color_t draw_color);

    // TODO: all members are currently public; hide them later
public:
    int32_t Id;             // GUI identifier
    String  Name;           // the name of the GUI
    int32_t Flags;          // style and behavior flags

    int32_t X;
    int32_t Y;
    int32_t Width;
    int32_t Height;
    color_t BgColor;        // background color
    int32_t BgImage;        // background sprite index
    color_t FgColor;        // foreground color
    int32_t Padding;        // padding surrounding a GUI text window
    GUIPopupStyle PopupStyle; // GUI popup behavior
    int32_t PopupAtMouseY;  // popup when mousey < this
    int32_t Transparency;   // "incorrect" alpha (in legacy 255-range units)
    int32_t ZOrder;

    int32_t FocusCtrl;      // which control has the focus
    int32_t HighlightCtrl;  // which control has the bounding selection rect
    int32_t MouseOverCtrl;  // which control has the mouse cursor over it
    int32_t MouseDownCtrl;  // which control has the mouse button pressed on it
    Point   MouseWasAt;     // last mouse cursor position

    String  OnClickHandler; // script function name

    std::vector<GUIObject*> Controls; // array of child controls
    std::vector<int32_t>    CtrlRefs; // for re-building controls array
    std::vector<int16_t>    CtrlDrawOrder;

    // TODO: remove these later
    int32_t ControlCount;   // number of objects on gui

private:
    GUIVisibilityState _visibility;
};

} // namespace Common
} // namespace AGS

extern GuiVersion GameGuiVersion;
extern std::vector<Common::GUIMain> guis;
extern int all_buttons_disabled, gui_inv_pic;
extern int gui_disabled_style;
extern char lines[MAXLINE][200];
extern int  numlines;

extern void read_gui(Common::Stream *in, std::vector<Common::GUIMain> &guiread, GameSetupStruct * gss);
extern void write_gui(Common::Stream *out, const std::vector<Common::GUIMain> &guiwrite, GameSetupStruct * gss, bool savedgame);

extern int mousex, mousey;

extern int get_adjusted_spritewidth(int spr);
extern int get_adjusted_spriteheight(int spr);
extern bool is_sprite_alpha(int spr);

// This function has distinct implementations in Engine and Editor
extern void draw_gui_sprite(Common::Bitmap *ds, int spr, int x, int y, bool use_alpha);

extern AGS_INLINE int divide_down_coordinate(int coord);
extern AGS_INLINE int multiply_up_coordinate(int coord);
extern AGS_INLINE void multiply_up_coordinates(int *x, int *y);
extern AGS_INLINE int get_fixed_pixel_size(int pixels);

// Those function have distinct implementations in Engine and Editor
extern void wouttext_outline(Common::Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, char *texx);
extern int wgettextwidth_compensate(Common::Bitmap *ds, const char *tex, int font) ;
extern void check_font(int *fontnum);

extern void set_our_eip(int eip);
#define SET_EIP(x) set_our_eip(x);
extern void set_eip_guiobj(int eip);
extern int get_eip_guiobj();

extern bool outlineGuiObjects;

#endif // __AC_GUIMAIN_H
