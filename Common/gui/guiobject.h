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

class GUIObject
{
public:
    GUIObject() = default;
    virtual ~GUIObject() = default;

    // Properties
    const String   &GetName() const { return _name; }
    void            SetName(const String &name);
    int             GetID() const { return _id; }
    void            SetID(int id);
    int             GetParentID() const { return _parentID; }
    void            SetParentID(int parent_id);

    bool            IsClickable() const { return (_flags & kGUICtrl_Clickable) != 0; }
    bool            IsDeleted() const { return (_flags & kGUICtrl_Deleted) != 0; }
    bool            IsEnabled() const { return (_flags & kGUICtrl_Enabled) != 0; }
    bool            IsTranslated() const { return (_flags & kGUICtrl_Translated) != 0; }
    bool            IsVisible() const { return (_flags & kGUICtrl_Visible) != 0; }
    bool            IsWrapText() const { return (_flags & kGUICtrl_WrapText) != 0; }
    // overridable routine to determine whether the mouse is over the control
    virtual bool    IsOverControl(int x, int y, int leeway) const;
    int             GetX() const { return _x; }
    void            SetX(int x);
    int             GetY() const { return _y; }
    void            SetY(int y);
    Point           GetPosition() const { return Point(_x, _y); }
    void            SetPosition(int x, int y);
    void            SetPosition(const Point &pos) { SetPosition(pos.X, pos.Y); }
    int             GetWidth() const { return _width; }
    void            SetWidth(int width);
    int             GetHeight() const { return _height; }
    void            SetHeight(int height);
    Size            GetSize() const { return Size(_width, _height); }
    void            SetSize(int width, int height);
    void            SetSize(const Size &sz) { SetSize(sz.Width, sz.Height); }
    Rect            GetRect() const { return RectWH(_x, _y, _width, _height); }
    int             GetTransparency() const { return _transparency; }
    void            SetTransparency(int trans);
    // Sets transparency as a legacy 255-unit value
    // Sets transparency as a percentage (0 - 100) where 100 = invisible
    void            SetTransparencyAsPercentage(int percent);
    BlendMode       GetBlendMode() const { return _blendMode; }
    void            SetBlendMode(BlendMode blend_mode);
    int             GetZOrder() const { return _zOrder; }
    void            SetZOrder(int zorder);
    void            SetClickable(bool on);
    void            SetEnabled(bool on);
    void            SetTranslated(bool on);
    void            SetVisible(bool on);
    bool            IsActivated() const { return _isActivated; }
    void            SetActivated(bool on);

    // Compatibility: should the control's graphic be clipped to its x,y,w,h
    virtual bool    IsContentClipped() const { return true; }

    int             GetEventCount() const { return _scEventCount; }
    String          GetEventName(uint32_t event) const;
    String          GetEventArgs(uint32_t event) const;
    // Gets a script function name for the given event
    String          GetEventHandler(uint32_t event) const;
    void            SetEventHandler(uint32_t event, const String &fn_name);
    
    // Operations
    // Returns the (untransformed!) visual rectangle of this control,
    // in *relative* coordinates, optionally clipped by the logical size
    virtual Rect    CalcGraphicRect(bool /*clipped*/) { return RectWH(0, 0, _width, _height); }
    virtual void    Draw(Bitmap *ds, int x = 0, int y = 0) { (void)ds; (void)x; (void)y; }

    // Events
    // Key pressed for control; returns if handled
    virtual bool    OnKeyPress(const KeyInput&) { return false; }
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

    // Manually marks GUIObject as graphically changed
    // NOTE: this only matters if control's own graphic changes, but not its
    // logical (visible, clickable, etc) or visual (e.g. transparency) state.
    void            MarkChanged();
    // Notifies parent GUI that this control has changed its visual state
    void            MarkParentChanged();
    // Notifies parent GUI that this control has changed its location (pos, size)
    void            MarkPositionChanged(bool self_changed);
    // Notifies parent GUI that this control's interactive state has changed
    void            MarkStateChanged(bool self_changed, bool parent_changed);
    bool            HasChanged() const { return _hasChanged; }
    void            ClearChanged();
  
protected:
    int      _id = -1;      // GUI object's identifier
    int      _parentID = -1;// id of parent GUI
    String   _name;         // script name

    int      _x = 0;
    int      _y = 0;
    int      _zOrder = 0;
    bool     _isActivated = false; // signals user interaction

    uint32_t _flags = kGUICtrl_DefFlags; // generic style and behavior flags
    int      _width = 0;
    int      _height = 0;
    int      _transparency = 0; // "incorrect" alpha (in legacy 255-range units)
    BlendMode _blendMode = kBlend_Normal;
    bool     _hasChanged = false;

    // TODO: explicit event names & handlers for every event
    // FIXME: these must be static!! per type
    uint32_t _scEventCount = 0u;               // number of supported script events
    String   _scEventNames[MAX_GUIOBJ_EVENTS]; // script event names
    String   _scEventArgs[MAX_GUIOBJ_EVENTS];  // script handler params
    String   _eventHandlers[MAX_GUIOBJ_EVENTS]; // script function names
};

} // namespace Common
} // namespace AGS

#endif // __AC_GUIOBJECT_H
