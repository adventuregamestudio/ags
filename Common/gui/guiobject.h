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
//
// GUIObject is a parent class for GUI and GUI controls.
//
//=============================================================================
#ifndef __AGS_CN_GUI__GUIOBJECT_H
#define __AGS_CN_GUI__GUIOBJECT_H

#include "core/types.h"
#include "game/scripteventtable.h"
#include "gfx/gfx_def.h"
#include "util/geometry.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class GUIObject
{
public:
    virtual ~GUIObject() = default;

    // Properties
    const String   &GetName() const { return _name; }
    void            SetName(const String &name);
    int             GetID() const { return _id; }
    void            SetID(int id);

    // TODO: the GUI/GUIControl implementations used separate state flag values historically;
    // it's possible to share these, in which case these methods can be made non-virtual and inlined,
    // but that would require updating data formats.
    virtual bool    IsClickable() const = 0;
    virtual bool    IsEnabled() const = 0;
    virtual bool    IsVisible() const = 0;
    virtual void    SetClickable(bool on) = 0;
    virtual void    SetEnabled(bool on) = 0;
    virtual void    SetVisible(bool on) = 0;

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
    Pointf          GetScale() const { return _scale; }
    void            SetScale(const Pointf &scale) { SetScale(scale.X, scale.Y); }
    void            SetScale(float sx, float sy);
    float           GetRotation() const { return _rotation; }
    void            SetRotation(float degrees);
    int             GetTransparency() const { return _transparency; }
    void            SetTransparency(int trans);
    // Sets transparency as a legacy 255-unit value
    // Sets transparency as a percentage (0 - 100) where 100 = invisible
    void            SetTransparencyAsPercentage(int percent);
    BlendMode       GetBlendMode() const { return _blendMode; }
    void            SetBlendMode(BlendMode blend_mode);
    int             GetShaderID() const { return _shaderID; }
    int             GetShaderHandle() const { return _shaderHandle; }
    void            SetShader(int shader_id, int shader_handle);
    int             GetZOrder() const { return _zOrder; }
    void            SetZOrder(int zorder);

    // Script Events
    // Gets a events schema corresponding to this object's type
    virtual const ScriptEventSchema *GetTypeEventSchema() const { return nullptr; }
    // Gets a (optional) script module which contains handlers
    // for this object's events
    const String   &GetScriptModule() const { return _events.GetScriptModule(); }
    // Sets a script module for this object's events
    void            SetScriptModule(const String &scmodule);
    // Provides a script events table
    const ScriptEventTable &GetEvents() const { return _events; }
    ScriptEventTable &GetEvents() { return _events; }
    // Gets a particular event's handler by event's index
    String          GetEventHandler(uint32_t event) const;
    // Sets a particular event's handler by event's index;
    // this function succeeds only if this index is found in the events schema
    void            SetEventHandler(uint32_t event, const String &fn_name);
    // Remap old-format events into new event table
    virtual void    RemapOldEvents();

    // Returns GUI object's graphic space params
    inline const GraphicSpace &GetGraphicSpace() const { return _gs; }

    // Operations
    // Returns the (untransformed!) visual rectangle of this control,
    // in *relative* coordinates, optionally clipped by the logical size
    virtual Rect    CalcGraphicRect(bool /*clipped*/) { return RectWH(0, 0, _width, _height); }

    // Events
    // TODO: get OnKey/Mouse events into this parent class too
    // 

    // Tells if object has graphically changed recently
    bool            HasChanged() const { return _hasChanged; }
    // TODO: hide these Mark* methods under protected, they should not be exposed,
    // unless there's a clear need.
    // 
    // Marks object as graphically changed.
    // NOTE: this only matters if object's own graphic changes (content, size etc),
    // but not its state (visible) or texture drawing mode (transparency, etc).
    virtual void    MarkChanged();
    // Marks object has its position changed, "self_changed" flag tells
    // if the whole object's rectangle has changed (otherwise it's just a x,y change).
    virtual void    MarkPositionChanged(bool self_changed, bool transform_changed);
    // Marks object has its visual state changed (transparency, blend mode, etc)
    virtual void    MarkVisualStateChanged();
    // Clears changed flag
    virtual void    ClearChanged();

    // Recalculate graphic space using current object properties
    void            UpdateGraphicSpace();
  
protected:
    GUIObject(const ScriptEventSchema *schema);

    // Object was resized; derived classes may override this and implement
    // their own additional handling
    virtual void    OnResized();

    int     _id = -1;      // GUI object's identifier
    String  _name;         // script name

    int     _x = 0;
    int     _y = 0;
    int     _width = 0;
    int     _height = 0;
    Pointf  _scale = Pointf(1.f, 1.f);; // x,y scale
    float   _rotation = 0.f;    // rotation, in degrees
    int     _zOrder = 0;

    int     _transparency = 0; // "incorrect" alpha (in legacy 255-range units)
    BlendMode _blendMode = kBlend_Normal;
    int     _shaderID = 0;
    int     _shaderHandle = 0; // runtime script shader handle

    ScriptEventTable _events = {};

    GraphicSpace _gs;
    bool    _hasChanged = false;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GUI__GUIOBJECT_H
