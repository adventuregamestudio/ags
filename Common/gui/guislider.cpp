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
#include "gui/guislider.h"
#include <algorithm>
#include "ac/spritecache.h"
#include "gui/guimain.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

/* static */ ScriptEventSchema GUISlider::_eventSchema = {{
        { "OnChange", kSliderEvent_OnChange }
    }};

GUISlider::GUISlider()
    : GUIControl(&GUISlider::_eventSchema)
{
    _backgroundColor = 16;
    _borderColor = 15;
    _handleColor = 7;
    _borderShadeColor = 8;
    _handleRange = 0;
}

void GUISlider::SetMinValue(int minval)
{
    if (_minValue != minval)
    {
        _minValue = minval;
        _maxValue = std::max(_minValue, _maxValue);
        _value = Math::Clamp(_value, _minValue, _maxValue);
        MarkChanged();
    }
}

void GUISlider::SetMaxValue(int maxval)
{
    if (_maxValue != maxval)
    {
        _maxValue = maxval;
        _minValue = std::min(_minValue, _maxValue);
        _value = Math::Clamp(_value, _minValue, _maxValue);
        MarkChanged();
    }
}

void GUISlider::SetValue(int value)
{
    value = Math::Clamp(value, _minValue, _maxValue);
    if (_value != value)
    {
        _value = value;
        MarkChanged();
    }
}

void GUISlider::SetBgImage(int image)
{
    if (_bgImage != image)
    {
        _bgImage = image;
        MarkChanged();
    }
}

void GUISlider::SetHandleImage(int image)
{
    if (_handleImage != image)
    {
        _handleImage = image;
        MarkChanged();
    }
}

void GUISlider::SetHandleOffset(int offset)
{
    if (_handleOffset != offset)
    {
        _handleOffset = offset;
        MarkChanged();
    }
}

void GUISlider::SetHandleColor(int color)
{
    if (_handleColor != color)
    {
        _handleColor = color;
        MarkChanged();
    }
}

void GUISlider::SetBorderShadeColor(int color)
{
    if (_borderShadeColor != color)
    {
        _borderShadeColor = color;
        MarkChanged();
    }
}

bool GUISlider::IsHorizontal() const
{
    return _width > _height;
}

bool GUISlider::IsOverControlImpl(int x, int y, int leeway) const
{
    // check the overall boundary
    if (GUIControl::IsOverControlImpl(x, y, leeway))
        return true;
    // now check the handle too
    return _cachedHandle.IsInside(Point(x, y));
}

Rect GUISlider::CalcGraphicRect(bool /*clipped*/)
{
    // Sliders are never clipped as of 3.6.0
    // TODO: precalculate everything on width/height/graphic change!!
    UpdateMetrics();
    const Rect logical = RectWH(0, 0, _width, _height);
    const Rect &bar = _cachedBar;
    const Rect &handle = _handleGraphRange;
    return Rect(
        std::min(std::min(logical.Left, bar.Left), handle.Left),
        std::min(std::min(logical.Top, bar.Top), handle.Top),
        std::max(std::max(logical.Right, bar.Right), handle.Right),
        std::max(std::max(logical.Bottom, bar.Bottom), handle.Bottom)
    );
}

