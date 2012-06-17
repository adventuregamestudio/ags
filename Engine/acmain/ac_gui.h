

void remove_popup_interface(int ifacenum);
// Returns 1 if user interface is enabled, 0 if disabled
int IsInterfaceEnabled();
void DisableInterface();
void EnableInterface();

extern int ifacepopped;  // currently displayed pop-up GUI (-1 if none)

extern int mouse_on_iface;   // mouse cursor is over this interface
extern int mouse_on_iface_button;
extern int mouse_pushed_iface;  // this BUTTON on interface MOUSE_ON_IFACE is pushed
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;

extern int eip_guinum, eip_guiobj;



