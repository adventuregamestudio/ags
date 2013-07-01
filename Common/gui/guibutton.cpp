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

namespace AGS
{
namespace Common
{

GuiButton::GuiButton()
{
    NormalImage = -1;
    MouseOverImage = -1;
    PushedImage = -1;
    CurrentImage = -1;
    TextFont = 0;
    TextColor = 0;
    TextAlignment = kAlignNone;
    ClickAction = kGuiBtnAction_RunScript;
    ClickActionData = 0;

    IsPushed = false;
    IsMouseOver = false;

    SupportedEventCount = 1;
    EventNames[0] = "Click";
    EventArgs[0] = "GUIControl *control, MouseButton button";
}

void GuiButton::Draw(Bitmap *ds)
{
    bool draw_disabled = IsDisabled();

    check_font(&TextFont);
    // if it's "Unchanged when disabled" or "GUI Off", don't grey out
    if (gui_disabled_style == kGuiDisabled_Unchanged ||
        gui_disabled_style == kGuiDisabled_Hide)
    {
        draw_disabled = false;
    }
    if (draw_disabled && gui_disabled_style == kGuiDisabled_HideControls)
    {
        // buttons off when disabled - no point carrying on
        return;
    }
    if (CurrentImage <= 0 || draw_disabled)
    {
        CurrentImage = NormalImage;
    }

    // CHECKME: why testing both CurrentImage and NormalImage?
    if (CurrentImage > 0 && NormalImage > 0)
    {
        DrawImageButton(ds, draw_disabled);
    }
    // CHECKME: why don't draw frame if no text? this will make button completely invisible!
    else if (!Text.IsEmpty())
    {
        DrawTextButton(ds, draw_disabled);
    }
}

bool GuiButton::OnMouseDown()
{
    if (PushedImage > 0)
    {
        CurrentImage = PushedImage;
    }

    IsPushed = true;
    return false;
}

void GuiButton::OnMouseLeave()
{
    CurrentImage = NormalImage;
    IsMouseOver = true;
}

void GuiButton::OnMouseOver()
{
    CurrentImage = IsPushed ? PushedImage : MouseOverImage;
    IsMouseOver = true;
}

void GuiButton::OnMouseUp()
{
    if (IsMouseOver)
    {
        CurrentImage = MouseOverImage;
        if (!IsDisabled() && IsClickable())
        {
            IsActivated = true;
        }
    }
    else
    {
        CurrentImage = NormalImage;
    }
    IsPushed = false;
}

void GuiButton::WriteToFile(Stream *out)
{
    GuiObject::WriteToFile(out);
    out->WriteInt32(NormalImage);
    out->WriteInt32(MouseOverImage);
    out->WriteInt32(PushedImage);
    out->WriteInt32(TextFont);
    out->WriteInt32(TextColor);
    out->WriteInt32(TextAlignment);
    out->WriteInt32(ClickAction);
    out->WriteInt32(ClickActionData);
    Text.Write(out);
}

void GuiButton::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GuiObject::ReadFromFile(in, gui_version);
    if (gui_version < kGuiVersion_340_alpha)
    {
        NormalImage = in->ReadInt32();
        MouseOverImage = in->ReadInt32();
        PushedImage = in->ReadInt32();
        CurrentImage = in->ReadInt32();
        IsPushed = in->ReadInt32() != 0;
        IsMouseOver = in->ReadInt32() != 0;
        TextFont = in->ReadInt32();
        TextColor = in->ReadInt32();
        ClickAction = (GuiButtonClickAction)in->ReadInt32();
        in->ReadInt32(); // rightclick
        ClickActionData = in->ReadInt32();
        in->ReadInt32(); // rclickdata
        Text.ReadCount(in, 50);
        LegacyGuiButtonAlignment legacy_align;
        if (gui_version >= kGuiVersion_272a)
        {
            legacy_align = (LegacyGuiButtonAlignment)in->ReadInt32();
            in->ReadInt32(); // reserved1
        }
        else
        {
            legacy_align = kLegacyGuiBtnAlign_TopCenter;
        }
        TextAlignment = ConvertLegacyButtonAlignment(legacy_align);
    }
    else
    {
        NormalImage = in->ReadInt32();
        MouseOverImage = in->ReadInt32();
        PushedImage = in->ReadInt32();
        TextFont = in->ReadInt32();
        TextColor = in->ReadInt32();
        TextAlignment = (Alignment)in->ReadInt32();
        ClickAction = (GuiButtonClickAction)in->ReadInt32();
        ClickActionData = in->ReadInt32();
        Text.Read(in);
    }

