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
#ifndef __AC_GUIBUTTON_H
#define __AC_GUIBUTTON_H

#include <vector>
#include "gui/guicontrol.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

enum GUIClickMouseButton
{
    kGUIClickLeft         = 0,
    kGUIClickRight        = 1,
    kNumGUIClicks
};

enum GUIClickAction
{
    kGUIAction_None       = 0,
    kGUIAction_SetMode    = 1,
    kGUIAction_RunScript  = 2,
};

// Defines button placeholder mode; the mode is set
// depending on special tags found in button text
enum GUIButtonPlaceholder
{
    kButtonPlace_None,
    kButtonPlace_InvItemStretch,
    kButtonPlace_InvItemCenter,
    kButtonPlace_InvItemAuto
};


class GUIButton : public GUIControl
{
public:
    // Default text padding
    static const int DefaultHorPadding = 2;
    static const int DefaultVerPadding = 2;

    GUIButton();

    // Properties
    int  GetFont() const { return _font; }
    void SetFont(int font);
    int  GetTextColor() const { return _textColor; }
    void SetTextColor(int color);
    FrameAlignment GetTextAlignment() const { return _textAlignment; }
    void SetTextAlignment(FrameAlignment align);
    int  GetTextPaddingHor() const { return _textPaddingHor; }
    void SetTextPaddingHor(int padding);
    int  GetTextPaddingVer() const { return _textPaddingVer; }
    void SetTextPaddingVer(int padding);

    int  GetCurrentImage() const;
    int  GetNormalImage() const;
    int  GetMouseOverImage() const;
    int  GetPushedImage() const;
    GUIButtonPlaceholder GetPlaceholder() const;
    const String &GetText() const;
    bool IsImageButton() const;
    bool IsClippingImage() const;
    int32_t CurrentImage() const;

    GUIClickAction GetClickAction(GUIClickMouseButton button) const;
    int  GetClickData(GUIClickMouseButton button) const;
    void SetClickAction(GUIClickMouseButton button, GUIClickAction action, int data);

    // Operations
    Rect CalcGraphicRect(bool clipped) override;
    void Draw(Bitmap *ds, int x = 0, int y = 0) override;
    void SetClipImage(bool on);
    void SetCurrentImage(int image, SpriteTransformFlags flags = kSprTf_None, int xoff = 0, int yoff = 0);
    void SetMouseOverImage(int image);
    void SetNormalImage(int image);
    void SetPushedImage(int image);
    void SetImages(int normal, int over, int pushed, SpriteTransformFlags flags = kSprTf_None, int xoff = 0, int yoff = 0);
    void SetText(const String &text);
    void SetWrapText(bool on);

    // Events
    bool OnMouseDown() override;
    void OnMouseEnter() override;
    void OnMouseLeave() override;
    void OnMouseUp() override;
  
    // Serialization
    void ReadFromFile(Stream *in, GuiVersion gui_version) override;
    void WriteToFile(Stream *out) const override;
    void ReadFromSavegame(Common::Stream *in, GuiSvgVersion svg_ver) override;
    void WriteToSavegame(Common::Stream *out) const override;

private:
    void DrawImageButton(Bitmap *ds, int x, int y, bool draw_disabled);
    void DrawText(Bitmap *ds, int x, int y, bool draw_disabled);
    void DrawTextButton(Bitmap *ds, int x, int y, bool draw_disabled);
    void PrepareTextToDraw();
    // Update current image depending on the button's state
    void UpdateCurrentImage();

    int     _font = 0;
    color_t _textColor = 0;
    FrameAlignment _textAlignment = kAlignTopCenter;
    // TODO: flags for each kind of image?
    SpriteTransformFlags _imageFlags = kSprTf_None;
    int     _imageXOff = 0;
    int     _imageYOff = 0;
    int     _textPaddingHor = DefaultHorPadding;
    int     _textPaddingVer = DefaultVerPadding;
    // Click actions for left and right mouse buttons
    // NOTE: only left click is currently in use
    GUIClickAction _clickAction[kNumGUIClicks];
    int     _clickData[kNumGUIClicks];

    bool    _isPushed = false;
    bool    _isMouseOver = false;

    int     _image = -1;
    int     _mouseOverImage = -1;
    int     _pushedImage = -1;
    // Active displayed image
    int     _currentImage = -1;
    SpriteTransformFlags _curImageFlags = kSprTf_None;
    int     _curImageXOff = 0;
    int     _curImageYOff = 0;
    // Text property set by user
    String  _text;
    // type of content placeholder, if any
    GUIButtonPlaceholder _placeholder = kButtonPlace_None;
    // A flag indicating unnamed button; this is a convenience trick:
    // buttons are created named "New Button" in the editor, and users
    // often do not clear text when they want a graphic button.
    bool    _unnamed = true;
    // Prepared text buffer/cache
    String  _textToDraw;
};

} // namespace Common
} // namespace AGS

#endif // __AC_GUIBUTTON_H
