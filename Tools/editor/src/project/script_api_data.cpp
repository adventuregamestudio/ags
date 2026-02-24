// AGS Editor ImGui - AGS Script auto-complete data implementation
#include "script_api_data.h"
#include <algorithm>
#include <cctype>

namespace AGSEditor
{

ScriptAPIData::ScriptAPIData()
{
    InitKeywords();
    InitBuiltinTypes();
    InitBuiltinFunctions();
    InitConstants();
}

void ScriptAPIData::InitKeywords()
{
    const char* kw[] = {
        "if", "else", "while", "for", "do", "return", "break", "continue",
        "switch", "case", "default", "new", "delete", "null", "true", "false",
        "function", "int", "short", "char", "float", "bool", "void", "string",
        "import", "export", "readonly", "writeprotected", "protected",
        "static", "managed", "struct", "extends", "enum",
        "attribute", "autoptr", "builtin", "internalstring", "noloopcheck",
        "this", "const"
    };
    for (auto k : kw)
    {
        keywords_.insert(k);
        entries_.push_back({k, "", 3});
    }
}

void ScriptAPIData::InitBuiltinTypes()
{
    const char* types[] = {
        "Character", "Object", "Hotspot", "Region", "Room", "Game",
        "GUI", "GUIControl", "Button", "Label", "Slider", "ListBox",
        "TextBox", "InvWindow", "InventoryItem", "Dialog", "DialogOptionsRenderingInfo",
        "DrawingSurface", "DynamicSprite", "File", "Maths", "Mouse",
        "Overlay", "Parser", "System", "String", "ViewFrame",
        "AudioChannel", "AudioClip", "DateTime", "Dictionary",
        "Set", "TextWindowGUI", "Camera", "Viewport", "Screen",
        "Speech", "Point"
    };
    for (auto t : types)
    {
        builtin_types_.insert(t);
        entries_.push_back({t, "", 2});
    }
}

void ScriptAPIData::InitBuiltinFunctions()
{
    // Common AGS global functions with declarations
    struct FnEntry { const char* name; const char* decl; };
    FnEntry funcs[] = {
        {"Display", "void Display(const string message, ...)"},
        {"DisplayAt", "void DisplayAt(int x, int y, int width, const string message, ...)"},
        {"DisplayMessage", "void DisplayMessage(int message_number)"},
        {"DisplayTopBar", "void DisplayTopBar(int y, int text_color, int back_color, const string title, const string text, ...)"},
        {"QuitGame", "void QuitGame(int ask_first)"},
        {"RestartGame", "void RestartGame()"},
        {"SaveGameSlot", "void SaveGameSlot(int slot, const string description)"},
        {"RestoreGameSlot", "void RestoreGameSlot(int slot)"},
        {"DeleteSaveSlot", "void DeleteSaveSlot(int slot)"},
        {"Wait", "void Wait(int num_loops)"},
        {"WaitKey", "int WaitKey(int num_loops)"},
        {"WaitMouse", "int WaitMouse(int num_loops)"},
        {"WaitMouseKey", "int WaitMouseKey(int num_loops)"},
        {"IsGamePaused", "bool IsGamePaused()"},
        {"PauseGame", "void PauseGame()"},
        {"UnPauseGame", "void UnPauseGame()"},
        {"SetTimer", "void SetTimer(int timer_id, int timeout)"},
        {"IsTimerExpired", "bool IsTimerExpired(int timer_id)"},
        {"SetGlobalInt", "void SetGlobalInt(int index, int value)"},
        {"GetGlobalInt", "int GetGlobalInt(int index)"},
        {"GiveScore", "void GiveScore(int points)"},
        {"InputBox", "String InputBox(const string prompt)"},
        {"Random", "int Random(int max)"},
        {"FloatToInt", "int FloatToInt(float value, RoundDirection)"},
        {"IntToFloat", "float IntToFloat(int value)"},
        {"AbortGame", "void AbortGame(const string message, ...)"},
        {"ClaimEvent", "void ClaimEvent()"},
        {"GetLocationType", "LocationType GetLocationType(int x, int y)"},
        {"GetWalkableAreaAtRoom", "int GetWalkableAreaAtRoom(int x, int y)"},
        {"GetWalkableAreaAtScreen", "int GetWalkableAreaAtScreen(int x, int y)"},
        {"GetScalingAt", "int GetScalingAt(int x, int y)"},
        {"GetRoomProperty", "int GetRoomProperty(const string property)"},
        {"GetRoomPropertyText", "String GetRoomPropertyText(const string property)"},
        {"ProcessClick", "void ProcessClick(int x, int y, CursorMode)"},
        {"IsInteractionAvailable", "bool IsInteractionAvailable(int x, int y, CursorMode)"},
        {"SetBackgroundFrame", "void SetBackgroundFrame(int frame)"},
        {"GetBackgroundFrame", "int GetBackgroundFrame()"},
        {"PlaySound", "void PlaySound(int sound_number)"},
        {"PlaySoundEx", "AudioChannel* PlaySoundEx(int sound_number, int channel)"},
        {"PlayMusic", "void PlayMusic(int music_number)"},
        {"StopMusic", "void StopMusic()"},
        {"IsMusicPlaying", "bool IsMusicPlaying()"},
        {"SetMusicVolume", "void SetMusicVolume(int volume)"},
        {"IsVoxAvailable", "bool IsVoxAvailable()"},
        {"SetSpeechVolume", "void SetSpeechVolume(int volume)"},
        {"StartCutscene", "void StartCutscene(CutsceneSkipType)"},
        {"EndCutscene", "void EndCutscene()"},
        {"SkippingCutscene", "bool SkippingCutscene()"},
        {"FadeIn", "void FadeIn(int speed)"},
        {"FadeOut", "void FadeOut(int speed)"},
        {"SetFadeColor", "void SetFadeColor(int red, int green, int blue)"},
        {"ShakeScreen", "void ShakeScreen(int amount)"},
        {"ShakeScreenBackground", "void ShakeScreenBackground(int delay, int amount, int length)"},
        {"SetScreenTransition", "void SetScreenTransition(TransitionStyle)"},
        {"SetNextScreenTransition", "void SetNextScreenTransition(TransitionStyle)"},
        {"Debug", "void Debug(int command, int data)"},
    };

    for (auto& fn : funcs)
    {
        builtin_functions_.insert(fn.name);
        entries_.push_back({fn.name, fn.decl, 0});
    }
}

void ScriptAPIData::InitConstants()
{
    const char* consts[] = {
        // eKey constants
        "eKeyNone", "eKeyCtrlA", "eKeyCtrlB", "eKeyCtrlC", "eKeyCtrlD",
        "eKeyCtrlE", "eKeyCtrlF", "eKeyCtrlG", "eKeyBackspace", "eKeyTab",
        "eKeyReturn", "eKeyEscape", "eKeySpace",
        "eKey0", "eKey1", "eKey2", "eKey3", "eKey4", "eKey5", "eKey6", "eKey7", "eKey8", "eKey9",
        "eKeyA", "eKeyB", "eKeyC", "eKeyD", "eKeyE", "eKeyF", "eKeyG", "eKeyH",
        "eKeyI", "eKeyJ", "eKeyK", "eKeyL", "eKeyM", "eKeyN", "eKeyO", "eKeyP",
        "eKeyQ", "eKeyR", "eKeyS", "eKeyT", "eKeyU", "eKeyV", "eKeyW", "eKeyX",
        "eKeyY", "eKeyZ",
        "eKeyF1", "eKeyF2", "eKeyF3", "eKeyF4", "eKeyF5", "eKeyF6",
        "eKeyF7", "eKeyF8", "eKeyF9", "eKeyF10", "eKeyF11", "eKeyF12",
        // Cursor modes
        "eModeWalkto", "eModeLook", "eModeInteract", "eModeTalkto",
        "eModeUseinv", "eModePickup", "eModeWait", "eModePointer",
        // Direction
        "eDirectionUp", "eDirectionDown", "eDirectionLeft", "eDirectionRight",
        "eDirectionDownLeft", "eDirectionDownRight", "eDirectionUpLeft", "eDirectionUpRight",
        // Alignment
        "eAlignLeft", "eAlignCentre", "eAlignRight", "eAlignHasLeft", "eAlignHasRight",
        "eAlignHasTop", "eAlignHasBottom", "eAlignHasHorCenter", "eAlignHasVerCenter",
        // Mouse buttons
        "eMouseLeft", "eMouseRight", "eMouseMiddle", "eMouseLeftInv", "eMouseRightInv",
        "eMouseMiddleInv", "eMouseWheelNorth", "eMouseWheelSouth",
        // Block/event
        "eBlock", "eNoBlock",
        // Bool-like
        "SCR_NO_VALUE",
    };

    for (auto c : consts)
    {
        constants_.insert(c);
        entries_.push_back({c, "", 4});
    }
}

std::vector<const AutoCompleteEntry*> ScriptAPIData::FindMatches(
    const std::string& prefix, int max_results) const
{
    std::vector<const AutoCompleteEntry*> results;
    if (prefix.empty())
        return results;

    // Case-insensitive prefix match
    std::string lower_prefix = prefix;
    std::transform(lower_prefix.begin(), lower_prefix.end(), lower_prefix.begin(), ::tolower);

    for (const auto& entry : entries_)
    {
        std::string lower_name = entry.name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

        if (lower_name.compare(0, lower_prefix.size(), lower_prefix) == 0)
        {
            results.push_back(&entry);
            if ((int)results.size() >= max_results)
                break;
        }
    }

    return results;
}

void ScriptAPIData::AddUserIdentifier(const std::string& name, const std::string& declaration)
{
    builtin_functions_.insert(name);
    entries_.push_back({name, declaration, 0});
}

} // namespace AGSEditor