    if (TextColor == 0)
    {
        TextColor = 16;
    }
    CurrentImage = NormalImage;

    // All buttons are translated at the moment
    Flags |= kGuiCtrl_Translated;
}

void GuiButton::WriteToSavedGame(Stream *out)
{
    GuiObject::WriteToSavedGame(out);
    out->WriteInt32(NormalImage);
    out->WriteInt32(MouseOverImage);
    out->WriteInt32(PushedImage);
    out->WriteInt32(TextFont);
    out->WriteInt32(TextColor);
    out->WriteInt32(TextAlignment);
    Text.Write(out);
}

void GuiButton::ReadFromSavedGame(Stream *in, RuntimeGuiVersion gui_version)
{
    GuiObject::ReadFromSavedGame(in, gui_version);
    NormalImage = in->ReadInt32();
    MouseOverImage = in->ReadInt32();
    PushedImage = in->ReadInt32();
    TextFont = in->ReadInt32();
    TextColor = in->ReadInt32();
    TextAlignment = (Alignment)in->ReadInt32();
    Text.Read(in);

    CurrentImage = NormalImage;
}

/* static */ Alignment GuiButton::ConvertLegacyButtonAlignment(LegacyGuiButtonAlignment legacy_align)
{
    switch (legacy_align)
    {
    case kLegacyGuiBtnAlign_TopCenter:
        return kAlignTopCenter;
    case kLegacyGuiBtnAlign_TopLeft:
        return kAlignTopLeft;
    case kLegacyGuiBtnAlign_TopRight:
        return kAlignTopRight;
    case kLegacyGuiBtnAlign_CenterLeft:
        return kAlignCenterLeft;
    case kLegacyGuiBtnAlign_Centered:
        return kAlignCentered;
    case kLegacyGuiBtnAlign_CenterRight:
        return kAlignCenterRight;
    case kLegacyGuiBtnAlign_BottomLeft:
        return kAlignBottomLeft;
    case kLegacyGuiBtnAlign_BottomCenter:
        return kAlignBottomCenter;
    case kLegacyGuiBtnAlign_BottomRight:
        return kAlignBottomRight;
    }
    return kAlignNone;
}

void GuiButton::DrawImageButton(Bitmap *ds, bool draw_disabled)
{
    if (Flags & kGuiCtrl_Clip)
    {
        ds->SetClip(Rect(Frame.Left, Frame.Top, Frame.Left + Frame.GetWidth() - 1, Frame.Top + Frame.GetHeight() - 1));
    }
    if (spriteset[CurrentImage] != NULL)
    {
        draw_sprite_compensate(ds, CurrentImage, Frame.Left, Frame.Top, 1);
    }

    enum DrawInvItemOnButtonStyle
    {
        kDrawInvItemOnBtn_None,
        kDrawInvItemOnBtn_Center,
        kDrawInvItemOnBtn_Stretch
    };
    DrawInvItemOnButtonStyle draw_invitem_style = kDrawInvItemOnBtn_None;
    if (gui_inv_pic >= 0)
    {
        // Stretch to fit button
        if (Text.CompareNoCase("(INV)") == 0)
        {
            draw_invitem_style = kDrawInvItemOnBtn_Stretch;
        }
        // Draw at actual size
        else if (Text.CompareNoCase("(INVNS)") == 0)
        {
            draw_invitem_style = kDrawInvItemOnBtn_Center;
        }
        // Stretch if too big, actual size if not
        else if (Text.CompareNoCase("(INVSHR)") == 0)
        {
            if ((get_adjusted_spritewidth(gui_inv_pic) > Frame.GetWidth() - 6) ||
                (get_adjusted_spriteheight(gui_inv_pic) > Frame.GetHeight() - 6))
            {
                draw_invitem_style = kDrawInvItemOnBtn_Stretch;
            }
            else
            {
                draw_invitem_style = kDrawInvItemOnBtn_Center;
            }
        }

        switch (draw_invitem_style)
        {
        case kDrawInvItemOnBtn_Center:
            draw_sprite_compensate(ds, gui_inv_pic,
                Frame.Left + Frame.GetWidth() / 2 - get_adjusted_spritewidth(gui_inv_pic) / 2,
                Frame.Top + Frame.GetHeight() / 2 - get_adjusted_spriteheight(gui_inv_pic) / 2, 1);
            break;
        case kDrawInvItemOnBtn_Stretch:
            ds->StretchBlt(spriteset[gui_inv_pic], RectWH(Frame.Left + 3, Frame.Top + 3, Frame.GetWidth() - 6, Frame.GetHeight() - 6), Common::kBitmap_Transparency);
            break;            
        }
    }

    if ((draw_disabled) && (gui_disabled_style == kGuiDisabled_GreyOut))
    {
        color_t draw_color = ds->GetCompatibleColor(8);
        // darken the button when disabled
        int32_t sprite_width = spriteset[CurrentImage]->GetWidth();
        int32_t sprite_height = spriteset[CurrentImage]->GetHeight();
        for (int x = 0; x < sprite_width; ++x)
        {
            for (int y = x % 2; y < sprite_height; y += 2)
            {
                ds->PutPixel(Frame.Left + x, Frame.Top + y, draw_color);
            }
        }
    }
    ds->SetClip(Rect(0, 0, ds->GetWidth() - 1, ds->GetHeight() - 1));

    // Don't print Text of (INV) (INVSHR) (INVNS)
    if (draw_invitem_style == kDrawInvItemOnBtn_None &&
        // Only for game version < 3.4.0 (to simulate older engine behavior):
        // don't print the Text if there's a graphic and it hasn't been named
        !(LoadedGuiVersion < kGuiVersion_340_alpha &&
        Text.Compare("New Button") == 0))
    {
        DrawText(ds, draw_disabled);
    }
}

void GuiButton::DrawText(Bitmap *ds, bool draw_disabled)
{
    if (Text.IsEmpty())
    {
        return;
    }
    // TODO: need to find a way to cache text prior to drawing;
    // but that will require to update all gui controls when translation is changed in game
    PrepareTextToDraw();

    Rect content_frame(Frame.Left + 2, Frame.Top + 2, Frame.Right - 2, Frame.Bottom - 2);
    if (IsPushed && IsMouseOver)
    {
        // move the Text a bit while pushed
        content_frame.Left++;
        content_frame.Top++;
    }
    int text_height = wgettextheight(TextToDraw, TextFont);
    // CHECKME: better way instead of this hack?
    if (TextAlignment & kAlignVCenter)
    {
        text_height++;
    }
    Rect text_frame(0, 0, wgettextwidth(TextToDraw, TextFont), text_height);
    Math::AlignInRect(content_frame, text_frame, TextAlignment);

    color_t text_color = ds->GetCompatibleColor(TextColor);
    if (draw_disabled)
    {
        text_color = ds->GetCompatibleColor(8);
    }
    wouttext_outline(ds, text_frame.Left, text_frame.Top, TextFont, text_color, TextToDraw);    
}

void GuiButton::DrawTextButton(Bitmap *ds, bool draw_disabled)
{
    color_t draw_color = ds->GetCompatibleColor(7);
    ds->FillRect(Rect(Frame.Left, Frame.Top, Frame.Left + Frame.GetWidth() - 1, Frame.Top + Frame.GetHeight() - 1), draw_color);
    if (Flags & kGuiCtrl_Default)
    {
        draw_color = ds->GetCompatibleColor(16);
        ds->DrawRect(Rect(Frame.Left - 1, Frame.Top - 1, Frame.Left + Frame.GetWidth(), Frame.Top + Frame.GetHeight()), draw_color);
    }

    if (IsMouseOver && IsPushed)
    {
        draw_color = ds->GetCompatibleColor(15);
    }
    else
    {
        draw_color = ds->GetCompatibleColor(8);
    }
    if (draw_disabled)
    {
        draw_color = ds->GetCompatibleColor(8);
    }
    ds->DrawLine(Line(Frame.Left, Frame.Top + Frame.GetHeight() - 1, Frame.Left + Frame.GetWidth() - 1, Frame.Top + Frame.GetHeight() - 1), draw_color);
    ds->DrawLine(Line(Frame.Left + Frame.GetWidth() - 1, Frame.Top, Frame.Left + Frame.GetWidth() - 1, Frame.Top + Frame.GetHeight() - 1), draw_color);
    if ((IsMouseOver) && (IsPushed))
    {
        draw_color = ds->GetCompatibleColor(8);
    }
    else
    {
        draw_color = ds->GetCompatibleColor(15);
    }
    if (draw_disabled)
    {
        draw_color = ds->GetCompatibleColor(8);
    }
    ds->DrawLine(Line(Frame.Left, Frame.Top, Frame.Left + Frame.GetWidth() - 1, Frame.Top), draw_color);
    ds->DrawLine(Line(Frame.Left, Frame.Top, Frame.Left, Frame.Top + Frame.GetHeight() - 1), draw_color);
    DrawText(ds, draw_disabled);
}

} // namespace Common
} // namespace AGS

AGS::Common::ObjectArray<AGS::Common::GuiButton> guibuts;
//GuiButton guibuts[MAX_OBJ_EACH_TYPE];
int numguibuts = 0;
