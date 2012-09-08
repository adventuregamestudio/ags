
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTGUI_H
#define __AGS_EE_DYNOBJ__SCRIPTGUI_H

#include "gui/guimain.h"

// 64 bit: This struct must be 8 byte long
struct ScriptGUI {
  int id;
  //GUIMain *gui;
  int __padding;
};

#endif // __AGS_EE_DYNOBJ__SCRIPTGUI_H
