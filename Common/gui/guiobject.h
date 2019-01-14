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

#ifndef __AC_GUIOBJECT_H
#define __AC_GUIOBJECT_H

#include "core/types.h"
#include "gfx/bitmap.h"
#include "gui/guidefines.h"
#include "util/string.h"

#define GUIDIS_GREYOUT   1
#define GUIDIS_BLACKOUT  2
#define GUIDIS_UNCHANGED 4
#define GUIDIS_GUIOFF  0x80




namespace AGS
{
namespace Common
{

enum LegacyGUIAlignment
{
    kLegacyGUIAlign_Left   = 0,
    kLegacyGUIAlign_Right  = 1,
    kLegacyGUIAlign_Center = 2
};

class GUIObject
{
public:
    GUIObject();
    virtual ~GUIObject(){}
    
    String          GetEventArgs(int event) const;
    int             GetEventCount() const;
    String          GetEventName(int event) const;
    bool            IsDeleted() const { return (Flags & kGUICtrl_Deleted) != 0; }
    // checks both control flag and global variable
    bool            IsEnabled() const;
    // overridable routine to determine whether the mouse is over the control
    virtual bool    IsOverControl(int x, int y, int leeway) const;
    inline bool     IsTranslated() const { return (Flags & kGUICtrl_Translated) != 0; }
    inline bool     IsVisible() const { return (Flags & kGUICtrl_Invisible) == 0; }
    // implemented separately in engine and editor
    bool            IsClickable() const;
    
    // Operations
    void            Disable();
    virtual void    Draw(Bitmap *ds) { }
    void            Enable();
    void            Hide();
    void            SetClickable(bool clickable);
    void            Show();

    // Events
    // Key pressed for control
    virtual void    OnKeyPress(int keycode) { }
    // Mouse button down - return 'True' to lock focus
    virtual bool    OnMouseDown() { return false; }
    // Mouse moves onto control
    virtual void    OnMouseEnter() { }
    // Mouse moves off control
    virtual void    OnMouseLeave() { }
    // Mouse moves over control - x,y relative to gui
    virtual void    OnMouseMove(int x, int y) { }
    // Mouse button up
    virtual void    OnMouseUp() { }
    // Control was resized
    virtual void    OnResized() { }

    // Serialization
    virtual void    WriteToFile(Common::Stream *out);
    virtual void    ReadFromFile(Common::Stream *in, GuiVersion gui_version);
    virtual void    ReadFromSavegame(Common::Stream *in);
    virtual void    WriteToSavegame(Common::Stream *out) const;

// TODO: these members are currently public; hide them later
public:
    int32_t  Id;         // GUI object's identifier
    int32_t  ParentId;   // id of parent GUI
    String   Name;       // script name
    uint32_t Flags;      // generic style and behavior flags

    int32_t  X;
    int32_t  Y;
    int32_t  Width;
    int32_t  Height;
    int32_t  ZOrder;
    bool     IsActivated; // signals user interaction

    String   EventHandlers[MAX_GUIOBJ_EVENTS]; // script function names
  
protected:
    // TODO: explicit event names & handlers for every event
    int32_t  _scEventCount;                    // number of supported script events
    String   _scEventNames[MAX_GUIOBJ_EVENTS]; // script event names
    String   _scEventArgs[MAX_GUIOBJ_EVENTS];  // script handler params
};


FrameAlignment ConvertLegacyGUIAlignment(int32_t align);

} // namespace Common
} // namespace AGS

#endif // __AC_GUIOBJECT_H
