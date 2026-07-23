//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_TOOL_DATA__GAMEUTIL_H
#define __AGS_TOOL_DATA__GAMEUTIL_H

#include <memory>
#include <map>
#include <vector>
#include "ac/gamestructdefines.h"
#include "ac/spritefile.h"
#include "game/customproperties.h"
#include "gui/guidefines.h"
#include "util/geometry.h"
#include "util/string.h"

namespace AGS
{
namespace DataUtil
{

using AGS::Common::String;
using AGS::Common::GuiDisableStyle;
using AGS::Common::PropertyType;
using AGS::Common::SpriteCompression;
using ::DialogOptionNumbering;
using ::GameGuiAlphaRenderingStyle;
using ::GameSpriteAlphaRenderingStyle;
using ::RenderAtScreenRes;
using ::ScreenTransitionStyle;

enum GameColorDepth
{
    kGameColorDepth_Palette = 1,
    kGameColorDepth_HighColor = 2,
    kGameColorDepth_TrueColor = 4
};

enum AndroidBuildFormat
{
    kAndroidBuild_ApkEmbedded = 0,
    kAndroidBuild_AabEmbedded = 1,
    kAndroidBuild_Aab = 2
};

enum FontHeightDefinition
{
    kFontHeight_NominalHeight = 0,
    kFontHeight_PixelHeight = 1,
    kFontHeight_CustomValue = 2
};

enum FontMetricsFixup
{
    kFontMetrics_None = 0,
    kFontMetrics_SetAscenderToHeight = 1
};

enum GUIPopupStyle
{
    kGUIPopupStyle_Normal = 0,
    kGUIPopupStyle_MouseYPos = 1,
    kGUIPopupStyle_PopupModal = 2,
    kGUIPopupStyle_Persistent = 3
};

enum PaletteColourType
{
    kPaletteColourType_Gamewide = 0,
    kPaletteColourType_Background = 2
};

enum SkipSpeechStyle
{
    kSkipSpeech_MouseOrKeyboardOrTimer = 0,
    kSkipSpeech_KeyboardOrTimer = 1,
    kSkipSpeech_TimerOnly = 2,
    kSkipSpeech_MouseOrKeyboard = 3,
    kSkipSpeech_MouseOrTimer = 4,
    kSkipSpeech_KeyboardOnly = 5,
    kSkipSpeech_MouseOnly = 6
};

enum SpeechPortraitSide
{
    kSpeechPortrait_Left = 0,
    kSpeechPortrait_Right = 1,
    kSpeechPortrait_Alternate = 2,
    kSpeechPortrait_BasedOnCharacterPosition = 3
};

enum SpeechStyle
{
    kSpeechStyle_Lucasarts = 0,
    kSpeechStyle_SierraTransparent = 1,
    kSpeechStyle_SierraWithBackground = 2,
    kSpeechStyle_WholeScreen = 3
};

enum InventoryHotspotMarkerStyle
{
    kInventoryHotspot_None = 0,
    kInventoryHotspot_Crosshair = 1,
    kInventoryHotspot_Sprite = 2
};

// EntityRef is a parent struct for a game object data;
// contains common fields such as a numeric ID (aka index) and script name.
struct EntityRef
{
    String TypeName; // name of type, for the reference when necessary
    int ID = -1;
    String ScriptName;
};

typedef EntityRef CharacterRef;

struct CustomPropertyValue
{
    String Name;
    String Value;
};

enum FontOutlineStyle
{
    kFontOutline_None = 0,
    kFontOutline_Automatic = 1,
    kFontOutline_UseOutlineFont = 2
};

enum FontAutoOutlineStyle
{
    kFontAutoOutline_Squared = 0,
    kFontAutoOutline_Rounded = 1
};

enum SpriteImportResolution
{
    kSpriteImport_Real = -1,
    kSpriteImport_LowRes = 0,
    kSpriteImport_HighRes = 1
};

enum AudioFileBundlingType
{
    kAudioBundling_InGameEXE = 1,
    kAudioBundling_InSeparateVOX = 2
};

enum AudioClipFileType
{
    kAudioFile_Undefined = 0,
    kAudioFile_OGG = 1,
    kAudioFile_MP3 = 2,
    kAudioFile_WAV = 3,
    kAudioFile_VOC = 4,
    kAudioFile_MIDI = 5,
    kAudioFile_MOD = 6
};

enum CrossfadeSpeed
{
    kCrossfade_No = 0,
    kCrossfade_Slow = 1,
    kCrossfade_Slowish = 2,
    kCrossfade_Medium = 3,
    kCrossfade_Fast = 4
};

enum ButtonColorStyle
{
    kButtonColor_Default = 0,
    kButtonColor_Dynamic = 1,
    kButtonColor_DynamicFlat = 2
};

enum CharacterTurnOrderPriority
{
    kTurnOrder_Clockwise = 0,
    kTurnOrder_CounterClockwise = 1,
    kTurnOrder_Random = 2,
    kTurnOrder_FaceDown = 3
};

enum GUITextBoxKeyClaimStyle
{
    kTextBoxKeyClaim_All = 0,
    kTextBoxKeyClaim_Handled = 1,
    kTextBoxKeyClaim_TextOnly = 2
};

enum LipSyncType
{
    kLipSync_None = 0,
    kLipSync_Text = 1,
    kLipSync_PamelaVoiceFiles = 2,
};

struct CharacterData : EntityRef
{
    bool AdjustSpeedWithScaling{};
    bool AdjustVolumeWithScaling{};
    int AnimationDelay{};
    int Baseline{};
    int BlinkingView{};
    int BlockingX{};
    int BlockingY{};
    int BlockingWidth{};
    int BlockingHeight{};
    bool Clickable{};
    bool DiagonalLoops{};
    int IdleAnimationDelay{};
    int IdleDelay{};
    int IdleView{};
    bool MovementLinkedToAnimation{};
    int MovementSpeed{};
    int MovementSpeedX{};
    int MovementSpeedY{};
    int NormalView{};
    String RealName;
    bool Solid{};
    int SpeechAnimationDelay{};
    int SpeechColor{};
    int SpeechView{};
    int StartX{};
    int StartY{};
    int StartingRoom{};
    int ThinkingView{};
    int Transparency{};
    bool TurnBeforeWalking{};
    bool TurnWhenFacing{};
    bool UniformMovementSpeed{};
    bool UseRoomAreaLighting{};
    bool UseRoomAreaScaling{};
    String ScriptModule;
    std::vector<String> InteractionEvents;
    std::vector<CustomPropertyValue> Properties;
};

struct ViewFrameData
{
    int Delay{};
    bool Flipped{};
    int Image{};
    int Sound{}; // stable audio clip index in the AGF
};

struct ViewLoopData
{
    bool RunNextLoop{};
    std::vector<ViewFrameData> Frames;
};

struct ViewData : EntityRef
{
    std::vector<ViewLoopData> Loops;
};

struct FontData : EntityRef
{
    // NOTE: We are using the ScriptName property as the ScriptID from Font.cs
    String Name;
    int AutoOutlineThickness{};
    FontAutoOutlineStyle AutoOutlineStyle = kFontAutoOutline_Squared;
    int CharacterSpacing{};
    int CustomHeightValue{};
    FontHeightDefinition HeightDefinedBy = kFontHeight_NominalHeight;
    int LineSpacing{};
    int OutlineFont{};
    FontOutlineStyle OutlineStyle = kFontOutline_None;
    int PointSize{};
    int SizeMultiplier = 1;
    FontMetricsFixup MetricsFixup = kFontMetrics_None;
    int VerticalOffset{};
};

struct SpriteData
{
    int Slot{};
    SpriteImportResolution Resolution = kSpriteImport_Real;
    bool AlphaChannel = false;
};

struct PaletteEntryData
{
    PaletteColourType ColourType = kPaletteColourType_Gamewide;
    int Red{};
    int Green{};
    int Blue{};
};

struct TextParserWordData
{
    String Word;
    int WordGroup{};
};

struct AudioTypeData : EntityRef
{
    // NOTE: We are using the ScriptName property as the ScriptID from AudioClipType.cs
    String Name;
    int MaxChannels{};
    int VolumeReductionWhileSpeechPlaying{};
    CrossfadeSpeed Crossfade = kCrossfade_No;
};

struct AudioClipData : EntityRef
{
    int Index{};
    String SourceFileName;
    String CacheFileName;
    AudioFileBundlingType BundlingType = kAudioBundling_InGameEXE;
    int Type{};
    AudioClipFileType FileType = kAudioFile_Undefined;
    bool Repeat{};
    int Priority = 50;
    int Volume = 100;
};

struct PluginData
{
    String Name;
    std::vector<uint8_t> Data;
};

struct DialogOptionData
{
    String Text;
    bool Say = true;
    bool Show = true;
};

struct DialogRef : EntityRef
{
    int OptionCount = 0;
    bool ShowTextParser = false;
    std::vector<DialogOptionData> Options;
};

struct GUIControlData : EntityRef
{
    virtual ~GUIControlData() = default;
    int Height{};
    int Width{};
    int Left{};
    int Top{};
    int ZOrder{};
    bool Clickable{};
    bool Enabled{};
    bool Visible{};
    bool Translated{};
    bool ShowBorder{};
    bool SolidBackground{};
    int BackgroundColor{};
    int BorderColor{};
    int BorderWidth{};
    int PaddingX{};
    int PaddingY{};
};

struct GUIButtonData : GUIControlData
{
    String ClickAction;
    bool ClipImage{};
    bool WrapText{};
    int Font{};
    int Image{};
    int MouseoverImage{};
    int NewModeNumber{};
    String OnClick;
    int PushedImage{};
    String Text;
    FrameAlignment TextAlignment = kAlignTopCenter;
    int TextColor{};
    ButtonColorStyle ColorStyle = kButtonColor_Default;
    int BorderShadeColor{};
    int MouseOverBackgroundColor{};
    int PushedBackgroundColor{};
    int MouseOverBorderColor{};
    int PushedBorderColor{};
    int MouseOverTextColor{};
    int PushedTextColor{};
    int TextOutlineColor{};
};

struct GUIInventoryData : GUIControlData
{
    int CharacterID{};
    int ItemHeight{};
    int ItemWidth{};
};

struct GUISliderData : GUIControlData
{
    int BackgroundImage{};
    int HandleImage{};
    int HandleOffset{};
    int MaxValue{};
    int MinValue{};
    String OnChange;
    int Value{};
    int HandleColor{};
    int BorderShadeColor{};
};

struct GUILabelData : GUIControlData
{
    int Font{};
    String Text;
    FrameAlignment TextAlignment = kAlignTopLeft;
    int TextColor{};
    int TextOutlineColor{};
};

struct GUITextBoxData : GUIControlData
{
    int Font{};
    String OnActivate;
    String Text;
    FrameAlignment TextAlignment = kAlignTopLeft;
    int TextColor{};
    int TextOutlineColor{};
};

struct GUIListBoxData : GUIControlData
{
    int Font{};
    String OnSelectionChanged;
    int SelectedBackgroundColor{};
    int SelectedTextColor{};
    bool ShowScrollArrows{};
    HorAlignment TextAlignment = kHAlignLeft;
    int TextColor{};
    int TextOutlineColor{};
};

// Lightweight GUI data used by script generation. GUIData is the full form
// used when serializing a compiled game.
struct GUIRef : EntityRef
{
    std::vector<EntityRef> Controls;
};

struct GUIData : EntityRef
{
    bool IsTextWindow{};
    int BackgroundColor{};
    int BackgroundImage{};
    int BorderColor{};
    bool Clickable{};
    int Height{};
    int Left{};
    String Name;
    String OnClick;
    String ScriptModule;
    GUIPopupStyle PopupStyle = kGUIPopupStyle_Normal;
    int PopupYPos{};
    int Top{};
    int Transparency{};
    int Visible{};
    int Width{};
    int ZOrder{};
    int Padding = TEXTWINDOW_PADDING_DEFAULT;
    int TextColor{};
    std::vector<std::shared_ptr<GUIControlData>> Controls;
};

// Game variable (for variables defined in the game project)
struct Variable
{
    String Type;
    String Name;
    String Value;
};

// Game settings
struct GameSettings
{
    bool AllowRelativeAssetResolutions;
    bool AlwaysDisplayTextAsSpeech;
    int AndroidAppVersionCode;
    String AndroidAppVersionName;
    String AndroidApplicationId;
    DataUtil::AndroidBuildFormat AndroidBuildFormat = kAndroidBuild_ApkEmbedded;
    bool AntiAliasFonts;
    bool AntiGlideMode;
    bool AttachDataToExe;
    int AudioIndexer;
    bool AutoMoveInWalkMode;
    bool BackwardsText;
    String BuildTargets;
    bool ClipGUIControls;
    GameColorDepth ColorDepth = kGameColorDepth_TrueColor;
    SpriteCompression CompressSpritesType = AGS::Common::kSprCompress_None;
    String CustomDataDir;
    String CustomResolution;
    bool DebugMode;
    int DefaultRoomMaskResolution;
    String Description;
    String DeveloperName;
    String DeveloperURL;
    bool DialogOptionsBackwards;
    int DialogOptionsBullet;
    int DialogOptionsGUI;
    int DialogOptionsGap;
    String DialogScriptNarrateFunction; // Custom narrate function name
    String DialogScriptSayFunction; // Custom speech function name
    bool DisplayMultipleInventory;
    bool DisplaySingleDialogOption;
    bool EnforceNewAudio;
    bool EnforceNewStrings;
    bool EnforceObjectBasedScript;
    GameGuiAlphaRenderingStyle GUIAlphaStyle = ::kGuiAlphaRender_Proper;
    bool GUIHandleOnlyLeftMouseButton;
    String GUIDAsString;
    String GameFileName;
    String GameName;
    int GameFPS = 40;
    String GameTextEncoding;
    String GameTextLanguage;
    String Genre;
    int GlobalSpeechAnimationDelay;
    bool HandleInvClicksInScript;
    bool InventoryCursors;
    unsigned int InventoryHotspotMarkerCrosshairColor;
    unsigned int InventoryHotspotMarkerDotColor;
    int InventoryHotspotMarkerSprite;
    DataUtil::InventoryHotspotMarkerStyle InventoryHotspotMarkerStyle = kInventoryHotspot_None;
    bool LeftToRightPrecedence;
    bool LetterboxMode;
    int MaximumScore;
    bool MouseWheelEnabled;
    DialogOptionNumbering NumberDialogOptions = ::kDlgOptKeysOnly;
    bool OptimizeSpriteStorage;
    bool PixelPerfect;
    int PlaySoundOnScore;
    String ReleaseDate;
    RenderAtScreenRes RenderAtScreenResolution = ::kRenderAtScreenRes_UserDefined;
    ScreenTransitionStyle RoomTransition = ::kScrTran_Fade;
    bool RunGameLoopsWhileDialogOptionsDisplayed;
    String SaveGameFileExtension;
    String SaveGameFolderName;
    bool SaveScreenshots;
    bool ScaleCharacterSpriteOffsets;
    bool ScaleMovementSpeedWithMaskResolution;
    String ScriptAPIVersion;
    String ScriptCompatLevel;
    SkipSpeechStyle SkipSpeech = kSkipSpeech_MouseOrKeyboardOrTimer;
    DataUtil::SpeechPortraitSide SpeechPortraitSide = kSpeechPortrait_Left;
    DataUtil::SpeechStyle SpeechStyle = kSpeechStyle_Lucasarts;
    String SplitResources;
    GameSpriteAlphaRenderingStyle SpriteAlphaStyle = ::kSpriteAlphaRender_Proper;
    FontHeightDefinition TTFHeightDefinedBy = kFontHeight_NominalHeight;
    FontMetricsFixup TTFMetricsFixup = kFontMetrics_None;
    int TextWindowGUI;
    int ThoughtGUI;
    bool TurnBeforeFacing;
    bool TurnBeforeWalking;
    int UniqueID;
    bool UseGlobalSpeechAnimationDelay;
    bool UseLowResCoordinatesInScript;
    bool UseOldCustomDialogOptionsAPI;
    bool UseOldKeyboardHandling;
    bool UseOldVoiceClipNaming;
    CharacterTurnOrderPriority TurnOrderPriority = kTurnOrder_Clockwise;
    GUITextBoxKeyClaimStyle TextBoxKeyClaimStyle = kTextBoxKeyClaim_All;
    String Version;
    bool WalkInLookMode;
    GuiDisableStyle WhenInterfaceDisabled = AGS::Common::kGuiDis_Greyout;
};

struct CustomPropertySchemaItem
{
    String Name;
    String Description;
    String DefaultValue;
    PropertyType Type = AGS::Common::kPropertyUndefined;
    bool AppliesToCharacters = true;
    bool AppliesToHotspots = true;
    bool AppliesToObjects = true;
    bool AppliesToInvItems = true;
    bool AppliesToRooms = true;
    bool Translated = false;
};

struct InventoryItemData : EntityRef
{
    String Description;
    int Image = 0;
    int CursorImage = 0;
    int HotspotX = 0;
    int HotspotY = 0;
    bool PlayerStartsWith = false;
    String ScriptModule;
    std::vector<String> InteractionEvents;
    std::vector<CustomPropertyValue> Properties;
};

struct CursorData : EntityRef
{
    // NOTE: We are using the ScriptName property as the ScriptID from MouseCursor.cs
    String Name;
    int Image = 0;
    int HotspotX = 0;
    int HotspotY = 0;
    bool Animate = false;
    int View = -1;
    bool AnimateOnlyOnHotspots = false;
    bool AnimateOnlyWhenMoving = false;
    bool StandardMode = false;
    int AnimationDelay = 5;
};

// Lightweight game data used by script generation and dialog conversion.
// GameData derives from this and adds the data required for serialization.
struct GameRef
{
    std::vector<EntityRef> AudioClips;
    std::vector<EntityRef> AudioTypes;
    std::vector<CharacterRef> Characters;
    std::vector<EntityRef> Cursors;
    std::vector<DialogRef> Dialogs;
    std::vector<EntityRef> Fonts;
    std::vector<GUIRef>    GUI;
    std::vector<EntityRef> Inventory;
    std::vector<EntityRef> Views;
    std::vector<std::pair<int, String>> Rooms;

    std::vector<Variable>  GlobalVars;
    std::vector<CustomPropertySchemaItem> PropertySchema;

    int                    PlayerCharacter = -1;
    GameSettings           Settings;
};

// Full game data required for serializing a compiled game. The base GameRef
// remains the lightweight representation used by the other data tools.
struct GameData : GameRef
{
    std::vector<AudioClipData> AudioClips;
    std::vector<AudioTypeData> AudioTypes;
    std::vector<CharacterData> Characters;
    std::vector<CursorData> Cursors;
    std::vector<FontData> Fonts;
    std::vector<GUIData> GUI;
    std::vector<InventoryItemData> Inventory;
    std::vector<ViewData> Views;
    std::vector<SpriteData> Sprites;
    std::vector<PaletteEntryData> Palette;
    std::vector<TextParserWordData> ParserWords;
    std::vector<String> LipSyncFrames;
    int LipSyncDefaultFrame = 0;
    LipSyncType LipSync = kLipSync_None;
    std::vector<String> GlobalMessages;
    std::vector<PluginData> Plugins;
    String EditorVersion;
};

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__GAMEUTIL_H
