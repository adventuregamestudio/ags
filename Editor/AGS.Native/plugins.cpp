
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include "agsplgint.h"
#include "fmem.h"
#include "cscomp.h"

extern char editorVersionNumber[50];

void warningBox (const char*fmt, ...) {
  char displbuf[1000];
  va_list ap;
  va_start(ap,fmt);
  vsprintf(displbuf,fmt,ap);
  va_end(ap);
  MessageBox(NULL, displbuf, "AGS Editor Warning", MB_OK | MB_ICONEXCLAMATION);
}

struct AGSPlugin {
  char        filename[50];
  char        description[300];
  HINSTANCE   dllHandle;
  bool        loaded;   // AGSInitialize has been called
  LPCSTR    (*f_getName)();
  int       (*f_editorStartup) (IAGSEditor *);
  void      (*f_editorShutdown) ();
  void      (*f_properties) (HWND);
  int       (*f_saveGame) (char *, int);
  void      (*f_loadGame) (char *, int);
  //IAGSEditor2 eiface;
  IAGSEditor eiface;
  bool        isPlainDLL;
  bool        isCOM;
  //ITheAGSEditor *comeiface;
  //IAGSPluginPtr  smartPtr;
  char        requiredAGSVersion[20];

  const char * GetPluginName();
  bool VerifyRequiredVersion();
  int  EditorStartup();
  void EditorShutdown();
  bool HasProperties();
  void PluginProperties(HWND);
  int  OnSaveGame(char *buffer, int size);
  void OnSaveEditor();
  void OnLoadGame(char *buffer, int size);
  void OnLoadEditor();
  void OnCommandEvent(int id);
 
  AGSPlugin() {
    filename[0] = 0;
    loaded = false;
    description[0] = 0;
    dllHandle = 0;
    isPlainDLL = true;
    isCOM = false;
    //smartPtr = NULL;
  }
};

const char *AGSPlugin::GetPluginName() {
  const char *retv = f_getName();
  if (retv == NULL)
    strcpy(description, "[no description]");
  else
    strncpy(description, retv, 300);

  description[299] = 0;
  return &description[0];
}

bool AGSPlugin::VerifyRequiredVersion() {
  requiredAGSVersion[0] = 0;
  return true;
}

int AGSPlugin::EditorStartup() {
  int retval = 0;
  if (isPlainDLL)
    retval += f_editorStartup(&eiface);
  return retval;
}

void AGSPlugin::EditorShutdown() {
  if (isPlainDLL)
    f_editorShutdown();
}

bool AGSPlugin::HasProperties() {

  if (isPlainDLL) 
    return (f_properties != NULL);
    
  // this case should never happen
  return false;
}

void AGSPlugin::PluginProperties(HWND parent) {
  if ((isPlainDLL) && (f_properties))
    f_properties(parent);
}

void AGSPlugin::OnLoadGame(char *buffer, int size) {
  if ((isPlainDLL) && (f_loadGame))
    f_loadGame(buffer, size);
}

int AGSPlugin::OnSaveGame(char *buffer, int size) {
  int retval = 0;

  if ((isPlainDLL) && (f_saveGame))
    retval = f_saveGame(buffer, size);

  return retval;
}

void AGSPlugin::OnCommandEvent(int id) {
}

void AGSPlugin::OnSaveEditor() {
}
void AGSPlugin::OnLoadEditor() {
}

struct RegisteredHeader {
  const char * text;
  int  addedBy;
};

#define MAXREGHEADERS 30
RegisteredHeader regHeaders[MAXREGHEADERS];
int numRegHeaders = 0;
char * pluginheaders = NULL;


#define MAXPLUGINS 40
AGSPlugin plugins[MAXPLUGINS];
int numPlugins = 0;

int find_plugin_by_pluginid(int pid) {
  int pl;
  for (pl = 0; pl < numPlugins; pl++) {
    if (plugins[pl].eiface.pluginId == pid)
      return pl;
  }
  return -1;
}

