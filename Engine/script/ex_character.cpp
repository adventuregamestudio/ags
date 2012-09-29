
//=============================================================================
//
// Exporting Character script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_character_script_functions()
{
	ccAddExternalObjectFunction("Character::AddInventory^2",(void *)Character_AddInventory);
	ccAddExternalObjectFunction("Character::AddWaypoint^2",(void *)Character_AddWaypoint);
	ccAddExternalObjectFunction("Character::Animate^5",(void *)Character_Animate);
	ccAddExternalObjectFunction("Character::ChangeRoom^3",(void *)Character_ChangeRoom);
	ccAddExternalObjectFunction("Character::ChangeRoomAutoPosition^2",(void *)Character_ChangeRoomAutoPosition);
	ccAddExternalObjectFunction("Character::ChangeView^1",(void *)Character_ChangeView);
	ccAddExternalObjectFunction("Character::FaceCharacter^2",(void *)Character_FaceCharacter);
	ccAddExternalObjectFunction("Character::FaceLocation^3",(void *)Character_FaceLocation);
	ccAddExternalObjectFunction("Character::FaceObject^2",(void *)Character_FaceObject);
	ccAddExternalObjectFunction("Character::FollowCharacter^3",(void *)Character_FollowCharacter);
	ccAddExternalObjectFunction("Character::GetProperty^1",(void *)Character_GetProperty);
	ccAddExternalObjectFunction("Character::GetPropertyText^2",(void *)Character_GetPropertyText);
	ccAddExternalObjectFunction("Character::GetTextProperty^1",(void *)Character_GetTextProperty);
	ccAddExternalObjectFunction("Character::HasInventory^1",(void *)Character_HasInventory);
	ccAddExternalObjectFunction("Character::IsCollidingWithChar^1",(void *)Character_IsCollidingWithChar);
	ccAddExternalObjectFunction("Character::IsCollidingWithObject^1",(void *)Character_IsCollidingWithObject);
	ccAddExternalObjectFunction("Character::LockView^1",(void *)Character_LockView);
	ccAddExternalObjectFunction("Character::LockViewAligned^3",(void *)Character_LockViewAligned);
	ccAddExternalObjectFunction("Character::LockViewFrame^3",(void *)Character_LockViewFrame);
	ccAddExternalObjectFunction("Character::LockViewOffset^3",(void *)Character_LockViewOffset);
	ccAddExternalObjectFunction("Character::LoseInventory^1",(void *)Character_LoseInventory);
	ccAddExternalObjectFunction("Character::Move^4",(void *)Character_Move);
	ccAddExternalObjectFunction("Character::PlaceOnWalkableArea^0",(void *)Character_PlaceOnWalkableArea);
	ccAddExternalObjectFunction("Character::RemoveTint^0",(void *)Character_RemoveTint);
	ccAddExternalObjectFunction("Character::RunInteraction^1",(void *)Character_RunInteraction);
	ccAddExternalObjectFunction("Character::Say^101",(void *)Character_Say);
	ccAddExternalObjectFunction("Character::SayAt^4",(void *)Character_SayAt);
	ccAddExternalObjectFunction("Character::SayBackground^1",(void *)Character_SayBackground);
	ccAddExternalObjectFunction("Character::SetAsPlayer^0",(void *)Character_SetAsPlayer);
	ccAddExternalObjectFunction("Character::SetIdleView^2",(void *)Character_SetIdleView);
	//ccAddExternalObjectFunction("Character::SetOption^2",(void *)Character_SetOption);
	ccAddExternalObjectFunction("Character::SetWalkSpeed^2",(void *)Character_SetSpeed);
	ccAddExternalObjectFunction("Character::StopMoving^0",(void *)Character_StopMoving);
	ccAddExternalObjectFunction("Character::Think^101",(void *)Character_Think);
	ccAddExternalObjectFunction("Character::Tint^5",(void *)Character_Tint);
	ccAddExternalObjectFunction("Character::UnlockView^0",(void *)Character_UnlockView);
	ccAddExternalObjectFunction("Character::Walk^4",(void *)Character_Walk);
	ccAddExternalObjectFunction("Character::WalkStraight^3",(void *)Character_WalkStraight);

	// static
	ccAddExternalObjectFunction("Character::GetAtScreenXY^2", (void *)GetCharacterAtLocation);

	ccAddExternalObjectFunction("Character::get_ActiveInventory",(void *)Character_GetActiveInventory);
	ccAddExternalObjectFunction("Character::set_ActiveInventory",(void *)Character_SetActiveInventory);
	ccAddExternalObjectFunction("Character::get_Animating", (void *)Character_GetAnimating);
	ccAddExternalObjectFunction("Character::get_AnimationSpeed", (void *)Character_GetAnimationSpeed);
	ccAddExternalObjectFunction("Character::set_AnimationSpeed", (void *)Character_SetAnimationSpeed);
	ccAddExternalObjectFunction("Character::get_Baseline",(void *)Character_GetBaseline);
	ccAddExternalObjectFunction("Character::set_Baseline",(void *)Character_SetBaseline);
	ccAddExternalObjectFunction("Character::get_BlinkInterval",(void *)Character_GetBlinkInterval);
	ccAddExternalObjectFunction("Character::set_BlinkInterval",(void *)Character_SetBlinkInterval);
	ccAddExternalObjectFunction("Character::get_BlinkView",(void *)Character_GetBlinkView);
	ccAddExternalObjectFunction("Character::set_BlinkView",(void *)Character_SetBlinkView);
	ccAddExternalObjectFunction("Character::get_BlinkWhileThinking",(void *)Character_GetBlinkWhileThinking);
	ccAddExternalObjectFunction("Character::set_BlinkWhileThinking",(void *)Character_SetBlinkWhileThinking);
	ccAddExternalObjectFunction("Character::get_BlockingHeight",(void *)Character_GetBlockingHeight);
	ccAddExternalObjectFunction("Character::set_BlockingHeight",(void *)Character_SetBlockingHeight);
	ccAddExternalObjectFunction("Character::get_BlockingWidth",(void *)Character_GetBlockingWidth);
	ccAddExternalObjectFunction("Character::set_BlockingWidth",(void *)Character_SetBlockingWidth);
	ccAddExternalObjectFunction("Character::get_Clickable",(void *)Character_GetClickable);
	ccAddExternalObjectFunction("Character::set_Clickable",(void *)Character_SetClickable);
	ccAddExternalObjectFunction("Character::get_DiagonalLoops", (void *)Character_GetDiagonalWalking);
	ccAddExternalObjectFunction("Character::set_DiagonalLoops", (void *)Character_SetDiagonalWalking);
	ccAddExternalObjectFunction("Character::get_Frame", (void *)Character_GetFrame);
	ccAddExternalObjectFunction("Character::set_Frame", (void *)Character_SetFrame);
	ccAddExternalObjectFunction("Character::get_HasExplicitTint", (void *)Character_GetHasExplicitTint);
	ccAddExternalObjectFunction("Character::get_ID", (void *)Character_GetID);
	ccAddExternalObjectFunction("Character::get_IdleView", (void *)Character_GetIdleView);
	ccAddExternalObjectFunction("Character::geti_InventoryQuantity", (void *)Character_GetIInventoryQuantity);
	ccAddExternalObjectFunction("Character::seti_InventoryQuantity", (void *)Character_SetIInventoryQuantity);
	ccAddExternalObjectFunction("Character::get_IgnoreLighting",(void *)Character_GetIgnoreLighting);
	ccAddExternalObjectFunction("Character::set_IgnoreLighting",(void *)Character_SetIgnoreLighting);
	ccAddExternalObjectFunction("Character::get_IgnoreScaling", (void *)Character_GetIgnoreScaling);
	ccAddExternalObjectFunction("Character::set_IgnoreScaling", (void *)Character_SetIgnoreScaling);
	ccAddExternalObjectFunction("Character::get_IgnoreWalkbehinds",(void *)Character_GetIgnoreWalkbehinds);
	ccAddExternalObjectFunction("Character::set_IgnoreWalkbehinds",(void *)Character_SetIgnoreWalkbehinds);
	ccAddExternalObjectFunction("Character::get_Loop", (void *)Character_GetLoop);
	ccAddExternalObjectFunction("Character::set_Loop", (void *)Character_SetLoop);
	ccAddExternalObjectFunction("Character::get_ManualScaling", (void *)Character_GetIgnoreScaling);
	ccAddExternalObjectFunction("Character::set_ManualScaling", (void *)Character_SetManualScaling);
	ccAddExternalObjectFunction("Character::get_MovementLinkedToAnimation",(void *)Character_GetMovementLinkedToAnimation);
	ccAddExternalObjectFunction("Character::set_MovementLinkedToAnimation",(void *)Character_SetMovementLinkedToAnimation);
	ccAddExternalObjectFunction("Character::get_Moving", (void *)Character_GetMoving);
	ccAddExternalObjectFunction("Character::get_Name", (void *)Character_GetName);
	ccAddExternalObjectFunction("Character::set_Name", (void *)Character_SetName);
	ccAddExternalObjectFunction("Character::get_NormalView",(void *)Character_GetNormalView);
	ccAddExternalObjectFunction("Character::get_PreviousRoom",(void *)Character_GetPreviousRoom);
	ccAddExternalObjectFunction("Character::get_Room",(void *)Character_GetRoom);
	ccAddExternalObjectFunction("Character::get_ScaleMoveSpeed", (void *)Character_GetScaleMoveSpeed);
	ccAddExternalObjectFunction("Character::set_ScaleMoveSpeed", (void *)Character_SetScaleMoveSpeed);
	ccAddExternalObjectFunction("Character::get_ScaleVolume", (void *)Character_GetScaleVolume);
	ccAddExternalObjectFunction("Character::set_ScaleVolume", (void *)Character_SetScaleVolume);
	ccAddExternalObjectFunction("Character::get_Scaling", (void *)Character_GetScaling);
	ccAddExternalObjectFunction("Character::set_Scaling", (void *)Character_SetScaling);
	ccAddExternalObjectFunction("Character::get_Solid", (void *)Character_GetSolid);
	ccAddExternalObjectFunction("Character::set_Solid", (void *)Character_SetSolid);
	ccAddExternalObjectFunction("Character::get_Speaking", (void *)Character_GetSpeaking);
	ccAddExternalObjectFunction("Character::get_SpeakingFrame", (void *)Character_GetSpeakingFrame);
	ccAddExternalObjectFunction("Character::get_SpeechAnimationDelay",(void *)GetCharacterSpeechAnimationDelay);
	ccAddExternalObjectFunction("Character::set_SpeechAnimationDelay",(void *)Character_SetSpeechAnimationDelay);
	ccAddExternalObjectFunction("Character::get_SpeechColor",(void *)Character_GetSpeechColor);
	ccAddExternalObjectFunction("Character::set_SpeechColor",(void *)Character_SetSpeechColor);
	ccAddExternalObjectFunction("Character::get_SpeechView",(void *)Character_GetSpeechView);
	ccAddExternalObjectFunction("Character::set_SpeechView",(void *)Character_SetSpeechView);
	ccAddExternalObjectFunction("Character::get_ThinkView",(void *)Character_GetThinkView);
	ccAddExternalObjectFunction("Character::set_ThinkView",(void *)Character_SetThinkView);
	ccAddExternalObjectFunction("Character::get_Transparency",(void *)Character_GetTransparency);
	ccAddExternalObjectFunction("Character::set_Transparency",(void *)Character_SetTransparency);
	ccAddExternalObjectFunction("Character::get_TurnBeforeWalking", (void *)Character_GetTurnBeforeWalking);
	ccAddExternalObjectFunction("Character::set_TurnBeforeWalking", (void *)Character_SetTurnBeforeWalking);
	ccAddExternalObjectFunction("Character::get_View", (void *)Character_GetView);
	ccAddExternalObjectFunction("Character::get_WalkSpeedX", (void *)Character_GetWalkSpeedX);
	ccAddExternalObjectFunction("Character::get_WalkSpeedY", (void *)Character_GetWalkSpeedY);
	ccAddExternalObjectFunction("Character::get_X", (void *)Character_GetX);
	ccAddExternalObjectFunction("Character::set_X", (void *)Character_SetX);
	ccAddExternalObjectFunction("Character::get_x", (void *)Character_GetX);
	ccAddExternalObjectFunction("Character::set_x", (void *)Character_SetX);
	ccAddExternalObjectFunction("Character::get_Y", (void *)Character_GetY);
	ccAddExternalObjectFunction("Character::set_Y", (void *)Character_SetY);
	ccAddExternalObjectFunction("Character::get_y", (void *)Character_GetY);
	ccAddExternalObjectFunction("Character::set_y", (void *)Character_SetY);
	ccAddExternalObjectFunction("Character::get_Z", (void *)Character_GetZ);
	ccAddExternalObjectFunction("Character::set_Z", (void *)Character_SetZ);
	ccAddExternalObjectFunction("Character::get_z", (void *)Character_GetZ);
	ccAddExternalObjectFunction("Character::set_z", (void *)Character_SetZ);
}
