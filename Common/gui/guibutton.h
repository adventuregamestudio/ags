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
//
// 
//
//=============================================================================
#ifndef __AGS_CN_GUI__GUIBUTTON_H
#define __AGS_CN_GUI__GUIBUTTON_H

#include "gui/guiobject.h"

namespace AGS
{
namespace Common
{

enum GuiButtonClickAction
{
    kGuiBtnAction_None      = 0,
    kGuiBtnAction_SetMode   = 1,
    kGuiBtnAction_RunScript = 2,
};

// Legacy alignment constants are used to load old games properly
enum LegacyGuiButtonAlignment
{
    kLegacyGuiBtnAlign_TopCenter     = 0,
    kLegacyGuiBtnAlign_TopLeft       = 1,
    kLegacyGuiBtnAlign_TopRight      = 2,
    kLegacyGuiBtnAlign_CenterLeft    = 3,
    kLegacyGuiBtnAlign_Centered      = 4,
    kLegacyGuiBtnAlign_CenterRight   = 5,
    kLegacyGuiBtnAlign_BottomLeft    = 6,
    kLegacyGuiBtnAlign_BottomCenter  = 7,
    kLegacyGuiBtnAlign_BottomRight   = 8,
};


class GuiButton : public GuiObject
{
public:
    GuiButton();

    void Init();

    inline String GetText() const { return Text; }

    virtual void Draw(Common::Bitmap *ds);

    void         SetText(const String &text);

    virtual bool OnMouseDown();
    virtual void OnMouseLeave();
    virtual void OnMouseOver();
    virtual void OnMouseUp();
  
    virtual void WriteToFile(Common::Stream *out);
    virtual void ReadFromFile(Common::Stream *in, GuiVersion gui_version);
    virtual void WriteToSavedGame(Common::Stream *out);
    virtual void ReadFromSavedGame(Common::Stream *in, RuntimeGuiVersion gui_version);

// TODO: these members are currently public; hide them later
public:
    int32_t     NormalImage;
    int32_t     MouseOverImage;
    int32_t     PushedImage;
    int32_t     CurrentImage;
private:
    String      Text;
public:
    int32_t     TextFont;
    color_t     TextColor;
    Alignment   TextAlignment;
    GuiButtonClickAction ClickAction;
    int         ClickActionData;

    bool        IsPushed;
    bool        IsMouseOver;

private:
    static Alignment ConvertLegacyButtonAlignment(LegacyGuiButtonAlignment legacy_align);

    void DrawImageButton(Bitmap *ds, bool draw_disabled);
    void DrawText(Bitmap *ds, bool draw_disabled);
    void DrawTextButton(Bitmap *ds, bool draw_disabled);
    void PrepareTextToDraw();

    enum GuiButtonPlaceholder
    {
        kGuiBtnPlaceholder_None,
        kGuiBtnPlaceholder_InvItemStretch,
        kGuiBtnPlaceholder_InvItemCenter,
        kGuiBtnPlaceholder_InvItemAuto
    };

    GuiButtonPlaceholder Placeholder;
    // prepared text buffer/cache
    String               TextToDraw;
};

} // namespace Common
} // namespace AGS

#include "util/array.h"

extern AGS::Common::ObjectArray<AGS::Common::GuiButton> guibuts;
extern int numguibuts;

int UpdateAnimatingButton(int bu);
void StopButtonAnimation(int idxn);

#endif // __AGS_CN_GUI__GUIBUTTON_H