int find_plugin_by_name(const char *name) {
  int pl;
  for (pl = 0; pl < numPlugins; pl++) {
    if (stricmp (plugins[pl].filename, name) == 0) {
      return pl;
    }
  }
  return -1;
}


struct PluginExtraDefine {
  char name[32];
  char value[50];
  int  pluginId;
};
PluginExtraDefine *pluginDefines = NULL;
int pluginDefineMemorySpace = 0;
int numPluginDefines = 0;

void AppendPluginDefines(FMEM *toText) {
  char tempbfr[120];
  for (int i = 0; i < numPluginDefines; i++) {
    sprintf(tempbfr, "#define %s %s", pluginDefines[i].name, pluginDefines[i].value);
    fmem_puts(tempbfr, toText);
  }
}

void AddPluginDefine(const char *name, const char *value, int plugId) {
  // increase memory buffer if necessary
  if (numPluginDefines >= pluginDefineMemorySpace) {
    pluginDefineMemorySpace += 10;
    pluginDefines = (PluginExtraDefine*)realloc(pluginDefines, sizeof(PluginExtraDefine) * pluginDefineMemorySpace);
  }
  strncpy(pluginDefines[numPluginDefines].name, name, 32);
  strncpy(pluginDefines[numPluginDefines].value, value, 50);
  pluginDefines[numPluginDefines].name[31] = 0;
  pluginDefines[numPluginDefines].value[49] = 0;
  pluginDefines[numPluginDefines].pluginId = plugId;
  numPluginDefines++;
}

int FindPluginDefine(const char *name) {
  for (int a = 0; a < numPluginDefines; a++) {
    if (strcmp(name, pluginDefines[a].name) == 0) {
      return a;
    }
  }
  return -1;
}

const char *GetPluginDefineValue(const char *name) {
  int idx = FindPluginDefine(name);
  if (idx < 0)
    return NULL;
  return pluginDefines[idx].value;
}

void RemovePluginDefine(const char *name, int forPlugin) {
  int a = FindPluginDefine(name);
  if ((a >= 0) && (pluginDefines[a].pluginId == forPlugin)) {
    // found it -- remove the entry
    numPluginDefines--;
    for (int b = a; b < numPluginDefines; b++)
      pluginDefines[b] = pluginDefines[b + 1];
  }
  else
    warningBox("Attempted to remove custom plugin define '%s' that did not exist", name);
}



// ** MISC PLUGIN FUNCTIONS ** //

char *generate_plugin_headers() {
  int a, totalsize = 0;
  for (a = 0; a < numRegHeaders; a++) {
    totalsize += strlen (regHeaders[a].text) + 5;
  }
  if (pluginheaders != NULL)
    free (pluginheaders);
  pluginheaders = NULL;

  if (totalsize == 0)
    return NULL;

  pluginheaders = (char*)malloc (totalsize);
  pluginheaders[0] = 0;
  for (a = 0; a < numRegHeaders; a++) {
    strcat (pluginheaders, regHeaders[a].text);
    strcat (pluginheaders, "\r\n\r\n");
  }
  return pluginheaders;
}

void merge_plugin_headers() {

  generate_plugin_headers();

  if (pluginheaders != NULL)
    ccAddDefaultHeader (pluginheaders, "Plugin definitions");
}


// *** IMPLEMENTATION OF IAGSEDITOR

HWND IAGSEditor::GetEditorHandle () {
	// TODO: Implement GetEditorHandle
  return NULL;
}
HWND IAGSEditor::GetWindowHandle () {
  return ::GetActiveWindow();
}
void IAGSEditor::RegisterScriptHeader (const char *header) {
  if (numRegHeaders >= MAXREGHEADERS) {
    warningBox("Too many script headers registered by plugins. Plugins should only need one registration each.");
    return;
  }

  regHeaders[numRegHeaders].text = header;
  regHeaders[numRegHeaders].addedBy = pluginId;
  numRegHeaders ++;
  // make it re-compile
  // TODO: force recompile script
  //globalScriptModified = true;
}

