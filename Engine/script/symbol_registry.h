
//=============================================================================
//
// Symbol registry functions
//
//=============================================================================
#ifndef __AGS_EE_SCRIPT__SYMBOLREGISTRY_H
#define __AGS_EE_SCRIPT__SYMBOLREGISTRY_H

//-----------------------------------------------------------------------------
// TODO
//-----------------------------------------------------------------------------
//
// 1. An exported function should be named according to conformant pattern,
// that would explicitly tell:
//      - that function is being exported to script system;
//      - that function should either be treated as a class member or global;
//      - minimal and maximal number of parameters.
// E.g.:
//      se_Button_Animate_4_5
//
//-----------------------------------------------------------------------------
//
// 2. I suppose it would be better if modules would declare a set of exported
// functions in headers intended to be included to symbol_registry.h.
//
// In such case each module will control what it allows to export, and exporting
// units will know what they may use. Export system should determine which of
// the available symbols are to be registered and under which script-names.
//
// I.e. e.g. Character module will have a character_export.h with a set of
// character-specific functions and/or symbols declared, symbol_registry.h
// will have that header permanently included in itself, ex_character.cpp
// will register all the symbols it wants from the given list.
//
// This is more a question of design rather than technical necessity, but I
// think this will allow more accurate control over exports.
//
//-----------------------------------------------------------------------------

#include "script/ex_defines.h"
#include "util/file.h" // for enum FileOpenMode

//-------------------------------------------------------------
void register_global_script_functions();
void register_builtin_plugins_script_functions();

//-------------------------------------------------------------
void register_audio_script_functions();
void register_button_script_functions();
void register_character_script_functions();
void register_datetime_script_functions();
void register_dialog_script_functions();
void register_drawingsurface_script_functions();
void register_dynamicsprite_script_functions();
void register_file_script_functions();
void register_game_script_functions();
void register_gui_script_functions();
void register_guicontrol_script_functions();
void register_hotspot_script_functions();
void register_inventoryitem_script_functions();
void register_invwindow_script_functions();
void register_label_script_functions();
void register_listbox_script_functions();
void register_math_script_functions();
void register_mouse_script_functions();
void register_object_script_functions();
void register_overlay_script_functions();
void register_parser_script_functions();
void register_region_script_functions();
void register_room_script_functions();
void register_slider_script_functions();
void register_string_script_functions();
void register_system_script_functions();
void register_textbox_script_functions();
void register_viewframe_script_functions();
//-------------------------------------------------------------


//=============================================================================
//
// Exported functions prototypes and forward declarations
//
//=============================================================================

//-----------------------------------------------------------------------------
// Audio
//-----------------------------------------------------------------------------
struct ScriptAudioChannel;
struct ScriptAudioClip;

int		AudioChannel_GetID(ScriptAudioChannel *channel);
int		AudioChannel_GetIsPlaying(ScriptAudioChannel *channel);
int		AudioChannel_GetPanning(ScriptAudioChannel *channel);
void	AudioChannel_SetPanning(ScriptAudioChannel *channel, int newPanning);
ScriptAudioClip* AudioChannel_GetPlayingClip(ScriptAudioChannel *channel);
int		AudioChannel_GetPosition(ScriptAudioChannel *channel);
int		AudioChannel_GetPositionMs(ScriptAudioChannel *channel);
int		AudioChannel_GetLengthMs(ScriptAudioChannel *channel);
int		AudioChannel_GetVolume(ScriptAudioChannel *channel);
int		AudioChannel_SetVolume(ScriptAudioChannel *channel, int newVolume);
void	AudioChannel_Stop(ScriptAudioChannel *channel);
void	AudioChannel_Seek(ScriptAudioChannel *channel, int newPosition);
void	AudioChannel_SetRoomLocation(ScriptAudioChannel *channel, int xPos, int yPos);

int		AudioClip_GetFileType(ScriptAudioClip *clip);
int		AudioClip_GetType(ScriptAudioClip *clip);
int		AudioClip_GetIsAvailable(ScriptAudioClip *clip);
void	AudioClip_Stop(ScriptAudioClip *clip);
ScriptAudioChannel* AudioClip_Play(ScriptAudioClip *clip, int priority, int repeat);
ScriptAudioChannel* AudioClip_PlayFrom(ScriptAudioClip *clip, int position, int priority, int repeat);
ScriptAudioChannel* AudioClip_PlayQueued(ScriptAudioClip *clip, int priority, int repeat);

void	Game_StopAudio(int audioType);
int		Game_IsAudioPlaying(int audioType);
void	Game_SetAudioTypeSpeechVolumeDrop(int audioType, int volumeDrop);
void	Game_SetAudioTypeVolume(int audioType, int volume, int changeType);

int		System_GetAudioChannelCount();
ScriptAudioChannel* System_GetAudioChannels(int index);


//-----------------------------------------------------------------------------
// Button
//-----------------------------------------------------------------------------
struct GUIButton;

void	Button_Animate(GUIButton *butt, int view, int loop, int speed, int repeat);
const char* Button_GetText_New(GUIButton *butt);
void	Button_GetText(GUIButton *butt, char *buffer);
void	Button_SetText(GUIButton *butt, const char *newtx);
void	Button_SetFont(GUIButton *butt, int newFont);
int		Button_GetFont(GUIButton *butt);
int		Button_GetClipImage(GUIButton *butt);
void	Button_SetClipImage(GUIButton *butt, int newval);
int		Button_GetGraphic(GUIButton *butt);
int		Button_GetMouseOverGraphic(GUIButton *butt);
void	Button_SetMouseOverGraphic(GUIButton *guil, int slotn);
int		Button_GetNormalGraphic(GUIButton *butt);
void	Button_SetNormalGraphic(GUIButton *guil, int slotn);
int		Button_GetPushedGraphic(GUIButton *butt);
void	Button_SetPushedGraphic(GUIButton *guil, int slotn);
int		Button_GetTextColor(GUIButton *butt);
void	Button_SetTextColor(GUIButton *butt, int newcol);


//-----------------------------------------------------------------------------
// Character
//-----------------------------------------------------------------------------
struct CharacterInfo;
struct ScriptObject;
struct ScriptInvItem;
struct ScriptOverlay;

void Character_AddInventory(CharacterInfo *chaa, ScriptInvItem *invi, int addIndex);
void Character_AddWaypoint(CharacterInfo *chaa, int x, int y);
void Character_Animate(CharacterInfo *chaa, int loop, int delay, int repeat, int blocking, int direction);
void Character_ChangeRoomAutoPosition(CharacterInfo *chaa, int room, int newPos);
void Character_ChangeRoom(CharacterInfo *chaa, int room, int x, int y);
void Character_ChangeView(CharacterInfo *chap, int vii);
void Character_FaceCharacter(CharacterInfo *char1, CharacterInfo *char2, int blockingStyle);
void Character_FaceLocation(CharacterInfo *char1, int xx, int yy, int blockingStyle);
void Character_FaceObject(CharacterInfo *char1, ScriptObject *obj, int blockingStyle);
void Character_FollowCharacter(CharacterInfo *chaa, CharacterInfo *tofollow, int distaway, int eagerness);
int Character_IsCollidingWithChar(CharacterInfo *char1, CharacterInfo *char2);
int Character_IsCollidingWithObject(CharacterInfo *chin, ScriptObject *objid);
void Character_LockView(CharacterInfo *chap, int vii);
void Character_LockViewAligned(CharacterInfo *chap, int vii, int loop, int align);
void Character_LockViewFrame(CharacterInfo *chaa, int view, int loop, int frame);
void Character_LockViewOffset(CharacterInfo *chap, int vii, int xoffs, int yoffs);
void Character_LoseInventory(CharacterInfo *chap, ScriptInvItem *invi);
void Character_PlaceOnWalkableArea(CharacterInfo *chap);
void Character_RemoveTint(CharacterInfo *chaa);
int Character_GetHasExplicitTint(CharacterInfo *chaa);
void Character_Say(CharacterInfo *chaa, const char *texx, ...);
void Character_SayAt(CharacterInfo *chaa, int x, int y, int width, const char *texx);
ScriptOverlay* Character_SayBackground(CharacterInfo *chaa, const char *texx);
void Character_SetAsPlayer(CharacterInfo *chaa);
void Character_SetIdleView(CharacterInfo *chaa, int iview, int itime);
void Character_SetOption(CharacterInfo *chaa, int flag, int yesorno);
void Character_SetSpeed(CharacterInfo *chaa, int xspeed, int yspeed);
void Character_StopMoving(CharacterInfo *charp);
void Character_Tint(CharacterInfo *chaa, int red, int green, int blue, int opacity, int luminance);
void Character_Think(CharacterInfo *chaa, const char *texx, ...);
void Character_UnlockView(CharacterInfo *chaa);
void Character_Walk(CharacterInfo *chaa, int x, int y, int blocking, int direct);
void Character_Move(CharacterInfo *chaa, int x, int y, int blocking, int direct);
void Character_WalkStraight(CharacterInfo *chaa, int xx, int yy, int blocking);

