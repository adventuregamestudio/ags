#ifndef __AC_INTERFACEBUTTON_H
#define __AC_INTERFACEBUTTON_H

#define MAXBUTTON       20
#define IBACT_SETMODE   1
#define IBACT_SCRIPT    2
#define IBFLG_ENABLED   1
#define IBFLG_INVBOX    2
struct InterfaceButton {
    int x, y, pic, overpic, pushpic, leftclick;
    int rightclick; // if inv, then leftclick = wid, rightclick = hit
    int reserved_for_future;
    char flags;
    void set(int xx, int yy, int picc, int overpicc, int actionn);
};

#endif // __AC_INTERFACEBUTTON_H