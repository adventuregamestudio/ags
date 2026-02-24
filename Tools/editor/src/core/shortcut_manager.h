// AGS Editor ImGui - Keyboard Shortcut Manager
// Centralizes keyboard shortcut handling. Shortcuts can be registered with
// callbacks and are processed each frame. Menu items display the same shortcut
// text but the actual key handling happens here.
#pragma once

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

#include "imgui.h"

namespace AGSEditor
{

// Modifier flags
enum ShortcutMod : int
{
    kModNone  = 0,
    kModCtrl  = 1 << 0,
    kModShift = 1 << 1,
    kModAlt   = 1 << 2,
};

// A registered shortcut
struct Shortcut
{
    std::string id;          // Unique ID (e.g., "file.save")
    std::string label;       // Display text (e.g., "Ctrl+S")
    ImGuiKey key;            // Primary key
    int modifiers;           // ShortcutMod flags
    std::string category;    // For preferences display
    std::function<void()> callback;
    bool enabled = true;
};

class ShortcutManager
{
public:
    ShortcutManager() = default;

    // Register a shortcut
    void Register(const std::string& id,
                  const std::string& label,
                  ImGuiKey key,
                  int modifiers,
                  const std::string& category,
                  std::function<void()> callback)
    {
        Shortcut sc;
        sc.id = id;
        sc.label = label;
        sc.key = key;
        sc.modifiers = modifiers;
        sc.category = category;
        sc.callback = std::move(callback);
        shortcuts_.push_back(std::move(sc));
        id_map_[id] = (int)shortcuts_.size() - 1;
    }

    // Enable/disable a specific shortcut
    void SetEnabled(const std::string& id, bool enabled)
    {
        auto it = id_map_.find(id);
        if (it != id_map_.end())
            shortcuts_[it->second].enabled = enabled;
    }

    // Process all shortcuts — call once per frame
    void ProcessShortcuts()
    {
        // Don't process shortcuts when typing in a text input
        if (ImGui::GetIO().WantTextInput)
            return;

        const ImGuiIO& io = ImGui::GetIO();
        for (auto& sc : shortcuts_)
        {
            if (!sc.enabled || !sc.callback)
                continue;

            // Check modifiers
            bool ctrl_ok  = ((sc.modifiers & kModCtrl)  != 0) == io.KeyCtrl;
            bool shift_ok = ((sc.modifiers & kModShift) != 0) == io.KeyShift;
            bool alt_ok   = ((sc.modifiers & kModAlt)   != 0) == io.KeyAlt;

            if (ctrl_ok && shift_ok && alt_ok && ImGui::IsKeyPressed(sc.key, false))
            {
                sc.callback();
            }
        }
    }

    // Get label for a shortcut by ID
    const char* GetLabel(const std::string& id) const
    {
        auto it = id_map_.find(id);
        if (it != id_map_.end())
            return shortcuts_[it->second].label.c_str();
        return "";
    }

    // Get all shortcuts (for preferences display)
    const std::vector<Shortcut>& GetAll() const { return shortcuts_; }

private:
    std::vector<Shortcut> shortcuts_;
    std::unordered_map<std::string, int> id_map_;
};

} // namespace AGSEditor
