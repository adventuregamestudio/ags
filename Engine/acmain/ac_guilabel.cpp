
#include "acmin/ac_maindefines.h"

// ** LABEL FUNCTIONS

const char* Label_GetText_New(GUILabel *labl) {
  return CreateNewScriptString(labl->GetText());
}

void Label_GetText(GUILabel *labl, char *buffer) {
  strcpy(buffer, labl->GetText());
}

void Label_SetText(GUILabel *labl, const char *newtx) {
  newtx = get_translation(newtx);

  if (strcmp(labl->GetText(), newtx)) {
    guis_need_update = 1;
    labl->SetText(newtx);
  }
}

int Label_GetColor(GUILabel *labl) {
  return labl->textcol;
}

void Label_SetColor(GUILabel *labl, int colr) {
  if (labl->textcol != colr) {
    labl->textcol = colr;
    guis_need_update = 1;
  }
}

int Label_GetFont(GUILabel *labl) {
  return labl->font;
}

void Label_SetFont(GUILabel *guil, int fontnum) {
  if ((fontnum < 0) || (fontnum >= game.numfonts))
    quit("!SetLabelFont: invalid font number.");

  if (fontnum != guil->font) {
    guil->font = fontnum;
    guis_need_update = 1;
  }
}


void SetLabelColor(int guin,int objn, int colr) {
  if ((guin<0) | (guin>=game.numgui))
    quit("!SetLabelColor: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs))
    quit("!SetLabelColor: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_LABEL)
    quit("!SetLabelColor: specified control is not a label");

  GUILabel*guil=(GUILabel*)guis[guin].objs[objn];
  Label_SetColor(guil, colr);
}

void SetLabelText(int guin,int objn,char*newtx) {
  VALIDATE_STRING(newtx);
  if ((guin<0) | (guin>=game.numgui)) quit("!SetLabelText: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetLabelTexT: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_LABEL)
    quit("!SetLabelText: specified control is not a label");

  GUILabel*guil=(GUILabel*)guis[guin].objs[objn];
  Label_SetText(guil, newtx);
}

void SetLabelFont(int guin,int objn, int fontnum) {

  if ((guin<0) | (guin>=game.numgui)) quit("!SetLabelFont: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetLabelFont: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_LABEL)
    quit("!SetLabelFont: specified control is not a label");

  GUILabel*guil=(GUILabel*)guis[guin].objs[objn];
  Label_SetFont(guil, fontnum);
}

