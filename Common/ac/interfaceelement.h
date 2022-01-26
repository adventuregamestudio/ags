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

#ifndef __AC_INTERFACEELEMENT_H
#define __AC_INTERFACEELEMENT_H

#include "ac/interfacebutton.h" // InterfaceButton

// this struct should go in a Game struct, not the room structure.
struct InterfaceElement {
    int             x, y, x2, y2;
    int             bgcol, fgcol, bordercol;
    int             vtextxp, vtextyp, vtextalign;  // X & Y relative to topleft of interface
    char            vtext[40];
    int             numbuttons;
    InterfaceButton button[MAXBUTTON];
    int             flags;
    int             reserved_for_future;
    int             popupyp;   // pops up when mousey < this
    char            popup;     // does it pop up? (like sierra icon bar)
    char            on;
    InterfaceElement();
};

#endif // __AC_INTERFACEELEMENT_H