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
#include "data/agfreader.h"
#include <algorithm>
#include <cstdio>
#include <tinyxml2.h>
#include "debug/out.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

const char *XML_ROOT_NODE_NAME = "AGSEditorDocument";
const char *XML_ATTRIBUTE_VERSION = "Version";
const char *XML_ATTRIBUTE_VERSION_INDEX = "VersionIndex";
const char *XML_ATTRIBUTE_EDITOR_VERSION = "EditorVersion";
const char *LATEST_XML_VERSION = "3.0.3.2";
const int  LOWEST_SUPPORTED_FORMAT = 26; // AGS 3.5.0

using namespace AGS::Common;
using namespace tinyxml2;

namespace
{

static const CstrArr<3> kGameColorDepthNames = {{
    "Palette", "HighColor", "TrueColor"
}};

static const CstrArr<4> kCompressSpritesTypeNames = {{
    "None", "RLE", "LZW", "Deflate"
}};

static const CstrArr<3> kAndroidBuildFormatNames = {{
    "ApkEmbedded", "AabEmbedded", "Aab"
}};

static const CstrArr<3> kGuiAlphaStyleNames = {{
    "Classic", "AdditiveOpacity", "MultiplyTranslucenceSrcBlend"
}};

static const CstrArr<2> kSpriteAlphaStyleNames = {{
    "Classic", "Improved"
}};

static const CstrArr<3> kRenderAtScreenResNames = {{
    "UserDefined", "True", "False"
}};

static const CstrArr<5> kRoomTransitionNames = {{
    "FadeOutAndIn", "Instant", "Dissolve", "BlackBoxOut", "CrossFade"
}};

static const CstrArr<3> kNumberDialogOptionsNames = {{
    "None", "KeyShortcutsOnly", "Normal"
}};

static const CstrArr<7> kSkipSpeechNames = {{
    "MouseOrKeyboardOrTimer", "KeyboardOnly", "TimerOnly",
    "MouseOrKeyboard", "MouseOnly", "KeyboardOnlyStrict", "MouseOnlyStrict"
}};

static const CstrArr<4> kSpeechPortraitSideNames = {{
    "Left", "Right", "Alternate", "BasedOnCharacterPosition"
}};

static const CstrArr<4> kSpeechStyleNames = {{
    "Lucasarts", "SierraTransparent", "SierraWithBackground", "WholeScreen"
}};

static const CstrArr<3> kFontHeightDefinitionNames = {{
    "NominalHeight", "PixelHeight", "CustomValue"
}};

static const CstrArr<2> kFontMetricsFixupNames = {{
    "None", "SetAscenderToHeight"
}};

static const CstrArr<4> kInterfaceDisabledNames = {{
    "GreyOut", "GoBlack", "Unchanged", "TurnOff"
}};

static const CstrArr<4> kGuiPopupStyleNames = {{
    "Normal", "MouseYPos", "PopupModal", "Persistent"
}};

static const CstrArr<3> kInventoryHotspotMarkerNames = {{
    "None", "Crosshair", "Sprite"
}};

static const CstrArr<3> kCustomPropertyTypeNames = {{
    "Boolean", "Number", "Text"
}};

static const CstrArr<9> kFrameAlignmentNames = {{
    "TopLeft", "TopCenter", "TopRight",
    "MiddleLeft", "MiddleCenter", "MiddleRight",
    "BottomLeft", "BottomCenter", "BottomRight"
}};

static const CstrArr<3> kFontOutlineStyleNames = {{
    "None", "Automatic", "UseOutlineFont"
}};

static const CstrArr<2> kFontAutoOutlineStyleNames = {{
    "Squared", "Rounded"
}};

static const CstrArr<3> kSpriteImportResolutionNames = {{
    "Real", "LowRes", "HighRes"
}};

static const CstrArr<2> kAudioBundlingTypeNames = {{
    "InGameEXE", "InSeparateVOX"
}};

static const CstrArr<6> kAudioFileTypeNames = {{
    "OGG", "MP3", "WAV", "VOC", "MIDI", "MOD"
}};

static const CstrArr<5> kCrossfadeSpeedNames = {{
    "No", "SlowFade", "SlowishFade", "MediumFade", "FastFade"
}};

static const CstrArr<3> kButtonColorStyleNames = {{
    "Default", "Dynamic", "DynamicFlat"
}};

static const CstrArr<3> kInheritableBoolNames = {{
    "Inherit", "False", "True"
}};

static const CstrArr<6> kAudioPriorityNames = {{
    "Inherit", "VeryLow", "Low", "Normal", "High", "VeryHigh"
}};

static const CstrArr<3> kLipSyncTypeNames = {{
    "None", "Text", "PamelaVoiceFiles"
}};

static const CstrArr<2> kPaletteTypeNames = {{
    "Gamewide", "Background"
}};

static const CstrArr<4> kTurnOrderPriorityNames = {{
    "Clockwise", "CounterClockwise", "Random", "FaceDown"
}};

static const CstrArr<3> kTextBoxKeyClaimStyleNames = {{
    "All", "Handled", "TextOnly"
}};

} // namespace

