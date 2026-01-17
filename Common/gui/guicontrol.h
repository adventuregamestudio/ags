//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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
    bool            IsShowBorder() const { return (_flags & kGUICtrl_ShowBorder) != 0; }
    bool            IsSolidBackground() const { return (_flags & kGUICtrl_SolidBack) != 0; }
    void            SetClickable(bool on) override;
    void            SetEnabled(bool on) override;
    void            SetTranslated(bool on);
    void            SetVisible(bool on) override;
    void            SetShowBorder(bool on);
    void            SetSolidBackground(bool on);
    int             GetBackColor() const { return _backgroundColor; }
    void            SetBackColor(int color);
    int             GetBorderColor() const { return _borderColor; }
    void            SetBorderColor(int color);
    int             GetBorderWidth() const { return _borderWidth; }
    void            SetBorderWidth(int border_size);
    int             GetPaddingX() const { return _paddingX; }
    void            SetPaddingX(int padx);
    int             GetPaddingY() const { return _paddingY; }
    void            SetPaddingY(int pady);
    bool            IsActivated() const { return _isActivated; }
    void            SetActivated(bool on);

    // Compatibility: should the control's graphic be clipped to its x,y,w,h
    virtual bool    IsContentClipped() const { return true; }
    
    // Operations
    // FIXME: it was a mistake to have coordinate origin as arguments to this method,
    // as this is bug prone when writing drawing code. Use sub-bitmaps when drawing controls instead.
    // -- there's going to be extra problem with non-clipped controls mode though. In this case
    // we might need another "bitmap draw mode" where it only offsets coordinates zero, but does not
    // constraint the drawing (like sub-bitmaps normally do).
    virtual void    Draw(Bitmap *ds, int x = 0, int y = 0) { (void)ds; (void)x; (void)y; }
    // Gets whether the *local GUI coordinates* are over this control
    bool            IsOverControl(int x, int y, int leeway) const;
    // Update visual state forces control to recalculate its elements.
    virtual void    UpdateVisualState();

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
    virtual void    ReadFromFile(Stream *in, GuiVersion gui_version);
    virtual void    ReadFromFile_Ext363(Stream *in, GuiVersion gui_version);
    virtual void    WriteToFile(Stream *out) const;
    virtual void    ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver);
    virtual void    WriteToSavegame(Stream *out) const;

    // Upgrades the GUI control to default looks for 3.6.3
    virtual void    SetDefaultLooksFor363() { /* do nothing */ }

    // Marks object as graphically changed.
    // NOTE: this only matters if object's own graphic changes (content, size etc),
    // but not its state (visible) or texture drawing mode (transparency, etc).
    void            MarkChanged() override;
    // Notifies parent GUI that this control has changed its visual state
    void            MarkVisualStateChanged() override;
    // Notifies parent GUI that this control has changed its location (pos, size)
    void            MarkPositionChanged(bool self_changed, bool transform_changed) override;
    // Notifies parent GUI that this control's interactive state has changed
    void            MarkStateChanged(bool self_changed, bool parent_changed);
  
protected:
    GUIControl(const ScriptEventSchema *schema);

    // Reports that any of the basic colors have changed,
    // to let child control handle this according to their needs
    virtual void    OnColorsChanged();
    // Internal control's region (content region) was resized
    virtual void    OnContentRectChanged();
    // Object was resized; derived classes may override this and implement
    // their own additional handling
    void            OnResized() override;

    // Overridable routine to determine whether the coordinates is over the control;
    // coordinates are guaranteed to be transformed to the control's local cs
    virtual bool    IsOverControlImpl(int x, int y, int leeway) const;

    // Draws control frame box, using common border and background settings
    void            DrawControlFrame(Bitmap *ds, int x, int y);
    // Updates control's inner region and marks for redraw
    void            UpdateControlRect();

    int      _parentID = -1;// id of parent GUI

    uint32_t _flags = kGUICtrl_DefFlags; // generic style and behavior flags
    int      _backgroundColor = 0;
    int      _borderColor = 0;
    int      _borderWidth = 1;
    int      _paddingX = 0;
    int      _paddingY = 0;
    Rect     _innerRect; // control's contents rect (excludes border + padding)

    bool     _isActivated = false; // signals user interaction
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GUI__GUICONTROL_H
