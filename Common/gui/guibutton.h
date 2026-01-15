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
#include "gui/guicontrol.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

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

// Button event indexes;
// these are used after resolving events map read from game file
enum ButonEventID
{
    kButtonEvent_OnClick,
    kButtonEvent_OnFrameEvent,
    kNumButtonEvents
};


class GUIButton : public GUIControl
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

    int  GetCurrentImage() const;
    int  GetNormalImage() const;
    int  GetMouseOverImage() const;
    int  GetPushedImage() const;
    SpriteTransformFlags GetImageFlags() const;
    GraphicFlip GetImageFlip() const;
    SpriteTransformFlags GetCurrentImageFlags() const;
    GraphicFlip GetCurrentImageFlip() const;
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
    // Gets a events schema corresponding to this object's type
    static const ScriptEventSchema &GetEventSchema() { return GUIButton::_eventSchema; }
    virtual const ScriptEventSchema *GetTypeEventSchema() const override { return &GUIButton::_eventSchema; }

    // Operations
    Rect CalcGraphicRect(bool clipped) override;
    void Draw(Bitmap *ds, int x = 0, int y = 0) override;
    void SetImages(int normal, int over, int pushed, SpriteTransformFlags flags = kSprTf_None, int xoff = 0, int yoff = 0);
    void SetImageFlags(SpriteTransformFlags flags);
    void SetImageFlip(GraphicFlip flip);

    // Events
    bool OnMouseDown() override;
    void OnMouseEnter() override;
    void OnMouseLeave() override;
    void OnMouseUp() override;
  
    // Serialization
    void ReadFromFile(Stream *in, GuiVersion gui_version) override;
    void ReadFromFile_Ext363(Stream *in, GuiVersion gui_version) override;
    void WriteToFile(Stream *out) const override;
    void ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver) override;
    void WriteToSavegame(Stream *out) const override;

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
    void SetCurrentImage(int image, SpriteTransformFlags flags, int xoff, int yoff);
    void SetCurrentColors(int bg_color, int border_color, int text_color);

    // Script events schema
    static ScriptEventSchema _eventSchema;

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
    // TODO: flags for each kind of image?
    SpriteTransformFlags _imageFlags = kSprTf_None;
    int     _imageXOff = 0;
    int     _imageYOff = 0;
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
    SpriteTransformFlags _curImageFlags = kSprTf_None;
    int     _curImageXOff = 0;
    int     _curImageYOff = 0;
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