namespace AGS
{
namespace AGF
{

AGFReader::AGFReader()
    : _gameRoot(nullptr)
{
}

AGFReader::~AGFReader()
{
}

// NOTE: When reading Game.agf files, it is possible they may have been authored by hand,
// so they could potentially contain some mistakes when comparing with one authored by
// the AGS Editor. We can reuse some logic from AGS.Types when reading the properties.

HError AGFReader::Open(const char *filename)
{
    Close();

    _doc.reset(new Document());
    if (_doc->LoadFile(filename) != XML_SUCCESS)
        return new Error("Failed to open XML", _doc->ErrorIDToName(_doc->ErrorID()));
    if (!_doc->RootElement() || strcmp(_doc->RootElement()->Name(), XML_ROOT_NODE_NAME))
        return new Error("Not a valid AGS game project");

    const char *attr_filever = _doc->RootElement()->Attribute(XML_ATTRIBUTE_VERSION);
    if (!attr_filever)
        return new Error("Game.agf format version is missing");
    if (strcmp(attr_filever, LATEST_XML_VERSION))
        return new Error(String::FromFormat("Unsupported Game.agf format: %s", attr_filever));

    const int attr_format = _doc->RootElement()->IntAttribute(XML_ATTRIBUTE_VERSION_INDEX);
    const char *attr_editorver = _doc->RootElement()->Attribute(XML_ATTRIBUTE_EDITOR_VERSION);
    _editorVersion = attr_editorver ? attr_editorver : "";
    Debug::Printf("AGFReader: opened %s,\n format tag: %s\n format index: %d\n saved by AGS %s",
        filename, attr_filever, attr_format, attr_editorver ? attr_editorver : "unknown");

    if (attr_format < LOWEST_SUPPORTED_FORMAT)
        return new Error(String::FromFormat("Unsupported Game.agf format index: %d", attr_format));

    DocElem game = _doc->RootElement()->FirstChildElement("Game");
    if (!game)
        return new Error("<Game> element not found");
    _gameRoot = game;
    return HError::None();
}

void AGFReader::Close()
{
    _doc.reset();
    _gameRoot = nullptr;
    _editorVersion.Empty();
}

//-----------------------------------------------------------------------------
// Entity list parsers
//-----------------------------------------------------------------------------

void EntityListParser::GetAllElems(DocElem game_root, std::vector<DocElem> &elems,
    const char *folder_elem, const char *list_elem, const char *type_elem)
{
    DocElem list_root = game_root->FirstChildElement(list_elem);
    if (!list_root)
        return;
    DocElem node = list_root;
    if (folder_elem)
    { // Get the main folder
        node = node->FirstChildElement(folder_elem);
        if (!node)
            return;
    }
    return GetElemsRecursive(node, elems, folder_elem, list_elem, type_elem);
}

void EntityListParser::GetAllElems(DocElem game_root, std::vector<DocElem> &elems,
    const char *root_elem, const char *folder_elem, const char *list_elem, const char *type_elem)
{
    DocElem list_root = game_root->FirstChildElement(root_elem);
    if (!list_root)
        return;
    DocElem node = list_root;
    if (folder_elem)
    { // Get the main folder
        node = node->FirstChildElement(folder_elem);
        if (!node)
            return;
    }
    return GetElemsRecursive(node, elems, folder_elem, list_elem, type_elem);
}

void EntityListParser::GetElemsRecursive(DocElem folder,  std::vector<DocElem> &elems,
     const char *folder_elem, const char *list_elem, const char *type_elem)
{
    if (!folder)
        return;
    // First pass subfolders
    DocElem list_node = folder;
    if (folder_elem)
    {
        DocElem sub = folder->FirstChildElement("SubFolders");
        if (sub)
        {
            for (DocElem node = sub->FirstChildElement(folder_elem);
                node; node = node->NextSiblingElement(folder_elem))
            {
                GetElemsRecursive(node, elems, folder_elem, list_elem, type_elem);
            }
        }
        // get to elements inside a folder
        list_node = folder->FirstChildElement(list_elem);
        if (!list_node)
            return;
    }
    // Then pass elements themselves
    for (DocElem node = list_node->FirstChildElement(type_elem);
        node; node = node->NextSiblingElement(type_elem))
    {
        elems.push_back(node);
    }
}

//-----------------------------------------------------------------------------
// Entity parsers
//-----------------------------------------------------------------------------

const char* ValueParser::ReadString(DocElem elem, const char *field, const char *def_value)
{
    if (!elem)
        return def_value;
    DocElem name_f = elem->FirstChildElement(field);
    if (name_f && name_f->GetText())
        return name_f->GetText();
    return def_value;
}

int ValueParser::ReadInt(DocElem elem, const char *field, int def_value)
{
    if (!elem)
        return def_value;
    DocElem name_f = elem->FirstChildElement(field);
    if (name_f && name_f->GetText())
        return StrUtil::StringToInt(name_f->GetText(), def_value);
    return def_value;
}

bool ValueParser::ReadBool(DocElem elem, const char *field, bool def_value)
{
    if (!elem)
        return def_value;
    DocElem name_f = elem->FirstChildElement(field);
    if (name_f && name_f->GetText())
        return ags_stricmp(name_f->GetText(), "True") == 0;
    return def_value;
}

const char* ValueParser::ReadAttributeString(DocElem elem, const char *field, const char *def_value)
{
    if (!elem)
        return def_value;
    const char *value = elem->Attribute(field);
    return value ? value : def_value;
}

int ValueParser::ReadAttributeInt(DocElem elem, const char *field, int def_value)
{
    return elem ? elem->IntAttribute(field, def_value) : def_value;
}

bool ValueParser::ReadAttributeBool(DocElem elem, const char *field, bool def_value)
{
    return elem ? elem->BoolAttribute(field, def_value) : def_value;
}

static DataUtil::GameColorDepth ReadGameColorDepth(const String &value)
{
    switch (StrUtil::ParseEnum(value, kGameColorDepthNames, 2))
    {
    case 0: return DataUtil::kGameColorDepth_Palette;
    case 1: return DataUtil::kGameColorDepth_HighColor;
    case 2:
    default: return DataUtil::kGameColorDepth_TrueColor;
    }
}

static SpriteCompression ReadSpriteCompression(const String &value)
{
    return StrUtil::ParseEnum(value, kCompressSpritesTypeNames, kSprCompress_None);
}

static DataUtil::AndroidBuildFormat ReadAndroidBuildFormat(const String &value)
{
    return StrUtil::ParseEnum(value, kAndroidBuildFormatNames, DataUtil::kAndroidBuild_ApkEmbedded);
}

static GameGuiAlphaRenderingStyle ReadGuiAlphaStyle(const String &value)
{
    return StrUtil::ParseEnum(value, kGuiAlphaStyleNames, kGuiAlphaRender_Legacy);
}

static GameSpriteAlphaRenderingStyle ReadSpriteAlphaStyle(const String &value)
{
    return StrUtil::ParseEnum(value, kSpriteAlphaStyleNames, kSpriteAlphaRender_Legacy);
}

static RenderAtScreenRes ReadRenderAtScreenResolution(const String &value)
{
    return StrUtil::ParseEnum(value, kRenderAtScreenResNames, kRenderAtScreenRes_UserDefined);
}

static ScreenTransitionStyle ReadRoomTransition(const String &value)
{
    return StrUtil::ParseEnum(value, kRoomTransitionNames, kScrTran_Fade);
}

static DialogOptionNumbering ReadDialogOptionsNumbering(const String &value)
{
    return StrUtil::ParseEnumWithBase(value, kNumberDialogOptionsNames,
        static_cast<DialogOptionNumbering>(-1), kDlgOptNoNumbering);
}

DataUtil::SkipSpeechStyle ReadSkipSpeech(const String &value)
{
    return StrUtil::ParseEnum(value, kSkipSpeechNames, DataUtil::kSkipSpeech_MouseOrKeyboardOrTimer);
}

static DataUtil::SpeechPortraitSide ReadSpeechPortraitSide(const String &value)
{
    return StrUtil::ParseEnum(value, kSpeechPortraitSideNames, DataUtil::kSpeechPortrait_Left);
}

static DataUtil::SpeechStyle ReadSpeechStyle(const String &value)
{
    switch (StrUtil::ParseEnum(value, kSpeechStyleNames, 0))
    {
    case 0: return DataUtil::kSpeechStyle_Lucasarts;
    case 1: return DataUtil::kSpeechStyle_SierraTransparent;
    case 2: return DataUtil::kSpeechStyle_SierraWithBackground;
    case 3:
    default: return DataUtil::kSpeechStyle_WholeScreen;
    }
}

static DataUtil::FontHeightDefinition ReadFontHeightDefinition(const String &value)
{
    return StrUtil::ParseEnum(value, kFontHeightDefinitionNames, DataUtil::kFontHeight_NominalHeight);
}

static DataUtil::FontMetricsFixup ReadFontMetricsFixup(const String &value)
{
    return StrUtil::ParseEnum(value, kFontMetricsFixupNames, DataUtil::kFontMetrics_None);
}

static GuiDisableStyle ReadGuiDisableStyle(const String &value)
{
    return StrUtil::ParseEnum(value, kInterfaceDisabledNames, kGuiDis_Greyout);
}

static DataUtil::GUIPopupStyle ReadGuiPopupStyle(const String &value)
{
    return StrUtil::ParseEnum(value, kGuiPopupStyleNames, DataUtil::kGUIPopupStyle_Normal);
}

static DataUtil::InventoryHotspotMarkerStyle ReadInventoryHotspotMarkerStyle(const String &value)
{
    return StrUtil::ParseEnum(value, kInventoryHotspotMarkerNames, DataUtil::kInventoryHotspot_None);
}

static AGS::Common::PropertyType ReadCustomPropertyType(const String &value)
{
    return StrUtil::ParseEnumWithBase(value, kCustomPropertyTypeNames,
        AGS::Common::kPropertyBoolean, AGS::Common::kPropertyUndefined);
}


static String BuildPrefixedScriptID(const String &name, const char *prefix)
{
    String script_id;
    for (size_t c = 0; c < name.GetLength(); ++c)
    {
        if (StrUtil::IsScriptWordChar(name[c]))
            script_id.AppendChar(name[c]);
    }
    // Font.cs and AudioClipType.cs retain the prefix when a nonempty name is
    // made entirely of invalid characters, unlike MouseCursor.cs. Such names
    // are improbable, so use the cursor behavior consistently for all three.
    if (!script_id.IsEmpty())
        script_id.Prepend(prefix);
    return script_id;
}

String BuildAudioTypePrefixedScriptID(const String &name)
{
    return BuildPrefixedScriptID(name, "eAudioType");
}

String BuildFontPrefixedScriptID(const String &name)
{
    return BuildPrefixedScriptID(name, "eFont");
}

String BuildMouseCursorPrefixedScriptID(const String &name)
{
    return BuildPrefixedScriptID(name, "eMode");
}

static DataUtil::FontOutlineStyle ReadFontOutlineStyle(const String &value);
static DataUtil::FontAutoOutlineStyle ReadFontAutoOutlineStyle(const String &value);
static DataUtil::SpriteImportResolution ReadSpriteImportResolution(const String &value);
static DataUtil::AudioFileBundlingType ReadAudioBundlingType(const String &value);
static DataUtil::AudioClipFileType ReadAudioFileType(const String &value);
static DataUtil::CrossfadeSpeed ReadCrossfadeSpeed(const String &value);
static DataUtil::ButtonColorStyle ReadButtonColorStyle(const String &value);
static DataUtil::CharacterTurnOrderPriority ReadTurnOrderPriority(const String &value);
static DataUtil::GUITextBoxKeyClaimStyle ReadTextBoxKeyClaimStyle(const String &value);
DataUtil::LipSyncType ReadLipSyncType(const String &value);

int Dialog::ReadOptionCount(DocElem elem)
{
    // Option count is not written in AGF, so we have to calculate number of elems
    elem = elem->FirstChildElement("DialogOptions");
    if (!elem)
        return 0;
    int count = 0;
    for (elem = elem->FirstChildElement("DialogOption");
        elem; elem = elem->NextSiblingElement("DialogOption"), count++);
    return count;
}

static FrameAlignment ReadFrameAlignment(const String &value, FrameAlignment def_value)
{
    // These aliases match AGS.Types.FrameAlignment's deserialization rules
    // and keep older Game.agf projects readable.
    if (value.CompareNoCase("Left") == 0)
        return kAlignTopLeft;
    if (value.CompareNoCase("Center") == 0)
        return kAlignTopCenter;
    if (value.CompareNoCase("Centre") == 0 || value.CompareNoCase("Centred") == 0)
        return kAlignMiddleCenter;
    if (value.CompareNoCase("Right") == 0)
        return kAlignTopRight;
    if (value.CompareNoCase("TopMiddle") == 0)
        return kAlignTopCenter;
    if (value.CompareNoCase("BottomMiddle") == 0)
        return kAlignBottomCenter;

    const int index = StrUtil::ParseEnum(value, kFrameAlignmentNames, -1);
    return (index >= 0) ? static_cast<FrameAlignment>(1 << index) : def_value;
}

static HorAlignment ReadHorizontalAlignment(const String &value, HorAlignment def_value)
{
    if (value.CompareNoCase("Left") == 0 || value.CompareNoCase("TopLeft") == 0)
        return kHAlignLeft;
    if (value.CompareNoCase("Center") == 0 || value.CompareNoCase("Centre") == 0 ||
        value.CompareNoCase("TopCenter") == 0 || value.CompareNoCase("TopMiddle") == 0)
        return kHAlignCenter;
    if (value.CompareNoCase("Right") == 0 || value.CompareNoCase("TopRight") == 0)
        return kHAlignRight;
    return def_value;
}

String Cursor::ReadScriptName(DocElem elem)
{
    return BuildMouseCursorPrefixedScriptID(ReadString(elem, "Name"));
}

void Cursor::ReadAllData(DocElem elem, DataUtil::CursorData &data)
{
    data.Name = ReadString(elem, "Name"); // This is MouseCursor Name
    data.Image = ReadInt(elem, "Image");
    data.HotspotX = ReadInt(elem, "HotspotX");
    data.HotspotY = ReadInt(elem, "HotspotY");
    data.Animate = ReadBool(elem, "Animate");
    data.View = ReadInt(elem, "View", -1);
    data.AnimateOnlyOnHotspots = ReadBool(elem, "AnimateOnlyOnHotspots");
    data.AnimateOnlyWhenMoving = ReadBool(elem, "AnimateOnlyWhenMoving");
    data.StandardMode = ReadBool(elem, "StandardMode");
    data.AnimationDelay = ReadInt(elem, "AnimationDelay", 5);
}

void InventoryItem::ReadAllData(DocElem elem, DataUtil::InventoryItemData &data)
{
    data.Description = ReadString(elem, "Description");
    data.Image = ReadInt(elem, "Image");
    data.CursorImage = ReadInt(elem, "CursorImage");
    data.HotspotX = ReadInt(elem, "HotspotX");
    data.HotspotY = ReadInt(elem, "HotspotY");
    data.PlayerStartsWith = ReadBool(elem, "PlayerStartsWithItem");
}

int GUIMain::ReadID(DocElem elem)
{
    DocElem self = GetNormalGUI(elem);
    if (!self)
        self = GetTextWindow(elem);
    if (!self)
        return -1;
    return ReadInt(self, "ID");
}

String GUIMain::ReadScriptName(DocElem elem)
{
    DocElem self = GetNormalGUI(elem);
    if (!self)
        self = GetTextWindow(elem);
    if (!self)
        return "";
    return ReadString(self, "Name");
}

void GUIMain::ReadAllData(DocElem elem, DataUtil::GUIData& gui_data)
{
    DocElem self = GetNormalGUI(elem);
    if (!self)
    {
        self = GetTextWindow(elem);
        gui_data.IsTextWindow = static_cast<bool>(self);
    }
    if (!self)
        return;
    gui_data.Clickable = ReadBool(self, "Clickable");
    gui_data.Height = ReadInt(self, "Height");
    gui_data.Left = ReadInt(self, "Left");
    gui_data.Top = ReadInt(self, "Top");
    gui_data.Width = ReadInt(self, "Width");
    gui_data.Transparency = ReadInt(self, "Transparency");
    gui_data.Visible = ReadBool(self, "Visible");
    gui_data.BackgroundColor = ReadInt(self, "BackgroundColor");
    gui_data.BackgroundImage = ReadInt(self, "BackgroundImage");
    gui_data.BorderColor = ReadInt(self, "BorderColor");
    gui_data.OnClick = ReadString(self, "OnClick");
    gui_data.ScriptModule = ReadString(self, "ScriptModule");
    gui_data.PopupStyle = ReadGuiPopupStyle(ReadString(self, "PopupStyle"));
    gui_data.ZOrder = ReadInt(self, "ZOrder");
    gui_data.PopupYPos = ReadInt(self, "PopupYPos");
    gui_data.Padding = ReadInt(self, "Padding", TEXTWINDOW_PADDING_DEFAULT);
    gui_data.TextColor = ReadInt(self, "TextColor");
}

DocElem GUIMain::GetNormalGUI(DocElem elem)
{
    return elem->FirstChildElement("NormalGUI");
}

DocElem GUIMain::GetTextWindow(DocElem elem)
{
    return elem->FirstChildElement("TextWindowGUI");
}

String GUIControl::ReadType(DocElem elem)
{
    const char *name = elem->Name();
    if (strcmp(name, "GUIButton") == 0 || strcmp(name, "GUITextWindowEdge") == 0)
        return "Button";
    if (strcmp(name, "GUILabel") == 0)
        return "Label";
    if (strcmp(name, "GUIInventory") == 0)
        return "InvWindow";
    if (strcmp(name, "GUIListBox") == 0)
        return "ListBox";
    if (strcmp(name, "GUISlider") == 0)
        return "Slider";
    if (strcmp(name, "GUITextBox") == 0)
        return "TextBox";
    return "GUIControl";
}

void GUIControl::ReadAllData(DocElem elem, DataUtil::GUIControlData& data)
{
    data.Clickable = ReadBool(elem, "Clickable");
    data.Height = ReadInt(elem, "Height");
    data.Left = ReadInt(elem, "Left");
    data.Top = ReadInt(elem, "Top");
    data.Width = ReadInt(elem, "Width");
    data.Enabled = ReadBool(elem, "Enabled");
    data.Visible = ReadBool(elem, "Visible");
    data.Translated = ReadBool(elem, "Translated", true);
    data.ShowBorder = ReadBool(elem, "ShowBorder");
    data.SolidBackground = ReadBool(elem, "SolidBackground");
    data.ZOrder = ReadInt(elem, "ZOrder");
    data.BackgroundColor = ReadInt(elem, "BackgroundColor");
    data.BorderColor = ReadInt(elem, "BorderColor");
    data.BorderWidth = ReadInt(elem, "BorderWidth");
    data.PaddingX = ReadInt(elem, "PaddingX");
    data.PaddingY = ReadInt(elem, "PaddingY");
}

void GUIControl::ReadButtonData(DocElem elem, DataUtil::GUIButtonData& data)
{
    data.ClickAction = ReadString(elem, "ClickAction");
    data.ClipImage = ReadBool(elem, "ClipImage");
    data.WrapText = ReadBool(elem, "WrapText");
    data.Font = ReadInt(elem, "Font");
    data.Image = ReadInt(elem, "Image");
    data.MouseoverImage = ReadInt(elem, "MouseoverImage");
    data.NewModeNumber = ReadInt(elem, "NewModeNumber");
    data.OnClick = ReadString(elem, "OnClick");
    data.PushedImage = ReadInt(elem, "PushedImage");
    data.Text = ReadString(elem, "Text");
    data.TextAlignment = ReadFrameAlignment(ReadString(elem, "TextAlignment"), kAlignTopCenter);
    data.TextColor = ReadInt(elem, "TextColor");
    data.ColorStyle = ReadButtonColorStyle(ReadString(elem, "ColorStyle"));
    data.BorderShadeColor = ReadInt(elem, "BorderShadeColor");
    data.MouseOverBackgroundColor = ReadInt(elem, "MouseOverBackgroundColor");
    data.PushedBackgroundColor = ReadInt(elem, "PushedBackgroundColor");
    data.MouseOverBorderColor = ReadInt(elem, "MouseOverBorderColor");
    data.PushedBorderColor = ReadInt(elem, "PushedBorderColor");
    data.MouseOverTextColor = ReadInt(elem, "MouseOverTextColor");
    data.PushedTextColor = ReadInt(elem, "PushedTextColor");
    data.TextOutlineColor = ReadInt(elem, "TextOutlineColor");
}

void GUIControl::ReadLabelData(DocElem elem, DataUtil::GUILabelData& data)
{
    data.Font = ReadInt(elem, "Font");
    data.Text = ReadString(elem, "Text");
    data.TextAlignment = ReadFrameAlignment(ReadString(elem, "TextAlignment"), kAlignTopLeft);
    data.TextColor = ReadInt(elem, "TextColor");
    data.TextOutlineColor = ReadInt(elem, "TextOutlineColor");
}

void GUIControl::ReadSliderData(DocElem elem, DataUtil::GUISliderData& data)
{
    data.BackgroundImage = ReadInt(elem, "BackgroundImage");
    data.HandleImage = ReadInt(elem, "HandleImage");
    data.HandleOffset = ReadInt(elem, "HandleOffset");
    data.MaxValue = ReadInt(elem, "MaxValue");
    data.MinValue = ReadInt(elem, "MinValue");
    data.OnChange = ReadString(elem, "OnChange");
    data.Value = ReadInt(elem, "Value");
    data.HandleColor = ReadInt(elem, "HandleColor");
    data.BorderShadeColor = ReadInt(elem, "BorderShadeColor");
}

void GUIControl::ReadInventoryData(DocElem elem, DataUtil::GUIInventoryData& data)
{
    data.CharacterID = ReadInt(elem, "CharacterID");
    data.ItemHeight = ReadInt(elem, "ItemHeight");
    data.ItemWidth = ReadInt(elem, "ItemWidth");
}

void GUIControl::ReadTextBoxData(DocElem elem, DataUtil::GUITextBoxData& data)
{
    data.Font = ReadInt(elem, "Font");
    data.OnActivate = ReadString(elem, "OnActivate");
    data.ShowBorder = ReadBool(elem, "ShowBorder");
    data.Text = ReadString(elem, "Text");
    data.TextAlignment = ReadFrameAlignment(ReadString(elem, "TextAlignment"), kAlignTopLeft);
    data.TextColor = ReadInt(elem, "TextColor");
    data.TextOutlineColor = ReadInt(elem, "TextOutlineColor");
}

void GUIControl::ReadListBoxData(DocElem elem, DataUtil::GUIListBoxData& data)
{
    data.Font = ReadInt(elem, "Font");
    data.OnSelectionChanged = ReadString(elem, "OnSelectionChanged");
    data.SelectedBackgroundColor = ReadInt(elem, "SelectedBackgroundColor");
    data.SelectedTextColor = ReadInt(elem, "SelectedTextColor");
    data.ShowBorder = ReadBool(elem, "ShowBorder");
    data.ShowScrollArrows = ReadBool(elem, "ShowScrollArrows");
    data.TextAlignment = ReadHorizontalAlignment(ReadString(elem, "TextAlignment"), kHAlignLeft);
    data.TextColor = ReadInt(elem, "TextColor");
    data.TextOutlineColor = ReadInt(elem, "TextOutlineColor");
}

void CustomPropertySchemaItem::ReadAllData(DocElem elem, DataUtil::CustomPropertySchemaItem &data)
{
    data.Name = ReadString(elem, "Name");
    data.Description = ReadString(elem, "Description");
    data.DefaultValue = ReadString(elem, "DefaultValue");
    data.Type = ReadCustomPropertyType(ReadString(elem, "Type"));
    data.AppliesToCharacters = ReadBool(elem, "AppliesToCharacters", true);
    data.AppliesToHotspots = ReadBool(elem, "AppliesToHotspots", true);
    data.AppliesToObjects = ReadBool(elem, "AppliesToObjects", true);
    data.AppliesToInvItems = ReadBool(elem, "AppliesToInvItems", true);
    data.AppliesToRooms = ReadBool(elem, "AppliesToRooms", true);
    data.Translated = ReadBool(elem, "Translated");
}

void GameSettings::ReadAllData(DocElem elem, DataUtil::GameSettings& s)
{
    s.AllowRelativeAssetResolutions = ReadBool(elem, "AllowRelativeAssetResolutions");
    s.AlwaysDisplayTextAsSpeech = ReadBool(elem, "AlwaysDisplayTextAsSpeech");
    s.AndroidAppVersionCode = ReadInt(elem, "AndroidAppVersionCode");
    s.AndroidAppVersionName = ReadString(elem, "AndroidAppVersionName");
    s.AndroidApplicationId = ReadString(elem, "AndroidApplicationId");
    s.AndroidBuildFormat = ReadAndroidBuildFormat(ReadString(elem, "AndroidBuildFormat"));
    s.AntiAliasFonts = ReadBool(elem, "AntiAliasFonts");
    s.AntiGlideMode = ReadBool(elem, "AntiGlideMode");
    s.AttachDataToExe = ReadBool(elem, "AttachDataToExe");
    s.AudioIndexer = ReadInt(elem, "AudioIndexer");
    s.AutoMoveInWalkMode = ReadBool(elem, "AutoMoveInWalkMode");
    s.BackwardsText = ReadBool(elem, "BackwardsText");
    s.BuildTargets = ReadString(elem, "BuildTargets");
    s.ClipGUIControls = ReadBool(elem, "ClipGUIControls");
    s.ColorDepth = ReadGameColorDepth(ReadString(elem, "ColorDepth"));
    s.CompressSpritesType = ReadSpriteCompression(ReadString(elem, "CompressSpritesType"));
    s.CustomDataDir = ReadString(elem, "CustomDataDir");
    s.CustomResolution = ReadString(elem, "CustomResolution");
    s.DebugMode = ReadBool(elem, "DebugMode");
    s.DefaultRoomMaskResolution = ReadInt(elem, "DefaultRoomMaskResolution");
    s.Description = ReadString(elem, "Description");
    s.DeveloperName = ReadString(elem, "DeveloperName");
    s.DeveloperURL = ReadString(elem, "DeveloperURL");
    s.DialogOptionsBackwards = ReadBool(elem, "DialogOptionsBackwards");
    s.DialogOptionsBullet = ReadInt(elem, "DialogOptionsBullet");
    s.DialogOptionsGUI = ReadInt(elem, "DialogOptionsGUI");
    s.DialogOptionsGap = ReadInt(elem, "DialogOptionsGap");
    s.DialogScriptNarrateFunction = ReadString(elem, "DialogScriptNarrateFunction");
    s.DialogScriptSayFunction = ReadString(elem, "DialogScriptSayFunction");
    s.DisplayMultipleInventory = ReadBool(elem, "DisplayMultipleInventory");
    s.DisplaySingleDialogOption = ReadBool(elem, "DisplaySingleDialogOption");
    s.EnforceNewAudio = ReadBool(elem, "EnforceNewAudio");
    s.EnforceNewStrings = ReadBool(elem, "EnforceNewStrings");
    s.EnforceObjectBasedScript = ReadBool(elem, "EnforceObjectBasedScript");
    s.GUIAlphaStyle = ReadGuiAlphaStyle(ReadString(elem, "GUIAlphaStyle"));
    s.GUIHandleOnlyLeftMouseButton = ReadBool(elem, "GUIHandleOnlyLeftMouseButton");
    s.GUIDAsString = ReadString(elem, "GUIDAsString");
    s.GameFileName = ReadString(elem, "GameFileName");
    s.GameName = ReadString(elem, "GameName");
    s.GameFPS = ReadInt(elem, "GameFPS", 40);
    s.GameTextEncoding = ReadString(elem, "GameTextEncoding");
    s.GameTextLanguage = ReadString(elem, "GameTextLanguage");
    s.Genre = ReadString(elem, "Genre");
    s.GlobalSpeechAnimationDelay = ReadInt(elem, "GlobalSpeechAnimationDelay");
    s.HandleInvClicksInScript = ReadBool(elem, "HandleInvClicksInScript");
    s.InventoryCursors = ReadBool(elem, "InventoryCursors");
    s.InventoryHotspotMarkerCrosshairColor = ReadInt(elem, "InventoryHotspotMarkerCrosshairColor");
    s.InventoryHotspotMarkerDotColor = ReadInt(elem, "InventoryHotspotMarkerDotColor");
    s.InventoryHotspotMarkerSprite = ReadInt(elem, "InventoryHotspotMarkerSprite");
    s.InventoryHotspotMarkerStyle = ReadInventoryHotspotMarkerStyle(ReadString(elem, "InventoryHotspotMarkerStyle"));
    s.LeftToRightPrecedence = ReadBool(elem, "LeftToRightPrecedence");
    s.LetterboxMode = ReadBool(elem, "LetterboxMode");
    s.MaximumScore = ReadInt(elem, "MaximumScore");
    s.MouseWheelEnabled = ReadBool(elem, "MouseWheelEnabled");
    s.NumberDialogOptions = ReadDialogOptionsNumbering(ReadString(elem, "NumberDialogOptions"));
    s.OptimizeSpriteStorage = ReadBool(elem, "OptimizeSpriteStorage");
    s.PixelPerfect = ReadBool(elem, "PixelPerfect");
    s.PlaySoundOnScore = ReadInt(elem, "PlaySoundOnScore");
    s.ReleaseDate = ReadString(elem, "ReleaseDate");
    s.RenderAtScreenResolution = ReadRenderAtScreenResolution(ReadString(elem, "RenderAtScreenResolution"));
    s.RoomTransition = ReadRoomTransition(ReadString(elem, "RoomTransition"));
    s.RunGameLoopsWhileDialogOptionsDisplayed = ReadBool(elem, "RunGameLoopsWhileDialogOptionsDisplayed");
    s.SaveGameFileExtension = ReadString(elem, "SaveGameFileExtension");
    s.SaveGameFolderName = ReadString(elem, "SaveGameFolderName");
    s.SaveScreenshots = ReadBool(elem, "SaveScreenshots");
    s.ScaleCharacterSpriteOffsets = ReadBool(elem, "ScaleCharacterSpriteOffsets");
    s.ScaleMovementSpeedWithMaskResolution = ReadBool(elem, "ScaleMovementSpeedWithMaskResolution");
    s.ScriptAPIVersion = ReadString(elem, "ScriptAPIVersion");
    s.ScriptCompatLevel = ReadString(elem, "ScriptCompatLevel");
    s.SkipSpeech = ReadSkipSpeech(ReadString(elem, "SkipSpeech"));
    s.SpeechPortraitSide = ReadSpeechPortraitSide(ReadString(elem, "SpeechPortraitSide"));
    s.SpeechStyle = ReadSpeechStyle(ReadString(elem, "SpeechStyle"));
    s.SplitResources = ReadString(elem, "SplitResources");
    s.SpriteAlphaStyle = ReadSpriteAlphaStyle(ReadString(elem, "SpriteAlphaStyle"));
    s.TTFHeightDefinedBy = ReadFontHeightDefinition(ReadString(elem, "TTFHeightDefinedBy"));
    s.TTFMetricsFixup = ReadFontMetricsFixup(ReadString(elem, "TTFMetricsFixup"));
    s.TextWindowGUI = ReadInt(elem, "TextWindowGUI");
    s.ThoughtGUI = ReadInt(elem, "ThoughtGUI");
    s.TurnBeforeFacing = ReadBool(elem, "TurnBeforeFacing");
    s.TurnBeforeWalking = ReadBool(elem, "TurnBeforeWalking");
    s.UniqueID = ReadInt(elem, "UniqueID");
    s.UseGlobalSpeechAnimationDelay = ReadBool(elem, "UseGlobalSpeechAnimationDelay");
    s.UseLowResCoordinatesInScript = ReadBool(elem, "UseLowResCoordinatesInScript");
    s.UseOldCustomDialogOptionsAPI = ReadBool(elem, "UseOldCustomDialogOptionsAPI");
    s.UseOldKeyboardHandling = ReadBool(elem, "UseOldKeyboardHandling");
    s.UseOldVoiceClipNaming = ReadBool(elem, "UseOldVoiceClipNaming");
    s.TurnOrderPriority = ReadTurnOrderPriority(ReadString(elem, "TurnOrderPriority"));
    s.TextBoxKeyClaimStyle = ReadTextBoxKeyClaimStyle(ReadString(elem, "TextBoxKeyClaimStyle"));
    s.Version = ReadString(elem, "Version");
    s.WalkInLookMode = ReadBool(elem, "WalkInLookMode");
    s.WhenInterfaceDisabled = ReadGuiDisableStyle(ReadString(elem, "WhenInterfaceDisabled"));
}

void GlobalVariables::GetAll(DocElem root, std::vector<DocElem> &elems)
{
    DocElem list_node = root->FirstChildElement("GlobalVariables");
    if (!list_node)
        return;
    list_node = list_node->FirstChildElement("Variables");
    if (!list_node)
        return;
    for (DocElem node = list_node->FirstChildElement("GlobalVariable");
        node; node = node->NextSiblingElement("GlobalVariable"))
    {
        elems.push_back(node);
    }
}

void CustomPropertySchema::GetAll(DocElem root, std::vector<DocElem> &elems)
{
    DocElem list_node = root->FirstChildElement("PropertyDefinitions");
    if (!list_node)
        return;
    for (DocElem node = list_node->FirstChildElement("CustomPropertySchemaItem");
        node; node = node->NextSiblingElement("CustomPropertySchemaItem"))
    {
        elems.push_back(node);
    }
}

DocElem Game::GetSettings(DocElem elem)
{
    return elem->FirstChildElement("Settings");
}

DocElem ScriptWithHeader::GetHeader(DocElem elem)
{
    DocElem headelem = elem->FirstChildElement("ScriptAndHeader_Header");
    if (headelem)
        return headelem->FirstChildElement("Script");
    return nullptr;
}

DocElem ScriptWithHeader::GetBody(DocElem elem)
{
    DocElem headelem = elem->FirstChildElement("ScriptAndHeader_Script");
    if (headelem)
        return headelem->FirstChildElement("Script");
    return nullptr;
}


//-----------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------

void ReadEntityRef(DataUtil::EntityRef &ent, EntityParser &parser, DocElem elem)
{
    ent.TypeName = parser.ReadType(elem);
    ent.ID = parser.ReadID(elem);
    String name = parser.ReadScriptName(elem);
    // Remove any non-alphanumeric characters from the script name
    for (size_t c = 0; c < name.GetLength(); ++c)
    {
        if (!StrUtil::IsScriptWordChar(name[c]))
            name.ClipMid(c--, 1);
    }
    ent.ScriptName = name;
}

void ReadAllEntityRefs(std::vector<DataUtil::EntityRef> &ents, EntityListParser &list_parser,
    EntityParser &parser, DocElem root)
{
    std::vector<DocElem> elems;
    list_parser.GetAll(root, elems);
    for (const auto &el : elems)
    {
        DataUtil::EntityRef ent;
        ReadEntityRef(ent, parser, el);
        ents.push_back(ent);
    }
}

static void ReadGUI(DataUtil::GUIData& gui_data, AGF::DocElem elem)
{
    AGF::GUIMain gui;
    AGF::GUIControls controls;
    AGF::GUIControl control;

    ReadEntityRef(gui_data, gui, elem);
    gui.ReadAllData(elem, gui_data);

    std::vector<DocElem> elems;
    controls.GetAll(elem, elems);
    for (const auto &el : elems)
    {
        String type = control.ReadType(el);
        if(type == "Button")
        {
            auto data = std::make_shared<DataUtil::GUIButtonData>();
            ReadEntityRef(*data, control, el);
            control.ReadAllData(el, *data);
            control.ReadButtonData(el, *data);
            if (strcmp(el->Name(), "GUITextWindowEdge") == 0)
            {
                data->ClickAction = "RunScript";
                data->MouseoverImage = -1;
                data->PushedImage = -1;
            }
            gui_data.Controls.push_back(data);
        }
        else if(type == "Label")
        {
            auto data = std::make_shared<DataUtil::GUILabelData>();
            ReadEntityRef(*data, control, el);
            control.ReadAllData(el, *data);
            control.ReadLabelData(el, *data);
            gui_data.Controls.push_back(data);
        }
        else if(type == "InvWindow")
        {
            auto data = std::make_shared<DataUtil::GUIInventoryData>();
            ReadEntityRef(*data, control, el);
            control.ReadAllData(el, *data);
            control.ReadInventoryData(el, *data);
            gui_data.Controls.push_back(data);
        }
        else if(type == "ListBox")
        {
            auto data = std::make_shared<DataUtil::GUIListBoxData>();
            ReadEntityRef(*data, control, el);
            control.ReadAllData(el, *data);
            control.ReadListBoxData(el, *data);
            gui_data.Controls.push_back(data);
        }
        else if(type == "Slider")
        {
            auto data = std::make_shared<DataUtil::GUISliderData>();
            ReadEntityRef(*data, control, el);
            control.ReadAllData(el, *data);
            control.ReadSliderData(el, *data);
            gui_data.Controls.push_back(data);
        }
        else if(type == "TextBox")
        {
            auto data = std::make_shared<DataUtil::GUITextBoxData>();
            ReadEntityRef(*data, control, el);
            control.ReadAllData(el, *data);
            control.ReadTextBoxData(el, *data);
            gui_data.Controls.push_back(data);
        }
    }
}

static void ReadDialog(DataUtil::DialogRef &dialog, AGF::DocElem elem)
{
    AGF::Dialog p_dialog;
    ReadEntityRef(dialog, p_dialog, elem);
    dialog.ShowTextParser = ValueParser::ReadBool(elem, "ShowTextParser");

    DocElem options = elem->FirstChildElement("DialogOptions");
    for (DocElem option = options ? options->FirstChildElement("DialogOption") : nullptr;
         option; option = option->NextSiblingElement("DialogOption"))
    {
        DataUtil::DialogOptionData data;
        data.Text = ValueParser::ReadString(option, "Text");
        data.Say = ValueParser::ReadBool(option, "Say", true);
        data.Show = ValueParser::ReadBool(option, "Show", true);
        dialog.Options.push_back(data);
    }
    dialog.OptionCount = static_cast<int>(dialog.Options.size());
}

void ReadGlobalVariables(std::vector<DataUtil::Variable> &vars, DocElem root)
{
    AGF::GlobalVariables glvars;
    std::vector<AGF::DocElem> var_elems;
    glvars.GetAll(root, var_elems);
    if (var_elems.size() == 0)
        return;

    AGF::GlobalVariable glvar;
    for (const auto &el : var_elems)
    {
        DataUtil::Variable var;
        var.Type = glvar.ReadType(el);
        var.Name = glvar.ReadScriptName(el);
        var.Value = glvar.ReadDefaultValue(el);
        vars.push_back(var);
    }
}

void ReadCustomPropertySchema(std::vector<DataUtil::CustomPropertySchemaItem> &schema, DocElem root)
{
    AGF::CustomPropertySchema props;
    AGF::CustomPropertySchemaItem parser;
    std::vector<AGF::DocElem> prop_elems;
    props.GetAll(root, prop_elems);
    if (prop_elems.size() == 0)
        return;

    for (const auto &el : prop_elems)
    {
        DataUtil::CustomPropertySchemaItem prop;
        parser.ReadAllData(el, prop);
        schema.push_back(prop);
    }
}

void ReadGameSettings(DataUtil::GameSettings &opt, DocElem elem)
{
    AGF::Game p_game;
    AGF::GameSettings p_set;
    DocElem set_elem = p_game.GetSettings(elem);
    p_set.ReadAllData(set_elem, opt);
}

static DataUtil::FontOutlineStyle ReadFontOutlineStyle(const String &value)
{
    return StrUtil::ParseEnum(value, kFontOutlineStyleNames, DataUtil::kFontOutline_None);
}

static DataUtil::FontAutoOutlineStyle ReadFontAutoOutlineStyle(const String &value)
{
    return StrUtil::ParseEnum(value, kFontAutoOutlineStyleNames, DataUtil::kFontAutoOutline_Squared);
}

static DataUtil::SpriteImportResolution ReadSpriteImportResolution(const String &value)
{
    return StrUtil::ParseEnumWithBase(value, kSpriteImportResolutionNames,
        DataUtil::kSpriteImport_Real, DataUtil::kSpriteImport_Real);
}

static DataUtil::AudioFileBundlingType ReadAudioBundlingType(const String &value)
{
    return StrUtil::ParseEnumWithBase(value, kAudioBundlingTypeNames,
        DataUtil::kAudioBundling_InGameEXE, DataUtil::kAudioBundling_InGameEXE);
}

static DataUtil::AudioClipFileType ReadAudioFileType(const String &value)
{
    return StrUtil::ParseEnumWithBase(value, kAudioFileTypeNames,
        DataUtil::kAudioFile_OGG, DataUtil::kAudioFile_Undefined);
}

static DataUtil::CrossfadeSpeed ReadCrossfadeSpeed(const String &value)
{
    return StrUtil::ParseEnum(value, kCrossfadeSpeedNames, DataUtil::kCrossfade_No);
}

static DataUtil::ButtonColorStyle ReadButtonColorStyle(const String &value)
{
    return StrUtil::ParseEnum(value, kButtonColorStyleNames, DataUtil::kButtonColor_Default);
}

static int ReadInheritableBool(const String &value)
{
    return StrUtil::ParseEnumWithBase(value, kInheritableBoolNames, -1, -1);
}

static int ReadAudioPriority(const String &value)
{
    switch (StrUtil::ParseEnum(value, kAudioPriorityNames, 0))
    {
    case 1: return 1;
    case 2: return 25;
    case 4: return 75;
    case 5: return 100;
    case 3: return 50;
    case 0:
    default: return -1;
    }
}

static DataUtil::CharacterTurnOrderPriority ReadTurnOrderPriority(const String &value)
{
    return StrUtil::ParseEnum(value, kTurnOrderPriorityNames, DataUtil::kTurnOrder_Clockwise);
}

static DataUtil::GUITextBoxKeyClaimStyle ReadTextBoxKeyClaimStyle(const String &value)
{
    return StrUtil::ParseEnum(value, kTextBoxKeyClaimStyleNames, DataUtil::kTextBoxKeyClaim_All);
}

DataUtil::LipSyncType ReadLipSyncType(const String &value)
{
    return StrUtil::ParseEnum(value, kLipSyncTypeNames, DataUtil::kLipSync_None);
}

static DataUtil::PaletteColourType ReadColourType(const String &value)
{
    switch (StrUtil::ParseEnum(value, kPaletteTypeNames, 0))
    {
    case 1: return DataUtil::kPaletteColourType_Background;
    case 0:
    default: return DataUtil::kPaletteColourType_Gamewide;
    }
}

static void ReadProperties(DocElem elem, std::vector<DataUtil::CustomPropertyValue> &props)
{
    DocElem list = elem ? elem->FirstChildElement("Properties") : nullptr;
    for (DocElem prop = list ? list->FirstChildElement("CustomProperty") : nullptr;
         prop; prop = prop->NextSiblingElement("CustomProperty"))
    {
        DataUtil::CustomPropertyValue value;
        value.Name = ValueParser::ReadString(prop, "Name");
        value.Value = ValueParser::ReadString(prop, "Value");
        props.push_back(value);
    }
}

static void ReadInteractions(DocElem elem, String &script_module, std::vector<String> &events)
{
    DocElem interactions = elem ? elem->FirstChildElement("Interactions") : nullptr;
    if (!interactions)
        return;
    script_module = ValueParser::ReadString(interactions, "ScriptModule");
    for (DocElem event = interactions->FirstChildElement("Event");
         event; event = event->NextSiblingElement("Event"))
    {
        const int index = event->IntAttribute("Index", static_cast<int>(events.size()));
        if (index >= 0)
        {
            if (events.size() <= static_cast<size_t>(index)) events.resize(index + 1);
            events[index] = event->GetText() ? event->GetText() : "";
        }
    }
}

void AudioClip::ReadAllData(DocElem elem, DataUtil::AudioClipData &data)
{
    data.Index = ReadInt(elem, "Index");
    data.SourceFileName = ReadString(elem, "SourceFileName");
    const char *dot = std::strrchr(data.SourceFileName.GetCStr(), '.');
    data.CacheFileName = String::FromFormat("au%06X%s", data.Index, dot ? dot : "");
    data.BundlingType = ReadAudioBundlingType(ReadString(elem, "BundlingType"));
    data.Type = ReadInt(elem, "Type");
    data.FileType = ReadAudioFileType(ReadString(elem, "FileType"));

    int repeat = ReadInheritableBool(ReadString(elem, "DefaultRepeat", "Inherit"));
    int priority = ReadAudioPriority(ReadString(elem, "DefaultPriority", "Inherit"));
    int volume = ReadInt(elem, "DefaultVolume", -1);
    for (XMLNode *node = elem->Parent(); node && (repeat < 0 || priority < 0 || volume < 0);
         node = node->Parent())
    {
        DocElem folder = node->ToElement();
        if (!folder || strcmp(folder->Name(), "AudioClipFolder") != 0) continue;
        if (repeat < 0) repeat = ReadInheritableBool(ReadString(folder, "DefaultRepeat", "Inherit"));
        if (priority < 0) priority = ReadAudioPriority(ReadString(folder, "DefaultPriority", "Inherit"));
        if (volume < 0) volume = ReadInt(folder, "DefaultVolume", -1);
    }
    data.Repeat = repeat > 0;
    data.Priority = priority < 0 ? 50 : priority;
    data.Volume = volume < 0 ? 100 : volume;
}

String AudioType::ReadScriptName(DocElem elem)
{
    return BuildAudioTypePrefixedScriptID(ReadString(elem, "Name"));
}

void AudioType::ReadAllData(DocElem elem, DataUtil::AudioTypeData &data)
{
    data.Name = ReadString(elem, "Name"); // This is AudioCliptType Name
    data.MaxChannels = ReadInt(elem, "MaxChannels");
    data.VolumeReductionWhileSpeechPlaying = ReadInt(elem, "VolumeReductionWhileSpeechPlaying");
    data.Crossfade = ReadCrossfadeSpeed(ReadString(elem, "CrossfadeClips"));
}

void Character::ReadAllData(DocElem elem, DataUtil::CharacterData &data)
{
    data.AdjustSpeedWithScaling = ValueParser::ReadBool(elem, "AdjustSpeedWithScaling");
    data.AdjustVolumeWithScaling = ValueParser::ReadBool(elem, "AdjustVolumeWithScaling");
    data.AnimationDelay = ValueParser::ReadInt(elem, "AnimationDelay");
    data.Baseline = ValueParser::ReadInt(elem, "Baseline");
    data.BlinkingView = ValueParser::ReadInt(elem, "BlinkingView");
    std::sscanf(ValueParser::ReadString(elem, "BlockingRectangle", "0,0,0,0"), "%d,%d,%d,%d",
        &data.BlockingX, &data.BlockingY, &data.BlockingWidth, &data.BlockingHeight);
    data.Clickable = ValueParser::ReadBool(elem, "Clickable", true);
    data.DiagonalLoops = ValueParser::ReadBool(elem, "DiagonalLoops", true);
    data.IdleAnimationDelay = ValueParser::ReadInt(elem, "IdleAnimationDelay");
    data.IdleDelay = ValueParser::ReadInt(elem, "IdleDelay");
    data.IdleView = ValueParser::ReadInt(elem, "IdleView");
    data.MovementLinkedToAnimation = ValueParser::ReadBool(elem, "MovementLinkedToAnimation");
    data.MovementSpeed = ValueParser::ReadInt(elem, "MovementSpeed");
    data.MovementSpeedX = ValueParser::ReadInt(elem, "MovementSpeedX");
    data.MovementSpeedY = ValueParser::ReadInt(elem, "MovementSpeedY");
    data.NormalView = ValueParser::ReadInt(elem, "NormalView");
    data.RealName = ValueParser::ReadString(elem, "RealName");
    data.Solid = ValueParser::ReadBool(elem, "Solid", true);
    data.SpeechAnimationDelay = ValueParser::ReadInt(elem, "SpeechAnimationDelay");
    data.SpeechColor = ValueParser::ReadInt(elem, "SpeechColor");
    data.SpeechView = ValueParser::ReadInt(elem, "SpeechView");
    data.StartX = ValueParser::ReadInt(elem, "StartX");
    data.StartY = ValueParser::ReadInt(elem, "StartY");
    data.StartingRoom = ValueParser::ReadInt(elem, "StartingRoom", -1);
    data.ThinkingView = ValueParser::ReadInt(elem, "ThinkingView");
    data.Transparency = ValueParser::ReadInt(elem, "Transparency");
    data.TurnBeforeWalking = ValueParser::ReadBool(elem, "TurnBeforeWalking", true);
    data.TurnWhenFacing = ValueParser::ReadBool(elem, "TurnWhenFacing", true);
    data.UniformMovementSpeed = ValueParser::ReadBool(elem, "UniformMovementSpeed", true);
    data.UseRoomAreaLighting = ValueParser::ReadBool(elem, "UseRoomAreaLighting", true);
    data.UseRoomAreaScaling = ValueParser::ReadBool(elem, "UseRoomAreaScaling", true);
    ReadProperties(elem, data.Properties);
    ReadInteractions(elem, data.ScriptModule, data.InteractionEvents);
}

void View::ReadAllData(DocElem elem, DataUtil::ViewData &data)
{
    DocElem loops = elem->FirstChildElement("Loops");
    for (DocElem loop = loops ? loops->FirstChildElement("Loop") : nullptr;
         loop; loop = loop->NextSiblingElement("Loop"))
    {
        DataUtil::ViewLoopData loop_data;
        loop_data.RunNextLoop = ValueParser::ReadBool(loop, "RunNextLoop");
        DocElem frames = loop->FirstChildElement("Frames");
        for (DocElem frame = frames ? frames->FirstChildElement("ViewFrame") : nullptr;
             frame; frame = frame->NextSiblingElement("ViewFrame"))
        {
            DataUtil::ViewFrameData frame_data;
            frame_data.Delay = ValueParser::ReadInt(frame, "Delay");
            frame_data.Flipped = ValueParser::ReadBool(frame, "Flipped");
            frame_data.Image = ValueParser::ReadInt(frame, "Image");
            frame_data.Sound = ValueParser::ReadInt(frame, "Sound");
            loop_data.Frames.push_back(frame_data);
        }
        data.Loops.push_back(loop_data);
    }
}

String Font::ReadScriptName(DocElem elem)
{
    return BuildFontPrefixedScriptID(ReadString(elem, "Name"));
}

void Font::ReadAllData(DocElem elem, DataUtil::FontData &data)
{
    data.Name = ReadString(elem, "Name"); // This is Font Name
    data.AutoOutlineThickness = ValueParser::ReadInt(elem, "AutoOutlineThickness");
    data.AutoOutlineStyle = ReadFontAutoOutlineStyle(ValueParser::ReadString(elem, "AutoOutlineStyle"));
    data.CharacterSpacing = ValueParser::ReadInt(elem, "CharacterSpacing");
    data.CustomHeightValue = ValueParser::ReadInt(elem, "CustomHeightValue");
    data.HeightDefinedBy = ReadFontHeightDefinition(ValueParser::ReadString(elem, "HeightDefinedBy"));
    data.LineSpacing = ValueParser::ReadInt(elem, "LineSpacing");
    data.OutlineFont = ValueParser::ReadInt(elem, "OutlineFont");
    data.OutlineStyle = ReadFontOutlineStyle(ValueParser::ReadString(elem, "OutlineStyle"));
    data.PointSize = ValueParser::ReadInt(elem, "PointSize");
    data.SizeMultiplier = ValueParser::ReadInt(elem, "SizeMultiplier", 1);
    data.MetricsFixup = ReadFontMetricsFixup(ValueParser::ReadString(elem, "TTFMetricsFixup"));
    data.VerticalOffset = ValueParser::ReadInt(elem, "VerticalOffset");
}

static void CollectElements(DocElem elem, const char *name, std::vector<DocElem> &result)
{
    if (!elem) return;
    for (DocElem child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
    {
        if (strcmp(child->Name(), name) == 0) result.push_back(child);
        CollectElements(child, name, result);
    }
}

static std::vector<uint8_t> DecodeBase64(const char *text)
{
    std::vector<uint8_t> result;
    int value = 0, bits = -8;
    for (const unsigned char *ptr = reinterpret_cast<const unsigned char*>(text ? text : ""); *ptr; ++ptr)
    {
        const unsigned char ch = *ptr;
        int digit = ch >= 'A' && ch <= 'Z' ? ch - 'A' : ch >= 'a' && ch <= 'z' ? ch - 'a' + 26 :
            ch >= '0' && ch <= '9' ? ch - '0' + 52 : ch == '+' ? 62 : ch == '/' ? 63 : -1;
        if (digit < 0) continue;
        value = (value << 6) | digit;
        bits += 6;
        if (bits >= 0)
        {
            result.push_back(static_cast<uint8_t>((value >> bits) & 0xff));
            bits -= 8;
        }
    }
    return result;
}

template <typename T>
static void SortByID(std::vector<T> &items)
{
    std::sort(items.begin(), items.end(), [](const T &a, const T &b) { return a.ID < b.ID; });
}

static void ReadGameCommon(DataUtil::GameRef &game, DocElem root)
{
    game.PlayerCharacter = ValueParser::ReadInt(root, "PlayerCharacter", -1);
    ReadRoomList(game.Rooms, root);
    ReadGlobalVariables(game.GlobalVars, root);
    ReadCustomPropertySchema(game.PropertySchema, root);
    ReadGameSettings(game.Settings, root);
}

void ReadGameData(DataUtil::GameData &game, AGFReader &reader)
{
    game = DataUtil::GameData{};
    DocElem root = reader.GetGameRoot();

    ReadGameCommon(game, root);
    game.EditorVersion = reader.GetEditorVersion();
    // Audio clips
    {
        AGF::AudioClips list;
        AGF::AudioClip parser;
        std::vector<DocElem> elems;
        list.GetAll(root, elems);
        for (DocElem elem : elems)
        {
            DataUtil::AudioClipData data;
            ReadEntityRef(data, parser, elem);
            parser.ReadAllData(elem, data);
            game.AudioClips.push_back(data);
        }
        SortByID(game.AudioClips);
    }
    // Audio types
    {
        AGF::AudioTypes list;
        AGF::AudioType parser;
        std::vector<DocElem> elems;
        list.GetAll(root, elems);
        for (DocElem elem : elems)
        {
            DataUtil::AudioTypeData data;
            ReadEntityRef(data, parser, elem);
            parser.ReadAllData(elem, data);
            game.AudioTypes.push_back(data);
        }
        SortByID(game.AudioTypes);
    }
    // Characters
    {
        AGF::Characters list;
        AGF::Character parser;
        std::vector<DocElem> elems;
        list.GetAll(root, elems);
        for (DocElem elem : elems)
        {
            DataUtil::CharacterData data;
            ReadEntityRef(data, parser, elem);
            parser.ReadAllData(elem, data);
            game.Characters.push_back(data);
        }
        SortByID(game.Characters);
    }
    // Cursors
    {
        AGF::Cursors cursors;
        AGF::Cursor cursor;
        std::vector<AGF::DocElem> elems;
        cursors.GetAll(root, elems);
        for (const auto &el : elems)
        {
            DataUtil::CursorData data;
            ReadEntityRef(data, cursor, el);
            cursor.ReadAllData(el, data);
            game.Cursors.push_back(data);
        }
        SortByID(game.Cursors);
    }
    // Dialogs
    {
        AGF::Dialogs dialogs;
        std::vector<AGF::DocElem> elems;
        dialogs.GetAll(root, elems);
        for (const auto &el : elems)
        {
            DataUtil::DialogRef dialog;
            ReadDialog(dialog, el);
            game.Dialogs.push_back(dialog);
        }
    }
    // Fonts
    {
        AGF::Fonts list;
        AGF::Font parser;
        std::vector<DocElem> elems;
        list.GetAll(root, elems);
        for (DocElem elem : elems)
        {
            DataUtil::FontData data;
            ReadEntityRef(data, parser, elem);
            parser.ReadAllData(elem, data);
            game.Fonts.push_back(data);
        }
        SortByID(game.Fonts);
    }
    // GUI and controls
    {
        AGF::GUIs guis;
        AGF::GUIMain gui;
        AGF::GUIControls controls;
        AGF::GUIControl control;
        std::vector<AGF::DocElem> elems;
        guis.GetAll(root, elems);
        for (const auto &el : elems)
        {
            DataUtil::GUIData gui_data;
            ReadGUI(gui_data, el);
            game.GUI.push_back(gui_data);
        }
    }
    // Inventory items
    {
        AGF::Inventory inventory;
        AGF::InventoryItem invitem;
        std::vector<AGF::DocElem> elems;
        inventory.GetAll(root, elems);
        for (const auto &el : elems)
        {
            DataUtil::InventoryItemData data;
            ReadEntityRef(data, invitem, el);
            invitem.ReadAllData(el, data);
            ReadProperties(el, data.Properties);
            ReadInteractions(el, data.ScriptModule, data.InteractionEvents);
            game.Inventory.push_back(data);
        }
        SortByID(game.Inventory);
    }
    // Views
    {
        AGF::Views list;
        AGF::View parser;
        std::vector<DocElem> elems;
        list.GetAll(root, elems);
        int max_id = 0;
        std::vector<DataUtil::ViewData> views;
        for (DocElem elem : elems)
        {
            DataUtil::ViewData data;
            ReadEntityRef(data, parser, elem);
            parser.ReadAllData(elem, data);
            max_id = std::max(max_id, data.ID);
            views.push_back(data);
        }
        game.Views.resize(max_id);
        for (const auto &view : views)
            if (view.ID > 0) game.Views[view.ID - 1] = view;
    }

    // Parser dictionary, lip-sync, global messages, palette and sprite flags.
    std::vector<DocElem> elems;
    CollectElements(root->FirstChildElement("TextParser"), "TextParserWord", elems);
    for (DocElem elem : elems)
    {
        DataUtil::TextParserWordData word;
        word.Word = ValueParser::ReadString(elem, "Word");
        word.WordGroup = ValueParser::ReadInt(elem, "WordGroup");
        game.ParserWords.push_back(word);
    }

    DocElem lipsync = root->FirstChildElement("LipSync");
    game.LipSyncDefaultFrame = ValueParser::ReadInt(lipsync, "DefaultFrame");
    game.LipSync = ReadLipSyncType(ValueParser::ReadString(lipsync, "Type"));
    DocElem frames = lipsync ? lipsync->FirstChildElement("Frames") : nullptr;
    for (DocElem frame = frames ? frames->FirstChildElement("CharsForFrame") : nullptr;
         frame; frame = frame->NextSiblingElement("CharsForFrame"))
        game.LipSyncFrames.push_back(frame->GetText() ? frame->GetText() : "");

    game.GlobalMessages.resize(500);
    DocElem messages = root->FirstChildElement("GlobalMessages");
    for (DocElem msg = messages ? messages->FirstChildElement("Message") : nullptr;
         msg; msg = msg->NextSiblingElement("Message"))
    {
        const int id = msg->IntAttribute("ID", 500) - 500;
        if (id >= 0 && id < 500) game.GlobalMessages[id] = msg->GetText() ? msg->GetText() : "";
    }

    game.Palette.resize(256);
    DocElem palette = root->FirstChildElement("Palette");
    for (DocElem entry = palette ? palette->FirstChildElement("PaletteEntry") : nullptr;
         entry; entry = entry->NextSiblingElement("PaletteEntry"))
    {
        const int index = ValueParser::ReadAttributeInt(entry, "Index", -1);
        if (index < 0 || index >= 256) continue;
        auto &data = game.Palette[index];
        data.ColourType = ReadColourType(ValueParser::ReadAttributeString(entry, "Type", "Gamewide"));
        data.Red = ValueParser::ReadAttributeInt(entry, "Red");
        data.Green = ValueParser::ReadAttributeInt(entry, "Green");
        data.Blue = ValueParser::ReadAttributeInt(entry, "Blue");
    }

    elems.clear();
    CollectElements(root->FirstChildElement("Sprites"), "Sprite", elems);
    for (DocElem sprite : elems)
    {
        DataUtil::SpriteData data;
        data.Slot = ValueParser::ReadAttributeInt(sprite, "Slot", -1);
        const String resolution = ValueParser::ReadAttributeString(sprite, "Resolution", "Real");
        data.Resolution = ReadSpriteImportResolution(resolution);
        data.AlphaChannel = ValueParser::ReadAttributeBool(sprite, "AlphaChannel");
        game.Sprites.push_back(data);
    }

    DocElem plugins = root->FirstChildElement("Plugins");
    for (DocElem plugin = plugins ? plugins->FirstChildElement("Plugin") : nullptr;
         plugin; plugin = plugin->NextSiblingElement("Plugin"))
    {
        DataUtil::PluginData data;
        data.Name = ValueParser::ReadString(plugin, "FileName");
        data.Data = DecodeBase64(ValueParser::ReadString(plugin, "Data"));
        game.Plugins.push_back(data);
    }
}

// lightweight, read only the EntityRefs
void ReadGameRef(DataUtil::GameRef &game, AGFReader &reader)
{
    game = DataUtil::GameRef{};
    DocElem root = reader.GetGameRoot();

    ReadGameCommon(game, root);

    // Simple entity references.
    {
        AGF::AudioClips list;
        AGF::AudioClip parser;
        ReadAllEntityRefs(game.AudioClips, list, parser, root);
        SortByID(game.AudioClips);
    }
    {
        AGF::AudioTypes list;
        AGF::AudioType parser;
        ReadAllEntityRefs(game.AudioTypes, list, parser, root);
        SortByID(game.AudioTypes);
    }
    {
        AGF::Characters list;
        AGF::Character parser;
        ReadAllEntityRefs(game.Characters, list, parser, root);
        SortByID(game.Characters);
    }
    {
        AGF::Cursors list;
        AGF::Cursor parser;
        ReadAllEntityRefs(game.Cursors, list, parser, root);
        SortByID(game.Cursors);
    }

    // DialogRef includes the option metadata required by dialog conversion.
    {
        AGF::Dialogs list;
        std::vector<DocElem> elems;
        list.GetAll(root, elems);
        for (DocElem elem : elems)
        {
            DataUtil::DialogRef dialog;
            ReadDialog(dialog, elem);
            game.Dialogs.push_back(dialog);
        }
    }

    {
        AGF::Fonts list;
        AGF::Font parser;
        ReadAllEntityRefs(game.Fonts, list, parser, root);
        SortByID(game.Fonts);
    }

    // GUIs retain only their own reference and their control references.
    {
        AGF::GUIs list;
        AGF::GUIMain parser;
        AGF::GUIControls control_list;
        AGF::GUIControl control_parser;
        std::vector<DocElem> elems;
        list.GetAll(root, elems);
        for (DocElem elem : elems)
        {
            DataUtil::GUIRef gui;
            ReadEntityRef(gui, parser, elem);
            ReadAllEntityRefs(gui.Controls, control_list, control_parser, elem);
            game.GUI.push_back(gui);
        }
    }

    {
        AGF::Inventory list;
        AGF::InventoryItem parser;
        ReadAllEntityRefs(game.Inventory, list, parser, root);
        SortByID(game.Inventory);
    }

    // Views are indexed by ID minus one in both representations; retain empty
    // slots so that script macro generation observes the same indices.
    {
        AGF::Views list;
        AGF::View parser;
        std::vector<DocElem> elems;
        std::vector<DataUtil::EntityRef> views;
        int max_id = 0;
        list.GetAll(root, elems);
        for (DocElem elem : elems)
        {
            DataUtil::EntityRef view;
            ReadEntityRef(view, parser, elem);
            max_id = std::max(max_id, view.ID);
            views.push_back(view);
        }
        game.Views.resize(max_id);
        for (const auto &view : views)
            if (view.ID > 0) game.Views[view.ID - 1] = view;
    }
}

void ReadScriptList(std::vector<String> &script_list, DocElem root)
{
    AGF::ScriptModules scmodules;
    AGF::ScriptWithHeader scmodule;
    AGF::ScriptElem scelem;
    std::vector<DocElem> modules;
    scmodules.GetAll(root, modules);
    for (const auto &m : modules)
    {
        DocElem body = scmodule.GetBody(m);
        if (!body) continue;
        script_list.push_back(scelem.ReadFilename(body));
    }
}

void ReadScriptHeaderList(std::vector<String> &headers_list, DocElem root)
{
    AGF::ScriptModules scmodules;
    AGF::ScriptWithHeader scmodule;
    AGF::ScriptElem scelem;
    std::vector<DocElem> modules;
    scmodules.GetAll(root, modules);
    for (const auto &m : modules)
    {
        DocElem header = scmodule.GetHeader(m);
        if (!header) continue;
        headers_list.push_back(scelem.ReadFilename(header));
    }
}

void ReadRoomList(std::vector<std::pair<int, String>> &room_list, DocElem root)
{
    AGF::Rooms rooms;
    AGF::Room room;
    std::vector<DocElem> room_els;
    rooms.GetAll(root, room_els);
    for (const auto &r : room_els)
    {
        room_list.push_back(std::make_pair(room.ReadNumber(r), room.ReadDescription(r)));
    }
}

} // namespace AGF
} // namespace AGS
