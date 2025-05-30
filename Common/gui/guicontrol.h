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
#ifndef __AGS_CN_GUI__GUICONTROL_H
#define __AGS_CN_GUI__GUICONTROL_H

#include "core/types.h"
#include "gfx/bitmap.h"
#include "gui/guidefines.h"
#include "gui/guiobject.h"
#include "util/string.h"

struct KeyInput;


namespace AGS
{
namespace Common
{

class GUIControl : public GUIObject
{
public:
    GUIControl() = default;
    virtual ~GUIControl() = default;

    // Properties
    int             GetParentID() const { return _parentID; }
    void            SetParentID(int parent_id);

    bool            IsClickable() const override { return (_flags & kGUICtrl_Clickable) != 0; }
    bool            IsDeleted() const { return (_flags & kGUICtrl_Deleted) != 0; }
    bool            IsEnabled() const override { return (_flags & kGUICtrl_Enabled) != 0; }
    bool            IsTranslated() const { return (_flags & kGUICtrl_Translated) != 0; }
    bool            IsVisible() const override { return (_flags & kGUICtrl_Visible) != 0; }
    bool            IsWrapText() const { return (_flags & kGUICtrl_WrapText) != 0; }
    void            SetClickable(bool on) override;
    void            SetEnabled(bool on) override;
    void            SetTranslated(bool on);
    void            SetVisible(bool on) override;
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
    virtual void    Draw(Bitmap *ds, int x = 0, int y = 0) { (void)ds; (void)x; (void)y; }
    // Gets whether the *local GUI coordinates* are over this control
    bool            IsOverControl(int x, int y, int leeway) const;

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
    virtual void    OnMouseMove(int /*mx*/, int /*my*/) { }
    // Mouse button up
    virtual void    OnMouseUp() { }

    // Serialization
    virtual void    ReadFromFile(Common::Stream *in, GuiVersion gui_version);
    virtual void    WriteToFile(Common::Stream *out) const;
    virtual void    ReadFromSavegame(Common::Stream *in, GuiSvgVersion svg_ver);
    virtual void    WriteToSavegame(Common::Stream *out) const;

    // Marks object as graphically changed.
    // NOTE: this only matters if object's own graphic changes (content, size etc),
    // but not its state (visible) or texture drawing mode (transparency, etc).
    void            MarkChanged() override;
    // Notifies parent GUI that this control has changed its visual state
    void            MarkVisualStateChanged() override;
    // Notifies parent GUI that this control has changed its location (pos, size)
    void            MarkPositionChanged(bool self_changed) override;
    // Notifies parent GUI that this control's interactive state has changed
    void            MarkStateChanged(bool self_changed, bool parent_changed);
  
protected:
    // Overridable routine to determine whether the coordinates is over the control;
    // coordinates are guaranteed to be transformed to the control's local cs
    virtual bool    IsOverControlImpl(int x, int y, int leeway) const;

    int      _parentID = -1;// id of parent GUI

    uint32_t _flags = kGUICtrl_DefFlags; // generic style and behavior flags
    bool     _isActivated = false; // signals user interaction

    // TODO: explicit event names & handlers for every event
    // FIXME: these must be static!! per type
    uint32_t _scEventCount = 0u;               // number of supported script events
    String   _scEventNames[MAX_GUIOBJ_EVENTS]; // script event names
    String   _scEventArgs[MAX_GUIOBJ_EVENTS];  // script handler params
    String   _eventHandlers[MAX_GUIOBJ_EVENTS]; // script function names
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GUI__GUICONTROL_H