CharacterInfo *GetCharacterAtLocation(int xx, int yy);

ScriptInvItem* Character_GetActiveInventory(CharacterInfo *chaa);
void Character_SetActiveInventory(CharacterInfo *chaa, ScriptInvItem* iit);
int Character_GetAnimating(CharacterInfo *chaa);
int Character_GetAnimationSpeed(CharacterInfo *chaa);
void Character_SetAnimationSpeed(CharacterInfo *chaa, int newval);
int Character_GetBaseline(CharacterInfo *chaa);
void Character_SetBaseline(CharacterInfo *chaa, int basel);
int Character_GetBlinkInterval(CharacterInfo *chaa);
void Character_SetBlinkInterval(CharacterInfo *chaa, int interval);
int Character_GetBlinkView(CharacterInfo *chaa);
void Character_SetBlinkView(CharacterInfo *chaa, int vii);
int Character_GetBlinkWhileThinking(CharacterInfo *chaa);
void Character_SetBlinkWhileThinking(CharacterInfo *chaa, int yesOrNo);
int Character_GetBlockingHeight(CharacterInfo *chaa);
void Character_SetBlockingHeight(CharacterInfo *chaa, int hit);
int Character_GetBlockingWidth(CharacterInfo *chaa);
void Character_SetBlockingWidth(CharacterInfo *chaa, int wid);
int Character_GetDiagonalWalking(CharacterInfo *chaa);
void Character_SetDiagonalWalking(CharacterInfo *chaa, int yesorno);
int Character_GetClickable(CharacterInfo *chaa);
void Character_SetClickable(CharacterInfo *chaa, int clik);
int Character_GetID(CharacterInfo *chaa);
int Character_GetFrame(CharacterInfo *chaa);
void Character_SetFrame(CharacterInfo *chaa, int newval);
int Character_GetIdleView(CharacterInfo *chaa);
int Character_GetIInventoryQuantity(CharacterInfo *chaa, int index);
int Character_HasInventory(CharacterInfo *chaa, ScriptInvItem *invi);
void Character_SetIInventoryQuantity(CharacterInfo *chaa, int index, int quant);
int Character_GetIgnoreLighting(CharacterInfo *chaa);
void Character_SetIgnoreLighting(CharacterInfo *chaa, int yesorno);
int Character_GetIgnoreScaling(CharacterInfo *chaa);
void Character_SetIgnoreScaling(CharacterInfo *chaa, int yesorno);
void Character_SetManualScaling(CharacterInfo *chaa, int yesorno);
int Character_GetIgnoreWalkbehinds(CharacterInfo *chaa);
void Character_SetIgnoreWalkbehinds(CharacterInfo *chaa, int yesorno);
int Character_GetMovementLinkedToAnimation(CharacterInfo *chaa);
void Character_SetMovementLinkedToAnimation(CharacterInfo *chaa, int yesorno);
int Character_GetLoop(CharacterInfo *chaa);
void Character_SetLoop(CharacterInfo *chaa, int newval);
int Character_GetMoving(CharacterInfo *chaa);
const char* Character_GetName(CharacterInfo *chaa);
void Character_SetName(CharacterInfo *chaa, const char *newName);
int Character_GetNormalView(CharacterInfo *chaa);
int Character_GetPreviousRoom(CharacterInfo *chaa);
int Character_GetRoom(CharacterInfo *chaa);
int Character_GetScaleMoveSpeed(CharacterInfo *chaa);
void Character_SetScaleMoveSpeed(CharacterInfo *chaa, int yesorno);
int Character_GetScaleVolume(CharacterInfo *chaa);
void Character_SetScaleVolume(CharacterInfo *chaa, int yesorno);
int Character_GetScaling(CharacterInfo *chaa);
void Character_SetScaling(CharacterInfo *chaa, int zoomlevel);
int Character_GetSolid(CharacterInfo *chaa);
void Character_SetSolid(CharacterInfo *chaa, int yesorno);
int Character_GetSpeaking(CharacterInfo *chaa);
int Character_GetSpeechColor(CharacterInfo *chaa);
void Character_SetSpeechColor(CharacterInfo *chaa, int ncol);
int GetCharacterSpeechAnimationDelay(CharacterInfo *cha);
void Character_SetSpeechAnimationDelay(CharacterInfo *chaa, int newDelay);
int Character_GetSpeechView(CharacterInfo *chaa);
void Character_SetSpeechView(CharacterInfo *chaa, int vii);
int Character_GetThinkView(CharacterInfo *chaa);
void Character_SetThinkView(CharacterInfo *chaa, int vii);
int Character_GetTransparency(CharacterInfo *chaa);
void Character_SetTransparency(CharacterInfo *chaa, int trans);
int Character_GetTurnBeforeWalking(CharacterInfo *chaa);
void Character_SetTurnBeforeWalking(CharacterInfo *chaa, int yesorno);
int Character_GetView(CharacterInfo *chaa);
int Character_GetWalkSpeedX(CharacterInfo *chaa);
int Character_GetWalkSpeedY(CharacterInfo *chaa);
int Character_GetX(CharacterInfo *chaa);
void Character_SetX(CharacterInfo *chaa, int newval);
int Character_GetY(CharacterInfo *chaa);
void Character_SetY(CharacterInfo *chaa, int newval);
int Character_GetZ(CharacterInfo *chaa);
void Character_SetZ(CharacterInfo *chaa, int newval);
void Character_GetPropertyText(CharacterInfo *chaa, const char *property, char *bufer);
const char* Character_GetTextProperty(CharacterInfo *chaa, const char *property);
int Character_GetProperty(CharacterInfo *chaa, const char *property);
void Character_RunInteraction(CharacterInfo *chaa, int mood);
int Character_GetSpeakingFrame(CharacterInfo *chaa);


//-----------------------------------------------------------------------------
// DateTime
//-----------------------------------------------------------------------------
struct ScriptDateTime;

ScriptDateTime* DateTime_Now_Core();
ScriptDateTime* DateTime_Now();
int DateTime_GetYear(ScriptDateTime *sdt);
int DateTime_GetMonth(ScriptDateTime *sdt);
int DateTime_GetDayOfMonth(ScriptDateTime *sdt);
int DateTime_GetHour(ScriptDateTime *sdt);
int DateTime_GetMinute(ScriptDateTime *sdt);
int DateTime_GetSecond(ScriptDateTime *sdt);
int DateTime_GetRawTime(ScriptDateTime *sdt);


//-----------------------------------------------------------------------------
// Dialog
//-----------------------------------------------------------------------------
struct ScriptDialog;
struct ScriptDialogOptionsRendering;
struct ScriptDrawingSurface;

int Dialog_GetID(ScriptDialog *sd);
int Dialog_GetOptionCount(ScriptDialog *sd);
int Dialog_GetShowTextParser(ScriptDialog *sd);
const char* Dialog_GetOptionText(ScriptDialog *sd, int option);
int Dialog_DisplayOptions(ScriptDialog *sd, int sayChosenOption);
int Dialog_GetOptionState(ScriptDialog *sd, int option);
int Dialog_HasOptionBeenChosen(ScriptDialog *sd, int option);
void Dialog_SetOptionState(ScriptDialog *sd, int option, int newState);
void Dialog_Start(ScriptDialog *sd);

