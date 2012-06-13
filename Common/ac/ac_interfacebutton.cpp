
#include "ac_interfacebutton.h"

#ifdef UNUSED_CODE
void InterfaceButton::set(int xx, int yy, int picc, int overpicc, int actionn) {
    x = xx; y = yy; pic = picc; overpic = overpicc; leftclick = actionn; pushpic = 0;
    rightclick = 0; flags = IBFLG_ENABLED;
    reserved_for_future = 0;
}
#endif
