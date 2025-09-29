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
    typedef std::pair<GUIControlType, int> ControlRef;

    GUIMain() = default;

    // Properties
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

    int     GetID() const { return _id; }
    void    SetID(int id) { _id = id; }
    const String &GetName() const { return _name; }
    void    SetName(const String &name) { _name = name; }
    int     GetX() const { return _x; }
    void    SetX(int x);
    int     GetY() const { return _y; }
    void    SetY(int y);
    int     GetWidth() const { return _width; }
    void    SetWidth(int width);
    int     GetHeight() const { return _height; }
    void    SetHeight(int height);
    Point   GetPosition() const { return Point(_x, _y); }
    void    SetPosition(int x, int y);
    void    SetPosition(const Point &pos) { SetPosition(pos.X, pos.Y); }
    Size    GetSize() const { return Size(_width, _height); }
    void    SetSize(int width, int height);
    void    SetSize(const Size &sz) { SetSize(sz.Width, sz.Height); }
    Rect    GetRect() const { return RectWH(_x, _y, _width, _height); }
    int     GetBgColor() const { return _bgColor; }
    void    SetBgColor(int color);
    int     GetFgColor() const { return _fgColor; }
    void    SetFgColor(int color);
    int     GetBgImage() const { return _bgImage; }
    void    SetBgImage(int image);
    GUIPopupStyle GetPopupStyle() const { return _popupStyle; }
    void    SetPopupStyle(GUIPopupStyle style);
    int     GetPopupAtY() const { return _popupAtMouseY; }
    void    SetPopupAtY(int popup_aty);
    int     GetPadding() const { return _padding; }
    void    SetPadding(int padding);
    int     GetTransparency() const { return _transparency; }
    void    SetTransparency(int trans);
    int     GetZOrder() const { return _zOrder; }
    void    SetZOrder(int zorder);
    const String &GetScriptModule() const { return _scriptModule; }
    void    SetScriptModule(const String &scmodule);
    const String &GetOnClickHandler() const { return _onClickHandler; }
    void    SetOnClickHandler(const String &handler);

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
    int     FindControlAt(int atx, int aty, int leeway = 0, bool must_be_clickable = true) const;
    // Returns the last control which was under mouse cursor, or -1 if none
    int     GetControlUnderMouse() const;
    // Gets the number of the GUI child controls
    int     GetControlCount() const;
    // Gets control by its child's index
    GUIObject *GetControl(int index) const;
    // Gets child control's type, looks up with child's index
    GUIControlType GetControlType(int index) const;
    // Gets child control's global ID, looks up with child's index
    int     GetControlID(int index) const;
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
    // Sets highlighted control index
    void    SetHighlightControl(int control_index);

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
    int     FindControlAtLocal(int atx, int aty, int leeway, bool must_be_clickable) const;

    static const int DefaultBgColor = 8;
    static const int DefaultFgColor = 1;

    int     _id = 0;            // GUI identifier
    String  _name;              // the name of the GUI

    int     _x = 0;
    int     _y = 0;
    int     _width = 0;
    int     _height = 0;
    color_t _bgColor = DefaultBgColor; // background color
    int     _bgImage = 0;       // background sprite index
    color_t _fgColor = DefaultFgColor; // foreground color (used as border color in normal GUIs,
                                // and text color in text windows)
    int     _padding = TEXTWINDOW_PADDING_DEFAULT; // padding surrounding a GUI text window
    GUIPopupStyle _popupStyle = kGUIPopupNormal; // GUI popup behavior
    int     _popupAtMouseY = -1; // popup when mousey < this
    int     _transparency = 0;  // "incorrect" alpha (in legacy 255-range units)
    int     _zOrder = 0;

    int     _focusCtrl     = -1; // which control has the focus
    int     _highlightCtrl = -1; // which control has the bounding selection rect
    int     _mouseOverCtrl = -1; // which control has the mouse cursor over it
    int     _mouseDownCtrl = -1; // which control has the mouse button pressed on it
    Point   _mouseWasAt = { -1, -1 }; // last mouse cursor position

    String  _scriptModule;      // (optional) script module which contains callbacks
                                // for this GUI and its controls
    String  _onClickHandler;    // script function name

    int     _flags = kGUIMain_DefFlags; // style and behavior flags
    bool    _hasChanged = false; // flag tells whether GUI has graphically changed recently
    bool    _hasControlsChanged = false; // flag tells that GUI controls have changed position or image
    bool    _polling = false;   // inside the polling process

    // Array of types and control indexes in global GUI object arrays;
    // maps GUI child slots to actual controls and used for rebuilding Controls array
    std::vector<ControlRef> _ctrlRefs;
    // Array of child control references (not exclusively owned!)
    std::vector<GUIObject*> _controls;
    // Sorted array of controls in z-order.
    std::vector<int>        _ctrlDrawOrder;
};


// Global GUI options
struct GuiOptions
{
    // Clip GUI control's contents to the control's rectangle
    bool ClipControls = true;
    // How the GUI controls are drawn when the interface is disabled.
    // This parameter is normally checked only when the *individual* gui control
    // is disabled on its own, and we need to know which visual effect to use.
    // If you need to check the global interface disabled state,
    // then refer to GuiContext::DisabledState instead.
    GuiDisableStyle DisabledStyle = kGuiDis_Unchanged;
    // Whether draw disabled effect over inventory windows
    bool GreyOutInvWindow = false;
    // Whether to graphically outline GUI controls
    bool OutlineControls = false;
};

