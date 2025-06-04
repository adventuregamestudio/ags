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
#include "gui/guibutton.h"
#include "ac/gamestructdefines.h"
#include "ac/spritecache.h"
#include "ac/view.h"
#include "font/fonts.h"
#include "gui/guimain.h" // TODO: extract helper functions
#include "util/stream.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{


GUIButton::GUIButton()
{
    _clickAction[kGUIClickLeft] = kGUIAction_RunScript;
    _clickAction[kGUIClickRight] = kGUIAction_RunScript;
    _clickData[kGUIClickLeft] = 0;

    _scEventCount = 1;
    _scEventNames[0] = "Click";
    _scEventArgs[0] = "GUIControl *control, MouseButton button";
}

void GUIButton::SetFont(int font)
{
    if (_font != font)
    {
        _font = font;
        MarkChanged();
    }
}

void GUIButton::SetTextColor(int color)
{
    if (_textColor != color)
    {
        _textColor = color;
        MarkChanged();
    }
}

void GUIButton::SetTextAlignment(FrameAlignment align)
{
    if (_textAlignment != align)
    {
        _textAlignment = align;
        MarkChanged();
    }
}

void GUIButton::SetTextPaddingHor(int padding)
{
    if (_textPaddingHor != padding)
    {
        _textPaddingHor = padding;
        MarkChanged();
    }
}

void GUIButton::SetTextPaddingVer(int padding)
{
    if (_textPaddingVer != padding)
    {
        _textPaddingVer = padding;
        MarkChanged();
    }
}

int GUIButton::GetCurrentImage() const
{
    return _currentImage;
}

int GUIButton::GetNormalImage() const
{
    return _image;
}

int GUIButton::GetMouseOverImage() const
{
    return _mouseOverImage;
}

int GUIButton::GetPushedImage() const
{
    return _pushedImage;
}

GUIButtonPlaceholder GUIButton::GetPlaceholder() const
{
    return _placeholder;
}

const String &GUIButton::GetText() const
{
    return _text;
}

bool GUIButton::IsImageButton() const
{
    return _image > 0;
}

bool GUIButton::IsClippingImage() const
{
    return (_flags & kGUICtrl_Clip) != 0;
}

GUIClickAction GUIButton::GetClickAction(GUIClickMouseButton button) const
{
    if (button < kGUIClickLeft || button >= kNumGUIClicks)
        return kGUIAction_None;

    return _clickAction[button];
}

int GUIButton::GetClickData(GUIClickMouseButton button) const
{
    if (button < kGUIClickLeft || button >= kNumGUIClicks)
        return kGUIAction_None;

    return _clickData[button];
}

void GUIButton::SetClickAction(GUIClickMouseButton button, GUIClickAction action, int data)
{
    if (button < kGUIClickLeft || button >= kNumGUIClicks)
        return;

    _clickAction[button] = action;
    _clickData[button] = data;
}

