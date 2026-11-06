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
#include <iterator>
#include <cstdio>
#include <tinyxml2.h>
#include "data/xml_fields.h"
#include "debug/out.h"
#include "util/file.h"
#include "util/memory_compat.h"
#include "util/path.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

const char *XML_ROOT_NODE_NAME = "AGSEditorDocument";
const char *XML_ATTRIBUTE_VERSION = "Version";
const char *XML_ATTRIBUTE_VERSION_INDEX = "VersionIndex";
const char *XML_ATTRIBUTE_EDITOR_VERSION = "EditorVersion";
// Up to 4.0.0.19 the Editor wrote this fixed tag plus a VersionIndex; since 4.0.0.20 it writes the
// project format version itself (AGSEditor.LATEST_XML_VERSION in the Editor).
const char *LEGACY_XML_VERSION_TAG = "3.0.3.2";
// The tools read only project formats that need no upgrade step; older projects must be opened
// and saved in the Editor first, so the upgrade code exists only there. 4.0.0.33 moved the dialogs
// out of Game.agf into their own files, so nothing older reads as is.
const char *OLDEST_READABLE_XML_VERSION = "4.0.0.33";
// The newest project format these tools know: keep in step with AGSEditor.LATEST_XML_VERSION.
const char *LATEST_XML_VERSION = "4.0.0.33";

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

static const CstrArr<8> kSpriteImportTransparencyNames = {{
    "PaletteIndex0", "TopLeft", "BottomLeft", "TopRight", "BottomRight", "LeaveAsIs", "NoTransparency", "PaletteIndex"
}};

static const CstrArr<2> kAudioBundlingTypeNames = {{
    "InGameEXE", "InSeparateVOX"
}};

static const CstrArr<7> kAudioFileTypeNames = {{"OGG", "MP3", "WAV", "VOC", "MIDI", "MOD", "FLAC"}};

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
        return new Error("Failed to open XML: %s", _doc->ErrorIDToName(_doc->ErrorID()));
    if (!_doc->RootElement() || strcmp(_doc->RootElement()->Name(), XML_ROOT_NODE_NAME))
        return new Error("Not a valid AGS game project");

    const char *attr_filever = _doc->RootElement()->Attribute(XML_ATTRIBUTE_VERSION);
    if (!attr_filever)
        return new Error("Game.agf format version is missing");
    const char *attr_editorver = _doc->RootElement()->Attribute(XML_ATTRIBUTE_EDITOR_VERSION);
    _editorVersion = attr_editorver ? attr_editorver : "";
    _projectDir = Path::GetParent(Path::MakeAbsolutePath(filename));
    Debug::Printf("AGFReader: opened %s,\n format: %s\n saved by AGS %s", filename, attr_filever,
                  attr_editorver ? attr_editorver : "unknown");

    if (strcmp(attr_filever, LEGACY_XML_VERSION_TAG) == 0)
        return new Error("Game.agf was saved by an older AGS Editor (format index %d). Open and save the project "
                         "in the current Editor first.",
                         _doc->RootElement()->IntAttribute(XML_ATTRIBUTE_VERSION_INDEX));
    if (CompareXmlVersions(attr_filever, OLDEST_READABLE_XML_VERSION) < 0)
        return new Error("Game.agf format %s is older than these tools can read (%s). Open and save the project in "
                         "the current Editor first.",
                         attr_filever, OLDEST_READABLE_XML_VERSION);
    if (CompareXmlVersions(attr_filever, LATEST_XML_VERSION) > 0)
        return new Error("Game.agf format %s is newer than these tools support (%s). Update the tools.", attr_filever,
                         LATEST_XML_VERSION);

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
    _projectDir.Empty();
    _dialogDocs.clear();
}

String AGFReader::GetDialogFileBase(DocElem dialog_ref) const
{
    const String name = ValueParser::ReadString(dialog_ref, "ScriptName");
    // a dialog without a script name uses its ID, after a '@' that a script name cannot have
    return name.IsEmpty() ? String::FromFormat("@dialog%d", ValueParser::ReadInt(dialog_ref, "ID")) : name;
}

DocElem AGFReader::GetDialogData(DocElem dialog_ref)
{
    const String base = GetDialogFileBase(dialog_ref);
    auto it = _dialogDocs.find(base);
    if (it == _dialogDocs.end())
    {
        auto doc = std::make_unique<Document>();
        const String filename = Path::ConcatPaths(_projectDir, String::FromFormat("Dialogs/%s.xml", base.GetCStr()));
        if (doc->LoadFile(filename.GetCStr()) != XML_SUCCESS)
            doc.reset();
        it = _dialogDocs.emplace(base, std::move(doc)).first;
    }
    return it->second ? it->second->FirstChildElement("Dialog") : nullptr;
}

String AGFReader::ReadDialogScript(DocElem dialog_ref)
{
    const String filename =
        Path::ConcatPaths(_projectDir, String::FromFormat("Dialogs/%s.dasc", GetDialogFileBase(dialog_ref).GetCStr()));
    auto in = File::OpenFileRead(filename);
    return in ? String::FromStream(in.get(), SIZE_MAX) : String();
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
    int value;
    if (name_f && name_f->GetText() && ParseXmlValue(name_f->GetText(), value))
        return value;
    return def_value;
}