class SpriteCache;

// Global GUI context, affects controls behavior (drawing, updating)
struct GuiContext
{
    // Sprite cache, for GUI drawing in software mode
    SpriteCache *Spriteset = nullptr;
    // Current disabled state.
    // Disabled state is set whenever the interface gets disabled, copying value from
    // the Disabled style. If it's set to anything but "undefined" - that means that
    // the interface is globally disabled now.
    GuiDisableStyle DisabledState = kGuiDis_Undefined;
    // Optional index of GUI and control excluded from applying a disabled effect.
    // This is currently used for the blocking Button Animation, because hiding
    // or greying out gui control that animates defeats the purpose of animating.
    int GuiExcludedFromDisabled = -1;
    int GuiControlExcludedFromDisabled = -1;
    // Resolved macro values for the label:
    // A title of the game
    String GameTitle;
    // Total game score
    int TotalScore = 0;
    // Current game score
    int Score = 0;
    // A label from the object under the cursor
    String Overhotspot;
    // Last selected inventory item's pic (for the button)
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

    // Tells if the game interface is currently in disabled state
    inline bool IsEnabledState()
    {
        return GUI::Context.DisabledState == kGuiDis_Undefined;
    }

    // Tells if the GUI should be drawn, taking global disabled interface state into account
    inline bool IsGUIVisible(const GUIMain *g)
    {
        return g->IsDisplayed() && 
            // If GUI is excluded from the disabled interface, then don't check the disabled state
            ((GUI::Context.GuiExcludedFromDisabled == g->GetID()) ||
            // If GUI is in a disabled state, which tells to hide guis, and current gui
            // does not have a "persistent" property, then it should not be visible
            !((GUI::Context.DisabledState == kGuiDis_Off) && (g->GetPopupStyle() != kGUIPopupNoAutoRemove)));
    }

    // Tells if the gui control should be excluded from the disabled state check
    inline bool IsExcludedFromDisabled(const GUIObject *g)
    {
        return (g->GetID() == GUI::Context.GuiControlExcludedFromDisabled) &&
            (g->GetParentID() == GUI::Context.GuiExcludedFromDisabled);
    }

    // Tells if the gui control is considered enabled, taking global disabled interface state into account
    inline bool IsGUIEnabled(const GUIObject *g)
    {
        return g->IsEnabled() && (GUI::Context.DisabledState == kGuiDis_Undefined);
    }

    // Tells if the gui control is considered visible, taking global disabled interface state into account
    inline bool IsGUIVisible(const GUIObject *g)
    {
        return g->IsVisible() &&
            // If control is excluded from the disabled interface, then don't check the disabled state
            (IsExcludedFromDisabled(g) ||
            // Control is also not shown if...
            // If interface is globally disabled and all controls should be hidden
            !(GUI::Context.DisabledState == kGuiDis_Blackout) &&
            // If control is individually disabled AND controls should be hidden when disabled
            !(!g->IsEnabled() && (GUI::Options.DisabledStyle == kGuiDis_Blackout)));
    }

    // Tells if the gui control should be drawn with disabled effect
    inline bool ShouldDrawDisabled(const GUIObject *g)
    {
        return 
            // If control is excluded from the disabled interface, then don't check the disabled state
            !IsExcludedFromDisabled(g) &&
            // If either whole interface is disabled with "grey out" style,
            // or this control is individually disabled, and "grey out" style is set
            ((GUI::Context.DisabledState == kGuiDis_Greyout) ||
            (!g->IsEnabled() && (GUI::Options.DisabledStyle == kGuiDis_Greyout)));
    }

    // Tells if the inventory window should be drawn with disabled effect;
    // this is a special handling for inventory window, which has its own setting in the engine
    inline bool ShouldDrawDisabled(const GUIInvWindow *inv)
    {
        // If either whole interface is disabled with "grey out" style,
        // or this control is individually disabled, and "grey out" style is set.
        // Inventory windows have a separate global option that tells if they should be greyed out
        return (GUI::Options.GreyOutInvWindow) &&
            ShouldDrawDisabled(static_cast<const GUIObject*>(inv));
    }

    // Tells if the child gui controls should not be drawn, because of the disabled state
    inline bool ShouldSkipControls(const GUIMain *g)
    {
        return (g->GetID() != GUI::Context.GuiExcludedFromDisabled) &&
            (GUI::Context.DisabledState == kGuiDis_Blackout);
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
    // Resolves macro tokens found in gui text, returns a new string where the macros are replaced by values
    String ResolveMacroTokens(const String &text);
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

    // Exclude the given gui control from disabled state. This means that it will not
    // be applied any "disabled" effect, nor will its parent GUI (if it matters).
    void SetExcludedFromDisabled(GUIObject *gc, bool on);
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
extern void wouttext_outline(AGS::Common::Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, color_t outline_color, const char *texx);

extern void set_our_eip(int eip);
extern void set_eip_guiobj(int eip);
extern int get_eip_guiobj();

#endif // __AC_GUIMAIN_H
