
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALLISTBOX_H
#define __AGS_EE_AC__GLOBALLISTBOX_H

void		ListBoxClear(int guin, int objn);
void		ListBoxAdd(int guin, int objn, const char*newitem);
void		ListBoxRemove(int guin, int objn, int itemIndex);
int			ListBoxGetSelected(int guin, int objn);
int			ListBoxGetNumItems(int guin, int objn);
char*		ListBoxGetItemText(int guin, int objn, int item, char*buffer);
void		ListBoxSetSelected(int guin, int objn, int newsel);
void		ListBoxSetTopItem (int guin, int objn, int item);
int			ListBoxSaveGameList (int guin, int objn);
void		ListBoxDirList (int guin, int objn, const char*filemask);

#endif // __AGS_EE_AC__GLOBALLISTBOX_H
