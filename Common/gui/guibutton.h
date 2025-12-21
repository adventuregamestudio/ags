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

enum GUIClickMouseButton
{
    kGUIClickLeft = 0,
    kGUIClickRight = 1,
    kNumGUIClicks
};

enum GUIClickAction
{
    kGUIAction_None = 0,
    kGUIAction_SetMode = 1,
    kGUIAction_RunScript = 2,
};


class GUIButton : public GUIObject
{
public:
    // Default text padding
    static const int DefaultHorPadding = 1;
    static const int DefaultVerPadding = 1;

    GUIButton();

    // Properties
    void SetButtonFlags(int flags);
    int  GetFont() const { return _font; }
    void SetFont(int font);
    bool IsDynamicColors() const { return ((_buttonFlags & kButton_DynamicColors) != 0); }
    void SetDynamicColors(bool on);
    bool IsFlatStyle() const { return ((_buttonFlags & kButton_FlatStyle) != 0); }
    void SetFlatStyle(bool on);
    int  GetMouseOverBackColor() const { return _mouseOverBackColor; }
    void SetMouseOverBackColor(int color);
    int  GetPushedBackColor() const { return _pushedBackColor; }
    void SetPushedBackColor(int color);
    int  GetMouseOverBorderColor() const { return _mouseOverBorderColor; }
    void SetMouseOverBorderColor(int color);
    int  GetPushedBorderColor() const { return _pushedBorderColor; }
    void SetPushedBorderColor(int color);
    int  GetShadowColor() const { return _shadowColor; }
    void SetShadowColor(int color);
    int  GetTextColor() const { return _textColor; }
    void SetTextColor(int color);
    int  GetMouseOverTextColor() const { return _mouseOverTextColor; }
    void SetMouseOverTextColor(int color);
    int  GetPushedTextColor() const { return _pushedTextColor; }
    void SetPushedTextColor(int color);
    FrameAlignment GetTextAlignment() const { return _textAlignment; }
    void SetTextAlignment(FrameAlignment align);

    bool HasAlphaChannel() const override;
    int  GetCurrentImage() const;
    int  GetNormalImage() const;
    int  GetMouseOverImage() const;
    int  GetPushedImage() const;
    GUIButtonPlaceholder GetPlaceholder() const;
    const String &GetText() const;
    bool IsImageButton() const;
    bool IsClippingImage() const;
    bool HasAction() const;

    void SetClipImage(bool on);
    void SetMouseOverImage(int image);
    void SetNormalImage(int image);
    void SetPushedImage(int image);
    void SetImages(int normal, int over, int pushed);
    void SetText(const String &text);
    void SetWrapText(bool on);

    GUIClickAction GetClickAction(GUIClickMouseButton button) const;
    int  GetClickData(GUIClickMouseButton button) const;
    void SetClickAction(GUIClickMouseButton button, GUIClickAction action, int data);
    
    // Script Events
    uint32_t GetEventCount() const override;
    String GetEventArgs(uint32_t event) const override;
    String GetEventName(uint32_t event) const override;

    // Operations
    Rect CalcGraphicRect(bool clipped) override;
    void Draw(Bitmap *ds, int x = 0, int y = 0) override;

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

    // Upgrades the GUI control to default looks for 3.6.3
    void SetDefaultLooksFor363() override;

private:
    // Reports that any of the basic colors have changed;
    // the button will update its current colors depending on its state
    void OnColorsChanged() override;

    void DrawImageButton(Bitmap *ds, int x, int y, bool draw_disabled);
    void DrawText(Bitmap *ds, int x, int y, bool draw_disabled);
    void DrawTextButton(Bitmap *ds, int x, int y, bool draw_disabled);
    void PrepareTextToDraw();
    // Update current image and colors depending on the button's state
    void UpdateCurrentImage();
    void SetCurrentImage(int image);
    void SetCurrentColors(int bg_color, int border_color, int text_color);

    static const int EventCount = 1;
    static String EventNames[EventCount];
    static String EventArgs[EventCount];

    uint32_t _buttonFlags = 0u;
    color_t _shadowColor = 0;
    color_t _textColor = 0;
    color_t _mouseOverBackColor = 0;
    color_t _pushedBackColor = 0;
    color_t _mouseOverBorderColor = 0;
    color_t _pushedBorderColor = 0;
    color_t _mouseOverTextColor = 0;
    color_t _pushedTextColor = 0;
    int     _font = 0;
    FrameAlignment _textAlignment = kAlignTopCenter;
    // Click actions for left and right mouse buttons
    // NOTE: only left click is currently in use
    GUIClickAction _clickAction[kNumGUIClicks];
    int     _clickData[kNumGUIClicks];

    bool    _isPushed = false;
    bool    _isMouseOver = false;

    int     _image = -1;
    int     _mouseOverImage = -1;
    int     _pushedImage = -1;
    // Active displayed image (depends on button state)
    int     _currentImage = -1;
    // Active colors (depend on button state)
    color_t _currentBgColor = 0;
    color_t _currentBorderColor = 0;
    color_t _currentTextColor = 0;
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
