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
#include "gui/guibutton.h"
#include "gui/guimain.h"
#include "util/stream.h"

std::vector<AGS::Common::GUIButton> guibuts;
int numguibuts = 0;

namespace AGS
{
namespace Common
{

GUIButton::GUIButton()
{
    Image = -1;
    MouseOverImage = -1;
    PushedImage = -1;
    CurrentImage = -1;
    Font = 0;
    TextColor = 0;
    TextAlignment = kButtonAlign_TopCenter;
    ClickAction[kMouseLeft] = kGUIAction_RunScript;
    ClickAction[kMouseRight] = kGUIAction_RunScript;
    ClickData[kMouseLeft] = 0;
    ClickData[kMouseRight] = 0;

    IsPushed = false;
    IsMouseOver = false;

    numSupportedEvents = 1;
    supportedEvents[0] = "Click";
    supportedEventArgs[0] = "GUIControl *control, MouseButton button";
}

const String &GUIButton::GetText() const
{
    return _text;
}

// Defines button placeholder mode; the mode is set
// depending on special tags found in button text
enum GUIButtonPlaceholder
{
    kButtonPlace_None,
    kButtonPlace_InvItemStretch,
    kButtonPlace_InvItemCenter
};

void GUIButton::Draw(Bitmap *ds)
{
    bool draw_disabled = IsDisabled() != 0;

    check_font(&Font);
    // if it's "Unchanged when disabled" or "GUI Off", don't grey out
    if (gui_disabled_style == GUIDIS_UNCHANGED ||
        gui_disabled_style == GUIDIS_GUIOFF)
    {
        draw_disabled = false;
    }
    // TODO: should only change properties in reaction to particular events
    if (CurrentImage <= 0 || draw_disabled)
        CurrentImage = Image;

    if (draw_disabled && gui_disabled_style == GUIDIS_BLACKOUT)
        // buttons off when disabled - no point carrying on
        return;

    GUIButtonPlaceholder draw_inv = kButtonPlace_None;
    //-------------------------------------------------------------------------
    // Image button's frame
    // CHECKME: why testing both CurrentImage and Image?
    if (CurrentImage > 0 && Image > 0)
    {
        // NOTE: the CLIP flag only clips the image, not the text
        if (flags & GUIF_CLIP)
            ds->SetClip(Rect(x, y, x + wid - 1, y + hit - 1));
        if (spriteset[CurrentImage] != NULL)
            draw_gui_sprite(ds, CurrentImage, x, y, true);

        // draw
        if (gui_inv_pic >= 0)
        {
            // stretch to fit button
            if (stricmp(_text, "(INV)") == 0)
                draw_inv = kButtonPlace_InvItemStretch;
            // draw at actual size
            else if (stricmp(_text, "(INVNS)") == 0)
                draw_inv = kButtonPlace_InvItemCenter;
            // Stretch if too big, actual size if not
            else if (stricmp(_text, "(INVSHR)") == 0)
            {
                if ((get_adjusted_spritewidth(gui_inv_pic) > wid - 6) ||
                    (get_adjusted_spriteheight(gui_inv_pic) > hit - 6))
                    draw_inv = kButtonPlace_InvItemStretch;
                else
                    draw_inv = kButtonPlace_InvItemCenter;
            }

            if (draw_inv == kButtonPlace_InvItemStretch)
            {
                ds->StretchBlt(spriteset[gui_inv_pic], RectWH(x + 3, y + 3, wid - 6, hit - 6), Common::kBitmap_Transparency);
            }
            else if (draw_inv == kButtonPlace_InvItemCenter)
            {
                draw_gui_sprite(ds, gui_inv_pic,
                               x + wid / 2 - get_adjusted_spritewidth(gui_inv_pic) / 2,
                               y + hit / 2 - get_adjusted_spriteheight(gui_inv_pic) / 2,
                               true);
            }
        }

        if (draw_disabled && gui_disabled_style == GUIDIS_GREYOUT)
        {
            color_t draw_color = ds->GetCompatibleColor(8);
            // darken the button when disabled
            // TODO: move this to a separate function?
            int32_t sprite_width = spriteset[CurrentImage]->GetWidth();
            int32_t sprite_height = spriteset[CurrentImage]->GetHeight();
            for (int at_x = 0; at_x < sprite_width; ++at_x)
            {
                for (int at_y = at_x % 2; at_y < sprite_height; at_y += 2)
                {
                    ds->PutPixel(x + at_x, y + at_y, draw_color);
                }
            }
        }
        // restore bitmap clip
        ds->SetClip(Rect(0, 0, ds->GetWidth() - 1, ds->GetHeight() - 1));
    }
    //-------------------------------------------------------------------------
    // Text button's frame
    // CHECKME: why don't draw frame if no Text? this will make button completely invisible!
    else if (!_text.IsEmpty())
    {
        color_t draw_color = ds->GetCompatibleColor(7);
        ds->FillRect(Rect(x, y, x + wid - 1, y + hit - 1), draw_color);
        if (flags & GUIF_DEFAULT)
        {
            draw_color = ds->GetCompatibleColor(16);
            ds->DrawRect(Rect(x - 1, y - 1, x + wid, y + hit), draw_color);
        }

        // TODO: use color constants instead of literal numbers
        if (!draw_disabled && IsMouseOver && IsPushed)
            draw_color = ds->GetCompatibleColor(15);
        else
            draw_color = ds->GetCompatibleColor(8);

        ds->DrawLine(Line(x, y + hit - 1, x + wid - 1, y + hit - 1), draw_color);
        ds->DrawLine(Line(x + wid - 1, y, x + wid - 1, y + hit - 1), draw_color);

        if (draw_disabled || IsMouseOver && IsPushed)
            draw_color = ds->GetCompatibleColor(8);
        else
            draw_color = ds->GetCompatibleColor(15);

        ds->DrawLine(Line(x, y, x + wid - 1, y), draw_color);
        ds->DrawLine(Line(x, y, x, y + hit - 1), draw_color);
    }

    //-------------------------------------------------------------------------
    // Draw text
    // Don't print Text of (INV) (INVSHR) (INVNS)
    if (draw_inv != kButtonPlace_None) ;
    // TODO: remove this bogus limitation ("New Button" is a valid Text too)
    // Don't print the Text if there's a graphic and it hasn't been named
    else if (CurrentImage > 0 && Image > 0 && strcmp(_text, "New Button") == 0);
    // if there is some Text, print it
    else if (!_text.IsEmpty())
    {
        // TODO: need to find a way to cache Text prior to drawing;
        // but that will require to update all gui controls when translation is changed in game
        PrepareTextToDraw();

        int at_x = x;
        int at_y = y;
        if (IsPushed && IsMouseOver)
        {
            // move the Text a bit while pushed
            at_x++;
            at_y++;
        }

        // TODO: replace with generic alignment-in-rect
        switch (TextAlignment)
        {
        case kButtonAlign_TopCenter:
            at_x += (wid / 2 - wgettextwidth(_textToDraw, Font) / 2);
            at_y += 2;
            break;
        case kButtonAlign_TopLeft:
            at_x += 2;
            at_y += 2;
            break;
        case kButtonAlign_TopRight:
            at_x += (wid - wgettextwidth(_textToDraw, Font)) - 2;
            at_y += 2;
            break;
        case kButtonAlign_CenterLeft:
            at_x += 2;
            at_y += (hit / 2 - (wgettextheight(_textToDraw, Font) + 1) / 2);
            break;
        case kButtonAlign_Centered:
            at_x += (wid / 2 - wgettextwidth(_textToDraw, Font) / 2);
            at_y += (hit / 2 - (wgettextheight(_textToDraw, Font) + 1) / 2);
            break;
        case kButtonAlign_CenterRight:
            at_x += (wid - wgettextwidth(_textToDraw, Font)) - 2;
            at_y += (hit / 2 - (wgettextheight(_textToDraw, Font) + 1) / 2);
            break;
        case kButtonAlign_BottomLeft:
            at_x += 2;
            at_y += (hit - wgettextheight(_textToDraw, Font)) - 2;
            break;
        case kButtonAlign_BottomCenter:
            at_x += (wid / 2 - wgettextwidth(_textToDraw, Font) / 2);
            at_y += (hit - wgettextheight(_textToDraw, Font)) - 2;
            break;
        case kButtonAlign_BottomRight:
            at_x += (wid - wgettextwidth(_textToDraw, Font)) - 2;
            at_y += (hit - wgettextheight(_textToDraw, Font)) - 2;
            break;
        }

        color_t text_color = ds->GetCompatibleColor(TextColor);
        if (draw_disabled)
            text_color = ds->GetCompatibleColor(8);
        wouttext_outline(ds, at_x, at_y, Font, text_color, _textToDraw);
    }
}

void GUIButton::SetText(const String &text)
{
    _text = text;
}

int GUIButton::MouseDown()
{
    if (PushedImage > 0)
        CurrentImage = PushedImage;
    IsPushed = true;
    return 0;
}

void GUIButton::MouseLeave()
{
    CurrentImage = Image;
    IsMouseOver = false;
}

void GUIButton::MouseOver()
{
    CurrentImage = IsPushed ? PushedImage : MouseOverImage;
    IsMouseOver = true;
}

void GUIButton::MouseUp()
{
    if (IsMouseOver)
    {
        CurrentImage = MouseOverImage;
        if (!IsDisabled() && IsClickable())
            activated++;
    }
    else
    {
        CurrentImage = Image;
    }

    IsPushed = false;
}

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUIButton::WriteToFile(Stream *out)
{
    GUIObject::WriteToFile(out);

    out->WriteInt32(Image);
    out->WriteInt32(MouseOverImage);
    out->WriteInt32(PushedImage);
    out->WriteInt32(CurrentImage);
    out->WriteInt32(IsPushed);
    out->WriteInt32(IsMouseOver);
    out->WriteInt32(Font);
    out->WriteInt32(TextColor);
    out->WriteInt32(ClickAction[kMouseLeft]);
    out->WriteInt32(ClickAction[kMouseRight]);
    out->WriteInt32(ClickData[kMouseLeft]);
    out->WriteInt32(ClickData[kMouseRight]);

    _text.WriteCount(out, GUIBUTTON_TEXTLENGTH);
    out->WriteInt32(TextAlignment);
    out->WriteInt32(0); // reserved int32
}

void GUIButton::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GUIObject::ReadFromFile(in, gui_version);

    Image = in->ReadInt32();
    MouseOverImage = in->ReadInt32();
    PushedImage = in->ReadInt32();
    CurrentImage = in->ReadInt32();
    IsPushed = in->ReadInt32() != 0;
    IsMouseOver = in->ReadInt32() != 0;
    Font = in->ReadInt32();
    TextColor = in->ReadInt32();
    ClickAction[kMouseLeft] = (GUIClickAction)in->ReadInt32();
    ClickAction[kMouseRight] = (GUIClickAction)in->ReadInt32();
    ClickData[kMouseLeft] = in->ReadInt32();
    ClickData[kMouseRight] = in->ReadInt32();
    _text.ReadCount(in, GUIBUTTON_TEXTLENGTH);

    if (gui_version >= kGuiVersion_272a)
    {
        TextAlignment = in->ReadInt32();
        in->ReadInt32(); // reserved1
    }
    else
    {
        TextAlignment = kButtonAlign_TopCenter;
    }

    if (TextColor == 0)
        TextColor = 16;
    CurrentImage = Image;
    // All buttons are translated at the moment
    flags |= GUIF_TRANSLATED;
}

} // namespace Common
} // namespace AGS
