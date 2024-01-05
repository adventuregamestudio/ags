//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#ifndef __AC_GUIOBJECT_H
#define __AC_GUIOBJECT_H

#include "core/types.h"
#include "gfx/bitmap.h"
#include "gui/guidefines.h"
#include "util/string.h"

struct KeyInput;


namespace AGS
{
namespace Common
{

enum LegacyGUIAlignment
{
    kLegacyGUIAlign_Left   = 0,
    kLegacyGUIAlign_Right  = 1,
    kLegacyGUIAlign_Center = 2
};

class GUIObject
{
public:
    GUIObject();
    virtual ~GUIObject() = default;

    String          GetScriptName() const;

    String          GetEventArgs(int event) const;
    int             GetEventCount() const;
    String          GetEventName(int event) const;
    bool            IsClickable() const { return (Flags & kGUICtrl_Clickable) != 0; }
    bool            IsDeleted() const { return (Flags & kGUICtrl_Deleted) != 0; }
    bool            IsEnabled() const { return (Flags & kGUICtrl_Enabled) != 0; }
    bool            IsTranslated() const { return (Flags & kGUICtrl_Translated) != 0; }
    bool            IsVisible() const { return (Flags & kGUICtrl_Visible) != 0; }
    // overridable routine to determine whether the mouse is over the control
    virtual bool    IsOverControl(int x, int y, int leeway) const;
    Size            GetSize() const { return Size(_width, _height); }
    int             GetWidth() const { return _width; }
    int             GetHeight() const { return _height; }
    int             GetTransparency() const { return _transparency; }
    // Compatibility: should the control's graphic be clipped to its x,y,w,h
    virtual bool    IsContentClipped() const { return true; }
    // Tells if the object image supports alpha channel
    virtual bool    HasAlphaChannel() const { return false; }
    
    // Operations
    // Returns the (untransformed!) visual rectangle of this control,
    // in *relative* coordinates, optionally clipped by the logical size
    virtual Rect    CalcGraphicRect(bool /*clipped*/) { return RectWH(0, 0, _width, _height); }
    virtual void    Draw(Bitmap *ds, int x = 0, int y = 0) { (void)ds; (void)x; (void)y; }
    void            SetClickable(bool on);
    void            SetEnabled(bool on);
    void            SetSize(int width, int height);
    inline void     SetWidth(int width) { SetSize(width, _height); }
    inline void     SetHeight(int height) { SetSize(_width, height); }
    void            SetTranslated(bool on);
    void            SetVisible(bool on);
    void            SetTransparency(int trans);

    // Events
    // Key pressed for control
    virtual void    OnKeyPress(const KeyInput&) { }
    // Mouse button down - return 'True' to lock focus
    virtual bool    OnMouseDown() { return false; }
    // Mouse moves onto control
    virtual void    OnMouseEnter() { }
    // Mouse moves off control
    virtual void    OnMouseLeave() { }
    // Mouse moves over control - x,y relative to gui
    virtual void    OnMouseMove(int /*x*/, int /*y*/) { }
    // Mouse button up
    virtual void    OnMouseUp() { }
    // Control was resized
    virtual void    OnResized() { MarkPositionChanged(true); }

    // Serialization
    virtual void    ReadFromFile(Common::Stream *in, GuiVersion gui_version);
    virtual void    WriteToFile(Common::Stream *out) const;
    virtual void    ReadFromSavegame(Common::Stream *in, GuiSvgVersion svg_ver);
    virtual void    WriteToSavegame(Common::Stream *out) const;

// TODO: these members are currently public; hide them later
public:
    // Manually marks GUIObject as graphically changed
    // NOTE: this only matters if control's own graphic changes, but not its
    // logical (visible, clickable, etc) or visual (e.g. transparency) state.
    void     MarkChanged();
    // Notifies parent GUI that this control has changed its visual state
    void     MarkParentChanged();
    // Notifies parent GUI that this control has changed its location (pos, size)
    void     MarkPositionChanged(bool self_changed);
    // Notifies parent GUI that this control's interactive state has changed
    void     MarkStateChanged(bool self_changed, bool parent_changed);
    bool     HasChanged() const { return _hasChanged; }
    void     ClearChanged();

    int32_t  Id;         // GUI object's identifier
    int32_t  ParentId;   // id of parent GUI
    String   Name;       // script name

    int32_t  X;
    int32_t  Y;
    int32_t  ZOrder;
    bool     IsActivated; // signals user interaction

    String   EventHandlers[MAX_GUIOBJ_EVENTS]; // script function names
  
protected:
    uint32_t Flags;      // generic style and behavior flags
    int32_t  _width;
    int32_t  _height;
    int32_t  _transparency; // "incorrect" alpha (in legacy 255-range units)
    bool     _hasChanged;

    // TODO: explicit event names & handlers for every event
    // FIXME: these must be static!! per type
    int32_t  _scEventCount;                    // number of supported script events
    String   _scEventNames[MAX_GUIOBJ_EVENTS]; // script event names
    String   _scEventArgs[MAX_GUIOBJ_EVENTS];  // script handler params
};

// Converts legacy alignment type used in GUI Label/ListBox data (only left/right/center)
HorAlignment ConvertLegacyGUIAlignment(LegacyGUIAlignment align);
LegacyGUIAlignment GetLegacyGUIAlignment(HorAlignment align);

} // namespace Common
} // namespace AGS

// Tells if all controls are disabled
extern int all_buttons_disabled;
// Tells if the given control is considered enabled, taking global flag into account
inline bool IsGUIEnabled(AGS::Common::GUIObject *g) { return (all_buttons_disabled < 0) && g->IsEnabled(); }

#endif // __AC_GUIOBJECT_H
