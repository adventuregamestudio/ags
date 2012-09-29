
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
	ccAddExternalStaticFunction("Game::ChangeTranslation^1", (void *)Game_ChangeTranslation);
	ccAddExternalStaticFunction("Game::DoOnceOnly^1", (void *)Game_DoOnceOnly);
	ccAddExternalStaticFunction("Game::GetColorFromRGB^3", (void *)Game_GetColorFromRGB);
	ccAddExternalStaticFunction("Game::GetFrameCountForLoop^2", (void *)Game_GetFrameCountForLoop);
	ccAddExternalStaticFunction("Game::GetLocationName^2",(void *)Game_GetLocationName);
	ccAddExternalStaticFunction("Game::GetLoopCountForView^1", (void *)Game_GetLoopCountForView);
	ccAddExternalStaticFunction("Game::GetMODPattern^0",(void *)Game_GetMODPattern);
	ccAddExternalStaticFunction("Game::GetRunNextSettingForLoop^2", (void *)Game_GetRunNextSettingForLoop);
	ccAddExternalStaticFunction("Game::GetSaveSlotDescription^1",(void *)Game_GetSaveSlotDescription);
	ccAddExternalStaticFunction("Game::GetViewFrame^3",(void *)Game_GetViewFrame);
	ccAddExternalStaticFunction("Game::InputBox^1",(void *)Game_InputBox);
	ccAddExternalStaticFunction("Game::SetSaveGameDirectory^1", (void *)Game_SetSaveGameDirectory);
	ccAddExternalStaticFunction("Game::StopSound^1", (void *)StopAllSounds);
	ccAddExternalStaticFunction("Game::get_CharacterCount", (void *)Game_GetCharacterCount);
	ccAddExternalStaticFunction("Game::get_DialogCount", (void *)Game_GetDialogCount);
	ccAddExternalStaticFunction("Game::get_FileName", (void *)Game_GetFileName);
	ccAddExternalStaticFunction("Game::get_FontCount", (void *)Game_GetFontCount);
	ccAddExternalStaticFunction("Game::geti_GlobalMessages",(void *)Game_GetGlobalMessages);
	ccAddExternalStaticFunction("Game::geti_GlobalStrings",(void *)Game_GetGlobalStrings);
	ccAddExternalStaticFunction("Game::seti_GlobalStrings",(void *)SetGlobalString);
	ccAddExternalStaticFunction("Game::get_GUICount", (void *)Game_GetGUICount);
	ccAddExternalStaticFunction("Game::get_IgnoreUserInputAfterTextTimeoutMs", (void *)Game_GetIgnoreUserInputAfterTextTimeoutMs);
	ccAddExternalStaticFunction("Game::set_IgnoreUserInputAfterTextTimeoutMs", (void *)Game_SetIgnoreUserInputAfterTextTimeoutMs);
	ccAddExternalStaticFunction("Game::get_InSkippableCutscene", (void *)Game_GetInSkippableCutscene);
	ccAddExternalStaticFunction("Game::get_InventoryItemCount", (void *)Game_GetInventoryItemCount);
	ccAddExternalStaticFunction("Game::get_MinimumTextDisplayTimeMs", (void *)Game_GetMinimumTextDisplayTimeMs);
	ccAddExternalStaticFunction("Game::set_MinimumTextDisplayTimeMs", (void *)Game_SetMinimumTextDisplayTimeMs);
	ccAddExternalStaticFunction("Game::get_MouseCursorCount", (void *)Game_GetMouseCursorCount);
	ccAddExternalStaticFunction("Game::get_Name", (void *)Game_GetName);
	ccAddExternalStaticFunction("Game::set_Name", (void *)Game_SetName);
	ccAddExternalStaticFunction("Game::get_NormalFont", (void *)Game_GetNormalFont);
	ccAddExternalStaticFunction("Game::set_NormalFont", (void *)SetNormalFont);
	ccAddExternalStaticFunction("Game::get_SkippingCutscene", (void *)Game_GetSkippingCutscene);
	ccAddExternalStaticFunction("Game::get_SpeechFont", (void *)Game_GetSpeechFont);
	ccAddExternalStaticFunction("Game::set_SpeechFont", (void *)SetSpeechFont);
	ccAddExternalStaticFunction("Game::geti_SpriteWidth", (void *)Game_GetSpriteWidth);
	ccAddExternalStaticFunction("Game::geti_SpriteHeight", (void *)Game_GetSpriteHeight);
	ccAddExternalStaticFunction("Game::get_TextReadingSpeed", (void *)Game_GetTextReadingSpeed);
	ccAddExternalStaticFunction("Game::set_TextReadingSpeed", (void *)Game_SetTextReadingSpeed);
	ccAddExternalStaticFunction("Game::get_TranslationFilename",(void *)Game_GetTranslationFilename);
	ccAddExternalStaticFunction("Game::get_UseNativeCoordinates", (void *)Game_GetUseNativeCoordinates);
	ccAddExternalStaticFunction("Game::get_ViewCount", (void *)Game_GetViewCount);
}