void IAGSEditor::UnregisterScriptHeader (const char *header) {
  int i;
  for (i = 0; i < numRegHeaders; i++) {
    if ((regHeaders[i].text == header) &&
        (regHeaders[i].addedBy == pluginId)) {
      numRegHeaders--;
      for (int k = i; k < numRegHeaders; k++)
        regHeaders[k] = regHeaders[k+1];
      i--;
    }
  }
  // make it re-compile
// TODO: force recompile
  //globalScriptModified = true;
}
/*
int IAGSEditor2::RegisterScriptDefine(const char *name, const char *value) {
  if (!IsScriptNameUnique(name, NULL))
    return -1;
  AddPluginDefine(name, value, pluginId);
  return 0;
}
void IAGSEditor2::UnregisterScriptDefine(const char *name) {
  RemovePluginDefine(name, pluginId);
}*/

// **** END IAGSEDITOR


void unload_plugin (int a) {
  plugins[a].loaded = false;
  plugins[a].EditorShutdown();
  // auto remove any headers that it didn't
  int i;
  for (i = 0; i < numRegHeaders; i++) {
    if (regHeaders[i].addedBy == plugins[a].eiface.pluginId) {
      numRegHeaders--;
      for (int k = i; k < numRegHeaders; k++)
        regHeaders[k] = regHeaders[k+1];
      i--;
    }
  }
/*  // remove any panes it added
  for (i = 0; i < numExtraPanes; i++) {
    if (extraPane[i].addedByPlugin == plugins[a].eiface.pluginId) {
      remove_extra_pane(i);
      i--;
    }
  }
  // remove any #defines it added
  for (i = 0; i < numPluginDefines; i++) {
    if (pluginDefines[i].pluginId == plugins[a].eiface.pluginId) {
      numPluginDefines--;
      for (int k = i; k < numPluginDefines; k++)
        pluginDefines[k] = pluginDefines[k + 1];
      i--;
    }
  }
  removePluginCmds(plugins[a].eiface.pluginId);*/
}

void reset_plugins() {
  for (int a = 0; a < numPlugins; a++) {
    if (plugins[a].loaded) 
      unload_plugin(a);
  }
  numRegHeaders = 0;
}

int get_num_plugins_in_use () {
  int total = 0;
  for (int a = 0; a < numPlugins; a++) {
    if (plugins[a].loaded) 
      total++;
  }
  return total;
}
/*
void copy_plugins_across(const char *destdir) {
  wxString source, dest;
  int i;

  for (i = 0; i < numPlugins; i++) {
    dest.Printf("%s\\%s", destdir, plugins[i].filename);
    if ((!plugins[i].loaded) || (!plugins[i].isPlainDLL)) {
      // not using anymore (or an editor-only plugin), remove it
      unlink (dest);
      continue;
    }
    source.Printf("%s\\%s", editorExeDir, plugins[i].filename);
    CopyFile (source, dest, FALSE);
  }
}

void remove_plugins_from_curdir () {
  int i;
  for (i = 0; i < numPlugins; i++)
    unlink (plugins[i].filename);
}*/

int count_plugins_in_use() {
  int numusing = 0;
  for (int a = 0; a < numPlugins; a++) {
    if (plugins[a].loaded)
      numusing++;
  }
  return numusing;
}

#define AFTER_PLUGIN_MAGIC 146937535
int currentSaveLoadingPlugin = -1;
FILE *currentlyWritingPluginFile = NULL;
FILE *currentlyReadingPluginFile = NULL;

