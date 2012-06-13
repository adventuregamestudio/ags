
#include <string.h>
#include "ac_interfaceelement.h"

// This is not even referenced from anywhere in the code, but I left it in Engine
// just in case, since it is a constructor of a declared class.

InterfaceElement::InterfaceElement() {
    vtextxp = 0; vtextyp = 1; strcpy(vtext,"@SCORETEXT@$r@GAMENAME@");
    numbuttons = 0; bgcol = 8; fgcol = 15; bordercol = 0; on = 1; flags = 0;
}
