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
#include "gfx/gfx_def.h"
#include "util/geometry.h"
#include "util/string.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

// There were issues when including header caused conflicts
struct GameSetupStruct;

#define LEGACY_MAX_OBJS_ON_GUI             30

#define GUIMAIN_LEGACY_RESERVED_INTS       5
#define GUIMAIN_LEGACY_NAME_LENGTH         16
#define GUIMAIN_LEGACY_EVENTHANDLER_LENGTH 20
#define GUIMAIN_LEGACY_TW_FLAGS_SIZE       4

namespace AGS
{
namespace Common
{

// Legacy GUIMain visibility state, which combined Visible property and override factor
enum LegacyGUIVisState
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

    void        InitDefaults();

    // Tells if the gui background supports alpha channel
    bool        HasAlphaChannel() const;
    // Tells if GUI will react on clicking on it
    bool        IsClickable() const;
    // Tells if GUI's visibility is overridden and it won't be displayed on
    // screen regardless of Visible property (until concealed mode is off).
    bool        IsConcealed() const;
    // Tells if gui is actually displayed on screen. Normally Visible property
    // determines whether GUI is allowed to be seen, but there may be other
    // settings that override GUI's visibility.
    bool        IsDisplayed() const;
    // Tells if given coordinates are within interactable area of gui
    // NOTE: this currently tests for actual visibility and Clickable property
    bool        IsInteractableAt(int x, int y) const;
    // Tells if gui is a text window
    bool        IsTextWindow() const;
    // Tells if GUI is *allowed* to be displayed and interacted with.
    // This does not necessarily mean that it is displayed right now, because
    // GUI may be hidden for other reasons, including overriding behavior.
    // For example GUI with kGUIPopupMouseY style will not be shown unless
    // mouse cursor is at certain position on screen.
    bool        IsVisible() const;

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
    // Sets whether GUI should react to player clicking on it
    void    SetClickable(bool on);
    // Override GUI visibility; when in concealed mode GUI won't show up
    // even if Visible = true
    void    SetConceal(bool on);
    // Attempts to change control's zorder; returns if zorder changed
    bool    SetControlZOrder(int index, int zorder);
    // Sets GUI transparency as a percentage (0 - 100) where 100 = invisible
    void    SetTransparencyAsPercentage(int percent);
    // Sets whether GUI is allowed to be displayed on screen
    void    SetVisible(bool on);

    // Events
    void    OnMouseButtonDown();
    void    OnMouseButtonUp();
    void    OnControlPositionChanged();
  
    // Serialization
    void    ReadFromFile(Stream *in, GuiVersion gui_version);
    void    WriteToFile(Stream *out) const;
    // TODO: move to engine, into gui savegame component unit
    // (should read/write GUI properties accessing them by interface)
    void    ReadFromSavegame(Stream *in, GuiSvgVersion svg_version);
    void    WriteToSavegame(Stream *out) const;

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
};


namespace GUI
{
    extern GuiVersion GameGuiVersion;

    // Draw standart "shading" effect over rectangle
    void DrawDisabledEffect(Bitmap *ds, const Rect &rc);
    // Draw text aligned inside rectangle
    void DrawTextAligned(Bitmap *ds, const char *text, int font, color_t text_color, const Rect &frame, FrameAlignment align);
    // Draw text aligned horizontally inside given bounds
    void DrawTextAlignedHor(Bitmap *ds, const char *text, int font, color_t text_color, int x1, int x2, int y, FrameAlignment align);

    // TODO: remove is_savegame param after dropping support for old saves
    // because only they use ReadGUI to read runtime GUI data
    void ReadGUI(std::vector<GUIMain> &guis, Stream *in, bool is_savegame = false);
    void WriteGUI(const std::vector<GUIMain> &guis, Stream *out);
    // Converts legacy GUIVisibility into appropriate GUIMain properties
    void ApplyLegacyVisibility(GUIMain &gui, LegacyGUIVisState vis);
}

} // namespace Common
} // namespace AGS

extern std::vector<Common::GUIMain> guis;
extern int all_buttons_disabled, gui_inv_pic;
extern int gui_disabled_style;
extern char lines[MAXLINE][200];
extern int  numlines;

extern int mousex, mousey;

extern int get_adjusted_spritewidth(int spr);
extern int get_adjusted_spriteheight(int spr);
extern bool is_sprite_alpha(int spr);

// This function has distinct implementations in Engine and Editor
extern void draw_gui_sprite(Common::Bitmap *ds, int spr, int x, int y, bool use_alpha = true,
                            Common::BlendMode blend_mode = Common::kBlendMode_Alpha);

extern AGS_INLINE int divide_down_coordinate(int coord);
extern AGS_INLINE int multiply_up_coordinate(int coord);
extern AGS_INLINE void multiply_up_coordinates(int *x, int *y);
extern AGS_INLINE int get_fixed_pixel_size(int pixels);

// Those function have distinct implementations in Engine and Editor
extern void wouttext_outline(Common::Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, const char *texx);
extern int wgettextwidth_compensate(Common::Bitmap *ds, const char *tex, int font) ;
extern void check_font(int *fontnum);

extern void set_our_eip(int eip);
#define SET_EIP(x) set_our_eip(x);
extern void set_eip_guiobj(int eip);
extern int get_eip_guiobj();

extern bool outlineGuiObjects;

#endif // __AC_GUIMAIN_H
