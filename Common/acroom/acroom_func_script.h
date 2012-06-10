#ifndef __CROOM_FUNC_SCRIPT_H
#define __CROOM_FUNC_SCRIPT_H

extern void load_script_configuration(FILE *);
extern void save_script_configuration(FILE *);
extern void load_graphical_scripts(FILE *, roomstruct *);
extern void save_graphical_scripts(FILE *, roomstruct *);
extern char *passwencstring;;

#endif // __CROOM_FUNC_SCRIPT_H