/*
void IAGSEditor2::SaveData(char *buffer, int size) {
  if ((!currentlyWritingPluginFile) || (currentSaveLoadingPlugin != pluginId)) {
    warningBox("The plugin '%s' attempted to save data while not in OnSave", plugins[find_plugin_by_pluginid(pluginId)].filename);
    return;
  }
  fwrite(buffer, size, 1, currentlyWritingPluginFile);
}

void IAGSEditor2::LoadData(char *buffer, int size) {
  if ((!currentlyReadingPluginFile) || (currentSaveLoadingPlugin != pluginId)) {
    warningBox("The plugin '%s' attempted to load data while not in OnLoad", plugins[find_plugin_by_pluginid(pluginId)].filename);
    return;
  }
  fread(buffer, size, 1, currentlyReadingPluginFile);
}
*/
// write editor-only info to the editor.dat file
void write_plugin_editor_state(FILE *ooo, const char *reopenfilename) {
  // version of plugin saving format
  putw (1, ooo);
  int a, numusing = count_plugins_in_use();
  putw (numusing, ooo);
  // call SaveGame on all the plugins
  for (a = 0; a < numPlugins; a++) {
    if (plugins[a].loaded) {
      fputstring(plugins[a].filename, ooo);
      // dummy size, will overwrite in a bit
      putw(0, ooo);
      long stoffs = ftell(ooo);

      currentSaveLoadingPlugin = plugins[a].eiface.pluginId;
      currentlyWritingPluginFile = ooo;

      plugins[a].OnSaveEditor();

      currentSaveLoadingPlugin = -1;
      currentlyWritingPluginFile = NULL;

      long datasize = ftell(ooo) - stoffs;
      
      if (datasize > 0) {
        fclose(ooo);
        // re-write the proper byte size
        ooo = fopen(reopenfilename, "r+b");
        fseek(ooo, stoffs - 4, SEEK_SET);
        putw(datasize, ooo);
        fclose(ooo);
        ooo = fopen(reopenfilename, "ab");
      }
      putw(AFTER_PLUGIN_MAGIC, ooo);
    }
  }

}

void read_plugin_editor_state(FILE *iii) {
  if (iii == NULL) {
    // an old game has been loaded with no plugin info
    // don't do anything, it will only confuse matters
    return;
  }
  if (getw(iii) != 1) {
    warningBox("Invalid file: editor plugin state is invalid");
	exit(1);
  }
  int numusing = getw(iii);
  char fbuffer[200];
  for (int a = 0; a < numusing; a++) {
    fgetstring(fbuffer, iii);
    int datasize = getw(iii);
    long seekTo = ftell(iii) + datasize;
    int pl = find_plugin_by_name(fbuffer);
    if (pl >= 0) {
      // the plugin is loaded
      currentSaveLoadingPlugin = plugins[pl].eiface.pluginId;
      currentlyReadingPluginFile = iii;

      plugins[pl].OnLoadEditor();

      currentSaveLoadingPlugin = -1;
      currentlyReadingPluginFile = NULL;

      // check if it read back the correct amount
      if (ftell(iii) != seekTo) {
        warningBox("The plugin '%s' did not read its data back correctly. Contact the plugin author to resolve the problem.", fbuffer);
        fseek(iii, seekTo, SEEK_SET);
      }
    }
    else if (datasize > 0) {
      // it isn't loaded
      warningBox("The plugin '%s' was not found, but was in use by this game. If you save the game now, any plugin-specific settings will be lost.", fbuffer);
      fseek(iii, datasize, SEEK_CUR);
    }
    if (getw(iii) != AFTER_PLUGIN_MAGIC)
      warningBox("Some plugin data for '%s' was not read/written correctly to the file. Your game may be corrupt; proceed with caution.", fbuffer);
  }
}

#define SAVEBUFFERSIZE 5120

