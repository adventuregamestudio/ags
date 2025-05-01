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
#include "gui/guislider.h"
#include <algorithm>
#include "ac/spritecache.h"
#include "gui/guimain.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

GUISlider::GUISlider()
{
    _scEventCount = 1;
    _scEventNames[0] = "Change";
    _scEventArgs[0] = "GUIControl *control";
    _handleRange = 0;
}

void GUISlider::SetMinValue(int minval)
{
    if (_minValue != minval)
    {
        _minValue = minval;
        _value = Math::Clamp(_value, _minValue, _maxValue);
        MarkChanged();
    }
}

void GUISlider::SetMaxValue(int maxval)
{
    if (_maxValue != maxval)
    {
        _maxValue = maxval;
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

bool GUISlider::IsHorizontal() const
{
    return _width > _height;
}

bool GUISlider::IsOverControl(int x, int y, int leeway) const
{
    // check the overall boundary
    if (GUIObject::IsOverControl(x, y, leeway))
        return true;
    // now check the handle too
    return _cachedHandle.IsInside(Point(x - _x, y - _y));
}

Rect GUISlider::CalcGraphicRect(bool /*clipped*/)
{
    // Sliders are never clipped as of 3.6.0
    // TODO: precalculate everything on width/height/graphic change!!
    UpdateMetrics();
    Rect logical = RectWH(0, 0, _width, _height);
    Rect bar = _cachedBar;
    Rect handle = _cachedHandle;
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

    // Clamp value
    // TODO: this is necessary here because some Slider fields are still public
    if (_minValue >= _maxValue)
        _maxValue = _minValue + 1;
    _value = Math::Clamp(_value, _minValue, _maxValue);
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
    Rect bar;
    Rect handle;
    int handle_range;
    if (IsHorizontal()) // horizontal slider
    {
        // _value pos is a coordinate corresponding to current slider's value
        bar = RectWH(1, _height / 2 - thick_f, _width - 1, bar_thick);
        handle_range = _width - 4;
        int value_pos = (int)(((float)(_value - _minValue) * (float)handle_range) / (float)(_maxValue - _minValue));
        handle = RectWH((bar.Left + 2) - (handle_sz.Width / 2) + 1 + value_pos - 2,
            bar.Top + (bar.GetHeight() - handle_sz.Height) / 2,
            handle_sz.Width, handle_sz.Height);
        handle.MoveToY(handle.Top + _handleOffset);
    }
    // vertical slider
    else
    {
        bar = RectWH(_width / 2 - thick_f, 1, bar_thick, _height - 1);
        handle_range = _height - 4;
        int value_pos = (int)(((float)(_maxValue - _value) * (float)handle_range) / (float)(_maxValue - _minValue));
        handle = RectWH(bar.Left + (bar.GetWidth() - handle_sz.Width) / 2,
            (bar.Top + 2) - (handle_sz.Height / 2) + 1 + value_pos - 2,
            handle_sz.Width, handle_sz.Height);
        handle.MoveToX(handle.Left + _handleOffset);
    }

    _cachedBar = bar;
    _cachedHandle = handle;
    _handleRange = std::max(1, handle_range);
}

void GUISlider::Draw(Bitmap *ds, int x, int y)
{
    assert(GUI::Context.Spriteset);
    SpriteCache &spriteset = *GUI::Context.Spriteset;

    UpdateMetrics();

    Rect bar = Rect::MoveBy(_cachedBar, x, y);
    Rect handle = Rect::MoveBy(_cachedHandle, x, y);

    color_t draw_color;
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
        draw_color = GUI::GetStandardColorForBitmap(16);
        ds->FillRect(bar, draw_color);
        draw_color = GUI::GetStandardColorForBitmap(8);
        ds->DrawLine(Line(bar.Left, bar.Top, bar.Left, bar.Bottom), draw_color);
        ds->DrawLine(Line(bar.Left, bar.Top, bar.Right, bar.Top), draw_color);
        draw_color = GUI::GetStandardColorForBitmap(15);
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
        draw_color = GUI::GetStandardColorForBitmap(7);
        ds->FillRect(handle, draw_color);
        draw_color = GUI::GetStandardColorForBitmap(15);
        ds->DrawLine(Line(handle.Left, handle.Top, handle.Right, handle.Top), draw_color);
        ds->DrawLine(Line(handle.Left, handle.Top, handle.Left, handle.Bottom), draw_color);
        draw_color = GUI::GetStandardColorForBitmap(16);
        ds->DrawLine(Line(handle.Right, handle.Top + 1, handle.Right, handle.Bottom), draw_color);
        ds->DrawLine(Line(handle.Left + 1, handle.Bottom, handle.Right, handle.Bottom), draw_color);
    }
}

bool GUISlider::OnMouseDown()
{
    _isMousePressed = true;
    // lock focus to ourselves
    return true;
}

void GUISlider::OnMouseMove(int x, int y)
{
    if (!_isMousePressed)
        return;

    int value;
    assert(_handleRange > 0);
    if (IsHorizontal())
        value = (int)(((float)((x - _x) - 2) * (float)(_maxValue - _minValue)) / (float)_handleRange) + _minValue;
    else
        value = (int)(((float)(((_y + _height) - y) - 2) * (float)(_maxValue - _minValue)) / (float)_handleRange) + _minValue;

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
    UpdateMetrics();
    MarkPositionChanged(true);
}

void GUISlider::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GUIObject::ReadFromFile(in, gui_version);
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
}

void GUISlider::WriteToFile(Stream *out) const
{
    GUIObject::WriteToFile(out);
    out->WriteInt32(_minValue);
    out->WriteInt32(_maxValue);
    out->WriteInt32(_value);
    out->WriteInt32(_handleImage);
    out->WriteInt32(_handleOffset);
    out->WriteInt32(_bgImage);
}

void GUISlider::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    GUIObject::ReadFromSavegame(in, svg_ver);
    _bgImage = in->ReadInt32();
    _handleImage = in->ReadInt32();
    _handleOffset = in->ReadInt32();
    _minValue = in->ReadInt32();
    _maxValue = in->ReadInt32();
    _value = in->ReadInt32();

    // Reset dynamic values
    _cachedBar = Rect();
    _cachedHandle = Rect();
    _handleRange = 0;
}

void GUISlider::WriteToSavegame(Stream *out) const
{
    GUIObject::WriteToSavegame(out);
    out->WriteInt32(_bgImage);
    out->WriteInt32(_handleImage);
    out->WriteInt32(_handleOffset);
    out->WriteInt32(_minValue);
    out->WriteInt32(_maxValue);
    out->WriteInt32(_value);
}

} // namespace Common
} // namespace AGS
