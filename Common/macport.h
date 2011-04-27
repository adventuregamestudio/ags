/*
 *  macport.h
 *  AGSRunTime
 *
 *  Created by Steve McCrea on 6/16/05.
 *  Part of the Adventure Game Studio source code (c)1999-2005 Chris Jones.
 *
 *  Misc, interface and display code for the macosx port
 *
 */

#ifndef __MACPORT_H__
#define __MACPORT_H__

// replacement for filelength(int handle)
long flength(FILE *);

struct DialogTopic;
void preprocess_dialog_script(DialogTopic *);

#endif  __MACPORT_H__