Rect GUIButton::CalcGraphicRect(bool clipped)
{
    if (clipped)
        return RectWH(0, 0, _width, _height);

    assert(GUI::Context.Spriteset);
    SpriteCache &spriteset = *GUI::Context.Spriteset;

    // TODO: need to find a way to cache image and text position, or there'll be some repetition
    Rect rc = RectWH(0, 0, _width, _height);
    if (IsImageButton())
    {
        if (IsClippingImage())
            return rc;
        // Main button graphic
        if (spriteset.DoesSpriteExist(_currentImage))
            rc = SumRects(rc, RectWH(0, 0, get_adjusted_spritewidth(_currentImage), get_adjusted_spriteheight(_currentImage)));
        // Optionally merge with the inventory pic
        const int gui_inv_pic = GUI::Context.InventoryPic;
        if (_placeholder != kButtonPlace_None && gui_inv_pic >= 0)
        {
            Size inv_sz = Size(get_adjusted_spritewidth(gui_inv_pic), get_adjusted_spriteheight(gui_inv_pic));
            GUIButtonPlaceholder place = _placeholder;
            if (place == kButtonPlace_InvItemAuto)
            {
                place = ((inv_sz.Width > _width - 6) || (inv_sz.Height > _height - 6)) ?
                    kButtonPlace_InvItemStretch : kButtonPlace_InvItemCenter;
            }

            Rect inv_rc = (place == kButtonPlace_InvItemStretch) ?
                RectWH(0 + 3, 0 + 3, _width - 6, _height - 6) :
                RectWH(0 + _width / 2 - inv_sz.Width / 2,
                       0 + _height / 2 - inv_sz.Height / 2,
                       inv_sz.Width, inv_sz.Height);
            rc = SumRects(rc, inv_rc);
        }
    }
    // Optionally merge with the button text
    if (!IsImageButton() || ((_placeholder == kButtonPlace_None) && !_unnamed))
    {
        PrepareTextToDraw();
        Rect frame = RectWH(_textPaddingHor, _textPaddingVer, _width - _textPaddingHor * 2, _height - _textPaddingVer * 2);
        if (_isPushed && _isMouseOver)
        {
            frame = frame.MoveBy(frame, 1, 1);
        }

        Rect text_rc = IsWrapText() ?
            GUI::CalcTextGraphicalRect(Lines.GetVector(), Lines.Count(), _font, get_font_linespacing(_font), frame, _textAlignment) :
            GUI::CalcTextGraphicalRect(_textToDraw, _font, frame, _textAlignment);
        rc = SumRects(rc, text_rc);
    }
    return rc;
}

void GUIButton::Draw(Bitmap *ds, int x, int y)
{
    // A non-clickable button is, in effect, just a label.
    // When the GUI is disabled, the user should not get the message that
    // the button is now unclickable since it had never been in the first place.
    bool const button_is_clickable =
        _clickAction[kGUIClickLeft] != kGUIAction_None ||
        _clickAction[kGUIClickRight] != kGUIAction_None;

    bool const draw_disabled =
        !GUI::IsGUIEnabled(this) &&
        button_is_clickable &&
        GUI::Options.DisabledStyle != kGuiDis_Unchanged &&
        GUI::Options.DisabledStyle != kGuiDis_Off;

    // TODO: should only change properties in reaction to particular events
    if (_currentImage <= 0 || draw_disabled)
        _currentImage = _image;

    // No need to check Image after the assignment directly above
    if (_currentImage > 0)
        DrawImageButton(ds, x, y, draw_disabled);

    // CHECKME: why don't draw frame if no Text? this will make button completely invisible!
    else if (!_text.IsEmpty())
        DrawTextButton(ds, x, y, draw_disabled);
}

void GUIButton::SetClipImage(bool on)
{
    if (on != ((_flags & kGUICtrl_Clip) != 0))
        MarkChanged();
    if (on)
        _flags |= kGUICtrl_Clip;
    else
        _flags &= ~kGUICtrl_Clip;
}

void GUIButton::SetMouseOverImage(int image)
{
    if (_mouseOverImage == image)
        return;

    _mouseOverImage = image;
    UpdateCurrentImage();
}

void GUIButton::SetNormalImage(int image)
{
    if (_image == image)
        return;

    _image = image;
    UpdateCurrentImage();
}

void GUIButton::SetPushedImage(int image)
{
    if (_pushedImage == image)
        return;

    _pushedImage = image;
    UpdateCurrentImage();
}

void GUIButton::SetImages(int normal, int over, int pushed,
                          SpriteTransformFlags flags, int xoff, int yoff)
{
    _image = normal;
    _mouseOverImage = over;
    _pushedImage = pushed;
    _imageFlags = flags;
    _imageXOff = xoff;
    _imageYOff = yoff;
    UpdateCurrentImage();
}