float ValueParser::ReadFloat(DocElem elem, const char *field, float def_value)
{
    DocElem name_f = elem->FirstChildElement(field);
    if (name_f)
        return StrUtil::StringToFloat(name_f->GetText(), def_value);
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

static bool ParseCursorRole(const char *text, int &value)
{
    static const char *const kCursorRoleNames[] = {"Walk", "Look", "Interact", "", "UseInv", "", "Pointer", "Wait"};
    if (std::strcmp(text, "None") == 0)
    {
        value = -1;
        return true;
    }
    return ParseXmlEnum(text, kCursorRoleNames, std::size(kCursorRoleNames), value);
}

static const XmlField<DataUtil::CursorData> kCursorFields[] = {
    {"Animate", XML_READ(&DataUtil::CursorData::Animate)},
    {"AnimateOnlyOnHotspots", XML_READ(&DataUtil::CursorData::AnimateOnlyOnHotspots)},
    {"AnimateOnlyWhenMoving", XML_READ(&DataUtil::CursorData::AnimateOnlyWhenMoving)},
    {"AnimationDelay", XML_READ(&DataUtil::CursorData::AnimationDelay)},
    {"CreateEvent", XML_READ(&DataUtil::CursorData::CreateEvent)},
    {"EventFunctionName", nullptr}, // names the handlers in the editor
    {"EventLabel", nullptr},        // the property grid label
    {"EventUID", nullptr},          // read with the interaction schema, see room_build.cpp
    {"HotspotX", XML_READ(&DataUtil::CursorData::HotspotX)},
    {"HotspotY", XML_READ(&DataUtil::CursorData::HotspotY)},
    {"ID", nullptr}, // the entity reference
    {"Image", XML_READ(&DataUtil::CursorData::Image)},
    {"Name", XML_READ(&DataUtil::CursorData::Name)}, // the MouseCursor name
    {"StandardMode", XML_READ(&DataUtil::CursorData::StandardMode)},
    {"StandardRole", XML_READ_WITH(&DataUtil::CursorData::StandardRole, ParseCursorRole)},
    {"View", XML_READ(&DataUtil::CursorData::View)},
};

void Cursor::ReadAllData(DocElem elem, DataUtil::CursorData &data)
{
    ReadFields(elem, data, kCursorFields, UnknownElements::Report);
}

// Row reader for an enum member, using one of the string-to-enum readers above;
// write the row with XML_READ_CONVERTED(&T::Member, Convert)
template <typename MP, MP Member, typename CP, CP Convert>
static HError ReadConverted(const tinyxml2::XMLElement *elem, typename XmlDetail::MemberPtr<MP>::Class &obj)
{
    obj.*Member = Convert(String(XmlDetail::ElementText(elem)));
    return HError::None();
}

#define XML_READ_CONVERTED(member, convert) ReadConverted<decltype(member), member, decltype(&convert), &convert>

static FrameAlignment ReadHotspotAlignment(const String &value)
{
    return ReadFrameAlignment(value, kAlignNone);
}

static HError ReadPropertyList(const tinyxml2::XMLElement *list, std::vector<DataUtil::CustomPropertyValue> &props);

static const XmlField<DataUtil::InventoryItemData> kInventoryItemFields[] = {
    {"CursorImage", XML_READ(&DataUtil::InventoryItemData::CursorImage)},
    {"DisplayName", XML_READ(&DataUtil::InventoryItemData::Description)},
    {"HotspotAlignment", XML_READ_CONVERTED(&DataUtil::InventoryItemData::HotspotAlignment, ReadHotspotAlignment)},
    {"HotspotX", XML_READ(&DataUtil::InventoryItemData::HotspotX)},
    {"HotspotY", XML_READ(&DataUtil::InventoryItemData::HotspotY)},
    {"ID", nullptr}, // the entity reference
    {"Image", XML_READ(&DataUtil::InventoryItemData::Image)},
    {"Interactions", nullptr}, // see ReadInteractions
    {"OnAnyClick", XML_READ(&DataUtil::InventoryItemData::OnAnyClick)},
    {"PlayerStartsWithItem", XML_READ(&DataUtil::InventoryItemData::PlayerStartsWith)},
    {"Properties", XML_READ_ELEMENT(&DataUtil::InventoryItemData::Properties, ReadPropertyList)},
    {"ScriptModule", nullptr}, // see ReadInteractions
    {"ScriptName", nullptr},   // the entity reference
};

void InventoryItem::ReadAllData(DocElem elem, DataUtil::InventoryItemData &data)
{
    ReadFields(elem, data, kInventoryItemFields, UnknownElements::Report);
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

static const XmlField<DataUtil::GUIData> kGuiMainFields[] = {
    {"BackgroundColor", XML_READ(&DataUtil::GUIData::BackgroundColor)},
    {"BackgroundImage", XML_READ(&DataUtil::GUIData::BackgroundImage)},
    {"BlendMode", XML_READ_WITH(&DataUtil::GUIData::BlendMode, ParseBlendMode)},
    {"BorderColor", XML_READ(&DataUtil::GUIData::BorderColor)},
    {"Clickable", XML_READ(&DataUtil::GUIData::Clickable)},
    {"Controls", nullptr}, // read with GUIControls
    {"Height", XML_READ(&DataUtil::GUIData::Height)},
    {"ID", nullptr}, // the entity reference
    {"Left", XML_READ(&DataUtil::GUIData::Left)},
    {"Name", nullptr}, // the entity reference
    {"OnClick", XML_READ(&DataUtil::GUIData::OnClick)},
    {"Transparency", XML_READ(&DataUtil::GUIData::Transparency)},
    {"Padding", XML_READ(&DataUtil::GUIData::Padding)},
    {"PopupStyle", XML_READ_CONVERTED(&DataUtil::GUIData::PopupStyle, ReadGuiPopupStyle)},
    {"PopupYPos", XML_READ(&DataUtil::GUIData::PopupYPos)},
    {"Properties", XML_READ_ELEMENT(&DataUtil::GUIData::Properties, ReadPropertyList)},
    {"ScriptModule", XML_READ(&DataUtil::GUIData::ScriptModule)},
    {"ScriptName", nullptr}, // the entity reference
    {"TextColor", XML_READ(&DataUtil::GUIData::TextColor)},
    {"Top", XML_READ(&DataUtil::GUIData::Top)},
    {"Visible", XML_READ(&DataUtil::GUIData::Visible)},
    {"Width", XML_READ(&DataUtil::GUIData::Width)},
    {"ZOrder", XML_READ(&DataUtil::GUIData::ZOrder)},
};

void GUIMain::ReadAllData(DocElem elem, DataUtil::GUIData &gui_data)
{
    // Normal GUIs and text windows keep their properties in elements of different names
    DocElem self = GetNormalGUI(elem);
    if (!self)
    {
        self = GetTextWindow(elem);
        gui_data.IsTextWindow = static_cast<bool>(self);
    }
    if (!self)
        return;
    ReadFields(self, gui_data, kGuiMainFields, UnknownElements::Report);
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

static FrameAlignment ReadTopCenterAlignment(const String &value)
{
    return ReadFrameAlignment(value, kAlignTopCenter);
}

static FrameAlignment ReadTopLeftAlignment(const String &value)
{
    return ReadFrameAlignment(value, kAlignTopLeft);
}

static HorAlignment ReadLeftHorAlignment(const String &value)
{
    return ReadHorizontalAlignment(value, kHAlignLeft);
}

static int ReadBlendModeName(const String &value)
{
    int mode = 0;
    ParseBlendMode(value.GetCStr(), mode);
    return mode;
}

static const XmlField<DataUtil::GUIControlData> kGuiControlFields[] = {
    {"BackgroundColor", XML_READ(&DataUtil::GUIControlData::BackgroundColor)},
    {"BorderColor", XML_READ(&DataUtil::GUIControlData::BorderColor)},
    {"BorderWidth", XML_READ(&DataUtil::GUIControlData::BorderWidth)},
    {"BlendMode", XML_READ_CONVERTED(&DataUtil::GUIControlData::BlendMode, ReadBlendModeName)},
    {"Clickable", XML_READ(&DataUtil::GUIControlData::Clickable)},
    {"Enabled", XML_READ(&DataUtil::GUIControlData::Enabled)},
    {"Height", XML_READ(&DataUtil::GUIControlData::Height)},
    {"ID", nullptr}, // the entity reference
    {"Left", XML_READ(&DataUtil::GUIControlData::Left)},
    {"Locked", nullptr}, // editor only
    {"Transparency", XML_READ(&DataUtil::GUIControlData::Transparency)},
    {"PaddingX", XML_READ(&DataUtil::GUIControlData::PaddingX)},
    {"PaddingY", XML_READ(&DataUtil::GUIControlData::PaddingY)},
    {"Properties", XML_READ_ELEMENT(&DataUtil::GUIControlData::Properties, ReadPropertyList)},
    {"ScriptName", nullptr}, // the entity reference
    {"ShowBorder", XML_READ(&DataUtil::GUIControlData::ShowBorder)},
    {"SolidBackground", XML_READ(&DataUtil::GUIControlData::SolidBackground)},
    {"Top", XML_READ(&DataUtil::GUIControlData::Top)},
    {"Translated", XML_READ(&DataUtil::GUIControlData::Translated)},
    {"Visible", XML_READ(&DataUtil::GUIControlData::Visible)},
    {"Width", XML_READ(&DataUtil::GUIControlData::Width)},
    {"ZOrder", XML_READ(&DataUtil::GUIControlData::ZOrder)},
};

// Tells the elements that every control has, which kGuiControlFields reads
static bool IsCommonControlElement(const char *element)
{
    return IsListed(kGuiControlFields, element);
}

void GUIControl::ReadAllData(DocElem elem, DataUtil::GUIControlData &data)
{
    // The control's own table reports the unknown elements, after this one
    ReadFields(elem, data, kGuiControlFields, UnknownElements::Skip);
}

static const XmlField<DataUtil::GUIButtonData> kGuiButtonFields[] = {
    {"BorderShadeColor", XML_READ(&DataUtil::GUIButtonData::BorderShadeColor)},
    {"ClickAction", XML_READ(&DataUtil::GUIButtonData::ClickAction)},
    {"ClipImage", XML_READ(&DataUtil::GUIButtonData::ClipImage)},
    {"ColorStyle", XML_READ_CONVERTED(&DataUtil::GUIButtonData::ColorStyle, ReadButtonColorStyle)},
    {"Font", XML_READ(&DataUtil::GUIButtonData::Font)},
    {"Image", XML_READ(&DataUtil::GUIButtonData::Image)},
    {"MouseOverBackgroundColor", XML_READ(&DataUtil::GUIButtonData::MouseOverBackgroundColor)},
    {"MouseOverBorderColor", XML_READ(&DataUtil::GUIButtonData::MouseOverBorderColor)},
    {"MouseOverTextColor", XML_READ(&DataUtil::GUIButtonData::MouseOverTextColor)},
    {"MouseoverImage", XML_READ(&DataUtil::GUIButtonData::MouseoverImage)},
    {"NewModeNumber", XML_READ(&DataUtil::GUIButtonData::NewModeNumber)},
    {"OnClick", XML_READ(&DataUtil::GUIButtonData::OnClick)},
    {"OnFrameEvent", XML_READ(&DataUtil::GUIButtonData::OnFrameEvent)},
    {"PushedBackgroundColor", XML_READ(&DataUtil::GUIButtonData::PushedBackgroundColor)},
    {"PushedBorderColor", XML_READ(&DataUtil::GUIButtonData::PushedBorderColor)},
    {"PushedImage", XML_READ(&DataUtil::GUIButtonData::PushedImage)},
    {"PushedTextColor", XML_READ(&DataUtil::GUIButtonData::PushedTextColor)},
    {"Text", XML_READ(&DataUtil::GUIButtonData::Text)},
    {"TextAlignment", XML_READ_CONVERTED(&DataUtil::GUIButtonData::TextAlignment, ReadTopCenterAlignment)},
    {"TextColor", XML_READ(&DataUtil::GUIButtonData::TextColor)},
    {"TextOutlineColor", XML_READ(&DataUtil::GUIButtonData::TextOutlineColor)},
    {"WrapText", XML_READ(&DataUtil::GUIButtonData::WrapText)},
};

void GUIControl::ReadButtonData(DocElem elem, DataUtil::GUIButtonData &data)
{
    ReadFields(elem, data, kGuiButtonFields, UnknownElements::Report, IsCommonControlElement);
}

static const XmlField<DataUtil::GUILabelData> kGuiLabelFields[] = {
    {"Font", XML_READ(&DataUtil::GUILabelData::Font)},
    {"Text", XML_READ(&DataUtil::GUILabelData::Text)},
    {"TextAlignment", XML_READ_CONVERTED(&DataUtil::GUILabelData::TextAlignment, ReadTopLeftAlignment)},
    {"TextColor", XML_READ(&DataUtil::GUILabelData::TextColor)},
    {"TextOutlineColor", XML_READ(&DataUtil::GUILabelData::TextOutlineColor)},
};

void GUIControl::ReadLabelData(DocElem elem, DataUtil::GUILabelData &data)
{
    ReadFields(elem, data, kGuiLabelFields, UnknownElements::Report, IsCommonControlElement);
}

static const XmlField<DataUtil::GUISliderData> kGuiSliderFields[] = {
    {"BackgroundImage", XML_READ(&DataUtil::GUISliderData::BackgroundImage)},
    {"BorderShadeColor", XML_READ(&DataUtil::GUISliderData::BorderShadeColor)},
    {"HandleColor", XML_READ(&DataUtil::GUISliderData::HandleColor)},
    {"HandleImage", XML_READ(&DataUtil::GUISliderData::HandleImage)},
    {"HandleOffset", XML_READ(&DataUtil::GUISliderData::HandleOffset)},
    {"MaxValue", XML_READ(&DataUtil::GUISliderData::MaxValue)},
    {"MinValue", XML_READ(&DataUtil::GUISliderData::MinValue)},
    {"OnChange", XML_READ(&DataUtil::GUISliderData::OnChange)},
    {"Value", XML_READ(&DataUtil::GUISliderData::Value)},
};

void GUIControl::ReadSliderData(DocElem elem, DataUtil::GUISliderData &data)
{
    ReadFields(elem, data, kGuiSliderFields, UnknownElements::Report, IsCommonControlElement);
}

static const XmlField<DataUtil::GUIInventoryData> kGuiInventoryFields[] = {
    {"CharacterID", XML_READ(&DataUtil::GUIInventoryData::CharacterID)},
    {"ItemHeight", XML_READ(&DataUtil::GUIInventoryData::ItemHeight)},
    {"ItemWidth", XML_READ(&DataUtil::GUIInventoryData::ItemWidth)},
};

void GUIControl::ReadInventoryData(DocElem elem, DataUtil::GUIInventoryData &data)
{
    ReadFields(elem, data, kGuiInventoryFields, UnknownElements::Report, IsCommonControlElement);
}

static const XmlField<DataUtil::GUITextBoxData> kGuiTextBoxFields[] = {
    {"Font", XML_READ(&DataUtil::GUITextBoxData::Font)},
    {"OnActivate", XML_READ(&DataUtil::GUITextBoxData::OnActivate)},
    {"Text", XML_READ(&DataUtil::GUITextBoxData::Text)},
    {"TextAlignment", XML_READ_CONVERTED(&DataUtil::GUITextBoxData::TextAlignment, ReadTopLeftAlignment)},
    {"TextColor", XML_READ(&DataUtil::GUITextBoxData::TextColor)},
    {"TextOutlineColor", XML_READ(&DataUtil::GUITextBoxData::TextOutlineColor)},
};

void GUIControl::ReadTextBoxData(DocElem elem, DataUtil::GUITextBoxData &data)
{
    ReadFields(elem, data, kGuiTextBoxFields, UnknownElements::Report, IsCommonControlElement);
}

static const XmlField<DataUtil::GUIListBoxData> kGuiListBoxFields[] = {
    {"Font", XML_READ(&DataUtil::GUIListBoxData::Font)},
    {"OnSelectionChanged", XML_READ(&DataUtil::GUIListBoxData::OnSelectionChanged)},
    {"SelectedBackgroundColor", XML_READ(&DataUtil::GUIListBoxData::SelectedBackgroundColor)},
    {"SelectedTextColor", XML_READ(&DataUtil::GUIListBoxData::SelectedTextColor)},
    {"ShowScrollArrows", XML_READ(&DataUtil::GUIListBoxData::ShowScrollArrows)},
    {"TextAlignment", XML_READ_CONVERTED(&DataUtil::GUIListBoxData::TextAlignment, ReadLeftHorAlignment)},
    {"TextColor", XML_READ(&DataUtil::GUIListBoxData::TextColor)},
    {"TextOutlineColor", XML_READ(&DataUtil::GUIListBoxData::TextOutlineColor)},
};

void GUIControl::ReadListBoxData(DocElem elem, DataUtil::GUIListBoxData &data)
{
    ReadFields(elem, data, kGuiListBoxFields, UnknownElements::Report, IsCommonControlElement);
}

static const XmlField<DataUtil::CustomPropertySchemaItem> kCustomPropertySchemaFields[] = {
    {"AppliesToAudioClips", nullptr}, // the schema in game data does not tell what a property applies to
    {"AppliesToCharacters", XML_READ(&DataUtil::CustomPropertySchemaItem::AppliesToCharacters)},
    {"AppliesToDialogs", nullptr},
    {"AppliesToGUIControls", nullptr},
    {"AppliesToGUIs", nullptr},
    {"AppliesToHotspots", XML_READ(&DataUtil::CustomPropertySchemaItem::AppliesToHotspots)},
    {"AppliesToInvItems", XML_READ(&DataUtil::CustomPropertySchemaItem::AppliesToInvItems)},
    {"AppliesToObjects", XML_READ(&DataUtil::CustomPropertySchemaItem::AppliesToObjects)},
    {"AppliesToRegions", nullptr},
    {"AppliesToRooms", XML_READ(&DataUtil::CustomPropertySchemaItem::AppliesToRooms)},
    {"AppliesToWalkableAreas", nullptr},
    {"DefaultValue", XML_READ(&DataUtil::CustomPropertySchemaItem::DefaultValue)},
    {"Description", XML_READ(&DataUtil::CustomPropertySchemaItem::Description)},
    {"Name", XML_READ(&DataUtil::CustomPropertySchemaItem::Name)},
    {"Translated", XML_READ(&DataUtil::CustomPropertySchemaItem::Translated)},
    {"Type", XML_READ_CONVERTED(&DataUtil::CustomPropertySchemaItem::Type, ReadCustomPropertyType)},
};

void CustomPropertySchemaItem::ReadAllData(DocElem elem, DataUtil::CustomPropertySchemaItem &data)
{
    ReadFields(elem, data, kCustomPropertySchemaFields, UnknownElements::Report);
}

static const XmlField<DataUtil::GameSettings> kGameSettingsFields[] = {
    {"AllowRelativeAssetResolutions", XML_READ(&DataUtil::GameSettings::AllowRelativeAssetResolutions)},
    {"AlwaysDisplayTextAsSpeech", XML_READ(&DataUtil::GameSettings::AlwaysDisplayTextAsSpeech)},
    {"AndroidAppVersionCode", XML_READ(&DataUtil::GameSettings::AndroidAppVersionCode)},
    {"AndroidAppVersionName", XML_READ(&DataUtil::GameSettings::AndroidAppVersionName)},
    {"AndroidApplicationId", XML_READ(&DataUtil::GameSettings::AndroidApplicationId)},
    {"AndroidBuildFormat", XML_READ_CONVERTED(&DataUtil::GameSettings::AndroidBuildFormat, ReadAndroidBuildFormat)},
    {"AntiAliasFonts", XML_READ(&DataUtil::GameSettings::AntiAliasFonts)},
    {"AntiGlideMode", XML_READ(&DataUtil::GameSettings::AntiGlideMode)},
    {"AttachDataToExe", XML_READ(&DataUtil::GameSettings::AttachDataToExe)},
    {"AudioIndexer", XML_READ(&DataUtil::GameSettings::AudioIndexer)},
    {"AutoMoveInWalkMode", XML_READ(&DataUtil::GameSettings::AutoMoveInWalkMode)},
    {"BackwardsText", XML_READ(&DataUtil::GameSettings::BackwardsText)},
    {"BuildTargets", XML_READ(&DataUtil::GameSettings::BuildTargets)},
    {"ClipGUIControls", XML_READ(&DataUtil::GameSettings::ClipGUIControls)},
    {"ColorDepth", XML_READ_CONVERTED(&DataUtil::GameSettings::ColorDepth, ReadGameColorDepth)},
    {"CompressSpritesType", XML_READ_CONVERTED(&DataUtil::GameSettings::CompressSpritesType, ReadSpriteCompression)},
    {"CursorEventIndexer", nullptr}, // the editor allocates cursor events
    {"CustomDataDir", XML_READ(&DataUtil::GameSettings::CustomDataDir)},
    {"CustomResolution", XML_READ(&DataUtil::GameSettings::CustomResolution)},
    {"DebugMode", XML_READ(&DataUtil::GameSettings::DebugMode)},
    {"DefaultRoomMaskResolution", XML_READ(&DataUtil::GameSettings::DefaultRoomMaskResolution)},
    {"Description", XML_READ(&DataUtil::GameSettings::Description)},
    {"DeveloperErrorText", XML_READ(&DataUtil::GameSettings::DeveloperErrorText)},
    {"DeveloperName", XML_READ(&DataUtil::GameSettings::DeveloperName)},
    {"DeveloperURL", XML_READ(&DataUtil::GameSettings::DeveloperURL)},
    {"DialogOptionsBackwards", XML_READ(&DataUtil::GameSettings::DialogOptionsBackwards)},
    {"DialogOptionsBullet", XML_READ(&DataUtil::GameSettings::DialogOptionsBullet)},
    {"DialogOptionsGUI", XML_READ(&DataUtil::GameSettings::DialogOptionsGUI)},
    {"DialogOptionsGap", XML_READ(&DataUtil::GameSettings::DialogOptionsGap)},
    {"DialogScriptNarrateFunction", XML_READ(&DataUtil::GameSettings::DialogScriptNarrateFunction)},
    {"DialogScriptSayFunction", XML_READ(&DataUtil::GameSettings::DialogScriptSayFunction)},
    {"DisplayMultipleInventory", XML_READ(&DataUtil::GameSettings::DisplayMultipleInventory)},
    {"DisplaySingleDialogOption", XML_READ(&DataUtil::GameSettings::DisplaySingleDialogOption)},
    {"EnforceNewAudio", XML_READ(&DataUtil::GameSettings::EnforceNewAudio)},
    {"EnforceNewStrings", XML_READ(&DataUtil::GameSettings::EnforceNewStrings)},
    {"EnforceObjectBasedScript", XML_READ(&DataUtil::GameSettings::EnforceObjectBasedScript)},
    {"FaceDirectionRatio", XML_READ(&DataUtil::GameSettings::FaceDirectionRatio)},
    {"GUIDAsString", XML_READ(&DataUtil::GameSettings::GUIDAsString)},
    {"GUIHandleOnlyLeftMouseButton", XML_READ(&DataUtil::GameSettings::GUIHandleOnlyLeftMouseButton)},
    {"GameFPS", XML_READ(&DataUtil::GameSettings::GameFPS)},
    {"GameFileName", XML_READ(&DataUtil::GameSettings::GameFileName)},
    {"GameName", XML_READ(&DataUtil::GameSettings::GameName)},
    {"GameTextEncoding", XML_READ(&DataUtil::GameSettings::GameTextEncoding)},
    {"GameTextLanguage", XML_READ(&DataUtil::GameSettings::GameTextLanguage)},
    {"Genre", XML_READ(&DataUtil::GameSettings::Genre)},
    {"GlobalSpeechAnimationDelay", XML_READ(&DataUtil::GameSettings::GlobalSpeechAnimationDelay)},
    {"HandleInvClicksInScript", XML_READ(&DataUtil::GameSettings::HandleInvClicksInScript)},
    {"InventoryCursors", XML_READ(&DataUtil::GameSettings::InventoryCursors)},
    {"InventoryHotspotMarkerCrosshairColor", XML_READ(&DataUtil::GameSettings::InventoryHotspotMarkerCrosshairColor)},
    {"InventoryHotspotMarkerDotColor", XML_READ(&DataUtil::GameSettings::InventoryHotspotMarkerDotColor)},
    {"InventoryHotspotMarkerSprite", XML_READ(&DataUtil::GameSettings::InventoryHotspotMarkerSprite)},
    {"InventoryHotspotMarkerStyle",
     XML_READ_CONVERTED(&DataUtil::GameSettings::InventoryHotspotMarkerStyle, ReadInventoryHotspotMarkerStyle)},
    {"LeftToRightPrecedence", XML_READ(&DataUtil::GameSettings::LeftToRightPrecedence)},
    {"LetterboxMode", XML_READ(&DataUtil::GameSettings::LetterboxMode)},
    {"MaximumScore", XML_READ(&DataUtil::GameSettings::MaximumScore)},
    {"MouseWheelEnabled", XML_READ(&DataUtil::GameSettings::MouseWheelEnabled)},
    {"NumberDialogOptions", XML_READ_CONVERTED(&DataUtil::GameSettings::NumberDialogOptions, ReadDialogOptionsNumbering)},
    {"OptimizeSpriteStorage", XML_READ(&DataUtil::GameSettings::OptimizeSpriteStorage)},
    {"PixelPerfect", XML_READ(&DataUtil::GameSettings::PixelPerfect)},
    {"PlaySoundOnScore", XML_READ(&DataUtil::GameSettings::PlaySoundOnScore)},
    {"ReleaseDate", XML_READ(&DataUtil::GameSettings::ReleaseDate)},
    {"RenderAtScreenResolution",
     XML_READ_CONVERTED(&DataUtil::GameSettings::RenderAtScreenResolution, ReadRenderAtScreenResolution)},
    {"RoomTransition", XML_READ_CONVERTED(&DataUtil::GameSettings::RoomTransition, ReadRoomTransition)},
    {"RunGameLoopsWhileDialogOptionsDisplayed", XML_READ(&DataUtil::GameSettings::RunGameLoopsWhileDialogOptionsDisplayed)},
    {"SaveGameFileExtension", XML_READ(&DataUtil::GameSettings::SaveGameFileExtension)},
    {"SaveGameFolderName", XML_READ(&DataUtil::GameSettings::SaveGameFolderName)},
    {"SaveScreenshots", XML_READ(&DataUtil::GameSettings::SaveScreenshots)},
    {"ScaleCharacterSpriteOffsets", XML_READ(&DataUtil::GameSettings::ScaleCharacterSpriteOffsets)},
    {"ScaleMovementSpeedWithMaskResolution", XML_READ(&DataUtil::GameSettings::ScaleMovementSpeedWithMaskResolution)},
    {"ScriptAPIVersion", XML_READ(&DataUtil::GameSettings::ScriptAPIVersion)},
    {"ScriptCompatLevel", XML_READ(&DataUtil::GameSettings::ScriptCompatLevel)},
    {"ScriptCompiler", nullptr}, // agscc is the compiler it was built with
    {"SkipSpeech", XML_READ_CONVERTED(&DataUtil::GameSettings::SkipSpeech, ReadSkipSpeech)},
    {"SpeechPortraitSide", XML_READ_CONVERTED(&DataUtil::GameSettings::SpeechPortraitSide, ReadSpeechPortraitSide)},
    {"SpeechStyle", XML_READ_CONVERTED(&DataUtil::GameSettings::SpeechStyle, ReadSpeechStyle)},
    {"SplitResources", XML_READ(&DataUtil::GameSettings::SplitResources)},
    {"TTFHeightDefinedBy", XML_READ_CONVERTED(&DataUtil::GameSettings::TTFHeightDefinedBy, ReadFontHeightDefinition)},
    {"TTFMetricsFixup", XML_READ_CONVERTED(&DataUtil::GameSettings::TTFMetricsFixup, ReadFontMetricsFixup)},
    {"TextBoxKeyClaimStyle", XML_READ_CONVERTED(&DataUtil::GameSettings::TextBoxKeyClaimStyle, ReadTextBoxKeyClaimStyle)},
    {"TextWindowGUI", XML_READ(&DataUtil::GameSettings::TextWindowGUI)},
    {"ThoughtGUI", XML_READ(&DataUtil::GameSettings::ThoughtGUI)},
    {"TranslateTextParser", nullptr},            // translation source files are made by the editor
    {"TranslationExcludeFunctionCall", nullptr}, // same
    {"TranslationExcludeScriptPrefix", nullptr}, // same
    {"TranslationIncludeScriptPrefix", nullptr}, // same
    {"TurnBeforeFacing", XML_READ(&DataUtil::GameSettings::TurnBeforeFacing)},
    {"TurnBeforeWalking", XML_READ(&DataUtil::GameSettings::TurnBeforeWalking)},
    {"TurnOrderPriority", XML_READ_CONVERTED(&DataUtil::GameSettings::TurnOrderPriority, ReadTurnOrderPriority)},
    {"UniqueID", XML_READ(&DataUtil::GameSettings::UniqueID)},
    {"UseGlobalSpeechAnimationDelay", XML_READ(&DataUtil::GameSettings::UseGlobalSpeechAnimationDelay)},
    {"UseLowResCoordinatesInScript", XML_READ(&DataUtil::GameSettings::UseLowResCoordinatesInScript)},
    {"UseOldCustomDialogOptionsAPI", XML_READ(&DataUtil::GameSettings::UseOldCustomDialogOptionsAPI)},
    {"UseOldKeyboardHandling", XML_READ(&DataUtil::GameSettings::UseOldKeyboardHandling)},
    {"UseOldVoiceClipNaming", XML_READ(&DataUtil::GameSettings::UseOldVoiceClipNaming)},
    {"Version", XML_READ(&DataUtil::GameSettings::Version)},
    {"WalkInLookMode", XML_READ(&DataUtil::GameSettings::WalkInLookMode)},
    {"WhenInterfaceDisabled", XML_READ_CONVERTED(&DataUtil::GameSettings::WhenInterfaceDisabled, ReadGuiDisableStyle)},
};

void GameSettings::ReadAllData(DocElem elem, DataUtil::GameSettings &s)
{
    ReadFields(elem, s, kGameSettingsFields, UnknownElements::Report);
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

static const XmlField<DataUtil::RuntimeSetup> kRuntimeSetupFields[] = {
    {"AAScaledSprites", XML_READ(&DataUtil::RuntimeSetup::AAScaledSprites)},
    {"AutoLockMouse", XML_READ(&DataUtil::RuntimeSetup::AutoLockMouse)},
    {"CompressSaves", XML_READ(&DataUtil::RuntimeSetup::CompressSaves)},
    {"CustomAppDataPath", XML_READ(&DataUtil::RuntimeSetup::CustomAppDataPath)},
    {"CustomSavePath", XML_READ(&DataUtil::RuntimeSetup::CustomSavePath)},
    {"DigitalSound", XML_READ(&DataUtil::RuntimeSetup::AudioDriver)},
    {"FullscreenDesktop", XML_READ(&DataUtil::RuntimeSetup::FullscreenDesktop)},
    {"FullscreenGameScaling", XML_READ(&DataUtil::RuntimeSetup::FullscreenGameScaling)},
    {"GameScaling", XML_READ(&DataUtil::RuntimeSetup::WindowGameScaling)},
    {"GameScalingMultiplier", XML_READ(&DataUtil::RuntimeSetup::GameScalingMultiplier)},
    {"GraphicsDriver", XML_READ(&DataUtil::RuntimeSetup::GraphicsDriver)},
    {"GraphicsFilter", XML_READ(&DataUtil::RuntimeSetup::GraphicsFilter)},
    {"MouseSpeed", XML_READ(&DataUtil::RuntimeSetup::MouseSpeed)},
    {"RenderAtScreenResolution", XML_READ(&DataUtil::RuntimeSetup::RenderAtScreenResolution)},
    {"Rotation", XML_READ(&DataUtil::RuntimeSetup::Rotation)},
    {"ShowFPS", XML_READ(&DataUtil::RuntimeSetup::ShowFPS)},
    {"SoundCacheSize", XML_READ(&DataUtil::RuntimeSetup::SoundCacheSize)},
    {"SpriteCacheSize", XML_READ(&DataUtil::RuntimeSetup::SpriteCacheSize)},
    {"TextureCacheSize", XML_READ(&DataUtil::RuntimeSetup::TextureCacheSize)},
    {"TitleText", XML_READ(&DataUtil::RuntimeSetup::TitleText)},
    {"TouchToMouseEmulation", XML_READ(&DataUtil::RuntimeSetup::TouchToMouseEmulation)},
    {"TouchToMouseMotionMode", XML_READ(&DataUtil::RuntimeSetup::TouchToMouseMotionMode)},
    {"Translation", XML_READ(&DataUtil::RuntimeSetup::Translation)},
    {"UseCustomAppDataPath", XML_READ(&DataUtil::RuntimeSetup::UseCustomAppDataPath)},
    {"UseCustomSavePath", XML_READ(&DataUtil::RuntimeSetup::UseCustomSavePath)},
    {"UseVoicePack", XML_READ(&DataUtil::RuntimeSetup::UseVoicePack)},
    {"VSync", XML_READ(&DataUtil::RuntimeSetup::VSync)},
    {"Windowed", XML_READ(&DataUtil::RuntimeSetup::Windowed)},
};

void RuntimeSetup::ReadAllData(DocElem elem, DataUtil::RuntimeSetup &setup)
{
    ReadFields(elem, setup, kRuntimeSetupFields, UnknownElements::Report);
}

DocElem Game::GetSettings(DocElem elem)
{
    return elem->FirstChildElement("Settings");
}

DocElem Game::GetDefaultSetup(DocElem elem)
{
    return elem->FirstChildElement("RuntimeSetup");
}

struct ScriptXml
{
    String FileName;
    bool IsHeader = false;
};

static const XmlField<ScriptXml> kScriptFields[] = {
    {"Author", nullptr},      // the editor shows these in the script's properties
    {"Description", nullptr}, // same
    {"FileName", XML_READ(&ScriptXml::FileName)},
    {"IsHeader", XML_READ(&ScriptXml::IsHeader)},
    {"Key", nullptr},     // the editor's own id
    {"Name", nullptr},    // same as the author
    {"Version", nullptr}, // same as the author
};

static ScriptXml ReadScriptXml(DocElem elem)
{
    ScriptXml script;
    ReadFields(elem, script, kScriptFields, UnknownElements::Report);
    return script;
}

bool ScriptElem::IsHeader(DocElem elem)
{
    return ReadScriptXml(elem).IsHeader;
}

String ScriptElem::ReadFilename(DocElem elem)
{
    return ReadScriptXml(elem).FileName;
}

struct ScriptAndHeaderXml
{
};

static const XmlField<ScriptAndHeaderXml> kScriptAndHeaderFields[] = {
    {"ScriptAndHeader_Header", nullptr}, // see ScriptWithHeader::GetHeader
    {"ScriptAndHeader_Script", nullptr}, // see ScriptWithHeader::GetBody
};

struct PluginXml
{
    String Data;
    String FileName;
};

static const XmlField<PluginXml> kPluginFields[] = {
    {"Data", XML_READ(&PluginXml::Data)},
    {"FileName", XML_READ(&PluginXml::FileName)},
};

static PluginXml ReadPluginXml(DocElem elem)
{
    PluginXml plugin;
    ReadFields(elem, plugin, kPluginFields, UnknownElements::Report);
    return plugin;
}

String Plugin::ReadFileName(DocElem elem)
{
    return ReadPluginXml(elem).FileName;
}

String Plugin::ReadData(DocElem elem)
{
    return ReadPluginXml(elem).Data;
}

struct UnloadedRoomXml
{
    String Description;
    int Number = -1;
};

static const XmlField<UnloadedRoomXml> kUnloadedRoomFields[] = {
    {"Description", XML_READ(&UnloadedRoomXml::Description)},
    {"Number", XML_READ(&UnloadedRoomXml::Number)},
};

static UnloadedRoomXml ReadUnloadedRoomXml(DocElem elem)
{
    UnloadedRoomXml room;
    ReadFields(elem, room, kUnloadedRoomFields, UnknownElements::Report);
    return room;
}

int Room::ReadNumber(DocElem elem)
{
    return ReadUnloadedRoomXml(elem).Number;
}

String Room::ReadDescription(DocElem elem)
{
    return ReadUnloadedRoomXml(elem).Description;
}

struct TranslationXml
{
    String Name;
};

static const XmlField<TranslationXml> kTranslationFields[] = {
    {"Name", XML_READ(&TranslationXml::Name)},
};

String Translation::ReadName(DocElem elem)
{
    TranslationXml translation;
    ReadFields(elem, translation, kTranslationFields, UnknownElements::Report);
    return translation.Name;
}

struct FontFileXml
{
    String FileName;
};

static const XmlField<FontFileXml> kFontFileFields[] = {
    {"FileName", XML_READ(&FontFileXml::FileName)}, {"SourceFilename", nullptr}, // where the editor copied the font from
};

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
                data->IsTextWindowEdge = true;
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

static const XmlField<DataUtil::DialogOptionData> kDialogOptionFields[] = {
    {"ID", nullptr}, // options are numbered by their order
    {"Say", XML_READ(&DataUtil::DialogOptionData::Say)},
    {"Show", XML_READ(&DataUtil::DialogOptionData::Show)},
    {"Text", XML_READ(&DataUtil::DialogOptionData::Text)},
};

static HError ReadDialogOptions(const tinyxml2::XMLElement *list, std::vector<DataUtil::DialogOptionData> &options)
{
    return ReadList(list, "DialogOption", options, kDialogOptionFields, SIZE_MAX, UnknownElements::Report);
}

// The dialog's entry in Game.agf only names it; the rest is in Dialogs/<name>.xml (see GetDialogData)
static const XmlField<DataUtil::DialogRef> kDialogRefFields[] = {
    {"ID", nullptr},
    {"ScriptName", nullptr},
};

static const XmlField<DataUtil::DialogRef> kDialogFields[] = {
    {"DialogOptions", XML_READ_ELEMENT(&DataUtil::DialogRef::Options, ReadDialogOptions)},
    {"ID", nullptr}, // the entity reference
    {"Properties", XML_READ_ELEMENT(&DataUtil::DialogRef::Properties, ReadPropertyList)},
    {"ScriptName", nullptr}, // the entity reference
    {"ShowTextParser", XML_READ(&DataUtil::DialogRef::ShowTextParser)},
};

static void ReadDialog(DataUtil::DialogRef &dialog, AGFReader &reader, AGF::DocElem dialog_ref)
{
    AGF::Dialog p_dialog;
    ReadEntityRef(dialog, p_dialog, dialog_ref);
    DocElem elem = reader.GetDialogData(dialog_ref);
    if (!elem)
        return;
    ReadFields(elem, dialog, kDialogFields, UnknownElements::Report);
    dialog.OptionCount = static_cast<int>(dialog.Options.size());
}

struct GlobalVariableXml
{
    int ArraySize = 0;
    int ArrayType = DataUtil::kVariable_Single;
    String DefaultValue;
    String Name;
    String Type;
};

static bool ParseVariableArrayType(const char *text, int &value)
{
    static const char *const kNames[] = {"None", "Array", "DynamicArray"};
    return ParseXmlEnum(text, kNames, std::size(kNames), value);
}

static const XmlField<GlobalVariableXml> kGlobalVariableFields[] = {
    {"ArraySize", XML_READ(&GlobalVariableXml::ArraySize)},
    {"ArrayType", XML_READ_WITH(&GlobalVariableXml::ArrayType, ParseVariableArrayType)},
    {"DefaultValue", XML_READ(&GlobalVariableXml::DefaultValue)},
    {"Name", XML_READ(&GlobalVariableXml::Name)},
    {"Type", XML_READ(&GlobalVariableXml::Type)},
};

static GlobalVariableXml ReadGlobalVariableXml(DocElem elem)
{
    GlobalVariableXml var;
    ReadFields(elem, var, kGlobalVariableFields, UnknownElements::Report);
    return var;
}

String GlobalVariable::ReadType(DocElem elem)
{
    return ReadGlobalVariableXml(elem).Type;
}

String GlobalVariable::ReadScriptName(DocElem elem)
{
    return ReadGlobalVariableXml(elem).Name;
}

String GlobalVariable::ReadDefaultValue(DocElem elem)
{
    return ReadGlobalVariableXml(elem).DefaultValue;
}

DataUtil::VariableArrayType GlobalVariable::ReadArrayType(DocElem elem)
{
    return static_cast<DataUtil::VariableArrayType>(ReadGlobalVariableXml(elem).ArrayType);
}

int GlobalVariable::ReadArraySize(DocElem elem)
{
    return ReadGlobalVariableXml(elem).ArraySize;
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
        var.ArrayType = glvar.ReadArrayType(el);
        var.ArraySize = glvar.ReadArraySize(el);
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

static DataUtil::SpriteImportTransparency ReadSpriteImportTransparency(const String &value)
{
    return StrUtil::ParseEnumWithBase(value, kSpriteImportTransparencyNames,
        DataUtil::kSpriteImport_PaletteIndex0, DataUtil::kSpriteImport_LeaveAsIs);
}

static DataUtil::AudioFileBundlingType ReadAudioBundlingType(const String &value)
{
    return StrUtil::ParseEnumWithBase(value, kAudioBundlingTypeNames,
        DataUtil::kAudioBundling_InMainData, DataUtil::kAudioBundling_InMainData);
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

static void ReadInteractions(DocElem elem, String &script_module, std::vector<String> &events)
{
    // The entity's own ScriptModule (Character.ScriptModule, InventoryItem.ScriptModule) sets the
    // interactions' module; older formats kept one inside <Interactions>, which then wins
    script_module = ValueParser::ReadString(elem, "ScriptModule", script_module.GetCStr());
    DocElem interactions = elem ? elem->FirstChildElement("Interactions") : nullptr;
    if (!interactions)
        return;
    script_module = ValueParser::ReadString(interactions, "ScriptModule", script_module.GetCStr());
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

// What an audio clip has in its element, before it is merged with the defaults of its folders
struct AudioClipXml
{
    int Index = 0;
    String SourceFileName;
    DataUtil::AudioFileBundlingType BundlingType = ReadAudioBundlingType("");
    int Type = 0;
    DataUtil::AudioClipFileType FileType = ReadAudioFileType("");
    String DefaultRepeat = "Inherit";
    String DefaultPriority = "Inherit";
    int DefaultVolume = -1;
    std::vector<DataUtil::CustomPropertyValue> Properties;
};

static const XmlField<AudioClipXml> kAudioClipFields[] = {
    {"BundlingType", XML_READ_CONVERTED(&AudioClipXml::BundlingType, ReadAudioBundlingType)},
    {"DefaultPriority", XML_READ(&AudioClipXml::DefaultPriority)},
    {"DefaultRepeat", XML_READ(&AudioClipXml::DefaultRepeat)},
    {"DefaultVolume", XML_READ(&AudioClipXml::DefaultVolume)},
    {"FileLastModifiedDate", nullptr}, // the editor checks if the source has changed
    {"FileType", XML_READ_CONVERTED(&AudioClipXml::FileType, ReadAudioFileType)},
    {"ID", nullptr}, // the entity reference
    {"Index", XML_READ(&AudioClipXml::Index)},
    {"Properties", XML_READ_ELEMENT(&AudioClipXml::Properties, ReadPropertyList)},
    {"ScriptName", nullptr}, // the entity reference
    {"SourceFileName", XML_READ(&AudioClipXml::SourceFileName)},
    {"Type", XML_READ(&AudioClipXml::Type)},
};

void AudioClip::ReadAllData(DocElem elem, DataUtil::AudioClipData &data)
{
    AudioClipXml clip;
    ReadFields(elem, clip, kAudioClipFields, UnknownElements::Report);
    data.Index = clip.Index;
    data.SourceFileName = clip.SourceFileName;
    const char *dot = std::strrchr(data.SourceFileName.GetCStr(), '.');
    data.CacheFileName = String::FromFormat("au%06X%s", data.Index, dot ? dot : "");
    data.BundlingType = clip.BundlingType;
    data.Type = clip.Type;
    data.FileType = clip.FileType;

    // The clip may inherit its repeat, priority and volume from its folders
    int repeat = ReadInheritableBool(clip.DefaultRepeat);
    int priority = ReadAudioPriority(clip.DefaultPriority);
    int volume = clip.DefaultVolume;
    for (XMLNode *node = elem->Parent(); node && (repeat < 0 || priority < 0 || volume < 0);
         node = node->Parent())
    {
        DocElem folder = node->ToElement();
        if (!folder || strcmp(folder->Name(), "AudioClipFolder") != 0) continue;
        // The folder's own settings are in a child element of the same name (AudioClipFolder(XmlNode))
        DocElem settings = folder->FirstChildElement("AudioClipFolder");
        if (!settings)
            continue;
        if (repeat < 0)
            repeat = ReadInheritableBool(ReadString(settings, "DefaultRepeat", "Inherit"));
        if (priority < 0)
            priority = ReadAudioPriority(ReadString(settings, "DefaultPriority", "Inherit"));
        if (volume < 0)
            volume = ReadInt(settings, "DefaultVolume", -1);
    }
    data.Repeat = repeat > 0;
    data.Priority = priority < 0 ? 50 : priority;
    data.Volume = volume < 0 ? 100 : volume;
    data.Properties = std::move(clip.Properties);
}

String AudioType::ReadScriptName(DocElem elem)
{
    return BuildAudioTypePrefixedScriptID(ReadString(elem, "Name"));
}

static const XmlField<DataUtil::AudioTypeData> kAudioTypeFields[] = {
    {"BackwardsCompatibilityType", nullptr}, // protects the type from deletion in the editor
    {"CrossfadeClips", XML_READ_CONVERTED(&DataUtil::AudioTypeData::Crossfade, ReadCrossfadeSpeed)},
    {"MaxChannels", XML_READ(&DataUtil::AudioTypeData::MaxChannels)},
    {"Name", XML_READ(&DataUtil::AudioTypeData::Name)}, // the AudioClipType name
    {"TypeID", nullptr},                            // the entity reference
    {"VolumeReductionWhileSpeechPlaying", XML_READ(&DataUtil::AudioTypeData::VolumeReductionWhileSpeechPlaying)},
};

void AudioType::ReadAllData(DocElem elem, DataUtil::AudioTypeData &data)
{
    ReadFields(elem, data, kAudioTypeFields, UnknownElements::Report);
}

static const XmlField<DataUtil::CustomPropertyValue> kCustomPropertyFields[] = {
    {"Name", XML_READ(&DataUtil::CustomPropertyValue::Name)},
    {"Value", XML_READ(&DataUtil::CustomPropertyValue::Value)},
};

// <Properties> element into a list of name/value pairs
static HError ReadPropertyList(const tinyxml2::XMLElement *list, std::vector<DataUtil::CustomPropertyValue> &props)
{
    return ReadList(list, "CustomProperty", props, kCustomPropertyFields, SIZE_MAX, UnknownElements::Report);
}

// Rectangle "x,y,width,height" into the character's blocking rectangle
static HError ReadBlockingRect(const tinyxml2::XMLElement *elem, DataUtil::CharacterData &data)
{
    XmlRect r;
    if (!ParseXmlValue(XmlDetail::ElementText(elem), r))
        return XmlDetail::InvalidValue(elem);
    data.BlockingX = r.X;
    data.BlockingY = r.Y;
    data.BlockingWidth = r.Width;
    data.BlockingHeight = r.Height;
    return HError::None();
}

static const XmlField<DataUtil::CharacterData> kCharacterFields[] = {
    {"AdjustSpeedWithScaling", XML_READ(&DataUtil::CharacterData::AdjustSpeedWithScaling)},
    {"AdjustVolumeWithScaling", XML_READ(&DataUtil::CharacterData::AdjustVolumeWithScaling)},
    {"AnimationDelay", XML_READ(&DataUtil::CharacterData::AnimationDelay)},
    {"Baseline", XML_READ(&DataUtil::CharacterData::Baseline)},
    {"BlendMode", XML_READ_WITH(&DataUtil::CharacterData::BlendMode, ParseBlendMode)},
    {"BlinkingView", XML_READ(&DataUtil::CharacterData::BlinkingView)},
    {"BlockingRectangle", ReadBlockingRect},
    {"Clickable", XML_READ(&DataUtil::CharacterData::Clickable)},
    {"DiagonalLoops", XML_READ(&DataUtil::CharacterData::DiagonalLoops)},
    {"DisplayName", XML_READ(&DataUtil::CharacterData::RealName)},
    {"Enabled", XML_READ(&DataUtil::CharacterData::Enabled)},
    {"GraphicAnchor", XML_READ_WITH(&DataUtil::CharacterData::GraphicAnchor, ParseGraphicAnchor)},
    {"GraphicOffset", XML_READ(&DataUtil::CharacterData::GraphicOffset)},
    {"IdleAnimationDelay", XML_READ(&DataUtil::CharacterData::IdleAnimationDelay)},
    {"IdleDelay", XML_READ(&DataUtil::CharacterData::IdleDelay)},
    {"ID", nullptr}, // the entity reference
    {"IdleView", XML_READ(&DataUtil::CharacterData::IdleView)},
    {"Interactions", nullptr}, // see ReadInteractions
    {"MovementLinkedToAnimation", XML_READ(&DataUtil::CharacterData::MovementLinkedToAnimation)},
    {"MovementSpeed", XML_READ(&DataUtil::CharacterData::MovementSpeed)},
    {"MovementSpeedX", XML_READ(&DataUtil::CharacterData::MovementSpeedX)},
    {"MovementSpeedY", XML_READ(&DataUtil::CharacterData::MovementSpeedY)},
    {"NormalView", XML_READ(&DataUtil::CharacterData::NormalView)},
    {"OnAnyClick", XML_READ(&DataUtil::CharacterData::OnAnyClick)},
    {"OnFrameEvent", XML_READ(&DataUtil::CharacterData::OnFrameEvent)},
    {"Transparency", XML_READ(&DataUtil::CharacterData::Transparency)},
    {"Properties", XML_READ_ELEMENT(&DataUtil::CharacterData::Properties, ReadPropertyList)},
    {"ScriptModule", nullptr}, // see ReadInteractions
    {"ScriptName", nullptr},   // the entity reference
    {"Solid", XML_READ(&DataUtil::CharacterData::Solid)},
    {"SpeechAnimationDelay", XML_READ(&DataUtil::CharacterData::SpeechAnimationDelay)},
    {"SpeechColor", XML_READ(&DataUtil::CharacterData::SpeechColor)},
    {"SpeechView", XML_READ(&DataUtil::CharacterData::SpeechView)},
    {"StartX", XML_READ(&DataUtil::CharacterData::StartX)},
    {"StartY", XML_READ(&DataUtil::CharacterData::StartY)},
    {"StartingRoom", XML_READ(&DataUtil::CharacterData::StartingRoom)},
    {"ThinkingView", XML_READ(&DataUtil::CharacterData::ThinkingView)},
    {"TurnBeforeWalking", XML_READ(&DataUtil::CharacterData::TurnBeforeWalking)},
    {"TurnWhenFacing", XML_READ(&DataUtil::CharacterData::TurnWhenFacing)},
    {"UniformMovementSpeed", XML_READ(&DataUtil::CharacterData::UniformMovementSpeed)},
    {"UseRoomAreaLighting", XML_READ(&DataUtil::CharacterData::UseRoomAreaLighting)},
    {"UseRoomAreaScaling", XML_READ(&DataUtil::CharacterData::UseRoomAreaScaling)},
    {"Visible", XML_READ(&DataUtil::CharacterData::Visible)},
};

void Character::ReadAllData(DocElem elem, DataUtil::CharacterData &data)
{
    ReadFields(elem, data, kCharacterFields, UnknownElements::Report);
    ReadInteractions(elem, data.ScriptModule, data.InteractionEvents);
}

static const CstrArr<4> kSpriteFlipStyleNames = {{"None", "Horizontal", "Vertical", "Both"}};

static bool ParseSpriteFlipStyle(const char *text, int &value)
{
    return ParseXmlEnum(text, kSpriteFlipStyleNames, value);
}

// ViewFrame.Image is never negative
static bool ParseFrameImage(const char *text, int &value)
{
    if (!ParseXmlValue(text, value))
        return false;
    value = std::max(value, 0);
    return true;
}

static const XmlField<DataUtil::ViewFrameData> kViewFrameFields[] = {
    {"Delay", XML_READ(&DataUtil::ViewFrameData::Delay)},
    {"EventName", XML_READ(&DataUtil::ViewFrameData::EventName)},
    {"Flip", XML_READ_WITH(&DataUtil::ViewFrameData::Flip, ParseSpriteFlipStyle)},
    {"Flipped", nullptr}, // obsolete; the editor ignores it
    {"ID", nullptr},      // the frame's index in its loop
    {"Image", XML_READ_WITH(&DataUtil::ViewFrameData::Image, ParseFrameImage)},
    {"Sound", XML_READ(&DataUtil::ViewFrameData::Sound)},
    {"XOffset", XML_READ(&DataUtil::ViewFrameData::XOffset)},
    {"YOffset", XML_READ(&DataUtil::ViewFrameData::YOffset)},
};

static HError ReadViewFrames(const tinyxml2::XMLElement *frames, std::vector<DataUtil::ViewFrameData> &list)
{
    return ReadList(frames, "ViewFrame", list, kViewFrameFields, SIZE_MAX, UnknownElements::Report);
}

static const XmlField<DataUtil::ViewLoopData> kViewLoopFields[] = {
    {"Frames", XML_READ_ELEMENT(&DataUtil::ViewLoopData::Frames, ReadViewFrames)},
    {"ID", nullptr}, // the loop's index in its view
    {"RunNextLoop", XML_READ(&DataUtil::ViewLoopData::RunNextLoop)},
};

static HError ReadViewLoops(const tinyxml2::XMLElement *loops, std::vector<DataUtil::ViewLoopData> &list)
{
    return ReadList(loops, "Loop", list, kViewLoopFields, SIZE_MAX, UnknownElements::Report);
}

static const XmlField<DataUtil::ViewData> kViewFields[] = {
    {"ID", nullptr}, // the entity reference
    {"Loops", XML_READ_ELEMENT(&DataUtil::ViewData::Loops, ReadViewLoops)},
    {"ScriptName", nullptr}, // the entity reference
};

void View::ReadAllData(DocElem elem, DataUtil::ViewData &data)
{
    ReadFields(elem, data, kViewFields, UnknownElements::Report);
}

String Font::ReadScriptName(DocElem elem)
{
    return BuildFontPrefixedScriptID(ReadString(elem, "Name"));
}

static const XmlField<DataUtil::FontData> kFontFields[] = {
    {"AutoOutlineStyle", XML_READ_CONVERTED(&DataUtil::FontData::AutoOutlineStyle, ReadFontAutoOutlineStyle)},
    {"AutoOutlineThickness", XML_READ(&DataUtil::FontData::AutoOutlineThickness)},
    {"CharacterSpacing", XML_READ(&DataUtil::FontData::CharacterSpacing)},
    {"CustomHeightValue", XML_READ(&DataUtil::FontData::CustomHeightValue)},
    {"FontFileName", XML_READ(&DataUtil::FontData::FontFileName)},
    {"HeightDefinedBy", XML_READ_CONVERTED(&DataUtil::FontData::HeightDefinedBy, ReadFontHeightDefinition)},
    {"ID", nullptr}, // the entity reference
    {"LineSpacing", XML_READ(&DataUtil::FontData::LineSpacing)},
    {"Name", XML_READ(&DataUtil::FontData::Name)}, // the Font name
    {"OutlineFont", XML_READ(&DataUtil::FontData::OutlineFont)},
    {"OutlineStyle", XML_READ_CONVERTED(&DataUtil::FontData::OutlineStyle, ReadFontOutlineStyle)},
    {"PointSize", XML_READ(&DataUtil::FontData::PointSize)},
    {"SizeMultiplier", XML_READ(&DataUtil::FontData::SizeMultiplier)},
    {"TTFMetricsFixup", XML_READ_CONVERTED(&DataUtil::FontData::MetricsFixup, ReadFontMetricsFixup)},
    {"VerticalOffset", XML_READ(&DataUtil::FontData::VerticalOffset)},
};

void Font::ReadAllData(DocElem elem, DataUtil::FontData &data)
{
    ReadFields(elem, data, kFontFields, UnknownElements::Report);
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

static const XmlField<DataUtil::GameRef> kGameFields[] = {
    // The lists are read by their own functions, see ReadGameData
    {"AudioClipTypes", nullptr},
    {"AudioClips", nullptr},
    {"Characters", nullptr},
    {"Cursors", nullptr},
    {"DeletedViews", nullptr}, // the editor keeps views you may undelete
    {"Dialogs", nullptr},
    {"FontFiles", nullptr},
    {"Fonts", nullptr},
    {"GUIs", nullptr},
    {"GlobalMessages", nullptr}, // older projects
    {"GlobalVariables", nullptr},
    {"InventoryItems", nullptr},
    {"LipSync", nullptr},
    {"Palette", nullptr},
    {"PlayerCharacter", XML_READ(&DataUtil::GameRef::PlayerCharacter)},
    {"Plugins", nullptr},
    {"PropertyDefinitions", nullptr},
    {"Rooms", nullptr},
    {"RuntimeSetup", nullptr},
    {"Scripts", nullptr},
    {"Settings", nullptr},
    {"Sprites", nullptr},
    {"TextParser", nullptr},
    {"Translations", nullptr},
    {"Views", nullptr},
};

static void ReadGameCommon(DataUtil::GameRef &game, DocElem root)
{
    ReadFields(root, game, kGameFields, UnknownElements::Report);
    ReadRoomList(game.Rooms, root);
    ReadGlobalVariables(game.GlobalVars, root);
    ReadCustomPropertySchema(game.PropertySchema, root);
    ReadGameSettings(game.Settings, root);
}

void ReadGamePalette(std::vector<DataUtil::PaletteEntryData> &pal_data, DocElem root)
{
    DocElem palette = root->FirstChildElement("Palette");
    for (DocElem entry = palette ? palette->FirstChildElement("PaletteEntry") : nullptr;
        entry; entry = entry->NextSiblingElement("PaletteEntry"))
    {
        const int index = ValueParser::ReadAttributeInt(entry, "Index", -1);
        if (index < 0 || index >= 256) continue;
        if (pal_data.size() <= static_cast<uint8_t>(index))
            pal_data.resize(index + 1);
        auto &data = pal_data[index];
        data.ColourType = ReadColourType(ValueParser::ReadAttributeString(entry, "Type", "Gamewide"));
        data.Red = ValueParser::ReadAttributeInt(entry, "Red");
        data.Green = ValueParser::ReadAttributeInt(entry, "Green");
        data.Blue = ValueParser::ReadAttributeInt(entry, "Blue");
    }
}

static bool ParseSpriteImportColorDepth(const char *text, int &value)
{
    static const char *const kNames[] = {"GameDefault", "", "", "", "", "", "", "", "Indexed8Bit"};
    return ParseXmlEnum(text, kNames, std::size(kNames), value);
}

static const XmlField<DataUtil::SpriteData> kSpriteSourceFields[] = {
    {"FileName", XML_READ(&DataUtil::SpriteData::SourceFile)},
    {"Frame", XML_READ(&DataUtil::SpriteData::ImportFrame)},
    {"ImportAlphaChannel", XML_READ(&DataUtil::SpriteData::ImportAlphaChannel)}, // older projects
    {"ImportAsTile", XML_READ(&DataUtil::SpriteData::ImportAsTile)},
    {"ImportColorDepth", XML_READ_WITH(&DataUtil::SpriteData::ImportColorDepth, ParseSpriteImportColorDepth)},
    {"ImportHeight", XML_READ(&DataUtil::SpriteData::ImportHeight)},
    {"ImportMethod", XML_READ_CONVERTED(&DataUtil::SpriteData::TransparentColour, ReadSpriteImportTransparency)},
    {"ImportWidth", XML_READ(&DataUtil::SpriteData::ImportWidth)},
    {"OffsetX", XML_READ(&DataUtil::SpriteData::ImportOffsetX)},
    {"OffsetY", XML_READ(&DataUtil::SpriteData::ImportOffsetY)},
    {"RemapToGamePalette", XML_READ(&DataUtil::SpriteData::RemapToGamePalette)},
    {"RemapToRoomPalette", XML_READ(&DataUtil::SpriteData::RemapToRoomPalette)},
    {"TransparentColorIndex", XML_READ(&DataUtil::SpriteData::TransparentColourIndex)},
};

static HError ReadSpriteSource(const tinyxml2::XMLElement *elem, DataUtil::SpriteData &data)
{
    return ReadFields(elem, data, kSpriteSourceFields, UnknownElements::Report);
}

// The sprite's slot, size and so on are attributes; the import settings are in its Source element
static const XmlField<DataUtil::SpriteData> kSpriteFields[] = {
    {"Source", ReadSpriteSource},
};

static void GetAllSprites(std::vector<DataUtil::SpriteData> &sprites, DocElem root)
{
    std::vector<DocElem> elems;
    CollectElements(root->FirstChildElement("Sprites"), "Sprite", elems);
    for (DocElem sprite : elems)
    {
        DataUtil::SpriteData data;
        data.Slot = ValueParser::ReadAttributeInt(sprite, "Slot", -1);
        data.Width = ValueParser::ReadAttributeInt(sprite, "Width", 0);
        data.Height = ValueParser::ReadAttributeInt(sprite, "Height", 0);
        data.ColorDepth = ValueParser::ReadAttributeInt(sprite, "ColorDepth", 0);
        data.Resolution = ReadSpriteImportResolution(ValueParser::ReadAttributeString(sprite, "Resolution", "Real"));
        data.AlphaChannel = ValueParser::ReadAttributeBool(sprite, "AlphaChannel");
        data.ColoursLockedToRoom = ValueParser::ReadAttributeInt(sprite, "ColoursLockedToRoom", 0);
        ReadFields(sprite, data, kSpriteFields, UnknownElements::Report);
        sprites.push_back(data);
    }
}

static const XmlField<DataUtil::TextParserWordData> kTextParserWordFields[] = {
    {"Type", nullptr}, // the editor's own grouping
    {"Word", XML_READ(&DataUtil::TextParserWordData::Word)},
    {"WordGroup", XML_READ(&DataUtil::TextParserWordData::WordGroup)},
};

// <Frames> element: the characters that make each lip sync frame
static HError ReadCharsForFrame(const tinyxml2::XMLElement *list, std::vector<String> &frames)
{
    frames.clear();
    for (const tinyxml2::XMLElement *frame = list->FirstChildElement(); frame; frame = frame->NextSiblingElement())
    {
        if (strcmp(frame->Name(), "CharsForFrame") != 0)
            return XmlDetail::UnknownElement(list, frame);
        frames.push_back(XmlDetail::ElementText(frame));
    }
    return HError::None();
}

static const XmlField<DataUtil::GameData> kLipSyncFields[] = {
    {"DefaultFrame", XML_READ(&DataUtil::GameData::LipSyncDefaultFrame)},
    {"Frames", XML_READ_ELEMENT(&DataUtil::GameData::LipSyncFrames, ReadCharsForFrame)},
    {"Type", XML_READ_CONVERTED(&DataUtil::GameData::LipSync, ReadLipSyncType)},
};

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
            ReadDialog(dialog, reader, el);
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
        SortByID(game.GUI); // folders may list them in another order
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
        ReadFields(elem, word, kTextParserWordFields, UnknownElements::Report);
        game.ParserWords.push_back(word);
    }

    if (DocElem lipsync = root->FirstChildElement("LipSync"))
        ReadFields(lipsync, game, kLipSyncFields, UnknownElements::Report);

    game.GlobalMessages.resize(500);
    DocElem messages = root->FirstChildElement("GlobalMessages");
    for (DocElem msg = messages ? messages->FirstChildElement("Message") : nullptr;
         msg; msg = msg->NextSiblingElement("Message"))
    {
        const int id = msg->IntAttribute("ID", 500) - 500;
        if (id >= 0 && id < 500) game.GlobalMessages[id] = msg->GetText() ? msg->GetText() : "";
    }

    game.Palette.resize(256);
    ReadGamePalette(game.Palette, root);

    GetAllSprites(game.Sprites, root);

    DocElem plugins = root->FirstChildElement("Plugins");
    AGF::Plugin plugin_parser;
    for (DocElem plugin = plugins ? plugins->FirstChildElement("Plugin") : nullptr; plugin;
         plugin = plugin->NextSiblingElement("Plugin"))
    {
        DataUtil::PluginData data;
        data.Name = plugin_parser.ReadFileName(plugin);
        data.Data = DecodeBase64(plugin_parser.ReadData(plugin).GetCStr());
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
            ReadDialog(dialog, reader, elem);
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
        SortByID(game.GUI); // folders may list them in another order
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

void ReadRuntimeSetup(DataUtil::RuntimeSetup &setup, DocElem elem)
{
    AGF::Game p_game;
    AGF::RuntimeSetup p_set;
    DocElem set_elem = p_game.GetDefaultSetup(elem);
    p_set.ReadAllData(set_elem, setup);
}

void ReadAudioClips(std::vector<DataUtil::AudioClipData> &clips, DocElem root)
{
    AGF::AudioClips aclips;
    AGF::AudioClip aclip;
    std::vector<DocElem> aclip_els;
    aclips.GetAll(root, aclip_els);
    for (const auto &a_el : aclip_els)
    {
        DataUtil::AudioClipData adata;
        aclip.ReadAllData(a_el, adata);
        clips.push_back(adata);
    }
}

void ReadCustomDataDirectories(std::vector<String> &dirs, DocElem root)
{
    AGF::Game p_game;
    AGF::GameSettings p_set;
    DataUtil::GameSettings set;
    p_set.ReadAllData(p_game.GetSettings(root), set);
    const auto game_dirs = set.CustomDataDir.Split(',');
    dirs.clear();
    for (const auto &dir : game_dirs)
        if (!dir.IsEmpty())
            dirs.push_back(dir);
}

void ReadFontList(std::vector<int> &font_list, DocElem root)
{
    AGF::Fonts fonts;
    AGF::Font font;
    std::vector<DocElem> font_els;
    fonts.GetAll(root, font_els);
    for (const auto &t : font_els)
    {
        font_list.push_back(font.ReadID(t));
    }
}

void ReadFontFileList(std::vector<String> &font_files, DocElem root)
{
    DocElem list = root ? root->FirstChildElement("FontFiles") : nullptr;
    for (DocElem file = list ? list->FirstChildElement("FontFile") : nullptr; file;
         file = file->NextSiblingElement("FontFile"))
    {
        FontFileXml font_file;
        ReadFields(file, font_file, kFontFileFields, UnknownElements::Report);
        if (!font_file.FileName.IsEmpty())
            font_files.push_back(font_file.FileName);
    }
}

void ReadPluginList(std::vector<String> &plugin_list, DocElem root)
{
    AGF::Plugins plugins;
    AGF::Plugin plugin;
    std::vector<DocElem> pl_els;
    plugins.GetAll(root, pl_els);
    for (const auto &pl : pl_els)
    {
        plugin_list.push_back(plugin.ReadFileName(pl));
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
    // by number, as the editor lists them, whatever the folders
    std::stable_sort(room_list.begin(), room_list.end(),
                     [](const std::pair<int, String> &a, const std::pair<int, String> &b)
                     { return a.first < b.first; });
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

void ReadSpriteList(std::vector<DataUtil::SpriteData> &sprites, DocElem root)
{
    GetAllSprites(sprites, root);
}

void ReadTranslationList(std::vector<String> &trs_list, DocElem root)
{
    AGF::Translations translations;
    AGF::Translation translation;
    std::vector<DocElem> trs_els;
    translations.GetAll(root, trs_els);
    for (const auto &t : trs_els)
    {
        trs_list.push_back(translation.ReadName(t));
    }
}

template <typename T, size_t N>
static void AppendElements(std::vector<const char *> &names, const XmlField<T> (&fields)[N])
{
    for (const auto &field : fields)
        names.push_back(field.Element);
}

template <typename T, size_t N>
static void AddKnown(std::vector<KnownElements> &list, const char *class_name, const XmlField<T> (&fields)[N])
{
    KnownElements entry{class_name, {}};
    AppendElements(entry.Elements, fields);
    list.push_back(std::move(entry));
}

// GUI controls read the common table first, then their own
template <typename T, size_t N>
static void AddKnownControl(std::vector<KnownElements> &list, const char *class_name,
                            const XmlField<T> (&own_fields)[N])
{
    KnownElements entry{class_name, {}};
    AppendElements(entry.Elements, kGuiControlFields);
    AppendElements(entry.Elements, own_fields);
    list.push_back(std::move(entry));
}

const std::vector<KnownElements> &GetKnownElements()
{
    static const std::vector<KnownElements> known = []
    {
        std::vector<KnownElements> list;
        AddKnown(list, "AudioClip", kAudioClipFields);
        AddKnown(list, "AudioClipType", kAudioTypeFields);
        AddKnown(list, "Character", kCharacterFields);
        AddKnown(list, "CustomProperty", kCustomPropertyFields);
        AddKnown(list, "CustomPropertySchemaItem", kCustomPropertySchemaFields);
        AddKnown(list, "Dialog", kDialogFields);
        AddKnown(list, "DialogOption", kDialogOptionFields);
        AddKnown(list, "DialogRef", kDialogRefFields);
        AddKnown(list, "FontFile", kFontFileFields);
        AddKnown(list, "Game", kGameFields);
        AddKnown(list, "GlobalVariable", kGlobalVariableFields);
        AddKnown(list, "LipSync", kLipSyncFields);
        AddKnown(list, "NormalGUI", kGuiMainFields);
        AddKnown(list, "Plugin", kPluginFields);
        AddKnown(list, "Script", kScriptFields);
        AddKnown(list, "ScriptAndHeader", kScriptAndHeaderFields);
        AddKnown(list, "Sprite", kSpriteFields);
        AddKnown(list, "Sprite.Source", kSpriteSourceFields);
        AddKnown(list, "TextParserWord", kTextParserWordFields);
        AddKnown(list, "TextWindowGUI", kGuiMainFields);
        AddKnown(list, "Translation", kTranslationFields);
        AddKnown(list, "UnloadedRoom", kUnloadedRoomFields);
        AddKnown(list, "Font", kFontFields);
        AddKnownControl(list, "GUIButton", kGuiButtonFields);
        AddKnownControl(list, "GUIInventory", kGuiInventoryFields);
        AddKnownControl(list, "GUILabel", kGuiLabelFields);
        AddKnownControl(list, "GUIListBox", kGuiListBoxFields);
        AddKnownControl(list, "GUISlider", kGuiSliderFields);
        AddKnownControl(list, "GUITextBox", kGuiTextBoxFields);
        AddKnownControl(list, "GUITextWindowEdge", kGuiButtonFields);
        AddKnown(list, "InventoryItem", kInventoryItemFields);
        AddKnown(list, "MouseCursor", kCursorFields);
        AddKnown(list, "RuntimeSetup", kRuntimeSetupFields);
        AddKnown(list, "Settings", kGameSettingsFields);
        AddKnown(list, "View", kViewFields);
        AddKnown(list, "ViewFrame", kViewFrameFields);
        AddKnown(list, "ViewLoop", kViewLoopFields);
        return list;
    }();
    return known;
}

} // namespace AGF
} // namespace AGS