void write_plugins_to_disk (FILE *ooo) {
  int a, numusing = 0;
  // version of plugin saving format
  putw (1, ooo);
  numusing = count_plugins_in_use();
  putw (numusing, ooo);
  char savebuffer[SAVEBUFFERSIZE + 200];
  // call SaveGame on all the plugins
  for (a = 0; a < numPlugins; a++) {
    if (plugins[a].loaded) {
      int savesize = 0;
      char nameToWrite[200];
      strcpy(nameToWrite, plugins[a].filename);
      // if it's a COM-only DLL, dont include it in the game
      if (!plugins[a].isPlainDLL)
        strcat(nameToWrite, "!");

      fputstring (nameToWrite, ooo);
      savesize = plugins[a].OnSaveGame(savebuffer, SAVEBUFFERSIZE);
      
      if ((savesize > SAVEBUFFERSIZE) || (savesize < 0)) {
        warningBox("Plugin tried to write too much data to game file.");
        savesize = 0;
      }
      putw (savesize, ooo);
      if (savesize > 0)
        fwrite (&savebuffer[0], savesize, 1, ooo);
    }
  }
}
/*
void read_plugins_from_disk (FILE *iii) {
  if (getw(iii) != 1) {
    warningBox("ERROR: unable to load game, invalid version of plugin data");
	exit(1);
  }

  int numusing = getw(iii), a;
  char buffer[SAVEBUFFERSIZE + 200];

  for (a = 0; a < numusing; a++) {
    // read the plugin name
    fgetstring (buffer, iii);
    int datasize = getw(iii);
    if (datasize > SAVEBUFFERSIZE) {
      warningBox("Invalid plugin save data format, plugin data is lost");
      fseek (iii, datasize, SEEK_CUR);
      continue;
    }
    // if it's an editor-only plugin, we don't care (info for the engine)
    if (buffer[strlen(buffer) - 1] == '!')
      buffer[strlen(buffer) - 1] = 0;

    // find the corresponding plugin in our current list
    int pl = find_plugin_by_name(buffer);
    // not there
    if (pl < 0) {
      warningBox ("The plugin '%s' was not found, and this game relies on it. You can continue, but any plugin-specific settings will be lost if you save the game.", buffer);
      fseek (iii, datasize, SEEK_CUR);
      continue;
    }

    if (datasize > 0)
      fread (buffer, datasize, 1, iii);
    
    if (!plugins[pl].EditorStartup()) {
      plugins[pl].loaded = true;
      if (datasize > 0)
        plugins[pl].OnLoadGame(buffer, datasize);
    }
    else
      warningBox ("There was an error attempting to start the plugin '%s'.", plugins[pl].filename);
  }

}
*/
void free_plugin_list () {
  for (int a = 0; a < numPlugins; a++) {
    // release the COM interface if necessary
	  /*
    if (plugins[a].isCOM) {
      plugins[a].smartPtr = NULL;
      // release the COM interface
      if (plugins[a].comeiface)
        plugins[a].comeiface->Release();
      plugins[a].comeiface = NULL;
    }*/
    // unload the DLL
    FreeLibrary (plugins[a].dllHandle);
  }
  numPlugins = 0;
  numRegHeaders = 0;
//  FreeCOMSupportDLL();
}

