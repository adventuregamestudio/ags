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
#ifndef __AGS_CN_GUI__GUIOBJECT_H
#define __AGS_CN_GUI__GUIOBJECT_H

#include "gui/guidefines.h"
#include "util/geometry.h"
#include "util/string.h"

#define MAX_GUIOBJ_EVENTS                  10
#define LEGACY_MAX_GUIOBJ_SCRIPTNAME_LEN   25
#define LEGACY_MAX_GUIOBJ_EVENTHANDLER_LEN 30

namespace AGS
{
namespace Common
{

class Bitmap;
class Stream;

enum GuiControlFlags
{
    kGuiCtrl_Default    = 0x0001,
    kGuiCtrl_Cancel     = 0x0002, // unused
    kGuiCtrl_Disabled   = 0x0004,
    kGuiCtrl_TabStop    = 0x0008, // unused
    kGuiCtrl_Invisible  = 0x0010,
    kGuiCtrl_Clip       = 0x0020,
    kGuiCtrl_NoClicks   = 0x0040,
    kGuiCtrl_Translated = 0x0080, // 3.3.0.1132
    kGuiCtrl_Deleted    = 0x8000,
};

enum LegacyGuiAlignment
{
    kLegacyGuiAlign_Left   = 0,
    kLegacyGuiAlign_Right  = 1,
    kLegacyGuiAlign_Center = 2
};


class GuiObject
{
public:
    GuiObject();
    
    virtual String  GetEventArgs(int event) const
    {
        if ((event < 0) || (event >= SupportedEventCount))
        {
            return "";
        }
        return EventArgs[event];
    }
    virtual int     GetEventCount() const { return SupportedEventCount; }
    virtual String  GetEventName(int event)
    {
        if ((event < 0) || (event >= SupportedEventCount))
        {
            return "";
        }
        return EventNames[event];
    }
    inline Rect     GetFrame()  const { return Frame; }
    inline int      GetX()      const { return Frame.Left; }
    inline int      GetY()      const { return Frame.Top; }
    inline int      GetWidth()  const { return Frame.GetWidth(); }
    inline int      GetHeight() const { return Frame.GetHeight(); }
    bool            IsDeleted() const { return (Flags & kGuiCtrl_Deleted) != 0; }
    bool            IsDisabled() const;
    // overridable routine to determine whether the mouse is over the control
    virtual bool    IsOverControl(int x, int y, int leeway) const;
    inline bool     IsTranslated() const { return (Flags & kGuiCtrl_Translated) != 0; }
    inline bool     IsVisible() const { return (Flags & kGuiCtrl_Invisible) == 0; }
    bool            IsClickable() const;
    
    inline void     Disable() { Flags |= kGuiCtrl_Disabled; }
    virtual void    Draw(Common::Bitmap *ds) = 0;
    inline void     Enable()  { Flags &= ~kGuiCtrl_Disabled; }
    inline void     Hide()    { Flags |= kGuiCtrl_Invisible; }
    inline void     SetClickable(bool clickable)
    {
        if (clickable)
        {
            Flags &= ~kGuiCtrl_NoClicks;
        }
        else
        {
            Flags |= kGuiCtrl_NoClicks;
        }            
    }
    void            SetFrame(const Rect &frame);
    void            SetX(int x);
    void            SetY(int y);
    void            SetWidth(int width);
    void            SetHeight(int height);
    inline void     Show()    { Flags &= ~kGuiCtrl_Invisible; }

    virtual void    OnKeyPress(int keycode) { }
    // button down - return True to lock focus
    virtual bool    OnMouseDown() { return false; }
    // mouse moves off object
    virtual void    OnMouseLeave() { }
    // x,y relative to gui
    virtual void    OnMouseMove(int x, int y) { }
    // mouse moves onto object
    virtual void    OnMouseOver() { }
    // button up
    virtual void    OnMouseUp() { }
    // called when the control is resized
    virtual void    OnResized() { }

    virtual void    WriteToFile(Common::Stream *out);
    virtual void    ReadFromFile(Common::Stream *in, GuiVersion gui_version);
    virtual void    WriteToSavedGame(Common::Stream *out);
    virtual void    ReadFromSavedGame(Common::Stream *in, RuntimeGuiVersion gui_version);

// TODO: these members are currently public; hide them later
public:
    int32_t         Id;
    int32_t         ParentId;
    uint32_t        Flags;
protected:
    Rect            Frame;
public:
    int32_t         ZOrder;
    bool            IsActivated;
    String          ScriptName;
    String          EventHandlers[MAX_GUIOBJ_EVENTS];
  
protected:
    int32_t         SupportedEventCount;
    String          EventNames[MAX_GUIOBJ_EVENTS];
    String          EventArgs[MAX_GUIOBJ_EVENTS];

protected:
    static Alignment ConvertLegacyAlignment(LegacyGuiAlignment legacy_align);
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GUI__GUIOBJECT_H
