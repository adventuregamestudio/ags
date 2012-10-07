//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// Exporting Game script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_game_script_functions()
{
	scAdd_External_Symbol("Game::ChangeTranslation^1", (void *)Game_ChangeTranslation);
	scAdd_External_Symbol("Game::DoOnceOnly^1", (void *)Game_DoOnceOnly);
	scAdd_External_Symbol("Game::GetColorFromRGB^3", (void *)Game_GetColorFromRGB);
	scAdd_External_Symbol("Game::GetFrameCountForLoop^2", (void *)Game_GetFrameCountForLoop);
	scAdd_External_Symbol("Game::GetLocationName^2",(void *)Game_GetLocationName);
	scAdd_External_Symbol("Game::GetLoopCountForView^1", (void *)Game_GetLoopCountForView);
	scAdd_External_Symbol("Game::GetMODPattern^0",(void *)Game_GetMODPattern);
	scAdd_External_Symbol("Game::GetRunNextSettingForLoop^2", (void *)Game_GetRunNextSettingForLoop);
	scAdd_External_Symbol("Game::GetSaveSlotDescription^1",(void *)Game_GetSaveSlotDescription);
	scAdd_External_Symbol("Game::GetViewFrame^3",(void *)Game_GetViewFrame);
	scAdd_External_Symbol("Game::InputBox^1",(void *)Game_InputBox);
	scAdd_External_Symbol("Game::SetSaveGameDirectory^1", (void *)Game_SetSaveGameDirectory);
	scAdd_External_Symbol("Game::StopSound^1", (void *)StopAllSounds);
	scAdd_External_Symbol("Game::get_CharacterCount", (void *)Game_GetCharacterCount);
	scAdd_External_Symbol("Game::get_DialogCount", (void *)Game_GetDialogCount);
	scAdd_External_Symbol("Game::get_FileName", (void *)Game_GetFileName);
	scAdd_External_Symbol("Game::get_FontCount", (void *)Game_GetFontCount);
	scAdd_External_Symbol("Game::geti_GlobalMessages",(void *)Game_GetGlobalMessages);
	scAdd_External_Symbol("Game::geti_GlobalStrings",(void *)Game_GetGlobalStrings);
	scAdd_External_Symbol("Game::seti_GlobalStrings",(void *)SetGlobalString);
	scAdd_External_Symbol("Game::get_GUICount", (void *)Game_GetGUICount);
	scAdd_External_Symbol("Game::get_IgnoreUserInputAfterTextTimeoutMs", (void *)Game_GetIgnoreUserInputAfterTextTimeoutMs);
	scAdd_External_Symbol("Game::set_IgnoreUserInputAfterTextTimeoutMs", (void *)Game_SetIgnoreUserInputAfterTextTimeoutMs);
	scAdd_External_Symbol("Game::get_InSkippableCutscene", (void *)Game_GetInSkippableCutscene);
	scAdd_External_Symbol("Game::get_InventoryItemCount", (void *)Game_GetInventoryItemCount);
	scAdd_External_Symbol("Game::get_MinimumTextDisplayTimeMs", (void *)Game_GetMinimumTextDisplayTimeMs);
	scAdd_External_Symbol("Game::set_MinimumTextDisplayTimeMs", (void *)Game_SetMinimumTextDisplayTimeMs);
	scAdd_External_Symbol("Game::get_MouseCursorCount", (void *)Game_GetMouseCursorCount);
	scAdd_External_Symbol("Game::get_Name", (void *)Game_GetName);
	scAdd_External_Symbol("Game::set_Name", (void *)Game_SetName);
	scAdd_External_Symbol("Game::get_NormalFont", (void *)Game_GetNormalFont);
	scAdd_External_Symbol("Game::set_NormalFont", (void *)SetNormalFont);
	scAdd_External_Symbol("Game::get_SkippingCutscene", (void *)Game_GetSkippingCutscene);
	scAdd_External_Symbol("Game::get_SpeechFont", (void *)Game_GetSpeechFont);
	scAdd_External_Symbol("Game::set_SpeechFont", (void *)SetSpeechFont);
	scAdd_External_Symbol("Game::geti_SpriteWidth", (void *)Game_GetSpriteWidth);
	scAdd_External_Symbol("Game::geti_SpriteHeight", (void *)Game_GetSpriteHeight);
	scAdd_External_Symbol("Game::get_TextReadingSpeed", (void *)Game_GetTextReadingSpeed);
	scAdd_External_Symbol("Game::set_TextReadingSpeed", (void *)Game_SetTextReadingSpeed);
	scAdd_External_Symbol("Game::get_TranslationFilename",(void *)Game_GetTranslationFilename);
	scAdd_External_Symbol("Game::get_UseNativeCoordinates", (void *)Game_GetUseNativeCoordinates);
	scAdd_External_Symbol("Game::get_ViewCount", (void *)Game_GetViewCount);
}
