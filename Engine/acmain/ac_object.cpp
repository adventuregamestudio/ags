
#include "acmain/ac_maindefines.h"


void GetObjectName(int obj, char *buffer) {
  VALIDATE_STRING(buffer);
  if (!is_valid_object(obj))
    quit("!GetObjectName: invalid object number");

  strcpy(buffer, get_translation(thisroom.objectnames[obj]));
}

void Object_GetName(ScriptObject *objj, char *buffer) {
  GetObjectName(objj->id, buffer);
}

const char* Object_GetName_New(ScriptObject *objj) {
  if (!is_valid_object(objj->id))
    quit("!Object.Name: invalid object number");

  return CreateNewScriptString(get_translation(thisroom.objectnames[objj->id]));
}


void MoveObject(int objj,int xx,int yy,int spp) {
  move_object(objj,xx,yy,spp,0);
  }
void MoveObjectDirect(int objj,int xx,int yy,int spp) {
  move_object(objj,xx,yy,spp,1);
  }

void Object_Move(ScriptObject *objj, int x, int y, int speed, int blocking, int direct) {
  if ((direct == ANYWHERE) || (direct == 1))
    direct = 1;
  else if ((direct == WALKABLE_AREAS) || (direct == 0))
    direct = 0;
  else
    quit("Object.Move: invalid DIRECT parameter");

  move_object(objj->id, x, y, speed, direct);

  if ((blocking == BLOCKING) || (blocking == 1))
    do_main_cycle(UNTIL_SHORTIS0,(int)&objj->obj->moving);
  else if ((blocking != IN_BACKGROUND) && (blocking != 0))
    quit("Object.Move: invalid BLOCKING paramter");
}


void SetObjectClickable (int cha, int clik) {
  if (!is_valid_object(cha))
    quit("!SetObjectClickable: Invalid object specified");
  objs[cha].flags&=~OBJF_NOINTERACT;
  if (clik == 0)
    objs[cha].flags|=OBJF_NOINTERACT;
  }

void Object_SetClickable(ScriptObject *objj, int clik) {
  SetObjectClickable(objj->id, clik);
}

int Object_GetClickable(ScriptObject *objj) {
  if (!is_valid_object(objj->id))
    quit("!Object.Clickable: Invalid object specified");

  if (objj->obj->flags & OBJF_NOINTERACT)
    return 0;
  return 1;
}

void Object_SetIgnoreScaling(ScriptObject *objj, int newval) {
  if (!is_valid_object(objj->id))
    quit("!Object.IgnoreScaling: Invalid object specified");

  objj->obj->flags &= ~OBJF_USEROOMSCALING;
  if (!newval)
    objj->obj->flags |= OBJF_USEROOMSCALING;

  // clear the cache
  objcache[objj->id].ywas = -9999;
}

int Object_GetIgnoreScaling(ScriptObject *objj) {
  if (!is_valid_object(objj->id))
    quit("!Object.IgnoreScaling: Invalid object specified");

  if (objj->obj->flags & OBJF_USEROOMSCALING)
    return 0;
  return 1;
}

void Object_SetSolid(ScriptObject *objj, int solid) {
  objj->obj->flags &= ~OBJF_SOLID;
  if (solid)
    objj->obj->flags |= OBJF_SOLID;
}

int Object_GetSolid(ScriptObject *objj) {
  if (objj->obj->flags & OBJF_SOLID)
    return 1;
  return 0;
}

void Object_SetBlockingWidth(ScriptObject *objj, int bwid) {
  objj->obj->blocking_width = bwid;
}

int Object_GetBlockingWidth(ScriptObject *objj) {
  return objj->obj->blocking_width;
}

void Object_SetBlockingHeight(ScriptObject *objj, int bhit) {
  objj->obj->blocking_height = bhit;
}

int Object_GetBlockingHeight(ScriptObject *objj) {
  return objj->obj->blocking_height;
}

void SetObjectIgnoreWalkbehinds (int cha, int clik) {
  if (!is_valid_object(cha))
    quit("!SetObjectIgnoreWalkbehinds: Invalid object specified");
  objs[cha].flags&=~OBJF_NOWALKBEHINDS;
  if (clik)
    objs[cha].flags|=OBJF_NOWALKBEHINDS;
  // clear the cache
  objcache[cha].ywas = -9999;
}

int Object_GetID(ScriptObject *objj) {
  return objj->id;
}

void Object_SetIgnoreWalkbehinds(ScriptObject *chaa, int clik) {
  SetObjectIgnoreWalkbehinds(chaa->id, clik);
}

int Object_GetIgnoreWalkbehinds(ScriptObject *chaa) {
  if (!is_valid_object(chaa->id))
    quit("!Object.IgnoreWalkbehinds: Invalid object specified");

  if (chaa->obj->flags & OBJF_NOWALKBEHINDS)
    return 1;
  return 0;
}
