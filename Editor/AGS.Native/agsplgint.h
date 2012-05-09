/* AGS Internal plugin header
   This file extends the main plugin header file (which is
   distributed with the Plugin API) with internal-only features
*/

#ifndef _AGS_PLUGINT_H
#define _AGS_PLUGINT_H

#include "agsplugin.h"

#define PANE_GLOBALMSG  3
#define PANE_CHARACTERS 9

struct AGSDialog {
  char          optionnames[30][150];
  int           optionflags[30];
  unsigned char *optionscripts;
  short         entrypoints[30];
  short         startupentrypoint;
  short         codesize;
  int           numoptions;
  int           topicFlags;
};


struct IAGSEditor2: public IAGSEditor {
  // **** NOTE: THE FOLLOWING EDITOR METHODS ARE FOR INTERNAL
  // **** USE ONLY, DO NOT USE THEM FROM YOUR PLUGIN
  // get the editor version number
  AGSIFUNC(const char*) GetEditorVersion();
  // add a new pane to the editor
  AGSIFUNC(int)  AddEditorPane(const char *progID, const char *paneName, char *icon, void *pData);
  // remove an added pane from the editor - returns 1 on success
  AGSIFUNC(int)  RemoveEditorPane(int handle);
  // get the number of menus
  AGSIFUNC(int)  GetMenuCount();
  // get the name of a menu
  AGSIFUNC(const char*) GetMenuName(int idx);
  // set the name of a menu
  AGSIFUNC(void) SetMenuName(int idx, const char*newname);
  // add new menu option
  AGSIFUNC(int)  AddMenuOption(int menuid, const char *optionname, const char *helpstring);
  // remove menu option
  AGSIFUNC(void) RemoveMenuOption(int option);
  // get number of dialog topics
  AGSIFUNC(int)  GetNumDialogTopics();
  // get a dialog topic
  AGSIFUNC(AGSDialog*) GetDialogTopic(int number);
  // get a dialog topic script
  AGSIFUNC(const char*) GetDialogScript(int number);
  // set a dialog topic script
  AGSIFUNC(void) SetDialogScript(int number, const char *newscript);
  // compile dialog scripts - returns non-zero on error
  AGSIFUNC(int)  CompileDialogs(int *topic, const char **errormsg, int *linenum);
  // write data (while in OnSaveGame for COM plugin)
  AGSIFUNC(void) SaveData(char *buffer, int size);
  // read data (while in OnLoadGame for COM plugin)
  AGSIFUNC(void) LoadData(char *buffer, int size);
  // refresh an editor pane
  AGSIFUNC(void) RefreshPane(int pane);
  // delete all dialog topics
  AGSIFUNC(void) DeleteAllDialogs();
  // create new dialog topic
  AGSIFUNC(int)  CreateNewDialogTopic();
  // if the plugin is not this version, exit
  AGSIFUNC(int)  VerifyPluginVersion(const char *versionRequired);
  // get number of characters
  AGSIFUNC(int)  GetNumCharacters();
  // get character struct
  AGSIFUNC(AGSCharacter*) GetCharacter(int number);
  // call before modifying any data displayed on a pane
  AGSIFUNC(void) PreUpdatePane(int pane);
  // attempt to change script name - 0 on success
  AGSIFUNC(int)  SetCharacterScriptName(int charid, const char *newname);
  // get/set player character
  AGSIFUNC(int)  GetPlayerCharacter();
  AGSIFUNC(void) SetPlayerCharacter(int charid);
  // various game functions
  AGSIFUNC(const char*) GetGameDirectory();
  AGSIFUNC(int)  SaveGame(bool quickSave);
  AGSIFUNC(void) Quit(bool prompt);
  // get/set global messages
  AGSIFUNC(void) SetGlobalMessage(int, const char*);
  AGSIFUNC(const char*) GetGlobalMessage(int);
  // adding custom #defines
  AGSIFUNC(int)  RegisterScriptDefine(const char *name, const char *value);
  AGSIFUNC(void) UnregisterScriptDefine(const char *name);
};

#endif
