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

#include "ac/spritecache.h"
#include "font/fonts.h"
#include "gui/guimain.h"
#include "gui/guislider.h"
#include "util/math.h"

namespace AGS
{
namespace Common
{

GuiSlider::GuiSlider()
{
    MinValue = 0;
    MaxValue = 10;
    Value = 0;
    BackgroundImage = 0;
    HandleImage = 0;
    HandleOffset = 0;
    IsMousePressed = false;

    SupportedEventCount = 1;
    EventNames[0] = "Change";
    EventArgs[0] = "GUIControl *control";
}

bool GuiSlider::IsOverControl(int x, int y, int leeway)
{
    // check the overall boundary
    if (GuiObject::IsOverControl(x, y, leeway))
    {
        return true;
    }
    // now check the handle too
    return CachedHandleFrame.IsInside(Point(x, y));
}

void GuiSlider::Draw(Common::Bitmap *ds)
{
    Rect bar_frame;
    Rect handle_frame;
    int thickness;

    if (MinValue >= MaxValue)
    {
        MaxValue = MinValue + 1;
    }
    Math::Clamp(MinValue, MaxValue, Value);
  
    // it's a horizontal slider
    if (Frame.GetWidth() > Frame.GetHeight())
    {
        thickness = Frame.GetHeight() / 3;
        bar_frame.Left = Frame.Left + 1;
        bar_frame.Top = Frame.Top + Frame.GetHeight() / 2 - thickness;
        bar_frame.Right = Frame.Left + Frame.GetWidth() - 1;
        bar_frame.Bottom = Frame.Top + Frame.GetHeight() / 2 + thickness + 1;
        handle_frame.Left = (int)(((float)(Value - MinValue) / (float)(MaxValue - MinValue)) * (float)(Frame.GetWidth() - 4) - 2) + bar_frame.Left + 1;
        handle_frame.Top = bar_frame.Top - (thickness - 1);
        handle_frame.Right = handle_frame.Left + get_fixed_pixel_size(4);
        handle_frame.Bottom = bar_frame.Bottom + (thickness - 1);
        if (HandleImage > 0)
        {
            // store the centre of the pic rather than the top
            handle_frame.Top = bar_frame.Top + (bar_frame.Bottom - bar_frame.Top) / 2 + get_fixed_pixel_size(1);
            handle_frame.Left += get_fixed_pixel_size(2);
        }
        handle_frame.Top += multiply_up_coordinate(HandleOffset);
        handle_frame.Bottom += multiply_up_coordinate(HandleOffset);
    }
    // vertical slider
    else
    {
        thickness = Frame.GetWidth() / 3;
        bar_frame.Left = Frame.Left + Frame.GetWidth() / 2 - thickness;
        bar_frame.Top = Frame.Top + 1;
        bar_frame.Right = Frame.Left + Frame.GetWidth() / 2 + thickness + 1;
        bar_frame.Bottom = Frame.Top + Frame.GetHeight() - 1;
        handle_frame.Top = (int)(((float)(MaxValue - Value) / (float)(MaxValue - MinValue)) * (float)(Frame.GetHeight() - 4) - 2) + bar_frame.Top + 1;
        handle_frame.Left = bar_frame.Left - (thickness - 1);
        handle_frame.Bottom = handle_frame.Top + get_fixed_pixel_size(4);
        handle_frame.Right = bar_frame.Right + (thickness - 1);
        if (HandleImage > 0)
        {
            // store the centre of the pic rather than the left
            handle_frame.Left = bar_frame.Left + (bar_frame.Right - bar_frame.Left) / 2 + get_fixed_pixel_size(1);
            handle_frame.Top += get_fixed_pixel_size(2);
        }
        handle_frame.Left += multiply_up_coordinate(HandleOffset);
        handle_frame.Right += multiply_up_coordinate(HandleOffset);
    }

    color_t draw_color;
    if (BackgroundImage > 0)
    {
        // tiled image as slider background
        int x_inc = 0;
        int y_inc = 0;
        if (Frame.GetWidth() > Frame.GetHeight())
        {
            // horizontal slider
            x_inc = get_adjusted_spritewidth(BackgroundImage);
            // centre the image vertically
            bar_frame.Top = Frame.Top + (Frame.GetHeight() / 2) - get_adjusted_spriteheight(BackgroundImage) / 2;
        }
        else
        {
            // vertical slider
            y_inc = get_adjusted_spriteheight(BackgroundImage);
            // centre the image horizontally
            bar_frame.Left = Frame.Left + (Frame.GetWidth() / 2) - get_adjusted_spritewidth(BackgroundImage) / 2;
        }
        int cx = bar_frame.Left;
        int cy = bar_frame.Top;
        // draw the tiled background image
        do
        {
            draw_sprite_compensate(ds, BackgroundImage, cx, cy, 1);
            cx += x_inc;
            cy += y_inc;
            // done as a do..while so that at least one of the image is drawn
        } while ((cx + x_inc <= bar_frame.Right) && (cy + y_inc <= bar_frame.Bottom));
    }
    else
    {
        // normal grey background
        draw_color = ds->GetCompatibleColor(16);
        ds->FillRect(Rect(bar_frame.Left + 1, bar_frame.Top + 1, bar_frame.Right - 1, bar_frame.Bottom - 1), draw_color);
        draw_color = ds->GetCompatibleColor(8);
        ds->DrawLine(Line(bar_frame.Left, bar_frame.Top, bar_frame.Left, bar_frame.Bottom), draw_color);
        ds->DrawLine(Line(bar_frame.Left, bar_frame.Top, bar_frame.Right, bar_frame.Top), draw_color);
        draw_color = ds->GetCompatibleColor(15);
        ds->DrawLine(Line(bar_frame.Right, bar_frame.Top + 1, bar_frame.Right, bar_frame.Bottom), draw_color);
        ds->DrawLine(Line(bar_frame.Left, bar_frame.Bottom, bar_frame.Right, bar_frame.Bottom), draw_color);
    }

    if (HandleImage > 0)
    {
        // an image for the slider handle
        if (spriteset[HandleImage] == NULL)
        {
            HandleImage = 0;
        }
        handle_frame.Left -= get_adjusted_spritewidth(HandleImage) / 2;
        handle_frame.Top -= get_adjusted_spriteheight(HandleImage) / 2;
        draw_sprite_compensate(ds, HandleImage, handle_frame.Left, handle_frame.Top, 1);
        handle_frame.Right = handle_frame.Left + get_adjusted_spritewidth(HandleImage);
        handle_frame.Bottom = handle_frame.Top + get_adjusted_spriteheight(HandleImage);
    }
    else
    {
        // normal grey tracker handle
        draw_color = ds->GetCompatibleColor(7);
        ds->FillRect(Rect(handle_frame.Left, handle_frame.Top, handle_frame.Right, handle_frame.Bottom), draw_color);
        draw_color = ds->GetCompatibleColor(15);
        ds->DrawLine(Line(handle_frame.Left, handle_frame.Top, handle_frame.Right, handle_frame.Top), draw_color);
        ds->DrawLine(Line(handle_frame.Left, handle_frame.Top, handle_frame.Left, handle_frame.Bottom), draw_color);
        draw_color = ds->GetCompatibleColor(16);
        ds->DrawLine(Line(handle_frame.Right, handle_frame.Top + 1, handle_frame.Right, handle_frame.Bottom), draw_color);
        ds->DrawLine(Line(handle_frame.Left + 1, handle_frame.Bottom, handle_frame.Right, handle_frame.Bottom), draw_color);
    }

    CachedHandleFrame = handle_frame;
}

bool GuiSlider::OnMouseDown()
{
    IsMousePressed = true;
    // lock focus to ourselves
    return true;
}

void GuiSlider::OnMouseMove(int xp, int yp)
{
    if (!IsMousePressed)
    {
        return;
    }

    if (Frame.GetWidth() > Frame.GetHeight())
    {
        // horizontal slider
        Value = (int)(((float)((xp - Frame.Left) - 2) / (float)(Frame.GetWidth() - 4)) * (float)(MaxValue - MinValue)) + MinValue;
    }
    else
    {
        // vertical slider
        Value = (int)(((float)(((Frame.Top + Frame.GetHeight()) - yp) - 2) / (float)(Frame.GetHeight() - 4)) * (float)(MaxValue - MinValue)) + MinValue;
    }
    Math::Clamp(MinValue, MaxValue, Value);
    guis_need_update = 1;
    IsActivated = true;
}

void GuiSlider::OnMouseUp()
{
    IsMousePressed = false;
}

void GuiSlider::WriteToFile(Stream *out)
{
    GuiObject::WriteToFile(out);
    out->WriteInt32(BackgroundImage);
    out->WriteInt32(HandleImage);
    out->WriteInt32(HandleOffset);
    out->WriteInt32(MinValue);
    out->WriteInt32(MaxValue);
    out->WriteInt32(Value);    
}

void GuiSlider::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GuiObject::ReadFromFile(in, gui_version);
    if (gui_version < kGuiVersion_340_alpha)
    {
        MinValue = in->ReadInt32();
        MaxValue = in->ReadInt32();
        Value = in->ReadInt32();
        in->ReadInt32(); // mpressed
        if (gui_version >= kGuiVersion_unkn_104)
        {
            HandleImage = in->ReadInt32();
            HandleOffset = in->ReadInt32();
            BackgroundImage = in->ReadInt32();
        }
        else
        {
            HandleImage = -1;
            HandleOffset = 0;
            BackgroundImage = 0;
        }
    }
    else
    {
        BackgroundImage = in->ReadInt32();
        HandleImage = in->ReadInt32();
        HandleOffset = in->ReadInt32();
        MinValue = in->ReadInt32();
        MaxValue = in->ReadInt32();
        Value = in->ReadInt32();
    }
}

void GuiSlider::WriteToSavedGame(Stream *out)
{
    GuiObject::WriteToSavedGame(out);
    out->WriteInt32(BackgroundImage);
    out->WriteInt32(HandleImage);
    out->WriteInt32(HandleOffset);
    out->WriteInt32(MinValue);
    out->WriteInt32(MaxValue);
    out->WriteInt32(Value);
}

void GuiSlider::ReadFromSavedGame(Common::Stream *in, RuntimeGuiVersion gui_version)
{
    GuiObject::ReadFromSavedGame(in, gui_version);
    BackgroundImage = in->ReadInt32();
    HandleImage = in->ReadInt32();
    HandleOffset = in->ReadInt32();
    MinValue = in->ReadInt32();
    MaxValue = in->ReadInt32();
    Value = in->ReadInt32();
}

} // namespace Common
} // namespace AGS

AGS::Common::ObjectArray<AGS::Common::GuiSlider> guislider;
int numguislider = 0;
