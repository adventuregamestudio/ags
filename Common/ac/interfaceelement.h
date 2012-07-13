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

/*struct InterfaceStyle {
int  playareax1,playareay1,playareax2,playareay2; // where the game takes place
int  vtextxp,vtextyp;
char vtext[40];
int  numbuttons,popupbuttons;
InterfaceButton button[MAXBUTTON];
int  invx1,invy1,invx2,invy2;  // set invx1=-1 for Sierra-style inventory
InterfaceStyle() {  // sierra interface
playareax1=0; playareay1=13; playareax2=319; playareay2=199;
vtextxp=160; vtextyp=2; strcpy(vtext,"@SCORETEXT@$r@GAMENAME@");
invx1=-1; numbuttons=2; popupbuttons=1;
button[0].set(0,13,3,-1,0);
}
};*/

#endif // __AC_INTERFACEELEMENT_H