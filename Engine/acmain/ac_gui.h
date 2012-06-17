

void remove_popup_interface(int ifacenum);
// Returns 1 if user interface is enabled, 0 if disabled
int IsInterfaceEnabled();
void DisableInterface();
void EnableInterface();
void process_interface_click(int ifce, int btn, int mbut);
void update_gui_zorder();
void export_gui_controls(int ee);
int convert_gui_disabled_style(int oldStyle);
void unexport_gui_controls(int ee);
void update_gui_disabled_status();

extern int ifacepopped;  // currently displayed pop-up GUI (-1 if none)

extern int mouse_on_iface;   // mouse cursor is over this interface
extern int mouse_on_iface_button;
extern int mouse_pushed_iface;  // this BUTTON on interface MOUSE_ON_IFACE is pushed
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;

extern int eip_guinum, eip_guiobj;




