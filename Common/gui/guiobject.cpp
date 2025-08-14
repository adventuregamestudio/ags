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
#include "gui/guiobject.h"
#include "gui/guimain.h"

namespace AGS
{
namespace Common
{

GUIObject::GUIObject()
{
    UpdateGraphicSpace();
}

void GUIObject::SetName(const String &name)
{
    _name = name;
}

void GUIObject::SetID(int id)
{
    _id = id;
}

void GUIObject::SetX(int x)
{
    SetPosition(x, _y);
}

void GUIObject::SetY(int y)
{
    SetPosition(_x, y);
}

void GUIObject::SetPosition(int x, int y)
{
    if (_x != x || _y != y)
    {
        _x = x;
        _y = y;
        UpdateGraphicSpace();
        MarkPositionChanged(false, false);
    }
}

void GUIObject::SetWidth(int width)
{
    SetSize(width, _height);
}

void GUIObject::SetHeight(int height)
{
    SetSize(_width, height);
}

void GUIObject::SetSize(int width, int height)
{
    if (_width != width || _height != height)
    {
        _width = width;
        _height = height;
        OnResized();
    }
}

void GUIObject::SetScale(float sx, float sy)
{
    _scale = Pointf(sx, sy);
    UpdateGraphicSpace();
    MarkPositionChanged(false, true);
}

void GUIObject::SetRotation(float degrees)
{
    _rotation = Math::ClampAngle360(degrees);
    UpdateGraphicSpace();
    MarkPositionChanged(false, true);
}

void GUIObject::SetZOrder(int zorder)
{
    _zOrder = zorder;
}

void GUIObject::SetTransparency(int trans)
{
    if (_transparency != trans)
    {
        _transparency = trans;
        MarkVisualStateChanged(); // for software mode
    }
}

void GUIObject::SetTransparencyAsPercentage(int percent)
{
    SetTransparency(GfxDef::Trans100ToLegacyTrans255(percent));
}

void GUIObject::SetBlendMode(BlendMode blend_mode)
{
    if (_blendMode != blend_mode)
    {
        _blendMode = blend_mode;
        MarkVisualStateChanged(); // for software mode
    }
}

void GUIObject::SetShader(int shader_id, int shader_handle)
{
    _shaderID = shader_id;
    _shaderHandle = shader_handle;
}

void GUIObject::OnResized()
{
    UpdateGraphicSpace();
    MarkPositionChanged(true, false);
}

void GUIObject::MarkChanged()
{
    _hasChanged = true;
}

void GUIObject::MarkPositionChanged(bool self_changed, bool transform_changed)
{
    _hasChanged |= self_changed || (transform_changed && GUI::Context.SoftwareRender);
}

void GUIObject::MarkVisualStateChanged()
{
    // Do nothing in the base class, but child classes may implement this differently
}

void GUIObject::ClearChanged()
{
    _hasChanged = false;
}

void GUIObject::UpdateGraphicSpace()
{
    Rect g_rc = CalcGraphicRect(GUI::Options.ClipControls);
    _gs = GraphicSpace(
        _x, _y, Pointf(0.f, 0.f), // origin
        Size(_width, _height), // source sprite size
        g_rc, // real graphical aabb (maybe with extra offsets)
        _scale.X, _scale.Y, _rotation // transforms
    );
}

} // namespace Common
} // namespace AGS