void GUIButton::SetCurrentImage(int32_t new_image, SpriteTransformFlags flags, int xoff, int yoff)
{
    if (_currentImage == new_image && _curImageFlags == flags)
        return;

    _currentImage = new_image;
    _curImageFlags = flags;
    _curImageXOff = xoff;
    _curImageYOff = yoff;
    MarkChanged();
}

void GUIButton::SetText(const String &text)
{
    if (_text == text)
        return;
    _text = text;
    // Active inventory item placeholders
    if (_text.CompareNoCase("(INV)") == 0)
        // Stretch to fit button
        _placeholder = kButtonPlace_InvItemStretch;
    else if (_text.CompareNoCase("(INVNS)") == 0)
        // Draw at actual size
        _placeholder = kButtonPlace_InvItemCenter;
    else if (_text.CompareNoCase("(INVSHR)") == 0)
        // Stretch if too big, actual size if not
        _placeholder = kButtonPlace_InvItemAuto;
    else
        _placeholder = kButtonPlace_None;

    // TODO: find a way to remove this bogus limitation ("New Button" is a valid Text too)
    _unnamed = _text.IsEmpty() || _text.Compare("New Button") == 0;
    MarkChanged();
}

void GUIButton::SetWrapText(bool on)
{
    if (on != ((_flags & kGUICtrl_WrapText) != 0))
    {
        _flags = (_flags & ~kGUICtrl_WrapText) | kGUICtrl_WrapText * on;
        MarkChanged();
    }
}

bool GUIButton::OnMouseDown()
{
    if (!IsImageButton())
        MarkChanged();
    _isPushed = true;
    UpdateCurrentImage();
    return false;
}

void GUIButton::OnMouseEnter()
{
    if (_isPushed && !IsImageButton())
        MarkChanged();
    _isMouseOver = true;
    UpdateCurrentImage();
}

void GUIButton::OnMouseLeave()
{
    if (_isPushed && !IsImageButton())
        MarkChanged();
    _isMouseOver = false;
    UpdateCurrentImage();
}

void GUIButton::OnMouseUp()
{
    if (_isMouseOver)
    {
        if (GUI::IsGUIEnabled(this) && IsClickable())
            _isActivated = true;
    }

    if (_isPushed && !IsImageButton())
        MarkChanged();
    _isPushed = false;
    UpdateCurrentImage();
}

void GUIButton::UpdateCurrentImage()
{
    int new_image = _currentImage;
    SpriteTransformFlags new_flags = kSprTf_None;
    int new_xoff = 0, new_yoff = 0;

    if (_isPushed && (_pushedImage > 0))
    {
        new_image = _pushedImage;
    }
    else if (_isMouseOver && (_mouseOverImage > 0))
    {
        new_image = _mouseOverImage;
    }
    else
    {
        new_image = _image;
        new_flags = _imageFlags;
        new_xoff = _imageXOff;
        new_yoff = _imageYOff;
    }

    SetCurrentImage(new_image, new_flags, new_xoff, new_yoff);
}

void GUIButton::WriteToFile(Stream *out) const
{
    GUIControl::WriteToFile(out);

    out->WriteInt32(_image);
    out->WriteInt32(_mouseOverImage);
    out->WriteInt32(_pushedImage);
    out->WriteInt32(_font);
    out->WriteInt32(_textColor);
    out->WriteInt32(_clickAction[kGUIClickLeft]);
    out->WriteInt32(_clickAction[kGUIClickRight]);
    out->WriteInt32(_clickData[kGUIClickLeft]);
    out->WriteInt32(_clickData[kGUIClickRight]);

    StrUtil::WriteString(_text, out);
    out->WriteInt32(_textAlignment);
}

