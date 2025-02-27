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
    bool            IsDeleted() const;
    // tells if control itself is enabled
    bool            IsEnabled() const;
    // overridable routine to determine whether the mouse is over the control
    virtual bool    IsOverControl(int x, int y, int leeway) const;
    bool            IsTranslated() const;
    bool            IsVisible() const;
    // implemented separately in engine and editor
    bool            IsClickable() const;
    int             GetTransparency() const { return _transparency; }
    // Compatibility: should the control's graphic be clipped to its x,y,w,h
    virtual bool    IsContentClipped() const { return true; }
    // Tells if the object image supports alpha channel
    virtual bool    HasAlphaChannel() const { return false; }
    
    // Operations
    // Returns the (untransformed!) visual rectangle of this control,
    // in *relative* coordinates, optionally clipped by the logical size
    virtual Rect    CalcGraphicRect(bool /*clipped*/) { return RectWH(0, 0, Width, Height); }
    virtual void    Draw(Bitmap *ds, int x = 0, int y = 0) { (void)ds; (void)x; (void)y; }
    void            SetClickable(bool on);
    void            SetEnabled(bool on);
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
    virtual void    OnResized() { MarkChanged(); }

    // Serialization
    virtual void    ReadFromFile(Common::Stream *in, GuiVersion gui_version);
    virtual void    WriteToFile(Common::Stream *out) const;
    virtual void    ReadFromSavegame(Common::Stream *in, GuiSvgVersion svg_ver);
    virtual void    WriteToSavegame(Common::Stream *out) const;

// TODO: these members are currently public; hide them later
public:
    // Manually marks GUIObject as graphically changed
    // NOTE: this only matters if control's own graphic changes (content, size etc),
    // but not its state (visible) or texture drawing mode (transparency, etc).
    void     MarkChanged();
    // Notifies parent GUI that this control has changed its state (but not graphic)
    void     NotifyParentChanged();
    bool     HasChanged() const;
    void     ClearChanged();

    int32_t  Id;         // GUI object's identifier
    int32_t  ParentId;   // id of parent GUI
    String   Name;       // script name

    int32_t  X;
    int32_t  Y;
    int32_t  Width;
    int32_t  Height;
    int32_t  ZOrder;
    bool     IsActivated; // signals user interaction

    String   EventHandlers[MAX_GUIOBJ_EVENTS]; // script function names
  
protected:
    uint32_t Flags;      // generic style and behavior flags
    int32_t  _transparency; // "incorrect" alpha (in legacy 255-range units)
    bool     _hasChanged;

    // TODO: explicit event names & handlers for every event
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
extern AGS::Common::GuiDisableStyle all_buttons_disabled;
// Tells if the given control is considered enabled, taking global flag into account
inline bool IsGUIEnabled(AGS::Common::GUIObject *g) { return (all_buttons_disabled < 0) && g->IsEnabled(); }

#endif // __AC_GUIOBJECT_H
