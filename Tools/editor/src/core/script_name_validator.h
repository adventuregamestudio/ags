// AGS Editor ImGui - Script Name Validation Utilities
// Validates script names for uniqueness and format correctness.
#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace AGSEditor
{

struct GameData;

// Validates that a script name follows AGS naming rules:
// - Starts with a letter or underscore
// - Contains only letters, digits, and underscores
// - Not empty
// Returns an error message, or empty string if valid.
inline std::string ValidateScriptNameFormat(const std::string& name)
{
    if (name.empty())
        return "Script name cannot be empty.";

    if (!isalpha(name[0]) && name[0] != '_')
        return "Script name must start with a letter or underscore.";

    for (size_t i = 0; i < name.size(); i++)
    {
        char ch = name[i];
        if (!isalnum(ch) && ch != '_')
            return std::string("Script name contains invalid character '") + ch +
                   "' at position " + std::to_string(i + 1) +
                   ". Only letters, digits, and underscores are allowed.";
    }

    // Check for reserved keywords
    static const char* const reserved[] = {
        "if", "else", "while", "for", "do", "return", "break", "continue",
        "switch", "case", "default", "new", "delete", "null", "true", "false",
        "function", "int", "short", "char", "float", "bool", "void", "string",
        "import", "export", "readonly", "writeprotected", "protected",
        "static", "managed", "struct", "extends", "enum",
        "attribute", "autoptr", "builtin", "internalstring", "noloopcheck",
        "this", "const"
    };
    for (const char* kw : reserved)
    {
        if (name == kw)
            return std::string("'") + name + "' is a reserved keyword.";
    }

    return "";
}

// Checks if a script name is unique across all game entities.
// entity_type and entity_id identify the item being renamed (to exclude it from checks).
// Returns an error message, or empty string if unique.
inline std::string ValidateScriptNameUnique(const GameData& gd,
                                            const std::string& name,
                                            const std::string& entity_type = "",
                                            int entity_id = -1)
{
    // Check characters
    for (const auto& ch : gd.characters)
    {
        if (ch.script_name == name && !(entity_type == "Character" && ch.id == entity_id))
            return "Script name '" + name + "' is already used by Character " +
                   std::to_string(ch.id) + " (" + ch.real_name + ").";
    }

    // Check inventory items
    for (const auto& inv : gd.inventory_items)
    {
        if (inv.script_name == name && !(entity_type == "Inventory" && inv.id == entity_id))
            return "Script name '" + name + "' is already used by Inventory Item " +
                   std::to_string(inv.id) + " (" + inv.description + ").";
    }

    // Check dialogs
    for (const auto& dlg : gd.dialogs)
    {
        if (dlg.name == name && !(entity_type == "Dialog" && dlg.id == entity_id))
            return "Script name '" + name + "' is already used by Dialog " +
                   std::to_string(dlg.id) + ".";
    }

    // Check GUIs
    for (const auto& gui : gd.guis)
    {
        if (gui.name == name && !(entity_type == "GUI" && gui.id == entity_id))
            return "Script name '" + name + "' is already used by GUI " +
                   std::to_string(gui.id) + ".";
    }

    // Check views
    for (const auto& view : gd.views)
    {
        if (view.name == name && !(entity_type == "View" && view.id == entity_id))
            return "Script name '" + name + "' is already used by View " +
                   std::to_string(view.id) + ".";
    }

    // Check audio clips
    for (const auto& clip : gd.audio_clips)
    {
        if (clip.name == name && !(entity_type == "Audio" && clip.id == entity_id))
            return "Script name '" + name + "' is already used by Audio Clip " +
                   std::to_string(clip.id) + ".";
    }

    // Check cursors
    for (const auto& cur : gd.cursors)
    {
        if (cur.name == name && !(entity_type == "Cursor" && cur.id == entity_id))
            return "Script name '" + name + "' is already used by Cursor " +
                   std::to_string(cur.id) + ".";
    }

    // Check global variables
    for (const auto& gv : gd.global_variables)
    {
        if (gv.name == name && entity_type != "GlobalVariable")
            return "Script name '" + name + "' is already used by Global Variable.";
    }

    return "";
}

// Combined format + uniqueness validation.
// Returns error message or empty string.
inline std::string ValidateScriptName(const GameData& gd,
                                      const std::string& name,
                                      const std::string& entity_type = "",
                                      int entity_id = -1)
{
    std::string err = ValidateScriptNameFormat(name);
    if (!err.empty())
        return err;

    return ValidateScriptNameUnique(gd, name, entity_type, entity_id);
}

// Helper: shows an ImGui warning text below a script name input if validation fails.
// Call right after ImGui::InputText for the script name field.
// Returns true if the name is valid.
inline bool ShowScriptNameValidation(const GameData& gd,
                                     const std::string& name,
                                     const std::string& entity_type = "",
                                     int entity_id = -1)
{
    std::string err = ValidateScriptName(gd, name, entity_type, entity_id);
    if (!err.empty())
    {
        // Use ImGui to show the error in red
        // (caller should include imgui.h)
        return false;
    }
    return true;
}

} // namespace AGSEditor
