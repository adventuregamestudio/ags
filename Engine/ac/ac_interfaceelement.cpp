
#include <string.h>
#include "ac/ac_interfaceelement.h"

InterfaceElement::InterfaceElement() {
    vtextxp = 0; vtextyp = 1; strcpy(vtext,"@SCORETEXT@$r@GAMENAME@");
    numbuttons = 0; bgcol = 8; fgcol = 15; bordercol = 0; on = 1; flags = 0;
}
