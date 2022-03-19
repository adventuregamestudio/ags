//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#include "gui/guislider.h"
#include "ac/game_version.h"
#include "ac/spritecache.h"
#include "gui/guimain.h"
#include "util/stream.h"

std::vector<AGS::Common::GUISlider> guislider;
int numguislider = 0;

namespace AGS
{
namespace Common
{

GUISlider::GUISlider()
{
    MinValue = 0;
    MaxValue = 10;
    Value = 0;
    BgImage = 0;
    HandleImage = 0;
    HandleOffset = 0;
    IsMousePressed = false;

    _scEventCount = 1;
    _scEventNames[0] = "Change";
    _scEventArgs[0] = "GUIControl *control";
}

bool GUISlider::IsHorizontal() const
{
    return Width > Height;
}

bool GUISlider::IsOverControl(int x, int y, int leeway) const
{
    // check the overall boundary
    if (GUIObject::IsOverControl(x, y, leeway))
        return true;
    // now check the handle too
    return _cachedHandle.IsInside(Point(x, y));
}

void GUISlider::Draw(Common::Bitmap *ds)
{
    // Clamp Value
    // TODO: this is necessary here because some Slider fields are still public
    if (MinValue >= MaxValue)
        MaxValue = MinValue + 1;
    Value = Math::Clamp(Value, MinValue, MaxValue);
    // Test if sprite is available
    // TODO: react to sprites initialization/deletion instead!
    if (spriteset[HandleImage] == nullptr)
        HandleImage = 0;

    // Depending on slider's orientation, thickness is either Height or Width
    const int thickness = IsHorizontal() ? Height : Width;
    // "thick_f" is the factor for calculating relative element positions
    const int thick_f = thickness / 3; // one third of the control's thickness
    // Bar thickness
    const int bar_thick = thick_f * 2 + 2;

    // Calculate handle size
    Size handle_sz;
    if (HandleImage > 0) // handle is a sprite
    {
        handle_sz = Size(get_adjusted_spritewidth(HandleImage),
            get_adjusted_spriteheight(HandleImage));
    }
    else // handle is a drawn rectangle
    {
        if (IsHorizontal())
            handle_sz = Size(get_fixed_pixel_size(4) + 1, bar_thick + (thick_f - 1) * 2);
        else
            handle_sz = Size(bar_thick + (thick_f - 1) * 2, get_fixed_pixel_size(4) + 1);
    }
  
    // Calculate bar and handle positions
    Rect bar;
    Rect handle;
    if (IsHorizontal()) // horizontal slider
    {
        // Value pos is a coordinate corresponding to current slider's value
        bar = RectWH(X + 1, Y + Height / 2 - thick_f, Width - 1, bar_thick);
        int value_pos = (int)(((float)(Value - MinValue) * (float)(Width - 4)) / (float)(MaxValue - MinValue)) - 2;
        handle = RectWH((bar.Left + get_fixed_pixel_size(2)) - (handle_sz.Width / 2) + 1 + value_pos,
            bar.Top + (bar.GetHeight() - handle_sz.Height) / 2,
            handle_sz.Width, handle_sz.Height);
        handle.MoveToY(handle.Top + data_to_game_coord(HandleOffset));
    }
    else // vertical slider
    {
        bar = RectWH(X + Width / 2 - thick_f, Y + 1, bar_thick, Height - 1);
        int value_pos = (int)(((float)(MaxValue - Value) * (float)(Height - 4)) / (float)(MaxValue - MinValue)) - 2;
        handle = RectWH(bar.Left + (bar.GetWidth() - handle_sz.Width) / 2,
            (bar.Top + get_fixed_pixel_size(2)) - (handle_sz.Height / 2) + 1 + value_pos,
            handle_sz.Width, handle_sz.Height);
        handle.MoveToX(handle.Left + data_to_game_coord(HandleOffset));
    }

    if (loaded_game_file_version >= kGameVersion_360_21)
    {
        Rect control = RectWH(X, Y, Width, Height);
        bar = ClampToRect(control, bar);
        handle = ClampToRect(control, handle);
    }

    // Draw the slider's body
    if (BgImage > 0)
    {
        // tiled image as slider background
        int x_inc = 0;
        int y_inc = 0;
        if (IsHorizontal())
        {
            x_inc = get_adjusted_spritewidth(BgImage);
            // centre the image vertically
            bar.Top = Y + (Height / 2) - get_adjusted_spriteheight(BgImage) / 2;
        }
        else
        {
            y_inc = get_adjusted_spriteheight(BgImage);
            // centre the image horizontally
            bar.Left = X + (Width / 2) - get_adjusted_spritewidth(BgImage) / 2;
        }
        int cx = bar.Left;
        int cy = bar.Top;
        // draw the tiled background image
        do
        {
            draw_gui_sprite(ds, BgImage, cx, cy, true);
            cx += x_inc;
            cy += y_inc;
            // done as a do..while so that at least one of the image is drawn
        }
        while ((cx + x_inc <= bar.Right) && (cy + y_inc <= bar.Bottom));
    }
    else
    {
        // normal grey background
        color_t draw_color = ds->GetCompatibleColor(16);
        ds->FillRect(Rect(bar.Left + 1, bar.Top + 1, bar.Right - 1, bar.Bottom - 1), draw_color);
        draw_color = ds->GetCompatibleColor(8);
        ds->DrawLine(Line(bar.Left, bar.Top, bar.Left, bar.Bottom), draw_color);
        ds->DrawLine(Line(bar.Left, bar.Top, bar.Right, bar.Top), draw_color);
        draw_color = ds->GetCompatibleColor(15);
        ds->DrawLine(Line(bar.Right, bar.Top + 1, bar.Right, bar.Bottom), draw_color);
        ds->DrawLine(Line(bar.Left, bar.Bottom, bar.Right, bar.Bottom), draw_color);
    }

    // Draw the slider's handle
    if (HandleImage > 0) // handle is a sprite
    {
        draw_gui_sprite(ds, HandleImage, handle.Left, handle.Top, true);
    }
    else // handle is a drawn rectangle
    {
        // normal grey tracker handle
        color_t draw_color = ds->GetCompatibleColor(7);
        ds->FillRect(Rect(handle.Left, handle.Top, handle.Right, handle.Bottom), draw_color);
        draw_color = ds->GetCompatibleColor(15);
        ds->DrawLine(Line(handle.Left, handle.Top, handle.Right, handle.Top), draw_color);
        ds->DrawLine(Line(handle.Left, handle.Top, handle.Left, handle.Bottom), draw_color);
        draw_color = ds->GetCompatibleColor(16);
        ds->DrawLine(Line(handle.Right, handle.Top + 1, handle.Right, handle.Bottom), draw_color);
        ds->DrawLine(Line(handle.Left + 1, handle.Bottom, handle.Right, handle.Bottom), draw_color);
    }

    _cachedHandle = handle;
}

bool GUISlider::OnMouseDown()
{
    IsMousePressed = true;
    // lock focus to ourselves
    return true;
}

void GUISlider::OnMouseMove(int x, int y)
{
    if (!IsMousePressed)
        return;

    if (IsHorizontal())
        Value = (int)(((float)((x - X) - 2) / (float)(Width - 4)) * (float)(MaxValue - MinValue)) + MinValue;
    else
        Value = (int)(((float)(((Y + Height) - y) - 2) / (float)(Height - 4)) * (float)(MaxValue - MinValue)) + MinValue;

    Value = Math::Clamp(Value, MinValue, MaxValue);
    NotifyParentChanged();
    IsActivated = true;
}

void GUISlider::OnMouseUp()
{
    IsMousePressed = false;
}

void GUISlider::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GUIObject::ReadFromFile(in, gui_version);
    MinValue = in->ReadInt32();
    MaxValue = in->ReadInt32();
    Value = in->ReadInt32();
    if (gui_version < kGuiVersion_350)
    { // NOTE: reading into actual variables only for old savegame support
        IsMousePressed = in->ReadInt32() != 0;
    }
    if (gui_version >= kGuiVersion_unkn_104)
    {
        HandleImage = in->ReadInt32();
        HandleOffset = in->ReadInt32();
        BgImage = in->ReadInt32();
    }
    else
    {
        HandleImage = -1;
        HandleOffset = 0;
        BgImage = 0;
    }
}

void GUISlider::WriteToFile(Stream *out) const
{
    GUIObject::WriteToFile(out);
    out->WriteInt32(MinValue);
    out->WriteInt32(MaxValue);
    out->WriteInt32(Value);
    out->WriteInt32(HandleImage);
    out->WriteInt32(HandleOffset);
    out->WriteInt32(BgImage);
}

void GUISlider::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    GUIObject::ReadFromSavegame(in, svg_ver);
    BgImage = in->ReadInt32();
    HandleImage = in->ReadInt32();
    HandleOffset = in->ReadInt32();
    MinValue = in->ReadInt32();
    MaxValue = in->ReadInt32();
    Value = in->ReadInt32();
}

void GUISlider::WriteToSavegame(Stream *out) const
{
    GUIObject::WriteToSavegame(out);
    out->WriteInt32(BgImage);
    out->WriteInt32(HandleImage);
    out->WriteInt32(HandleOffset);
    out->WriteInt32(MinValue);
    out->WriteInt32(MaxValue);
    out->WriteInt32(Value);
}

} // namespace Common
} // namespace AGS