void GUIButton::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GUIControl::ReadFromFile(in, gui_version);

    _image = in->ReadInt32();
    _mouseOverImage = in->ReadInt32();
    _pushedImage = in->ReadInt32();
    _font = in->ReadInt32();
    _textColor = in->ReadInt32();
    _clickAction[kGUIClickLeft] = (GUIClickAction)in->ReadInt32();
    _clickAction[kGUIClickRight] = (GUIClickAction)in->ReadInt32();
    _clickData[kGUIClickLeft] = in->ReadInt32();
    _clickData[kGUIClickRight] = in->ReadInt32();
    SetText(StrUtil::ReadString(in));
    _textAlignment = (FrameAlignment)in->ReadInt32();

    if (_textColor == 0)
        _textColor = 16; // FIXME: adjust this using GetStandardColor where is safe to access GuiContext
    _currentImage = _image;
}

void GUIButton::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    GUIControl::ReadFromSavegame(in, svg_ver);
    _image = in->ReadInt32();
    _mouseOverImage = in->ReadInt32();
    _pushedImage = in->ReadInt32();
    _font = in->ReadInt32();
    _textColor = in->ReadInt32();
    SetText(StrUtil::ReadString(in));
    if (svg_ver >= kGuiSvgVersion_350)
        _textAlignment = (FrameAlignment)in->ReadInt32();
    // CHECKME: possibly this may be avoided, and currentimage updated according
    // to the button state after load
    _currentImage = in->ReadInt32();
    if (svg_ver >= kGuiSvgVersion_36202 && (svg_ver < kGuiSvgVersion_400 || svg_ver >= kGuiSvgVersion_40010))
    {
        _textPaddingHor = in->ReadInt32();
        _textPaddingVer = in->ReadInt32();
        in->ReadInt32(); // reserve 2 ints
        in->ReadInt32();
    }
    else
    {
        _textPaddingHor = DefaultHorPadding;
        _textPaddingVer = DefaultVerPadding;
    }

    if (svg_ver >= kGuiSvgVersion_400)
    {
        _imageFlags = (SpriteTransformFlags)in->ReadInt32();
    }
    else
    {
        _imageFlags = kSprTf_None;
    }

    // Update current state after reading
    _isPushed = false;
    _isMouseOver = false;
    _curImageFlags = _imageFlags;
    UpdateCurrentImage();
}

void GUIButton::WriteToSavegame(Stream *out) const
{
    GUIControl::WriteToSavegame(out);
    out->WriteInt32(_image);
    out->WriteInt32(_mouseOverImage);
    out->WriteInt32(_pushedImage);
    out->WriteInt32(_font);
    out->WriteInt32(_textColor);
    StrUtil::WriteString(GetText(), out);
    out->WriteInt32(_textAlignment);
    out->WriteInt32(_currentImage);
    out->WriteInt32(_textPaddingHor);
    out->WriteInt32(_textPaddingVer);
    out->WriteInt32(0); // reserve 2 ints
    out->WriteInt32(0);
    //since kGuiSvgVersion_3991
    out->WriteInt32(_imageFlags);
}