void load_plugin_list () {

/*  if (!LoadCOMSupportDLL())
    return;
*/
  struct _finddata_t c_file;
  long hFile;
  // on the first pass, get DLLs; on the second, OCXs
  bool firstPass = true;
  bool firstLoop = true;

  // we're in the editor EXE dir at the moment

  hFile = _findfirst ("ags*.dll", &c_file);
  if (hFile == -1) {
    firstPass = false;
    hFile = _findfirst ("ags*.ocx", &c_file);
    if (hFile == -1) {
      return;
    }
  }

  do {
    if (firstLoop) firstLoop = false;
    else if (_findnext(hFile, &c_file)) {
      if (firstPass) {
        _findclose(hFile);
        hFile = _findfirst ("ags*.ocx", &c_file);
        if (hFile == -1)
          break;
        firstPass = false;
      }
      else
        break;
    }

    AGSPlugin *apl = &plugins[numPlugins];
    apl->isCOM = false;
    apl->isPlainDLL = false;
//    apl->smartPtr = NULL;
    strcpy(apl->filename, c_file.name);
    apl->loaded = false;
//    apl->comeiface = NULL;
    apl->dllHandle = LoadLibrary (c_file.name);
    if (apl->dllHandle == NULL) {
      warningBox("Unable to load plugin '%s'", c_file.name);
      continue;
    }

    if (GetProcAddress(apl->dllHandle, "AGS_PluginV2") != NULL)
      apl->isPlainDLL = true;
/*
    RegServerProc regServer;
    regServer = (RegServerProc)GetProcAddress(apl->dllHandle, "DllRegisterServer");

    // If there is a DLLRegisterServer, it might be a COM plugin
    // .net COM-interop DLLs don't export this though, so
    // if it's not there we can't rule out it being a plugin
    if ((!debugPlugins) && (regServer))
      regServer();

    // If it's a COM DLL, or it's not a plain DLL (might be .net),
    // then try the COM way
    if ((regServer) || (apl->isPlainDLL == false)) {
      // concatenate together the main class ProgID
      char pluginProgid[100];
      strcpy(pluginProgid, c_file.name);
      // strip the extension
      strchr(pluginProgid, '.')[0] = 0;
      // create the progid as "Filename.PluginMain"
      strcat(pluginProgid, ".PluginMain");

      IUnknown *thePlugin = CreateInstanceFromProgID(pluginProgid);
      if ((thePlugin == NULL) && (apl->isPlainDLL == false)) {
        warningBox("Unable to create instance for plugin: '%s'.\n\nEnsure that the plugin filename has the correct case and that it really is an AGS plugin.", pluginProgid);
        continue;
      }

      if (thePlugin) {
        // it's a COM plugin!
        apl->smartPtr = thePlugin;
        thePlugin->Release();

        if (apl->smartPtr == NULL) {
          warningBox("Unable to load the plugin '%s'. It is implemented as a COM plugin, however the main class does not implement the IAGSPlugin interface.", c_file.name);
          continue;
        }

        apl->isCOM = true;
      }
    }*/

    if ((!apl->isCOM) && (!apl->isPlainDLL)) {
      warningBox("A plugin '%s' was found in the editor folder, but it did not support the required interfaces.", c_file.name);
      FreeLibrary (apl->dllHandle);
      continue;
    }

    apl->f_getName = (LPCSTR(*)())GetProcAddress (apl->dllHandle, "AGS_GetPluginName");
    apl->f_editorStartup = (int(*)(IAGSEditor*))GetProcAddress (apl->dllHandle, "AGS_EditorStartup");
    apl->f_editorShutdown = (void(*)())GetProcAddress (apl->dllHandle, "AGS_EditorShutdown");
    apl->f_properties = (void(*)(HWND))GetProcAddress (apl->dllHandle, "AGS_EditorProperties");
    apl->f_saveGame = (int(*)(char*,int))GetProcAddress (apl->dllHandle, "AGS_EditorSaveGame");
    apl->f_loadGame = (void(*)(char*,int))GetProcAddress (apl->dllHandle, "AGS_EditorLoadGame");
    int failerr = 0;
    if (apl->f_getName == NULL) failerr = 1;
    else if (apl->f_editorStartup == NULL) failerr = 2;
    else if (apl->f_editorShutdown == NULL) failerr = 3;

    if ((failerr) && (apl->isPlainDLL)) {
      warningBox ("File '%s' is not a valid AGS plugin (Error PST%02d)", c_file.name, failerr);
      FreeLibrary (apl->dllHandle);
      continue;
    }
    apl->eiface.pluginId = numPlugins+10;
    apl->eiface.version = 2;
    apl->GetPluginName();
    if (!apl->VerifyRequiredVersion()) {
//      apl->smartPtr = NULL;
      warningBox("Cannot load the plugin '%s', because it requires a newer version of AGS.", c_file.name);
      FreeLibrary(apl->dllHandle);
      continue;
    }

/*    if (apl->isCOM)
      apl->comeiface = pCreateCOMIFace(&apl->eiface);
*/
    numPlugins++;
    if (numPlugins >= MAXPLUGINS) {
      warningBox("There are too many plugins in the editor directory. Remove some that you are not using.");
      break;
    }

  } while (true);

  _findclose (hFile);
}