void GUISlider::UpdateMetrics()
{
    assert(GUI::Context.Spriteset);
    SpriteCache &spriteset = *GUI::Context.Spriteset;

    // Test if sprite is available; // TODO: return a placeholder from spriteset instead!
    const int handle_im = ((_handleImage > 0) && spriteset.DoesSpriteExist(_handleImage)) ? _handleImage : 0;

    // Depending on slider's orientation, thickness is either Height or Width
    const int thickness = IsHorizontal() ? _height : _width;
    // "thick_f" is the factor for calculating relative element positions
    const int thick_f = thickness / 3; // one third of the control's thickness
    // Bar thickness
    const int bar_thick = thick_f * 2 + 2;

    // Calculate handle size
    Size handle_sz;
    if (handle_im > 0) // handle is a sprite
    {
        handle_sz = Size(get_adjusted_spritewidth(handle_im),
            get_adjusted_spriteheight(handle_im));
    }
    else // handle is a drawn rectangle
    {
        if (IsHorizontal())
            handle_sz = Size(4 + 1, bar_thick + (thick_f - 1) * 2);
        else
            handle_sz = Size(bar_thick + (thick_f - 1) * 2, 4 + 1);
    }

    // Calculate bar and handle positions
    // FIXME: handle position should be recalculated only from knowing slider ranges and current value
    Rect bar;
    Rect handle;
    int handle_range;
    Rect handle_gfrange;
    if (IsHorizontal()) // horizontal slider
    {
        // _value pos is a coordinate corresponding to current slider's value
        bar = RectWH(1, _height / 2 - thick_f, _width - 1, bar_thick);
        handle_range = _width - 4;
        int value_pos = (int)(((float)(_value - _minValue) * (float)handle_range) / (float)(_maxValue - _minValue));
        handle = RectWH((bar.Left + 2) - (handle_sz.Width / 2) + value_pos - 1,
            bar.Top + (bar.GetHeight() - 1 - handle_sz.Height) / 2,
            handle_sz.Width, handle_sz.Height);
        handle = Rect::MoveBy(handle, 0, _handleOffset
            // Backwards-compatibility: handle graphic had extra 1 pixel offset (2 in hires games)
            + ((handle_im > 0) ? 1 : 0));
        handle_gfrange = Rect(
            (bar.Left + 2) - (handle_sz.Width / 2) + 0 - 1,
            handle.Top,
            (bar.Left + 2) - (handle_sz.Width / 2) + handle_range - 1 + (handle_sz.Width - 1),
            handle.Top + handle_sz.Height - 1);
    }
    // vertical slider
    else
    {
        bar = RectWH(_width / 2 - thick_f, 1, bar_thick, _height - 1);
        handle_range = _height - 4;
        int value_pos = (int)(((float)(_maxValue - _value) * (float)handle_range) / (float)(_maxValue - _minValue));
        handle = RectWH(bar.Left + (bar.GetWidth() - 1 - handle_sz.Width) / 2,
            (bar.Top + 2) - (handle_sz.Height / 2) + value_pos - 1,
            handle_sz.Width, handle_sz.Height);
        handle = Rect::MoveBy(handle, _handleOffset
            // Backwards-compatibility: handle graphic had extra 1 pixel offset
            + ((handle_im > 0) ? 1 : 0)
            , 0);
        handle_gfrange = Rect(
            handle.Left,
            (bar.Top + 2) - (handle_sz.Height / 2) + 0 - 1,
            handle.Left + handle_sz.Width - 1,
            (bar.Top + 2) - (handle_sz.Height / 2) + handle_range - 1 + (handle_sz.Height - 1));
    }

    _cachedBar = bar;
    _cachedHandle = handle;
    _handleRange = std::max(1, handle_range);
    _handleGraphRange = handle_gfrange;
}

void GUISlider::Draw(Bitmap *ds, int x, int y)
{
    assert(GUI::Context.Spriteset);
    SpriteCache &spriteset = *GUI::Context.Spriteset;

    UpdateMetrics();

    Rect bar = Rect::MoveBy(_cachedBar, x, y);
    Rect handle = Rect::MoveBy(_cachedHandle, x, y);

    if (_bgImage > 0)
    {
        // tiled image as slider background
        int x_inc = 0;
        int y_inc = 0;
        if (IsHorizontal())
        {
            x_inc = get_adjusted_spritewidth(_bgImage);
            // centre the image vertically
            bar.Top = y + (_height / 2) - get_adjusted_spriteheight(_bgImage) / 2;
        }
        else
        {
            y_inc = get_adjusted_spriteheight(_bgImage);
            // centre the image horizontally
            bar.Left = x + (_width / 2) - get_adjusted_spritewidth(_bgImage) / 2;
        }
        int cx = bar.Left;
        int cy = bar.Top;
        // draw the tiled background image
        do
        {
            draw_gui_sprite(ds, _bgImage, cx, cy);
            cx += x_inc;
            cy += y_inc;
            // done as a do..while so that at least one of the image is drawn
        }
        while ((cx + x_inc <= bar.Right) && (cy + y_inc <= bar.Bottom));
    }
    else
    {
        // normal grey background
        color_t draw_color = ds->GetCompatibleColor(_backgroundColor);
        ds->FillRect(bar, draw_color);
        draw_color = ds->GetCompatibleColor(_borderShadeColor);
        ds->DrawLine(Line(bar.Left, bar.Top, bar.Left, bar.Bottom), draw_color);
        ds->DrawLine(Line(bar.Left, bar.Top, bar.Right, bar.Top), draw_color);
        draw_color = ds->GetCompatibleColor(_borderColor);
        ds->DrawLine(Line(bar.Right, bar.Top + 1, bar.Right, bar.Bottom), draw_color);
        ds->DrawLine(Line(bar.Left, bar.Bottom, bar.Right, bar.Bottom), draw_color);
    }

    // Test if sprite is available; // TODO: return a placeholder from spriteset instead!
    const int handle_im = ((_handleImage > 0) && spriteset.DoesSpriteExist(_handleImage)) ? _handleImage : 0;
    if (handle_im > 0) // handle is a sprite
    {
        draw_gui_sprite(ds, handle_im, handle.Left, handle.Top);
    }
    else // handle is a drawn rectangle
    {
        // normal grey tracker handle
        color_t draw_color = ds->GetCompatibleColor(_handleColor);
        ds->FillRect(handle, draw_color);
        draw_color = ds->GetCompatibleColor(_borderColor);
        ds->DrawLine(Line(handle.Left, handle.Top, handle.Right, handle.Top), draw_color);
        ds->DrawLine(Line(handle.Left, handle.Top, handle.Left, handle.Bottom), draw_color);
        draw_color = ds->GetCompatibleColor(_borderShadeColor);
        ds->DrawLine(Line(handle.Right, handle.Top + 1, handle.Right, handle.Bottom), draw_color);
        ds->DrawLine(Line(handle.Left + 1, handle.Bottom, handle.Right, handle.Bottom), draw_color);
    }
}

