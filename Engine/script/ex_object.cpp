
//=============================================================================
//
// Exporting Object script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_object_script_functions()
{
	scAdd_External_Symbol("Object::Animate^5", (void *)Object_Animate);
	scAdd_External_Symbol("Object::IsCollidingWithObject^1", (void *)Object_IsCollidingWithObject);
	scAdd_External_Symbol("Object::GetName^1", (void *)Object_GetName);
	scAdd_External_Symbol("Object::GetProperty^1", (void *)Object_GetProperty);
	scAdd_External_Symbol("Object::GetPropertyText^2", (void *)Object_GetPropertyText);
	scAdd_External_Symbol("Object::GetTextProperty^1",(void *)Object_GetTextProperty);
	scAdd_External_Symbol("Object::MergeIntoBackground^0", (void *)Object_MergeIntoBackground);
	scAdd_External_Symbol("Object::Move^5", (void *)Object_Move);
	scAdd_External_Symbol("Object::RemoveTint^0", (void *)Object_RemoveTint);
	scAdd_External_Symbol("Object::RunInteraction^1", (void *)Object_RunInteraction);
	scAdd_External_Symbol("Object::SetPosition^2", (void *)Object_SetPosition);
	scAdd_External_Symbol("Object::SetView^3", (void *)Object_SetView);
	scAdd_External_Symbol("Object::StopAnimating^0", (void *)Object_StopAnimating);
	scAdd_External_Symbol("Object::StopMoving^0", (void *)Object_StopMoving);
	scAdd_External_Symbol("Object::Tint^5", (void *)Object_Tint);

	// static
	scAdd_External_Symbol("Object::GetAtScreenXY^2", (void *)GetObjectAtLocation);

	scAdd_External_Symbol("Object::get_Animating", (void *)Object_GetAnimating);
	scAdd_External_Symbol("Object::get_Baseline", (void *)Object_GetBaseline);
	scAdd_External_Symbol("Object::set_Baseline", (void *)Object_SetBaseline);
	scAdd_External_Symbol("Object::get_BlockingHeight",(void *)Object_GetBlockingHeight);
	scAdd_External_Symbol("Object::set_BlockingHeight",(void *)Object_SetBlockingHeight);
	scAdd_External_Symbol("Object::get_BlockingWidth",(void *)Object_GetBlockingWidth);
	scAdd_External_Symbol("Object::set_BlockingWidth",(void *)Object_SetBlockingWidth);
	scAdd_External_Symbol("Object::get_Clickable", (void *)Object_GetClickable);
	scAdd_External_Symbol("Object::set_Clickable", (void *)Object_SetClickable);
	scAdd_External_Symbol("Object::get_Frame", (void *)Object_GetFrame);
	scAdd_External_Symbol("Object::get_Graphic", (void *)Object_GetGraphic);
	scAdd_External_Symbol("Object::set_Graphic", (void *)Object_SetGraphic);
	scAdd_External_Symbol("Object::get_ID", (void *)Object_GetID);
	scAdd_External_Symbol("Object::get_IgnoreScaling", (void *)Object_GetIgnoreScaling);
	scAdd_External_Symbol("Object::set_IgnoreScaling", (void *)Object_SetIgnoreScaling);
	scAdd_External_Symbol("Object::get_IgnoreWalkbehinds", (void *)Object_GetIgnoreWalkbehinds);
	scAdd_External_Symbol("Object::set_IgnoreWalkbehinds", (void *)Object_SetIgnoreWalkbehinds);
	scAdd_External_Symbol("Object::get_Loop", (void *)Object_GetLoop);
	scAdd_External_Symbol("Object::get_Moving", (void *)Object_GetMoving);
	scAdd_External_Symbol("Object::get_Name", (void *)Object_GetName_New);
	scAdd_External_Symbol("Object::get_Solid", (void *)Object_GetSolid);
	scAdd_External_Symbol("Object::set_Solid", (void *)Object_SetSolid);
	scAdd_External_Symbol("Object::get_Transparency", (void *)Object_GetTransparency);
	scAdd_External_Symbol("Object::set_Transparency", (void *)Object_SetTransparency);
	scAdd_External_Symbol("Object::get_View", (void *)Object_GetView);
	scAdd_External_Symbol("Object::get_Visible", (void *)Object_GetVisible);
	scAdd_External_Symbol("Object::set_Visible", (void *)Object_SetVisible);
	scAdd_External_Symbol("Object::get_X", (void *)Object_GetX);
	scAdd_External_Symbol("Object::set_X", (void *)Object_SetX);
	scAdd_External_Symbol("Object::get_Y", (void *)Object_GetY);
	scAdd_External_Symbol("Object::set_Y", (void *)Object_SetY);
}
