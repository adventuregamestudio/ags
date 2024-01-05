//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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
#include "gui/guiobject.h"
#include "util/string.h"

#define GUIBUTTON_LEGACY_TEXTLENGTH 50

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

enum LegacyButtonAlignment
{
    kLegacyButtonAlign_TopCenter = 0,
    kLegacyButtonAlign_TopLeft = 1,
    kLegacyButtonAlign_TopRight = 2,
    kLegacyButtonAlign_CenterLeft = 3,
    kLegacyButtonAlign_Centered = 4,
    kLegacyButtonAlign_CenterRight = 5,
    kLegacyButtonAlign_BottomLeft = 6,
    kLegacyButtonAlign_BottomCenter = 7,
    kLegacyButtonAlign_BottomRight = 8,
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


class GUIButton : public GUIObject
{
public:
    GUIButton();

    bool HasAlphaChannel() const override;
    int32_t GetCurrentImage() const;
    int32_t GetNormalImage() const;
    int32_t GetMouseOverImage() const;
    int32_t GetPushedImage() const;
    GUIButtonPlaceholder GetPlaceholder() const;
    const String &GetText() const;
    bool IsImageButton() const;
    bool IsClippingImage() const;

    // Operations
    Rect CalcGraphicRect(bool clipped) override;
    void Draw(Bitmap *ds, int x = 0, int y = 0) override;
    void SetClipImage(bool on);
    void SetCurrentImage(int32_t image);
    void SetMouseOverImage(int32_t image);
    void SetNormalImage(int32_t image);
    void SetPushedImage(int32_t image);
    void SetImages(int32_t normal, int32_t over, int32_t pushed);
    void SetText(const String &text);

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

// TODO: these members are currently public; hide them later
public:
    int32_t     Font;
    color_t     TextColor;
    FrameAlignment TextAlignment;
    // Click actions for left and right mouse buttons
    // NOTE: only left click is currently in use
    GUIClickAction ClickAction[kNumGUIClicks];
    int32_t        ClickData[kNumGUIClicks];

    bool        IsPushed;
    bool        IsMouseOver;

private:
    void DrawImageButton(Bitmap *ds, int x, int y, bool draw_disabled);
    void DrawText(Bitmap *ds, int x, int y, bool draw_disabled);
    void DrawTextButton(Bitmap *ds, int x, int y, bool draw_disabled);
    void PrepareTextToDraw();
    // Update current image depending on the button's state
    void UpdateCurrentImage();

    int32_t _image;
    int32_t _mouseOverImage;
    int32_t _pushedImage;
    // Active displayed image
    int32_t _currentImage;
    // Text property set by user
    String _text;
    // type of content placeholder, if any
    GUIButtonPlaceholder _placeholder;
    // A flag indicating unnamed button; this is a convenience trick:
    // buttons are created named "New Button" in the editor, and users
    // often do not clear text when they want a graphic button.
    bool _unnamed;
    // Prepared text buffer/cache
    String _textToDraw;
};

} // namespace Common
} // namespace AGS

extern std::vector<AGS::Common::GUIButton> guibuts;

#endif // __AC_GUIBUTTON_H
