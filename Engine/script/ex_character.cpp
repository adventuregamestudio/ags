
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
	scAdd_External_Symbol("Character::AddInventory^2",(void *)Character_AddInventory);
	scAdd_External_Symbol("Character::AddWaypoint^2",(void *)Character_AddWaypoint);
	scAdd_External_Symbol("Character::Animate^5",(void *)Character_Animate);
	scAdd_External_Symbol("Character::ChangeRoom^3",(void *)Character_ChangeRoom);
	scAdd_External_Symbol("Character::ChangeRoomAutoPosition^2",(void *)Character_ChangeRoomAutoPosition);
	scAdd_External_Symbol("Character::ChangeView^1",(void *)Character_ChangeView);
	scAdd_External_Symbol("Character::FaceCharacter^2",(void *)Character_FaceCharacter);
	scAdd_External_Symbol("Character::FaceLocation^3",(void *)Character_FaceLocation);
	scAdd_External_Symbol("Character::FaceObject^2",(void *)Character_FaceObject);
	scAdd_External_Symbol("Character::FollowCharacter^3",(void *)Character_FollowCharacter);
	scAdd_External_Symbol("Character::GetProperty^1",(void *)Character_GetProperty);
	scAdd_External_Symbol("Character::GetPropertyText^2",(void *)Character_GetPropertyText);
	scAdd_External_Symbol("Character::GetTextProperty^1",(void *)Character_GetTextProperty);
	scAdd_External_Symbol("Character::HasInventory^1",(void *)Character_HasInventory);
	scAdd_External_Symbol("Character::IsCollidingWithChar^1",(void *)Character_IsCollidingWithChar);
	scAdd_External_Symbol("Character::IsCollidingWithObject^1",(void *)Character_IsCollidingWithObject);
	scAdd_External_Symbol("Character::LockView^1",(void *)Character_LockView);
	scAdd_External_Symbol("Character::LockViewAligned^3",(void *)Character_LockViewAligned);
	scAdd_External_Symbol("Character::LockViewFrame^3",(void *)Character_LockViewFrame);
	scAdd_External_Symbol("Character::LockViewOffset^3",(void *)Character_LockViewOffset);
	scAdd_External_Symbol("Character::LoseInventory^1",(void *)Character_LoseInventory);
	scAdd_External_Symbol("Character::Move^4",(void *)Character_Move);
	scAdd_External_Symbol("Character::PlaceOnWalkableArea^0",(void *)Character_PlaceOnWalkableArea);
	scAdd_External_Symbol("Character::RemoveTint^0",(void *)Character_RemoveTint);
	scAdd_External_Symbol("Character::RunInteraction^1",(void *)Character_RunInteraction);
	scAdd_External_Symbol("Character::Say^101",(void *)Character_Say);
	scAdd_External_Symbol("Character::SayAt^4",(void *)Character_SayAt);
	scAdd_External_Symbol("Character::SayBackground^1",(void *)Character_SayBackground);
	scAdd_External_Symbol("Character::SetAsPlayer^0",(void *)Character_SetAsPlayer);
	scAdd_External_Symbol("Character::SetIdleView^2",(void *)Character_SetIdleView);
	//scAdd_External_Symbol("Character::SetOption^2",(void *)Character_SetOption);
	scAdd_External_Symbol("Character::SetWalkSpeed^2",(void *)Character_SetSpeed);
	scAdd_External_Symbol("Character::StopMoving^0",(void *)Character_StopMoving);
	scAdd_External_Symbol("Character::Think^101",(void *)Character_Think);
	scAdd_External_Symbol("Character::Tint^5",(void *)Character_Tint);
	scAdd_External_Symbol("Character::UnlockView^0",(void *)Character_UnlockView);
	scAdd_External_Symbol("Character::Walk^4",(void *)Character_Walk);
	scAdd_External_Symbol("Character::WalkStraight^3",(void *)Character_WalkStraight);

	// static
	scAdd_External_Symbol("Character::GetAtScreenXY^2", (void *)GetCharacterAtLocation);

	scAdd_External_Symbol("Character::get_ActiveInventory",(void *)Character_GetActiveInventory);
	scAdd_External_Symbol("Character::set_ActiveInventory",(void *)Character_SetActiveInventory);
	scAdd_External_Symbol("Character::get_Animating", (void *)Character_GetAnimating);
	scAdd_External_Symbol("Character::get_AnimationSpeed", (void *)Character_GetAnimationSpeed);
	scAdd_External_Symbol("Character::set_AnimationSpeed", (void *)Character_SetAnimationSpeed);
	scAdd_External_Symbol("Character::get_Baseline",(void *)Character_GetBaseline);
	scAdd_External_Symbol("Character::set_Baseline",(void *)Character_SetBaseline);
	scAdd_External_Symbol("Character::get_BlinkInterval",(void *)Character_GetBlinkInterval);
	scAdd_External_Symbol("Character::set_BlinkInterval",(void *)Character_SetBlinkInterval);
	scAdd_External_Symbol("Character::get_BlinkView",(void *)Character_GetBlinkView);
	scAdd_External_Symbol("Character::set_BlinkView",(void *)Character_SetBlinkView);
	scAdd_External_Symbol("Character::get_BlinkWhileThinking",(void *)Character_GetBlinkWhileThinking);
	scAdd_External_Symbol("Character::set_BlinkWhileThinking",(void *)Character_SetBlinkWhileThinking);
	scAdd_External_Symbol("Character::get_BlockingHeight",(void *)Character_GetBlockingHeight);
	scAdd_External_Symbol("Character::set_BlockingHeight",(void *)Character_SetBlockingHeight);
	scAdd_External_Symbol("Character::get_BlockingWidth",(void *)Character_GetBlockingWidth);
	scAdd_External_Symbol("Character::set_BlockingWidth",(void *)Character_SetBlockingWidth);
	scAdd_External_Symbol("Character::get_Clickable",(void *)Character_GetClickable);
	scAdd_External_Symbol("Character::set_Clickable",(void *)Character_SetClickable);
	scAdd_External_Symbol("Character::get_DiagonalLoops", (void *)Character_GetDiagonalWalking);
	scAdd_External_Symbol("Character::set_DiagonalLoops", (void *)Character_SetDiagonalWalking);
	scAdd_External_Symbol("Character::get_Frame", (void *)Character_GetFrame);
	scAdd_External_Symbol("Character::set_Frame", (void *)Character_SetFrame);
	scAdd_External_Symbol("Character::get_HasExplicitTint", (void *)Character_GetHasExplicitTint);
	scAdd_External_Symbol("Character::get_ID", (void *)Character_GetID);
	scAdd_External_Symbol("Character::get_IdleView", (void *)Character_GetIdleView);
	scAdd_External_Symbol("Character::geti_InventoryQuantity", (void *)Character_GetIInventoryQuantity);
	scAdd_External_Symbol("Character::seti_InventoryQuantity", (void *)Character_SetIInventoryQuantity);
	scAdd_External_Symbol("Character::get_IgnoreLighting",(void *)Character_GetIgnoreLighting);
	scAdd_External_Symbol("Character::set_IgnoreLighting",(void *)Character_SetIgnoreLighting);
	scAdd_External_Symbol("Character::get_IgnoreScaling", (void *)Character_GetIgnoreScaling);
	scAdd_External_Symbol("Character::set_IgnoreScaling", (void *)Character_SetIgnoreScaling);
	scAdd_External_Symbol("Character::get_IgnoreWalkbehinds",(void *)Character_GetIgnoreWalkbehinds);
	scAdd_External_Symbol("Character::set_IgnoreWalkbehinds",(void *)Character_SetIgnoreWalkbehinds);
	scAdd_External_Symbol("Character::get_Loop", (void *)Character_GetLoop);
	scAdd_External_Symbol("Character::set_Loop", (void *)Character_SetLoop);
	scAdd_External_Symbol("Character::get_ManualScaling", (void *)Character_GetIgnoreScaling);
	scAdd_External_Symbol("Character::set_ManualScaling", (void *)Character_SetManualScaling);
	scAdd_External_Symbol("Character::get_MovementLinkedToAnimation",(void *)Character_GetMovementLinkedToAnimation);
	scAdd_External_Symbol("Character::set_MovementLinkedToAnimation",(void *)Character_SetMovementLinkedToAnimation);
	scAdd_External_Symbol("Character::get_Moving", (void *)Character_GetMoving);
	scAdd_External_Symbol("Character::get_Name", (void *)Character_GetName);
	scAdd_External_Symbol("Character::set_Name", (void *)Character_SetName);
	scAdd_External_Symbol("Character::get_NormalView",(void *)Character_GetNormalView);
	scAdd_External_Symbol("Character::get_PreviousRoom",(void *)Character_GetPreviousRoom);
	scAdd_External_Symbol("Character::get_Room",(void *)Character_GetRoom);
	scAdd_External_Symbol("Character::get_ScaleMoveSpeed", (void *)Character_GetScaleMoveSpeed);
	scAdd_External_Symbol("Character::set_ScaleMoveSpeed", (void *)Character_SetScaleMoveSpeed);
	scAdd_External_Symbol("Character::get_ScaleVolume", (void *)Character_GetScaleVolume);
	scAdd_External_Symbol("Character::set_ScaleVolume", (void *)Character_SetScaleVolume);
	scAdd_External_Symbol("Character::get_Scaling", (void *)Character_GetScaling);
	scAdd_External_Symbol("Character::set_Scaling", (void *)Character_SetScaling);
	scAdd_External_Symbol("Character::get_Solid", (void *)Character_GetSolid);
	scAdd_External_Symbol("Character::set_Solid", (void *)Character_SetSolid);
	scAdd_External_Symbol("Character::get_Speaking", (void *)Character_GetSpeaking);
	scAdd_External_Symbol("Character::get_SpeakingFrame", (void *)Character_GetSpeakingFrame);
	scAdd_External_Symbol("Character::get_SpeechAnimationDelay",(void *)GetCharacterSpeechAnimationDelay);
	scAdd_External_Symbol("Character::set_SpeechAnimationDelay",(void *)Character_SetSpeechAnimationDelay);
	scAdd_External_Symbol("Character::get_SpeechColor",(void *)Character_GetSpeechColor);
	scAdd_External_Symbol("Character::set_SpeechColor",(void *)Character_SetSpeechColor);
	scAdd_External_Symbol("Character::get_SpeechView",(void *)Character_GetSpeechView);
	scAdd_External_Symbol("Character::set_SpeechView",(void *)Character_SetSpeechView);
	scAdd_External_Symbol("Character::get_ThinkView",(void *)Character_GetThinkView);
	scAdd_External_Symbol("Character::set_ThinkView",(void *)Character_SetThinkView);
	scAdd_External_Symbol("Character::get_Transparency",(void *)Character_GetTransparency);
	scAdd_External_Symbol("Character::set_Transparency",(void *)Character_SetTransparency);
	scAdd_External_Symbol("Character::get_TurnBeforeWalking", (void *)Character_GetTurnBeforeWalking);
	scAdd_External_Symbol("Character::set_TurnBeforeWalking", (void *)Character_SetTurnBeforeWalking);
	scAdd_External_Symbol("Character::get_View", (void *)Character_GetView);
	scAdd_External_Symbol("Character::get_WalkSpeedX", (void *)Character_GetWalkSpeedX);
	scAdd_External_Symbol("Character::get_WalkSpeedY", (void *)Character_GetWalkSpeedY);
	scAdd_External_Symbol("Character::get_X", (void *)Character_GetX);
	scAdd_External_Symbol("Character::set_X", (void *)Character_SetX);
	scAdd_External_Symbol("Character::get_x", (void *)Character_GetX);
	scAdd_External_Symbol("Character::set_x", (void *)Character_SetX);
	scAdd_External_Symbol("Character::get_Y", (void *)Character_GetY);
	scAdd_External_Symbol("Character::set_Y", (void *)Character_SetY);
	scAdd_External_Symbol("Character::get_y", (void *)Character_GetY);
	scAdd_External_Symbol("Character::set_y", (void *)Character_SetY);
	scAdd_External_Symbol("Character::get_Z", (void *)Character_GetZ);
	scAdd_External_Symbol("Character::set_Z", (void *)Character_SetZ);
	scAdd_External_Symbol("Character::get_z", (void *)Character_GetZ);
	scAdd_External_Symbol("Character::set_z", (void *)Character_SetZ);
}
