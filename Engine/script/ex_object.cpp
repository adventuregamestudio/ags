
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
	ccAddExternalObjectFunction("Object::Animate^5", (void *)Object_Animate);
	ccAddExternalObjectFunction("Object::IsCollidingWithObject^1", (void *)Object_IsCollidingWithObject);
	ccAddExternalObjectFunction("Object::GetName^1", (void *)Object_GetName);
	ccAddExternalObjectFunction("Object::GetProperty^1", (void *)Object_GetProperty);
	ccAddExternalObjectFunction("Object::GetPropertyText^2", (void *)Object_GetPropertyText);
	ccAddExternalObjectFunction("Object::GetTextProperty^1",(void *)Object_GetTextProperty);
	ccAddExternalObjectFunction("Object::MergeIntoBackground^0", (void *)Object_MergeIntoBackground);
	ccAddExternalObjectFunction("Object::Move^5", (void *)Object_Move);
	ccAddExternalObjectFunction("Object::RemoveTint^0", (void *)Object_RemoveTint);
	ccAddExternalObjectFunction("Object::RunInteraction^1", (void *)Object_RunInteraction);
	ccAddExternalObjectFunction("Object::SetPosition^2", (void *)Object_SetPosition);
	ccAddExternalObjectFunction("Object::SetView^3", (void *)Object_SetView);
	ccAddExternalObjectFunction("Object::StopAnimating^0", (void *)Object_StopAnimating);
	ccAddExternalObjectFunction("Object::StopMoving^0", (void *)Object_StopMoving);
	ccAddExternalObjectFunction("Object::Tint^5", (void *)Object_Tint);

	// static
	ccAddExternalStaticFunction("Object::GetAtScreenXY^2", (void *)GetObjectAtLocation);

	ccAddExternalObjectFunction("Object::get_Animating", (void *)Object_GetAnimating);
	ccAddExternalObjectFunction("Object::get_Baseline", (void *)Object_GetBaseline);
	ccAddExternalObjectFunction("Object::set_Baseline", (void *)Object_SetBaseline);
	ccAddExternalObjectFunction("Object::get_BlockingHeight",(void *)Object_GetBlockingHeight);
	ccAddExternalObjectFunction("Object::set_BlockingHeight",(void *)Object_SetBlockingHeight);
	ccAddExternalObjectFunction("Object::get_BlockingWidth",(void *)Object_GetBlockingWidth);
	ccAddExternalObjectFunction("Object::set_BlockingWidth",(void *)Object_SetBlockingWidth);
	ccAddExternalObjectFunction("Object::get_Clickable", (void *)Object_GetClickable);
	ccAddExternalObjectFunction("Object::set_Clickable", (void *)Object_SetClickable);
	ccAddExternalObjectFunction("Object::get_Frame", (void *)Object_GetFrame);
	ccAddExternalObjectFunction("Object::get_Graphic", (void *)Object_GetGraphic);
	ccAddExternalObjectFunction("Object::set_Graphic", (void *)Object_SetGraphic);
	ccAddExternalObjectFunction("Object::get_ID", (void *)Object_GetID);
	ccAddExternalObjectFunction("Object::get_IgnoreScaling", (void *)Object_GetIgnoreScaling);
	ccAddExternalObjectFunction("Object::set_IgnoreScaling", (void *)Object_SetIgnoreScaling);
	ccAddExternalObjectFunction("Object::get_IgnoreWalkbehinds", (void *)Object_GetIgnoreWalkbehinds);
	ccAddExternalObjectFunction("Object::set_IgnoreWalkbehinds", (void *)Object_SetIgnoreWalkbehinds);
	ccAddExternalObjectFunction("Object::get_Loop", (void *)Object_GetLoop);
	ccAddExternalObjectFunction("Object::get_Moving", (void *)Object_GetMoving);
	ccAddExternalObjectFunction("Object::get_Name", (void *)Object_GetName_New);
	ccAddExternalObjectFunction("Object::get_Solid", (void *)Object_GetSolid);
	ccAddExternalObjectFunction("Object::set_Solid", (void *)Object_SetSolid);
	ccAddExternalObjectFunction("Object::get_Transparency", (void *)Object_GetTransparency);
	ccAddExternalObjectFunction("Object::set_Transparency", (void *)Object_SetTransparency);
	ccAddExternalObjectFunction("Object::get_View", (void *)Object_GetView);
	ccAddExternalObjectFunction("Object::get_Visible", (void *)Object_GetVisible);
	ccAddExternalObjectFunction("Object::set_Visible", (void *)Object_SetVisible);
	ccAddExternalObjectFunction("Object::get_X", (void *)Object_GetX);
	ccAddExternalObjectFunction("Object::set_X", (void *)Object_SetX);
	ccAddExternalObjectFunction("Object::get_Y", (void *)Object_GetY);
	ccAddExternalObjectFunction("Object::set_Y", (void *)Object_SetY);
}
