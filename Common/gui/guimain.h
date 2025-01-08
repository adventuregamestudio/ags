//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_GUIMAIN_H
#define __AC_GUIMAIN_H

#include <vector>
#include "ac/common_defines.h" // TODO: split out gui drawing helpers
#include "gfx/gfx_def.h" // TODO: split out gui drawing helpers
#include "gui/guidefines.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"
#include "util/error.h"
#include "util/geometry.h"
#include "util/string.h"

class SplitLines;

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
    kGUIVisibility_LockedOff = -1, // locked hidden (used by PopupMouseY guis)
    kGUIVisibility_Off       =  0, // hidden
    kGUIVisibility_On        =  1  // shown
};

// GUICollection is a helper struct for grouping gui object arrays
// together when passing them to a processing function.
struct GUICollection
{
    std::vector<GUIButton>  Buttons;
    std::vector<GUIInvWindow> InvWindows;
    std::vector<GUILabel>   Labels;
    std::vector<GUIListBox> ListBoxes;
    std::vector<GUISlider>  Sliders;
    std::vector<GUITextBox> TextBoxes;
};

// GUIRefCollection is a helper struct for grouping *references* to
// gui object arrays together when passing them to a processing function.
struct GUIRefCollection
{
    std::vector<GUIButton>  &Buttons;
    std::vector<GUIInvWindow> &InvWindows;
    std::vector<GUILabel>   &Labels;
    std::vector<GUIListBox> &ListBoxes;
    std::vector<GUISlider>  &Sliders;
    std::vector<GUITextBox> &TextBoxes;

    GUIRefCollection(std::vector<GUIButton> &guibuts,
        std::vector<GUIInvWindow> &guiinv, std::vector<GUILabel> &guilabels,
        std::vector<GUIListBox> &guilist, std::vector<GUISlider> &guislider,
        std::vector<GUITextBox> &guitext)
        : Buttons(guibuts), InvWindows(guiinv), Labels(guilabels)
        , ListBoxes(guilist), Sliders(guislider), TextBoxes(guitext) {}
    GUIRefCollection(GUICollection &guiobjs)
        : Buttons(guiobjs.Buttons), InvWindows(guiobjs.InvWindows), Labels(guiobjs.Labels)
        , ListBoxes(guiobjs.ListBoxes), Sliders(guiobjs.Sliders), TextBoxes(guiobjs.TextBoxes) {}
};

class Bitmap;


class GUIMain
{
public:
    // ControlRef describes a child control type and its index in an external list
    typedef std::pair<GUIControlType, int32_t> ControlRef;

    GUIMain();

    void    InitDefaults();

    // Tells if the gui background supports alpha channel
    bool    HasAlphaChannel() const;
    // Tells if GUI will react on clicking on it
    bool    IsClickable() const { return (_flags & kGUIMain_Clickable) != 0; }
    // Tells if GUI's visibility is overridden and it won't be displayed on
    // screen regardless of Visible property (until concealed mode is off).
    bool    IsConcealed() const { return (_flags & kGUIMain_Concealed) != 0; }
    // Tells if gui is actually meant to be displayed on screen.
    // Normally Visible property determines whether GUI is allowed to be seen,
    // but there may be other settings that override it.
    bool    IsDisplayed() const { return IsVisible() && !IsConcealed(); }
    // Tells if given coordinates are within interactable area of gui
    // NOTE: this currently tests for actual visibility and Clickable property
    bool    IsInteractableAt(int x, int y) const;
    // Tells if gui is a text window
    bool    IsTextWindow() const { return (_flags & kGUIMain_TextWindow) != 0; }
    // Tells if GUI is *allowed* to be displayed and interacted with.
    // This does not necessarily mean that it is displayed right now, because
    // GUI may be hidden for other reasons, including overriding behavior.
    // For example GUI with kGUIPopupMouseY style will not be shown unless
    // mouse cursor is at certain position on screen.
    bool    IsVisible() const { return (_flags & kGUIMain_Visible) != 0; }

    // Tells if GUI has graphically changed recently
    bool    HasChanged() const { return _hasChanged; }
    bool    HasControlsChanged() const { return _hasControlsChanged; }
    // Manually marks GUI as graphically changed.
    // NOTE: this only matters if GUI's own graphic changes (content, size etc),
    // but not its state (visible) or texture drawing mode (transparency, etc).
    void    MarkChanged();
    // Marks GUI as having any of its controls changed its looks.
    void    MarkControlChanged();
    // Clears changed flag
    void    ClearChanged();
    // Notify GUI about any of its controls changing its location.
    void    NotifyControlPosition();
    // Notify GUI about one of its controls changing its interactive state.
    void    NotifyControlState(int objid, bool mark_changed);
    // Resets control-under-mouse detection.
    void    ResetOverControl();

