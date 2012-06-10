#ifndef __CROOM_FUNC_INTERACTION_H
#define __CROOM_FUNC_INTERACTION_H

extern int in_interaction_editor;

extern InteractionVariable globalvars[MAX_GLOBAL_VARIABLES];
extern int numGlobalVars;

void serialize_command_list (NewInteractionCommandList *nicl, FILE*ooo);
void serialize_new_interaction (NewInteraction *nint, FILE*ooo);
NewInteractionCommandList *deserialize_command_list (FILE *ooo);

extern NewInteraction *nitemp;
NewInteraction *deserialize_new_interaction (FILE *ooo);

//void NewInteractionCommandList::reset ();

#endif // __CROOM_FUNC_INTERACTION_H