void GUIButton::DrawImageButton(Bitmap *ds, int x, int y, bool draw_disabled)
{
    if (draw_disabled && GUI::Options.DisabledStyle == kGuiDis_Blackout)
        return; // button should not be shown at all

    assert(_currentImage >= 0);
    assert(GUI::Context.Spriteset);
    SpriteCache &spriteset = *GUI::Context.Spriteset;

    // NOTE: the CLIP flag only clips the image, not the text
    if (IsClippingImage() && !GUI::Options.ClipControls)
        ds->SetClip(RectWH(x, y, _width, _height));

    if (spriteset.DoesSpriteExist(_currentImage))
    {
        draw_gui_sprite_flipped(ds, _currentImage, x + _curImageXOff, y + _curImageYOff,
                                kBlend_Normal, GfxDef::GetFlipFromFlags(_curImageFlags));
    }

    // Draw active inventory item
    const int gui_inv_pic = GUI::Context.InventoryPic;
    if (_placeholder != kButtonPlace_None && gui_inv_pic >= 0)
    {
        Size inv_sz = Size(get_adjusted_spritewidth(gui_inv_pic), get_adjusted_spriteheight(gui_inv_pic));
        GUIButtonPlaceholder place = _placeholder;
        if (place == kButtonPlace_InvItemAuto)
        {
            place = ((inv_sz.Width > _width - 6) || (inv_sz.Height > _height - 6)) ?
                kButtonPlace_InvItemStretch : kButtonPlace_InvItemCenter;
        }

        if (place == kButtonPlace_InvItemStretch)
        {
            ds->StretchBlt(spriteset[gui_inv_pic], RectWH(x + 3, y + 3, _width - 6, _height - 6),
                kBitmap_Transparency);
        }
        else
        {
            draw_gui_sprite(ds, gui_inv_pic,
                x + _width / 2 - inv_sz.Width / 2,
                y + _height / 2 - inv_sz.Height / 2);
        }
    }

    if ((draw_disabled) && (GUI::Options.DisabledStyle == kGuiDis_Greyout))
    {
        // darken the button when disabled
        const Size sz = spriteset.GetSpriteResolution(_currentImage);
        GUI::DrawDisabledEffect(ds, RectWH(x, y, sz.Width, sz.Height));
    }

    // Don't print Text of (INV) (INVSHR) (INVNS)
    if ((_placeholder == kButtonPlace_None) && !_unnamed)
        DrawText(ds, x, y, draw_disabled);

    if (IsClippingImage() && !GUI::Options.ClipControls)
        ds->ResetClip();
}

void GUIButton::DrawText(Bitmap *ds, int x, int y, bool draw_disabled)
{
    // TODO: need to find a way to cache Text prior to drawing;
    // but that will require to update all gui controls when translation is changed in game
    PrepareTextToDraw();

    Rect frame = RectWH(x + _textPaddingHor, y + _textPaddingVer, _width - _textPaddingHor * 2, _height - _textPaddingVer * 2);
    if (_isPushed && _isMouseOver)
    {
        // move the Text a bit while pushed
        frame = frame.MoveBy(frame, 1, 1);
    }
    color_t text_color = ds->GetCompatibleColor(_textColor);
    if (draw_disabled)
        text_color = GUI::GetStandardColorForBitmap(8);

    if (IsWrapText())
        GUI::DrawTextLinesAligned(ds, Lines.GetVector(), Lines.Count(), _font, get_font_linespacing(_font), text_color,
            frame, _textAlignment);
    else
        GUI::DrawTextAligned(ds, _textToDraw, _font, text_color, frame, _textAlignment);
}

void GUIButton::DrawTextButton(Bitmap *ds, int x, int y, bool draw_disabled)
{
    if (draw_disabled && GUI::Options.DisabledStyle == kGuiDis_Blackout)
        return; // button should not be shown at all

    color_t draw_color = GUI::GetStandardColorForBitmap(7);
    ds->FillRect(Rect(x, y, x + _width - 1, y + _height - 1), draw_color);
    if (_flags & kGUICtrl_Default)
    {
        draw_color = GUI::GetStandardColorForBitmap(16);
        ds->DrawRect(Rect(x - 1, y - 1, x + _width, y + _height), draw_color);
    }

    // TODO: use color constants instead of literal numbers
    if (!draw_disabled && _isMouseOver && _isPushed)
        draw_color = GUI::GetStandardColorForBitmap(15);
    else
        draw_color = GUI::GetStandardColorForBitmap(8);

    ds->DrawLine(Line(x, y + _height - 1, x + _width - 1, y + _height - 1), draw_color);
    ds->DrawLine(Line(x + _width - 1, y, x + _width - 1, y + _height - 1), draw_color);

    if (draw_disabled || (_isMouseOver && _isPushed))
        draw_color = GUI::GetStandardColorForBitmap(8);
    else
        draw_color = GUI::GetStandardColorForBitmap(15);

    ds->DrawLine(Line(x, y, x + _width - 1, y), draw_color);
    ds->DrawLine(Line(x, y, x, y + _height - 1), draw_color);

    DrawText(ds, x, y, draw_disabled);
}

} // namespace Common
} // namespace AGS