    // Finds a control under given screen coordinates, returns control's child ID.
    // Optionally allows extra leeway (offset in all directions) to let the user grab tiny controls.
    // Optionally only allows clickable controls, ignoring non-clickable ones.
    int32_t FindControlAt(int atx, int aty, int leeway = 0, bool must_be_clickable = true) const;
    // Gets the number of the GUI child controls
    int32_t GetControlCount() const;
    // Gets control by its child's index
    GUIObject *GetControl(int index) const;
    // Gets child control's type, looks up with child's index
    GUIControlType GetControlType(int index) const;
    // Gets child control's global ID, looks up with child's index
    int32_t GetControlID(int index) const;
    // Gets an array of child control indexes in the z-order, from bottom to top
    const std::vector<int> &GetControlsDrawOrder() const;
    // Gets an array of child control references (control types and indexes in global control arrays)
    const std::vector<ControlRef> &GetControlRefs() const;

    // Child control management
    // Note that currently GUIMain does not own controls (should not delete them)
    void    AddControl(GUIControlType type, int id, GUIObject *control);
    void    RemoveAllControls();

    // Operations
    bool    BringControlToFront(int index);
    void    DrawSelf(Bitmap *ds);
    void    DrawWithControls(Bitmap *ds);
    void    DrawControls(Bitmap *ds);
    // Polls GUI state, providing current cursor (mouse) coordinates
    void    Poll(int mx, int my);
    // Reconnects this GUIMain with the child controls from the global guiobject collection
    HError  RebuildArray(GUIRefCollection &guiobjs);
    void    ResortZOrder();
    bool    SendControlToBack(int index);
    // Sets whether GUI should react to player clicking on it
    void    SetClickable(bool on);
    // Override GUI visibility; when in concealed mode GUI won't show up
    // even if Visible = true
    void    SetConceal(bool on);
    // Attempts to change control's zorder; returns if zorder changed
    bool    SetControlZOrder(int index, int zorder);
    // Changes GUI style to the text window or back
    void    SetTextWindow(bool on);
    // Sets GUI transparency as a percentage (0 - 100) where 100 = invisible
    void    SetTransparencyAsPercentage(int percent);
    // Sets whether GUI is allowed to be displayed on screen
    void    SetVisible(bool on);

    // Events
    void    OnMouseButtonDown(int mx, int my);
    void    OnMouseButtonUp();
  
    // Serialization
    void    ReadFromFile(Stream *in, GuiVersion gui_version);
    void    WriteToFile(Stream *out) const;
    // Savegame reading and writing
    // TODO: move to engine, but will require to split GUI into data and runtime classes
    // NOTE: when reading from a save state we read control refs into an external array,
    // instead of applying directly to GUI. That is because the older saves may contain
    // control arrays mismatching current game. This also *potentially* allows the engine
    // to fixup data read from the older saves.
    // Perhaps review this later (maybe when there's a split runtime GUI class).
    void    ReadFromSavegame(Stream *in, GuiSvgVersion svg_version, std::vector<ControlRef> &ctrl_refs);
    void    WriteToSavegame(Stream *out) const;
    static void SkipSavestate(Stream *in, GuiSvgVersion svg_version, std::vector<ControlRef> *ctrl_refs);

private:
    void    DrawBlob(Bitmap *ds, int x, int y, color_t draw_color);
    // Same as FindControlAt but expects local space coordinates
    int32_t FindControlAtLocal(int atx, int aty, int leeway, bool must_be_clickable) const;

    // TODO: all members are currently public; hide them later
public:
    int32_t ID;             // GUI identifier
    String  Name;           // the name of the GUI

    int32_t X;
    int32_t Y;
    int32_t Width;
    int32_t Height;
    color_t BgColor;        // background color
    int32_t BgImage;        // background sprite index
    color_t FgColor;        // foreground color (used as border color in normal GUIs,
                            // and text color in text windows)
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

    String  ScriptModule;   // (optional) script module which contains callbacks
                            // for this GUI and its controls
    String  OnClickHandler; // script function name

private:
    int32_t _flags;         // style and behavior flags
    bool    _hasChanged;    // flag tells whether GUI has graphically changed recently
    bool    _hasControlsChanged;
    bool    _polling;       // inside the polling process

    // Array of types and control indexes in global GUI object arrays;
    // maps GUI child slots to actual controls and used for rebuilding Controls array
    std::vector<ControlRef> _ctrlRefs;
    // Array of child control references (not exclusively owned!)
    std::vector<GUIObject*> _controls;
    // Sorted array of controls in z-order.
    std::vector<int32_t>    _ctrlDrawOrder;
};


// Global GUI options
struct GuiOptions
{
    // Clip GUI control's contents to the control's rectangle
    bool ClipControls = true;
    // How the GUI controls are drawn when the interface is disabled
    GuiDisableStyle DisabledStyle = kGuiDis_Unchanged;
    // Whether to graphically outline GUI controls
    bool OutlineControls = false;
};

class SpriteCache;

// Global GUI context, affects controls behavior (drawing, updating)
struct GuiContext
{
    // Sprite cache, for GUI drawing in software mode
    SpriteCache *Spriteset = nullptr;
    // Current disabled state
    GuiDisableStyle DisabledState = kGuiDis_Undefined;
    // Last selected inventory item's pic
    int InventoryPic = -1;
};