void GUISlider::UpdateVisualState()
{
    UpdateMetrics();
    MarkPositionChanged(true, true);
}

bool GUISlider::OnMouseDown()
{
    _isMousePressed = true;
    // lock focus to ourselves
    return true;
}

void GUISlider::OnMouseMove(int mx, int my)
{
    if (!_isMousePressed)
        return;

    Point mp = _gs.WorldToLocal(mx, my);

    int value;
    assert(_handleRange > 0);
    if (IsHorizontal())
        value = (int)(((float)(mp.X - 2) * (float)(_maxValue - _minValue)) / (float)_handleRange) + _minValue;
    else
        value = (int)(((float)((_height - mp.Y) - 2) * (float)(_maxValue - _minValue)) / (float)_handleRange) + _minValue;

    value = Math::Clamp(value, _minValue, _maxValue);
    if (value != _value)
    {
        _value = value;
        MarkChanged();
    }
    _isActivated = true;
}

void GUISlider::OnMouseUp()
{
    _isMousePressed = false;
}

void GUISlider::OnResized()
{
    GUIControl::OnResized();
    UpdateMetrics();
    UpdateGraphicSpace();
}

void GUISlider::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GUIControl::ReadFromFile(in, gui_version);
    _minValue = in->ReadInt32();
    _maxValue = in->ReadInt32();
    _value = in->ReadInt32();
    _handleImage = in->ReadInt32();
    _handleOffset = in->ReadInt32();
    _bgImage = in->ReadInt32();

    // Reset dynamic values
    _cachedBar = Rect();
    _cachedHandle = Rect();
    _handleRange = 0;
    // Clamp value range, in case the data is wrong
    _maxValue = std::max(_minValue, _maxValue);
    _minValue = std::min(_minValue, _maxValue);
    _value = Math::Clamp(_value, _minValue, _maxValue);

    //UpdateMetrics();
    //UpdateGraphicSpace(); // can't do here, because sprite infos may not be loaded yet
}

void GUISlider::ReadFromFile_Ext363(Stream *in, GuiVersion gui_version)
{
    GUIControl::ReadFromFile_Ext363(in, gui_version);

    _handleColor = in->ReadInt32();
    _borderShadeColor = in->ReadInt32();
    in->ReadInt32(); // reserved
    in->ReadInt32();
    in->ReadInt32();
    in->ReadInt32();
}

void GUISlider::WriteToFile(Stream *out) const
{
    GUIControl::WriteToFile(out);
    out->WriteInt32(_minValue);
    out->WriteInt32(_maxValue);
    out->WriteInt32(_value);
    out->WriteInt32(_handleImage);
    out->WriteInt32(_handleOffset);
    out->WriteInt32(_bgImage);
}

void GUISlider::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    GUIControl::ReadFromSavegame(in, svg_ver);
    _bgImage = in->ReadInt32();
    _handleImage = in->ReadInt32();
    _handleOffset = in->ReadInt32();
    _minValue = in->ReadInt32();
    _maxValue = in->ReadInt32();
    _value = in->ReadInt32();

    if ((svg_ver >= kGuiSvgVersion_36304) && (svg_ver < kGuiSvgVersion_400) ||
        (svg_ver >= kGuiSvgVersion_40026))
    {
        _handleColor = in->ReadInt32();
        _borderShadeColor = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
        in->ReadInt32();
        in->ReadInt32();
    }
    else
    {
        SetDefaultLooksFor363();
    }

    // Reset dynamic values
    _cachedBar = Rect();
    _cachedHandle = Rect();
    _handleRange = 0;
    // Clamp value range, in case the data is wrong
    _maxValue = std::max(_minValue, _maxValue);
    _minValue = std::min(_minValue, _maxValue);
    _value = Math::Clamp(_value, _minValue, _maxValue);
    
    UpdateMetrics();
    UpdateGraphicSpace();
}

void GUISlider::WriteToSavegame(Stream *out) const
{
    GUIControl::WriteToSavegame(out);
    out->WriteInt32(_bgImage);
    out->WriteInt32(_handleImage);
    out->WriteInt32(_handleOffset);
    out->WriteInt32(_minValue);
    out->WriteInt32(_maxValue);
    out->WriteInt32(_value);
    // kGuiSvgVersion_36304
    out->WriteInt32(_handleColor);
    out->WriteInt32(_borderShadeColor);
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);
}

void GUISlider::SetDefaultLooksFor363()
{
    _flags |= kGUICtrl_SolidBack | kGUICtrl_ShowBorder;
    _backgroundColor = 16;
    _borderColor = 15;
    _handleColor = 7;
    _borderShadeColor = 8;
    UpdateControlRect();
}

} // namespace Common
} // namespace AGS
