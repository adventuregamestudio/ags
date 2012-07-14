
#include "ac/global_listbox.h"
#include "ac/common.h"
#include "ac/listbox.h"
#include "ac/string.h"

void ListBoxClear(int guin, int objn) {
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  ListBox_Clear(guisl);
}
void ListBoxAdd(int guin, int objn, const char*newitem) {
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  ListBox_AddItem(guisl, newitem);
}
void ListBoxRemove(int guin, int objn, int itemIndex) {
  GUIListBox*guisl = is_valid_listbox(guin,objn);
  ListBox_RemoveItem(guisl, itemIndex);
}
int ListBoxGetSelected(int guin, int objn) {
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  return ListBox_GetSelectedIndex(guisl);
}
int ListBoxGetNumItems(int guin, int objn) {
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  return ListBox_GetItemCount(guisl);
}
char* ListBoxGetItemText(int guin, int objn, int item, char*buffer) {
  VALIDATE_STRING(buffer);
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  return ListBox_GetItemText(guisl, item, buffer);
}
void ListBoxSetSelected(int guin, int objn, int newsel) {
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  ListBox_SetSelectedIndex(guisl, newsel);
}
void ListBoxSetTopItem (int guin, int objn, int item) {
  GUIListBox*guisl = is_valid_listbox(guin,objn);
  ListBox_SetTopItem(guisl, item);
}

int ListBoxSaveGameList (int guin, int objn) {
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  return ListBox_FillSaveGameList(guisl);
}

void ListBoxDirList (int guin, int objn, const char*filemask) {
  GUIListBox *guisl = is_valid_listbox(guin,objn);
  ListBox_FillDirList(guisl, filemask);
}
