#ifndef __CROOM_FUNC_GAMESTRUCT_H
#define __CROOM_FUNC_GAMESTRUCT_H

extern void add_to_eventblock(EventBlock *evpt, int evnt, int whatac, int val1, int data, short scorr);
extern int usesmisccond;

#define COPY_CHAR_VAR(name) ci->name = oci->name
void ConvertOldCharacterToNew (OldCharacterInfo *oci, CharacterInfo *ci);
void ConvertOldGameStruct (OldGameSetupStruct *ogss, GameSetupStruct *gss);
void Convert272ViewsToNew (int numof, ViewStruct272 *oldv, ViewStruct *newv);

#endif // __CROOM_FUNC_GAMESTRUCT_H