namespace GUI
{
    // These are still global objects for now, but in the future it will be
    // optimal to have these as an object allocated on heap, and passed to
    // GUI functions as a pointer.
    extern GuiVersion GameGuiVersion;
    extern GuiOptions Options;
    extern GuiContext Context;

    // Tells if the given control is considered enabled, taking global flag into account
    inline bool IsGUIEnabled(GUIObject *g)
    {
        return (GUI::Context.DisabledState == kGuiDis_Undefined) && g->IsEnabled();
    }

    // Fixups a GUI name loaded from v2.72 and earlier games
    String FixupGUIName272(const String &name);

    // Applies current text direction setting (may depend on multiple factors)
    String ApplyTextDirection(const String &text);
    // Calculates the text's draw position, given the alignment
    // optionally returns the real graphical rect that the text would occupy
    Point CalcTextPosition(const String &text, int font, const Rect &frame, FrameAlignment align, Rect *gr_rect = nullptr);
    // Calculates the text's draw position and horizontal extent,
    // using strictly horizontal alignment
    Line CalcTextPositionHor(const String &text, int font, int x1, int x2, int y, FrameAlignment align);
    // Calculates the graphical rect that the text would occupy
    // if drawn at the given coordinates
    Rect CalcTextGraphicalRect(const String &text, int font, const Point &at);
    // Calculates the graphical rect that the text would occupy
    // if drawn aligned to the given frame
    Rect CalcTextGraphicalRect(const String &text, int font, const Rect &frame, FrameAlignment align);
    // Calculates the graphical rect that the multiline text would occupy
    // if drawn aligned to the given frame
    Rect CalcTextGraphicalRect(const std::vector<String> &text, size_t item_count, int font, int linespace, 
        const Rect &frame, FrameAlignment align, bool limit_by_frame = true);
    // Calculates a vertical graphical extent for a given font,
    // which is a top and bottom offsets in zero-based coordinates.
    // NOTE: this applies font size fixups.
    Line CalcFontGraphicalVExtent(int font);
    // Draw standart "shading" effect over rectangle
    void DrawDisabledEffect(Bitmap *ds, const Rect &rc);
    // Draw text aligned inside rectangle
    void DrawTextAligned(Bitmap *ds, const String &text, int font, color_t text_color, const Rect &frame, FrameAlignment align);
    // Draw text aligned horizontally inside given bounds
    void DrawTextAlignedHor(Bitmap *ds, const String &text, int font, color_t text_color, int x1, int x2, int y, FrameAlignment align);
    // Draw wrapped text aligned inside rectangle:
    // a block of text is aligned vertically, while each line is aligned horizontally.
    void DrawTextLinesAligned(Bitmap *ds, const std::vector<String> &text, size_t item_count,
        int font, int linespace, color_t text_color, const Rect &frame, FrameAlignment align,
        bool limit_by_frame = true);

    // Parses the string and returns combination of label macro flags
    GUILabelMacro FindLabelMacros(const String &text);
    // Applies text transformation necessary for rendering, in accordance to the
    // current game settings, such as right-to-left render, and anything else
    String TransformTextForDrawing(const String &text, bool translate, bool apply_direction);
    // Wraps given text to make it fit into width, stores it in the lines;
    // apply_direction param tells whether text direction setting should be applied
    size_t SplitLinesForDrawing(const String &text, bool apply_direction, SplitLines &lines, int font, int width, size_t max_lines = -1);

    // Reads all GUIs and their controls.
    HError ReadGUI(std::vector<GUIMain> &guis, GUIRefCollection &guiobjs, Stream *in);
    // Writes all GUIs and their controls.
    void WriteGUI(const std::vector<GUIMain> &guis, const GUIRefCollection &guiobjs, Stream *out);
    // Converts legacy GUIVisibility into appropriate GUIMain properties
    void ApplyLegacyVisibility(GUIMain &gui, LegacyGUIVisState vis);

    // Rebuilds GUIs, connecting them to the child controls in memory.
    HError RebuildGUI(std::vector<GUIMain> &guis, GUIRefCollection &guiobjs);
}

} // namespace Common
} // namespace AGS

extern int get_adjusted_spritewidth(int spr);
extern int get_adjusted_spriteheight(int spr);
extern bool is_sprite_alpha(int spr);

// This function has distinct implementations in Engine and Editor
extern void draw_gui_sprite(AGS::Common::Bitmap *ds, int spr, int x, int y, bool use_alpha = true,
                            AGS::Common::BlendMode blend_mode = AGS::Common::kBlendMode_Alpha);
extern void draw_gui_sprite(AGS::Common::Bitmap *ds, bool use_alpha, int x, int y,
                            AGS::Common::Bitmap *image, bool src_has_alpha,
                            AGS::Common::BlendMode blend_mode, int alpha);

extern int game_to_data_coord(int coord);
extern int data_to_game_coord(int coord);
extern void data_to_game_coords(int *x, int *y);
extern int get_fixed_pixel_size(int pixels);

// Those function have distinct implementations in Engine and Editor
extern void wouttext_outline(AGS::Common::Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, const char *texx);

extern void set_our_eip(int eip);
extern void set_eip_guiobj(int eip);
extern int get_eip_guiobj();

#endif // __AC_GUIMAIN_H