int DialogOptionsRendering_GetX(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetX(ScriptDialogOptionsRendering *dlgOptRender, int newX);
int DialogOptionsRendering_GetY(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetY(ScriptDialogOptionsRendering *dlgOptRender, int newY);
int DialogOptionsRendering_GetWidth(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetWidth(ScriptDialogOptionsRendering *dlgOptRender, int newWidth);
int DialogOptionsRendering_GetHeight(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetHeight(ScriptDialogOptionsRendering *dlgOptRender, int newHeight);
int DialogOptionsRendering_GetParserTextboxX(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetParserTextboxX(ScriptDialogOptionsRendering *dlgOptRender, int newX);
int DialogOptionsRendering_GetParserTextboxY(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetParserTextboxY(ScriptDialogOptionsRendering *dlgOptRender, int newY);
int DialogOptionsRendering_GetParserTextboxWidth(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetParserTextboxWidth(ScriptDialogOptionsRendering *dlgOptRender, int newWidth);
ScriptDialog* DialogOptionsRendering_GetDialogToRender(ScriptDialogOptionsRendering *dlgOptRender);
ScriptDrawingSurface* DialogOptionsRendering_GetSurface(ScriptDialogOptionsRendering *dlgOptRender);
int DialogOptionsRendering_GetActiveOptionID(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetActiveOptionID(ScriptDialogOptionsRendering *dlgOptRender, int activeOptionID);


//-----------------------------------------------------------------------------
// DrawingSurface
//-----------------------------------------------------------------------------
struct ScriptDrawingSurface;

void DrawingSurface_Release(ScriptDrawingSurface* sds);
ScriptDrawingSurface* DrawingSurface_CreateCopy(ScriptDrawingSurface *sds);
void DrawingSurface_DrawSurface(ScriptDrawingSurface* target, ScriptDrawingSurface* source, int translev);
void DrawingSurface_DrawImage(ScriptDrawingSurface* sds, int xx, int yy, int slot, int trans, int width, int height);
void DrawingSurface_SetDrawingColor(ScriptDrawingSurface *sds, int newColour);
int DrawingSurface_GetDrawingColor(ScriptDrawingSurface *sds);
void DrawingSurface_SetUseHighResCoordinates(ScriptDrawingSurface *sds, int highRes);
int DrawingSurface_GetUseHighResCoordinates(ScriptDrawingSurface *sds);
int DrawingSurface_GetHeight(ScriptDrawingSurface *sds);
int DrawingSurface_GetWidth(ScriptDrawingSurface *sds);
void DrawingSurface_Clear(ScriptDrawingSurface *sds, int colour);
void DrawingSurface_DrawCircle(ScriptDrawingSurface *sds, int x, int y, int radius);
void DrawingSurface_DrawRectangle(ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2);
void DrawingSurface_DrawTriangle(ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2, int x3, int y3);
void DrawingSurface_DrawString(ScriptDrawingSurface *sds, int xx, int yy, int font, const char* texx, ...);
void DrawingSurface_DrawStringWrapped(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int alignment, const char *msg);
void DrawingSurface_DrawMessageWrapped(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int msgm);
void DrawingSurface_DrawLine(ScriptDrawingSurface *sds, int fromx, int fromy, int tox, int toy, int thickness);
void DrawingSurface_DrawPixel(ScriptDrawingSurface *sds, int x, int y);
int DrawingSurface_GetPixel(ScriptDrawingSurface *sds, int x, int y);


//-----------------------------------------------------------------------------
// DrawingSurface
//-----------------------------------------------------------------------------
struct ScriptDynamicSprite;
struct ScriptDrawingSurface;

void DynamicSprite_Delete(ScriptDynamicSprite *sds);
ScriptDrawingSurface* DynamicSprite_GetDrawingSurface(ScriptDynamicSprite *dss);
int DynamicSprite_GetGraphic(ScriptDynamicSprite *sds);
int DynamicSprite_GetWidth(ScriptDynamicSprite *sds);
int DynamicSprite_GetHeight(ScriptDynamicSprite *sds);
int DynamicSprite_GetColorDepth(ScriptDynamicSprite *sds);
void DynamicSprite_Resize(ScriptDynamicSprite *sds, int width, int height);
void DynamicSprite_Flip(ScriptDynamicSprite *sds, int direction);
void DynamicSprite_CopyTransparencyMask(ScriptDynamicSprite *sds, int sourceSprite);
void DynamicSprite_ChangeCanvasSize(ScriptDynamicSprite *sds, int width, int height, int x, int y);
void DynamicSprite_Crop(ScriptDynamicSprite *sds, int x1, int y1, int width, int height);
void DynamicSprite_Rotate(ScriptDynamicSprite *sds, int angle, int width, int height);
void DynamicSprite_Tint(ScriptDynamicSprite *sds, int red, int green, int blue, int saturation, int luminance);
int DynamicSprite_SaveToFile(ScriptDynamicSprite *sds, const char* namm);
ScriptDynamicSprite* DynamicSprite_CreateFromSaveGame(int sgslot, int width, int height);
ScriptDynamicSprite* DynamicSprite_CreateFromFile(const char *filename);
ScriptDynamicSprite* DynamicSprite_CreateFromScreenShot(int width, int height);
ScriptDynamicSprite* DynamicSprite_CreateFromExistingSprite(int slot, int preserveAlphaChannel);
ScriptDynamicSprite* DynamicSprite_CreateFromDrawingSurface(ScriptDrawingSurface *sds, int x, int y, int width, int height);
ScriptDynamicSprite* DynamicSprite_Create(int width, int height, int alphaChannel);
ScriptDynamicSprite* DynamicSprite_CreateFromExistingSprite_Old(int slot);
ScriptDynamicSprite* DynamicSprite_CreateFromBackground(int frame, int x1, int y1, int width, int height);


//-----------------------------------------------------------------------------
// File
//-----------------------------------------------------------------------------
struct sc_File;

int File_Exists(const char *fnmm);
int File_Delete(const char *fnmm);
void *sc_OpenFile(const char *fnmm, int mode);
void File_Close(sc_File *fil);
void File_WriteString(sc_File *fil, const char *towrite);
void File_WriteInt(sc_File *fil, int towrite);
void File_WriteRawChar(sc_File *fil, int towrite);
void File_WriteRawLine(sc_File *fil, const char *towrite);
void File_ReadRawLine(sc_File *fil, char* buffer);
const char* File_ReadRawLineBack(sc_File *fil);
void File_ReadString(sc_File *fil, char *toread);
const char* File_ReadStringBack(sc_File *fil);
int File_ReadInt(sc_File *fil);
int File_ReadRawChar(sc_File *fil);
int File_ReadRawInt(sc_File *fil);
int File_GetEOF(sc_File *fil);
int File_GetError(sc_File *fil);

//-----------------------------------------------------------------------------
// Game
//-----------------------------------------------------------------------------
struct ScriptViewFrame;

int Game_ChangeTranslation(const char *newFilename);
int Game_SetSaveGameDirectory(const char *newFolder);
int Game_GetInventoryItemCount();
int Game_GetFontCount();
int Game_GetMouseCursorCount();
int Game_GetCharacterCount();
int Game_GetGUICount();
int Game_GetViewCount();
int Game_GetUseNativeCoordinates();
int Game_GetSpriteWidth(int spriteNum);
int Game_GetSpriteHeight(int spriteNum);
int Game_GetLoopCountForView(int viewNumber);
int Game_GetRunNextSettingForLoop(int viewNumber, int loopNumber);
int Game_GetFrameCountForLoop(int viewNumber, int loopNumber);
ScriptViewFrame* Game_GetViewFrame(int viewNumber, int loopNumber, int frame);
int Game_DoOnceOnly(const char *token);
int Game_GetTextReadingSpeed();
void Game_SetTextReadingSpeed(int newTextSpeed);
int Game_GetMinimumTextDisplayTimeMs();
void Game_SetMinimumTextDisplayTimeMs(int newTextMinTime);
int Game_GetIgnoreUserInputAfterTextTimeoutMs();
void Game_SetIgnoreUserInputAfterTextTimeoutMs(int newValueMs);
const char *Game_GetFileName();
const char *Game_GetName();
void Game_SetName(const char *newName);
int Game_GetColorFromRGB(int red, int grn, int blu);
const char* Game_GetLocationName(int x, int y);
int Game_GetMODPattern();
const char* Game_GetSaveSlotDescription(int slnum);
const char* Game_InputBox(const char *msg);
void StopAllSounds(int evenAmbient);
int Game_GetDialogCount();
const char* Game_GetGlobalMessages(int index);
void SetGlobalString (int index, char *newval);
const char* Game_GetGlobalStrings(int index);
int Game_GetInSkippableCutscene();
int Game_GetSpeechFont();
int Game_GetNormalFont();
void SetSpeechFont (int fontnum);
void SetNormalFont (int fontnum);
int Game_GetSkippingCutscene();
const char* Game_GetTranslationFilename();


//-----------------------------------------------------------------------------
// Gui
//-----------------------------------------------------------------------------
struct ScriptGUI;
struct GUIObject;

void GUI_SetBackgroundGraphic(ScriptGUI *tehgui, int slotn);
int GUI_GetBackgroundGraphic(ScriptGUI *tehgui);
void GUI_Centre(ScriptGUI *sgui);
int GUI_GetControlCount(ScriptGUI *tehgui);
void GUI_SetTransparency(ScriptGUI *tehgui, int trans);
int GUI_GetTransparency(ScriptGUI *tehgui);
void GUI_SetVisible(ScriptGUI *tehgui, int isvisible);
int GUI_GetVisible(ScriptGUI *tehgui);
int GUI_GetX(ScriptGUI *tehgui);
void GUI_SetX(ScriptGUI *tehgui, int xx);
int GUI_GetY(ScriptGUI *tehgui);
void GUI_SetY(ScriptGUI *tehgui, int yy);
ScriptGUI *GetGUIAtLocation(int xx, int yy);
void GUI_SetPosition(ScriptGUI *tehgui, int xx, int yy);
void GUI_SetSize(ScriptGUI *sgui, int widd, int hitt);
int GUI_GetWidth(ScriptGUI *sgui);
int GUI_GetHeight(ScriptGUI *sgui);
void GUI_SetWidth(ScriptGUI *sgui, int newwid);
void GUI_SetHeight(ScriptGUI *sgui, int newhit);
int GUI_GetClickable(ScriptGUI *tehgui);
void SetGUIClickable(int guin, int clickable);
int GUI_GetID(ScriptGUI *tehgui);
GUIObject* GUI_GetiControls(ScriptGUI *tehgui, int idx);
void GUI_SetClickable(ScriptGUI *tehgui, int clickable);
void GUI_SetZOrder(ScriptGUI *tehgui, int z);
int GUI_GetZOrder(ScriptGUI *tehgui);


//-----------------------------------------------------------------------------
// GuiControl
//-----------------------------------------------------------------------------
struct GUIObject;
struct GUIButton;
struct GUIInv;
struct GUILabel;
struct GUIListBox;
struct GUISlider;
struct GUITextBox;

int GUIControl_GetX(GUIObject *guio);
void GUIControl_SetX(GUIObject *guio, int xx);
int GUIControl_GetY(GUIObject *guio);
void GUIControl_SetY(GUIObject *guio, int yy);
void GUIControl_SetPosition(GUIObject *guio, int xx, int yy);
int GUIControl_GetWidth(GUIObject *guio);
void GUIControl_SetWidth(GUIObject *guio, int newwid);
int GUIControl_GetHeight(GUIObject *guio);
void GUIControl_SetHeight(GUIObject *guio, int newhit);
void GUIControl_SetSize(GUIObject *guio, int newwid, int newhit);
void GUIControl_SendToBack(GUIObject *guio);
void GUIControl_BringToFront(GUIObject *guio);
GUIObject *GetGUIControlAtLocation(int xx, int yy);
GUIButton* GUIControl_GetAsButton(GUIObject *guio);
GUIInv* GUIControl_GetAsInvWindow(GUIObject *guio);
GUILabel* GUIControl_GetAsLabel(GUIObject *guio);
GUIListBox* GUIControl_GetAsListBox(GUIObject *guio);
GUISlider* GUIControl_GetAsSlider(GUIObject *guio);
GUITextBox* GUIControl_GetAsTextBox(GUIObject *guio);
int GUIControl_GetVisible(GUIObject *guio);
void GUIControl_SetVisible(GUIObject *guio, int visible);
int GUIControl_GetClickable(GUIObject *guio);
void GUIControl_SetClickable(GUIObject *guio, int enabled);
int GUIControl_GetEnabled(GUIObject *guio);
void GUIControl_SetEnabled(GUIObject *guio, int enabled);
int GUIControl_GetID(GUIObject *guio);
ScriptGUI* GUIControl_GetOwningGUI(GUIObject *guio);


//-----------------------------------------------------------------------------
// Hotspot
//-----------------------------------------------------------------------------
struct ScriptHotspot;

ScriptHotspot *GetHotspotAtLocation(int xx, int yy);
void Hotspot_GetName(ScriptHotspot *hss, char *buffer);
void Hotspot_GetPropertyText (ScriptHotspot *hss, const char *property, char *bufer);
const char* Hotspot_GetTextProperty(ScriptHotspot *hss, const char *property);
int Hotspot_GetProperty (ScriptHotspot *hss, const char *property);
void Hotspot_RunInteraction (ScriptHotspot *hss, int mood);
int Hotspot_GetWalkToX(ScriptHotspot *hss);
int Hotspot_GetWalkToY(ScriptHotspot *hss);
void Hotspot_SetEnabled(ScriptHotspot *hss, int newval);
int Hotspot_GetEnabled(ScriptHotspot *hss);
int Hotspot_GetID(ScriptHotspot *hss);
const char* Hotspot_GetName_New(ScriptHotspot *hss);


//-----------------------------------------------------------------------------
// InventoryItem
//-----------------------------------------------------------------------------
struct ScriptInvItem;

void InventoryItem_RunInteraction(ScriptInvItem *iitem, int mood);
ScriptInvItem *GetInvAtLocation(int xx, int yy);
void InventoryItem_GetName(ScriptInvItem *iitem, char *buff);
const char* InventoryItem_GetName_New(ScriptInvItem *invitem);
int InventoryItem_GetGraphic(ScriptInvItem *iitem);
void InventoryItem_SetName(ScriptInvItem *scii, const char *newname);
int InventoryItem_GetID(ScriptInvItem *scii);
void InventoryItem_SetGraphic(ScriptInvItem *iitem, int piccy);
void InventoryItem_SetCursorGraphic(ScriptInvItem *iitem, int newSprite);
int InventoryItem_GetCursorGraphic(ScriptInvItem *iitem);
int InventoryItem_GetProperty(ScriptInvItem *scii, const char *property);
void InventoryItem_GetPropertyText(ScriptInvItem *scii, const char *property, char *bufer);
int InventoryItem_CheckInteractionAvailable(ScriptInvItem *iitem, int mood);
const char* InventoryItem_GetTextProperty(ScriptInvItem *scii, const char *property);


//-----------------------------------------------------------------------------
// InvWindow
//-----------------------------------------------------------------------------
struct GUIInv;
struct ScriptInvItem;

void InvWindow_SetCharacterToUse(GUIInv *guii, CharacterInfo *chaa);
CharacterInfo* InvWindow_GetCharacterToUse(GUIInv *guii);
void InvWindow_SetItemWidth(GUIInv *guii, int newwidth);
int InvWindow_GetItemWidth(GUIInv *guii);
void InvWindow_SetItemHeight(GUIInv *guii, int newhit);
int InvWindow_GetItemHeight(GUIInv *guii);
void InvWindow_SetTopItem(GUIInv *guii, int topitem);
int InvWindow_GetTopItem(GUIInv *guii);
int InvWindow_GetItemsPerRow(GUIInv *guii);
int InvWindow_GetItemCount(GUIInv *guii);
int InvWindow_GetRowCount(GUIInv *guii);
void InvWindow_ScrollDown(GUIInv *guii);
void InvWindow_ScrollUp(GUIInv *guii);
ScriptInvItem* InvWindow_GetItemAtIndex(GUIInv *guii, int index);


//-----------------------------------------------------------------------------
// Label
//-----------------------------------------------------------------------------
struct GUILabel;

const char* Label_GetText_New(GUILabel *labl);
void Label_GetText(GUILabel *labl, char *buffer);
void Label_SetText(GUILabel *labl, const char *newtx);
int Label_GetColor(GUILabel *labl);
void Label_SetColor(GUILabel *labl, int colr);
int Label_GetFont(GUILabel *labl);
void Label_SetFont(GUILabel *guil, int fontnum);


//-----------------------------------------------------------------------------
// ListBox
//-----------------------------------------------------------------------------
struct GUIListBox;

int ListBox_AddItem(GUIListBox *lbb, const char *text);
int ListBox_InsertItemAt(GUIListBox *lbb, int index, const char *text);
void ListBox_Clear(GUIListBox *listbox);
void ListBox_FillDirList(GUIListBox *listbox, const char *filemask);
int ListBox_GetSaveGameSlots(GUIListBox *listbox, int index);
int ListBox_FillSaveGameList(GUIListBox *listbox);
int ListBox_GetItemAtLocation(GUIListBox *listbox, int x, int y);
char *ListBox_GetItemText(GUIListBox *listbox, int index, char *buffer);
const char* ListBox_GetItems(GUIListBox *listbox, int index);
void ListBox_SetItemText(GUIListBox *listbox, int index, const char *newtext);
void ListBox_RemoveItem(GUIListBox *listbox, int itemIndex);
int ListBox_GetItemCount(GUIListBox *listbox);
int ListBox_GetFont(GUIListBox *listbox);
void ListBox_SetFont(GUIListBox *listbox, int newfont);
int ListBox_GetHideBorder(GUIListBox *listbox);
void ListBox_SetHideBorder(GUIListBox *listbox, int newValue);
int ListBox_GetHideScrollArrows(GUIListBox *listbox);
void ListBox_SetHideScrollArrows(GUIListBox *listbox, int newValue);
int ListBox_GetSelectedIndex(GUIListBox *listbox);
void ListBox_SetSelectedIndex(GUIListBox *guisl, int newsel);
int ListBox_GetTopItem(GUIListBox *listbox);
void ListBox_SetTopItem(GUIListBox *guisl, int item);
int ListBox_GetRowCount(GUIListBox *listbox);
void ListBox_ScrollDown(GUIListBox *listbox);
void ListBox_ScrollUp(GUIListBox *listbox);


//-----------------------------------------------------------------------------
// Math
//-----------------------------------------------------------------------------
// unfortunately MSVC and GCC automatically push floats as doubles
// to functions, thus we need to manually access it as 32-bit
#define SCRIPT_FLOAT(x) long __script_float##x
#define FLOAT_RETURN_TYPE long

FLOAT_RETURN_TYPE Math_Cos(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_Sin(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_Tan(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_ArcCos(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_ArcSin(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_ArcTan(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_ArcTan2(SCRIPT_FLOAT(yval), SCRIPT_FLOAT(xval));
FLOAT_RETURN_TYPE Math_Log(SCRIPT_FLOAT(num));
FLOAT_RETURN_TYPE Math_Log10(SCRIPT_FLOAT(num));
FLOAT_RETURN_TYPE Math_Exp(SCRIPT_FLOAT(num));
FLOAT_RETURN_TYPE Math_Cosh(SCRIPT_FLOAT(num));
FLOAT_RETURN_TYPE Math_Sinh(SCRIPT_FLOAT(num));
FLOAT_RETURN_TYPE Math_Tanh(SCRIPT_FLOAT(num));
FLOAT_RETURN_TYPE Math_RaiseToPower(SCRIPT_FLOAT(base), SCRIPT_FLOAT(exp));
FLOAT_RETURN_TYPE Math_DegreesToRadians(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_RadiansToDegrees(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_GetPi();
FLOAT_RETURN_TYPE Math_Sqrt(SCRIPT_FLOAT(value));


//-----------------------------------------------------------------------------
// Mouse
//-----------------------------------------------------------------------------

void Mouse_ChangeModeView(int curs, int newview);
void ChangeCursorGraphic (int curs, int newslot);
int Mouse_GetModeGraphic(int curs);
void ChangeCursorHotspot (int curs, int x, int y);
void Mouse_ChangeModeView(int curs, int newview);
void set_cursor_mode(int newmode);
void enable_cursor_mode(int modd);
void disable_cursor_mode(int modd);
int IsButtonDown(int which);
void SaveCursorForLocationChange();
void SetNextCursor ();
void SetMouseBounds (int x1, int y1, int x2, int y2);
void SetMousePosition (int newx, int newy);
void RefreshMouse();
void set_mouse_cursor(int newcurs);
void set_default_cursor();
int GetCursorMode();
void Mouse_SetVisible(int isOn);
int Mouse_GetVisible();


//-----------------------------------------------------------------------------
// Object
//-----------------------------------------------------------------------------
struct ScriptObject;

void Object_SetClickable(ScriptObject *objj, int clik);
int Object_GetClickable(ScriptObject *objj);
void Object_SetIgnoreScaling(ScriptObject *objj, int newval);
int Object_GetIgnoreScaling(ScriptObject *objj);
void Object_SetSolid(ScriptObject *objj, int solid);
int Object_GetSolid(ScriptObject *objj);
void Object_SetBlockingWidth(ScriptObject *objj, int bwid);
int Object_GetBlockingWidth(ScriptObject *objj);
void Object_SetBlockingHeight(ScriptObject *objj, int bhit);
int Object_GetBlockingHeight(ScriptObject *objj);
void SetObjectIgnoreWalkbehinds (int cha, int clik);
int Object_GetID(ScriptObject *objj);
void Object_SetIgnoreWalkbehinds(ScriptObject *chaa, int clik);
int Object_GetIgnoreWalkbehinds(ScriptObject *chaa);
void Object_Animate(ScriptObject *objj, int loop, int delay, int repeat, int blocking, int direction);
void Object_StopAnimating(ScriptObject *objj);
int Object_IsCollidingWithObject(ScriptObject *objj, ScriptObject *obj2);
void Object_GetName(ScriptObject *objj, char *buffer);
int Object_GetProperty (ScriptObject *objj, const char *property);
void GetObjectPropertyText (int item, const char *property, char *bufer);
void Object_GetPropertyText(ScriptObject *objj, const char *property, char *bufer);
const char* Object_GetTextProperty(ScriptObject *objj, const char *property);
void Object_MergeIntoBackground(ScriptObject *objj);
void Object_StopMoving(ScriptObject *objj);
void ObjectOff(int obn);
void ObjectOn(int obn);
void Object_SetVisible(ScriptObject *objj, int onoroff);
void Object_Move(ScriptObject *objj, int x, int y, int speed, int blocking, int direct);
void Object_RemoveTint(ScriptObject *objj);
void Object_RunInteraction(ScriptObject *objj, int mode);
void Object_SetPosition(ScriptObject *objj, int xx, int yy);
void Object_SetX(ScriptObject *objj, int xx);
void Object_SetY(ScriptObject *objj, int yy);
void Object_SetView(ScriptObject *objj, int view, int loop, int frame);
void Object_Tint(ScriptObject *objj, int red, int green, int blue, int saturation, int luminance);
ScriptObject *GetObjectAtLocation(int xx, int yy);
int Object_GetAnimating(ScriptObject *objj);
int Object_GetBaseline(ScriptObject *objj);
void Object_SetBaseline(ScriptObject *objj, int basel);
int Object_GetBaseline(ScriptObject *objj);
int Object_GetGraphic(ScriptObject *objj);
void Object_SetGraphic(ScriptObject *objj, int slott);
int Object_GetView(ScriptObject *objj);
int Object_GetLoop(ScriptObject *objj);
int Object_GetFrame(ScriptObject *objj);
int Object_GetMoving(ScriptObject *objj);
const char* Object_GetName_New(ScriptObject *objj);
void Object_SetTransparency(ScriptObject *objj, int trans);
int Object_GetTransparency(ScriptObject *objj);
int Object_GetVisible(ScriptObject *objj);
int Object_GetX(ScriptObject *objj);
int Object_GetY(ScriptObject *objj);


//-----------------------------------------------------------------------------
// Overlay
//-----------------------------------------------------------------------------
struct ScriptOverlay;

void Overlay_Remove(ScriptOverlay *sco);
int CreateGraphicOverlay(int xx,int yy,int slott,int trans);
int CreateTextOverlayCore(int xx, int yy, int wii, int fontid, int clr, const char *tex, int allowShrink);
int CreateTextOverlay(int xx,int yy,int wii,int fontid,int clr,char*texx, ...);
void SetTextOverlay(int ovrid,int xx,int yy,int wii,int fontid,int clr,char*texx,...);
void Overlay_SetText(ScriptOverlay *scover, int wii, int fontid, int clr, char*texx, ...);
int Overlay_GetX(ScriptOverlay *scover);
void Overlay_SetX(ScriptOverlay *scover, int newx);
int Overlay_GetY(ScriptOverlay *scover);
void Overlay_SetY(ScriptOverlay *scover, int newy);
int Overlay_GetValid(ScriptOverlay *scover);
ScriptOverlay* Overlay_CreateGraphical(int x, int y, int slot, int transparent);
ScriptOverlay* Overlay_CreateTextual(int x, int y, int width, int font, int colour, const char* text, ...);


//-----------------------------------------------------------------------------
// Parser
//-----------------------------------------------------------------------------

const char* Parser_SaidUnknownWord();
void ParseText (char*text);
int Parser_FindWordID(const char *wordToFind);
int Said (char*checkwords);


//-----------------------------------------------------------------------------
// Region
//-----------------------------------------------------------------------------
struct ScriptRegion;

int Region_GetTintEnabled(ScriptRegion *srr);
int Region_GetTintRed(ScriptRegion *srr);
int Region_GetTintGreen(ScriptRegion *srr);
int Region_GetTintBlue(ScriptRegion *srr);
int Region_GetTintSaturation(ScriptRegion *srr);
void Region_Tint(ScriptRegion *srr, int red, int green, int blue, int amount);
ScriptRegion *GetRegionAtLocation(int xx, int yy);
void Region_RunInteraction(ScriptRegion *ssr, int mood);
void Region_SetEnabled(ScriptRegion *ssr, int enable);
int Region_GetEnabled(ScriptRegion *ssr);
int Region_GetID(ScriptRegion *ssr);
void Region_SetLightLevel(ScriptRegion *ssr, int brightness);
int Region_GetLightLevel(ScriptRegion *ssr);


//-----------------------------------------------------------------------------
// Room
//-----------------------------------------------------------------------------
struct ScriptDrawingSurface;

ScriptDrawingSurface* Room_GetDrawingSurfaceForBackground(int backgroundNumber);
int Room_GetObjectCount();
int Room_GetWidth();
int Room_GetHeight();
int Room_GetColorDepth();
int Room_GetLeftEdge();
int Room_GetRightEdge();
int Room_GetTopEdge();
int Room_GetBottomEdge();
int Room_GetMusicOnLoad();
const char* Room_GetTextProperty(const char *property);
const char* Room_GetMessages(int index);


//-----------------------------------------------------------------------------
// Slider
//-----------------------------------------------------------------------------
struct GUISlider;

void Slider_SetMax(GUISlider *guisl, int valn);
int Slider_GetMax(GUISlider *guisl);
void Slider_SetMin(GUISlider *guisl, int valn);
int Slider_GetMin(GUISlider *guisl);
void Slider_SetValue(GUISlider *guisl, int valn);
int Slider_GetValue(GUISlider *guisl);
void SetSliderValue(int guin,int objn, int valn);
int GetSliderValue(int guin,int objn);
int Slider_GetBackgroundGraphic(GUISlider *guisl);
void Slider_SetBackgroundGraphic(GUISlider *guisl, int newImage);
int Slider_GetHandleGraphic(GUISlider *guisl);
void Slider_SetHandleGraphic(GUISlider *guisl, int newImage);
int Slider_GetHandleOffset(GUISlider *guisl);
void Slider_SetHandleOffset(GUISlider *guisl, int newOffset);


//-----------------------------------------------------------------------------
// String
//-----------------------------------------------------------------------------

int String_IsNullOrEmpty(const char *thisString);
const char* String_Copy(const char *srcString);
const char* String_Append(const char *thisString, const char *extrabit);
const char* String_AppendChar(const char *thisString, char extraOne);
const char* String_ReplaceCharAt(const char *thisString, int index, char newChar);
const char* String_Truncate(const char *thisString, int length);
const char* String_Substring(const char *thisString, int index, int length);
int String_CompareTo(const char *thisString, const char *otherString, bool caseSensitive);
int String_StartsWith(const char *thisString, const char *checkForString, bool caseSensitive);
int String_EndsWith(const char *thisString, const char *checkForString, bool caseSensitive);
const char* String_Replace(const char *thisString, const char *lookForText, const char *replaceWithText, bool caseSensitive);
const char* String_LowerCase(const char *thisString);
const char* String_UpperCase(const char *thisString);
const char* String_Format(const char *texx, ...);
int String_GetChars(const char *texx, int index);
int StrContains (const char *s1, const char *s2);
FLOAT_RETURN_TYPE StringToFloat(const char *theString);
int StringToInt(char*stino);


//-----------------------------------------------------------------------------
// System
//-----------------------------------------------------------------------------

int System_GetColorDepth();
int System_GetOS();
int System_GetScreenWidth();
int System_GetScreenHeight();
int System_GetViewportHeight();
int System_GetViewportWidth();
const char *System_GetVersion();
int System_GetHardwareAcceleration();
int System_GetNumLock();
int System_GetCapsLock();
int System_GetScrollLock();
void System_SetNumLock(int newValue);
int System_GetVsync();
void System_SetVsync(int newValue);
int System_GetWindowed();
int System_GetSupportsGammaControl();
int System_GetGamma();
void System_SetGamma(int newValue);
int System_GetVolume();
void System_SetVolume(int newvol);


//-----------------------------------------------------------------------------
// TextBox
//-----------------------------------------------------------------------------
struct GUITextBox;

const char* TextBox_GetText_New(GUITextBox *texbox);
void TextBox_GetText(GUITextBox *texbox, char *buffer);
void TextBox_SetText(GUITextBox *texbox, const char *newtex);
int TextBox_GetTextColor(GUITextBox *guit);
void TextBox_SetTextColor(GUITextBox *guit, int colr);
int TextBox_GetFont(GUITextBox *guit);
void TextBox_SetFont(GUITextBox *guit, int fontnum);


//-----------------------------------------------------------------------------
// ViewFrame
//-----------------------------------------------------------------------------
struct ScriptViewFrame;

int ViewFrame_GetFlipped(ScriptViewFrame *svf);
int ViewFrame_GetGraphic(ScriptViewFrame *svf);
void ViewFrame_SetGraphic(ScriptViewFrame *svf, int newPic);
ScriptAudioClip* ViewFrame_GetLinkedAudio(ScriptViewFrame *svf);
void ViewFrame_SetLinkedAudio(ScriptViewFrame *svf, ScriptAudioClip* clip);
int ViewFrame_GetSound(ScriptViewFrame *svf);
void ViewFrame_SetSound(ScriptViewFrame *svf, int newSound);
int ViewFrame_GetSpeed(ScriptViewFrame *svf);
int ViewFrame_GetView(ScriptViewFrame *svf);
int ViewFrame_GetLoop(ScriptViewFrame *svf);
int ViewFrame_GetFrame(ScriptViewFrame *svf);


//-----------------------------------------------------------------------------
// globals
//
// Many of the following are backwards-compatibility functions
//-----------------------------------------------------------------------------

namespace AGS { namespace Common {
    class DataStream; 
    enum FileOpenMode;
    enum FileWorkMode;
}}
using namespace AGS; // FIXME later

void _sc_AbortGame(char*texx, ...);
void SetActiveInventory(int iit);
void add_inventory(int inum);
void lose_inventory(int inum);
void AddInventoryToCharacter(int charid, int inum);
void LoseInventoryFromCharacter(int charid, int inum);
void AnimateButton(int guin, int objn, int view, int loop, int speed, int repeat);
void scAnimateCharacter (int chh, int loopn, int sppd, int rept);
void AnimateCharacterEx(int chh, int loopn, int sppd, int rept, int direction, int blocking);
void AnimateObject(int obn,int loopn,int spdd,int rept);
void AnimateObjectEx(int obn,int loopn,int spdd,int rept, int direction, int blocking);
int AreCharObjColliding(int charid,int objid);
int AreCharactersColliding(int cchar1,int cchar2);
int AreObjectsColliding(int obj1,int obj2);
int AreThingsOverlapping(int thing1, int thing2);
int HasPlayerBeenInRoom(int roomnum);
void CallRoomScript (int value);
Common::DataStream * FileOpen(const char*fnmm, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode);
void FileClose(Common::DataStream*hha);
void FileWrite(Common::DataStream*haa, const char *towrite);
void FileWriteRawLine(Common::DataStream*haa, const char*towrite);
void FileRead(Common::DataStream*haa,char*toread);
int FileIsEOF (Common::DataStream *haa);
int FileIsError(Common::DataStream *haa);
void FileWriteInt(Common::DataStream*haa,int into);
int FileReadInt(Common::DataStream*haa);
char FileReadRawChar(Common::DataStream*haa);
int FileReadRawInt(Common::DataStream*haa);
void FileWriteRawChar(Common::DataStream *haa, int chartoWrite);
void FaceLocation(int cha, int xx, int yy);
void FaceCharacter(int cha,int toface);
void FadeIn(int sppd);
int FloatToInt(SCRIPT_FLOAT(value), int roundDirection);
void SetGameSpeed(int newspd);
int GetGameSpeed();
int SetGameOption (int opt, int setting);
int GetGameOption (int opt);
int GetGameParameter (int parm, int data1, int data2, int data3);
int cd_manager(int cmdd,int datt) ;
void DisableGroundLevelAreas(int alsoEffects);
void DisableHotspot(int hsnum);
void EnableHotspot(int hsnum);
void SetCharacterViewEx (int chaa, int vii, int loop, int align);
void SetCharacterViewOffset (int chaa, int vii, int xoffs, int yoffs);
void ChangeCharacterView(int chaa,int vii);
void SetCharacterClickable (int cha, int clik);
void SetCharacterIgnoreWalkbehinds (int cha, int clik);
void CentreGUI (int ifn);
int GetTextWidth(char *text, int fontnum);
int GetTextHeight(char *text, int fontnum, int width);
int GetRegionAt (int xxx, int yyy);
int GetWalkableAreaAt(int xxx,int yyy);
void GiveScore(int amnt);
void ClaimEvent();
void CyclePalette(int strt,int eend);
void script_debug(int cmdd,int dataa);
void DeleteSaveSlot (int slnum);
void free_dynamic_sprite (int gotSlot);
void DisableInterface();
void EnableInterface();
void DisableRegion(int hsnum);
void EnableRegion(int hsnum);
void DisplayAt(int xxp,int yyp,int widd,char*texx, ...);
void DisplayAtY (int ypos, char *texx);
void Display(char*texx, ...);
void DisplayMessageAtY(int msnum, int ypos);
void DisplayMessage(int msnum);
void DisplayMessageBar(int ypos, int ttexcol, int backcol, char *title, int msgnum);
void __sc_displayspeech(int chid,char*texx, ...);
void DisplaySpeechAt (int xx, int yy, int wii, int aschar, char*spch);
int DisplaySpeechBackground(int charid,char*speel);
void DisplayThought(int chid, const char*texx, ...);
void DisplayTopBar(int ypos, int ttexcol, int backcol, char *title, char*texx, ...);
void DisableGroundLevelAreas(int alsoEffects);
void EnableGroundLevelAreas(); 
void StartCutscene (int skipwith);
int EndCutscene ();
void my_fade_out(int spdd);
int FindGUIID (const char* GUIName);
void FlipScreen(int amount);
void FollowCharacterEx(int who, int tofollow, int distaway, int eagerness);
void FollowCharacter(int who, int tofollow);
void SetBackgroundFrame(int frnum);
int GetBackgroundFrame() ;
int GetButtonPic(int guin, int objn, int ptype);
void SetButtonPic(int guin,int objn,int ptype,int slotn);
int GetCharacterAt (int xx, int yy);
int GetCharacterProperty (int cha, const char *property);
void GetCharacterPropertyText (int item, const char *property, char *bufer);
int GetCurrentMusic();
void SetDialogOption(int dlg,int opt,int onoroff);
int GetDialogOption (int dlg, int opt);
void SetGlobalInt(int index,int valu);
int GetGlobalInt(int index);
void SetGlobalString (int index, char *newval);
void GetGlobalString (int index, char *strval);
int GetGraphicalVariable (const char *varName);
void SetGraphicalVariable (const char *varName, int p_value);
int GetGUIAt (int xx,int yy);
int GetGUIObjectAt (int xx, int yy);
int GetHotspotAt(int xxx,int yyy);
void GetHotspotName(int hotspot, char *buffer);
int GetHotspotPointX (int hotspot);
int GetHotspotPointY (int hotspot);
int GetHotspotProperty (int hss, const char *property);
void GetHotspotPropertyText (int item, const char *property, char *bufer);
int GetInvAt (int xxx, int yyy);
int GetInvGraphic(int indx);
void GetInvName(int indx,char*buff);
int GetInvProperty (int item, const char *property);
void GetInvPropertyText (int item, const char *property, char *bufer);
void GetLocationName(int xxx,int yyy,char*tempo);
int GetLocationType(int xxx,int yyy);
void GetMessageText (int msg, char *buffer);
int GetMIDIPosition ();
int GetMP3PosMillis ();
int GetObjectAt(int xx,int yy);
int GetObjectBaseline(int obn);
int GetObjectGraphic(int obn);
void GetObjectName(int obj, char *buffer);
int GetObjectProperty (int hss, const char *property);
int GetObjectX (int objj);
int GetObjectY (int objj);
int GetPlayerCharacter();
int GetRawTime ();
int GetRoomProperty (const char *property);
void GetRoomPropertyText (const char *property, char *bufer);
int GetSaveSlotDescription(int slnum,char*desbuf);
int GetScalingAt (int x, int y);
void GetTextBoxText(int guin, int objn, char*txbuf);
void SetTextBoxText(int guin, int objn, char*txbuf);
int sc_GetTime(int whatti) ;
char *get_translation (const char *text);
int GetTranslationName (char* buffer);
int GetViewportX ();
int GetViewportY ();
void HideMouseCursor ();
void ShowMouseCursor ();
void sc_inputbox(const char*msg,char*bufr);
void InterfaceOn(int ifn);
void InterfaceOff(int ifn);
FLOAT_RETURN_TYPE IntToFloat(int value);
void sc_invscreen();
int IsChannelPlaying(int chan);
int IsGamePaused();
int IsGUIOn (int guinum);
int IsInteractionAvailable (int xx,int yy,int mood);
int IsInventoryInteractionAvailable (int item, int mood);
int IsKeyPressed (int keycode);
int IsMusicPlaying();
int IsMusicVoxAvailable ();
int IsObjectAnimating(int objj);
int IsObjectMoving(int objj);
int IsObjectOn (int objj);
int IsOverlayValid(int ovrid);
int IsSoundPlaying();
int IsTimerExpired(int tnum);
int IsTranslationAvailable ();
int IsVoxAvailable();
void ListBoxAdd(int guin, int objn, const char*newitem);
void ListBoxRemove(int guin, int objn, int itemIndex);
int ListBoxGetSelected(int guin, int objn);
int ListBoxGetNumItems(int guin, int objn);
char* ListBoxGetItemText(int guin, int objn, int item, char*buffer);
void ListBoxSetSelected(int guin, int objn, int newsel);
void ListBoxSetTopItem (int guin, int objn, int item);
int ListBoxSaveGameList (int guin, int objn);
void ListBoxDirList (int guin, int objn, const char*filemask);
int LoadImageFile(const char *filename);
int LoadSaveSlotScreenshot(int slnum, int width, int height);
void MergeObject(int obn);
void MoveCharacterBlocking(int chaa,int xx,int yy,int direct);
void MoveCharacterToObject(int chaa,int obbj);
void MoveCharacterToHotspot(int chaa,int hotsp);
void MoveCharacterDirect(int cc,int xx, int yy);
void MoveCharacterStraight(int cc,int xx, int yy);
void MoveCharacter(int cc,int xx,int yy);
void MoveCharacterPath (int chac, int tox, int toy);
void MoveObject(int objj,int xx,int yy,int spp);
void MoveObjectDirect(int objj,int xx,int yy,int spp);
void MoveOverlay(int ovrid, int newx,int newy);
void MoveToWalkableArea(int charid);
int IsInterfaceEnabled();
void ListBoxClear(int guin, int objn);
void NewRoom(int nrnum);
void NewRoomEx(int nrnum,int newx,int newy);
void NewRoomNPC(int charid, int nrnum, int newx, int newy);
void ResetRoom(int nrnum);
void PauseGame();
void UnPauseGame();
void PlayAmbientSound (int channel, int sndnum, int vol, int x, int y);
void play_flc_file(int numb,int playflags);
void PlayMP3File (char *filename);
void PlaySilentMIDI (int mnum);
void PlayMusicResetQueue(int newmus);
void SeekMIDIPosition (int position);
int GetMIDIPosition ();
int play_sound(int val1);
int PlaySoundEx(int val1, int channel);
int PlayMusicQueued(int musnum);
void __scr_play_speech(int who, int which);
void scrPlayVideo(const char* name, int skip, int flags);
void ProcessClick(int xx,int yy,int mood);
void QuitGame(int dialog);
int __Rand(int upto);
void RawSaveScreen ();
void RawRestoreScreen();
void RawRestoreScreenTinted(int red, int green, int blue, int opacity);
void RawDrawFrameTransparent (int frame, int translev);
void RawClear (int clr);
void RawSetColor (int clr);
void RawSetColorRGB(int red, int grn, int blu);
void RawPrint (int xx, int yy, char*texx, ...);
void RawPrintMessageWrapped (int xx, int yy, int wid, int font, int msgm);
void RawDrawImageCore(int xx, int yy, int slot);
void RawDrawImage(int xx, int yy, int slot);
void RawDrawImageOffset(int xx, int yy, int slot);
void RawDrawImageTransparent(int xx, int yy, int slot, int trans);
void RawDrawImageResized(int xx, int yy, int gotSlot, int width, int height);
void RawDrawLine (int fromx, int fromy, int tox, int toy);
void RawDrawCircle (int xx, int yy, int rad);
void RawDrawRectangle(int x1, int y1, int x2, int y2);
void RawDrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3);
void CyclePalette(int strt,int eend);
void SetPalRGB(int inndx,int rr,int gg,int bb);
void UpdatePalette();
void TintScreen(int red, int grn, int blu);
void ReleaseCharacterView(int chat);
void SetViewport(int offsx,int offsy);
void ReleaseViewport();
void RemoveObjectTint(int obj);
void RemoveOverlay(int ovrid);
void RemoveWalkableArea(int areanum);
void RestoreWalkableArea(int areanum);
void restart_game();
void set_game_speed(int fps);
void setup_for_dialog();
void restore_after_dialog();
void RestoreGameSlot(int slnum);
int RunAGSGame (char *newgame, unsigned int mode, int data);
void RunCharacterInteraction (int cc, int mood);
void RunDialog(int tum);
void SetDialogOption(int dlg,int opt,int onoroff);
void RunHotspotInteraction (int hotspothere, int mood);
void RunRegionInteraction (int regnum, int mood);
void RunCharacterInteraction (int cc, int mood);
void RunObjectInteraction (int aa, int mood);
void RunInventoryInteraction (int iit, int modd);
int SaidUnknownWord (char*buffer);
void restore_game_dialog();
void save_game_dialog();
void save_game(int slotn, const char*descript);
int SaveScreenShot(char*namm);
void SeekMODPattern(int patnum);
void SeekMP3PosMillis (int posn);
void SetMusicRepeat(int loopflag);
void SetMusicVolume(int newvol);
void SetMusicMasterVolume(int newvol);
void SetSoundVolume(int newvol);
void SetChannelVolume(int chan, int newvol);
void SetDigitalMasterVolume (int newvol);
void SetAmbientTint (int red, int green, int blue, int opacity, int luminance);
void SetAreaLightLevel(int area, int brightness);
void SetRegionTint (int area, int red, int green, int blue, int amount);
void SetAreaScaling(int area, int min, int max);
void SetButtonText(int guin,int objn,char*newtx);
void SetCharacterBaseline (int obn, int basel);
void SetCharacterTransparency(int obn,int trans);
void SetPlayerCharacter(int newchar);
void SetCharacterSpeedEx(int chaa, int xspeed, int yspeed);
void SetCharacterSpeed(int chaa,int nspeed);
void SetTalkingColor(int chaa,int ncol);
void SetCharacterSpeechView (int chaa, int vii);
void SetCharacterBlinkView (int chaa, int vii, int intrv);
void SetCharacterView(int chaa,int vii);
void SetCharacterFrame(int chaa, int view, int loop, int frame);
void SetCharacterViewEx (int chaa, int vii, int loop, int align);
void SetCharacterViewOffset (int chaa, int vii, int xoffs, int yoffs);
void ChangeCharacterView(int chaa,int vii);
void SetCharacterClickable (int cha, int clik);
void SetCharacterIgnoreWalkbehinds (int cha, int clik);
void SetCharacterIdle(int who, int iview, int itime);
void SetCharacterIgnoreLight (int who, int yesorno);
void SetCharacterProperty (int who, int flag, int yesorno);
void SetScreenTransition(int newtrans);
void SetNextScreenTransition(int newtrans);
void SetFadeColor(int red, int green, int blue);
void SetFrameSound (int vii, int loop, int frame, int sound);
void SetGUIBackgroundPic (int guin, int slotn);
void SetGUIClickable(int guin, int clickable);
void SetGUIZOrder(int guin, int z);
void SetGUISize (int ifn, int widd, int hitt);
void SetGUIPosition(int ifn,int xx,int yy);
void SetGUIObjectSize(int ifn, int objn, int newwid, int newhit);
void SetGUIObjectEnabled(int guin, int objn, int enabled);
void SetGUIObjectPosition(int guin, int objn, int xx, int yy);
void SetInvItemName(int invi, const char *newName);
void set_inv_item_pic(int invi, int piccy);
void SetInvDimensions(int ww,int hh);
void SetLabelColor(int guin,int objn, int colr);
void SetLabelText(int guin,int objn,char*newtx);
void SetLabelFont(int guin,int objn, int fontnum);
void SetGUITransparency(int ifn, int trans);
void SetMultitasking (int mode);
void SetObjectBaseline (int obn, int basel);
void SetObjectView(int obn,int vii);
void SetObjectFrame(int obn,int viw,int lop,int fra);
void SetObjectTint(int obj, int red, int green, int blue, int opacity, int luminance);
void SetObjectTransparency(int obn,int trans);
void StopObjectMoving(int objj);
void SetObjectGraphic(int obn,int slott);
void SetObjectPosition(int objj, int tox, int toy);
void SetObjectClickable (int cha, int clik);
void SetRestartPoint();
void SetSkipSpeech (int newval);
void SetSpeechStyle (int newstyle);
void SetSpeechVolume(int newvol);
void SetTextBoxFont(int guin,int objn, int fontnum);
void SetTextWindowGUI (int guinum);
void script_SetTimer(int tnum,int timeout);
void SetVoiceMode (int newmod);
void SetWalkBehindBase(int wa,int bl);
void ShakeScreen(int severe);
void ShakeScreenBackground (int delay, int amount, int length);
void SkipUntilCharacterStops(int cc);
void scStartRecording (int keyToStop);
void StopAmbientSound (int channel);
void stop_and_destroy_channel (int chid);
void StopDialog();
void StopMoving(int chaa);
void scr_StopMusic();
void _sc_strcat(char*s1,char*s2);
void _sc_strcpy(char*s1,char*s2);
void _sc_sprintf(char*destt,char*texx, ...);
void _sc_strlower (char *desbuf);
void _sc_strupper (char *desbuf);
void update_invorder();
int StrGetCharAt (char *strin, int posn);
void StrSetCharAt (char *strin, int posn, int nchar);
void scrWait(int nloops);
int WaitKey(int nloops);
int WaitMouseKey(int nloops);


struct GameState;
extern GameState play;
struct ScriptMouse;
extern ScriptMouse scmouse;
struct RGB;
extern RGB palette[256];
struct ScriptSystem;
extern ScriptSystem scsystem;


//-----------------------------------------------------------------------------
// Built-in plugins
//
// Stubs for plugin functions.
//-----------------------------------------------------------------------------

void PluginSimulateMouseClick(int pluginButtonID);
void ScriptStub_ShellExecute();
void srSetSnowDriftRange(int min_value, int max_value);
void srSetSnowDriftSpeed(int min_value, int max_value);
void srSetSnowFallSpeed(int min_value, int max_value);
void srChangeSnowAmount(int amount);
void srSetSnowBaseline(int top, int bottom);
void srSetSnowTransparency(int min_value, int max_value);
void srSetSnowDefaultView(int view, int loop);
void srSetSnowWindSpeed(int value);
void srSetSnowAmount(int amount);
void srSetSnowView(int kind_id, int event, int view, int loop);
void srChangeRainAmount(int amount);
void srSetRainView(int kind_id, int event, int view, int loop);
void srSetRainDefaultView(int view, int loop);
void srSetRainTransparency(int min_value, int max_value);
void srSetRainWindSpeed(int value);
void srSetRainBaseline(int top, int bottom);
void srSetRainAmount(int amount);
void srSetRainFallSpeed(int min_value, int max_value);
void srSetWindSpeed(int value);
void srSetBaseline(int top, int bottom);
int JoystickCount();
int Joystick_Open(int a);
int Joystick_IsButtonDown(int a);
void Joystick_EnableEvents(int a);
void Joystick_DisableEvents();
void Joystick_Click(int a);
int Joystick_Valid();
int Joystick_Unplugged();
int DrawAlpha(int destination, int sprite, int x, int y, int transparency);
int GetAlpha(int sprite, int x, int y);
int PutAlpha(int sprite, int x, int y, int alpha);
int Blur(int sprite, int radius);
int HighPass(int sprite, int threshold);
int DrawAdd(int destination, int sprite, int x, int y, float scale);
int GetFlashlightInt();
void SetFlashlightInt1(int Param1);
void SetFlashlightInt2(int Param1, int Param2);
void SetFlashlightInt3(int Param1, int Param2, int Param3);
void SetFlashlightInt5(int Param1, int Param2, int Param3, int Param4, int Param5);

#endif // __AGS_EE_SCRIPT__SYMBOLREGISTRY_H
