// AGS Editor ImGui - Plugin API
// ============================================================================
// This is the plugin extension API for the AGS Editor (ImGui edition).
// Plugins can extend the editor with new menu items, tool panes, property
// editors, and build pipeline steps.
//
// STATUS: Placeholder — API surface defined but not yet wired to a dynamic
// loading mechanism. Plugins must currently be compiled into the editor.
//
// FUTURE: Shared library (.so/.dll/.dylib) loading with a C ABI entry point.
// ============================================================================
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace AGSEditor
{

// Forward declarations
class EditorUI;

// ============================================================================
// Plugin metadata — returned by the plugin's entry point
// ============================================================================
struct PluginInfo
{
    std::string name;           // e.g., "My Custom Tool"
    std::string author;         // e.g., "Jane Doe"
    std::string version;        // e.g., "1.0.0"
    std::string description;    // Short description
    int api_version = 1;        // Plugin API version (bump on breaking changes)
};

// ============================================================================
// Extension points — what a plugin can contribute
// ============================================================================

// A menu item contributed by a plugin (appears under a "Plugins" top-level menu)
struct PluginMenuItem
{
    std::string label;                      // Display text
    std::string shortcut;                   // Optional shortcut hint (display only)
    std::function<void()> action;           // Callback when clicked
    bool enabled = true;
};

// A custom editor pane contributed by a plugin
struct PluginPane
{
    std::string title;                      // Tab title
    std::function<void()> draw_func;        // ImGui draw callback (called every frame when visible)
};

// A build pipeline hook
enum class BuildHookPhase
{
    PreCompile,     // Before script compilation
    PostCompile,    // After script compilation, before packaging
    PostPackage,    // After game data is packaged
};

struct PluginBuildHook
{
    BuildHookPhase phase;
    std::string description;
    std::function<bool(const std::string& project_dir)> callback;   // Return false to abort build
};

// ============================================================================
// Plugin interface — implemented by each plugin
// ============================================================================
class IEditorPlugin
{
public:
    virtual ~IEditorPlugin() = default;

    /// Return metadata about the plugin
    virtual PluginInfo GetInfo() const = 0;

    /// Called once when the plugin is loaded. Use to register extensions.
    virtual bool Initialize(EditorUI& editor) = 0;

    /// Called when the plugin is unloaded
    virtual void Shutdown() = 0;

    /// Return menu items to add under Plugins menu
    virtual std::vector<PluginMenuItem> GetMenuItems() { return {}; }

    /// Return custom panes
    virtual std::vector<PluginPane> GetPanes() { return {}; }

    /// Return build hooks
    virtual std::vector<PluginBuildHook> GetBuildHooks() { return {}; }
};

// ============================================================================
// Plugin manager — manages plugin lifecycle
// ============================================================================
class PluginManager
{
public:
    /// Register a built-in plugin (compiled into the editor)
    void RegisterPlugin(std::unique_ptr<IEditorPlugin> plugin)
    {
        plugins_.push_back(std::move(plugin));
    }

    /// Initialize all registered plugins
    void InitializeAll(EditorUI& editor)
    {
        for (auto& p : plugins_)
            p->Initialize(editor);
    }

    /// Shutdown all plugins
    void ShutdownAll()
    {
        for (auto& p : plugins_)
            p->Shutdown();
        plugins_.clear();
    }

    /// Get all registered plugins
    const std::vector<std::unique_ptr<IEditorPlugin>>& GetPlugins() const
    {
        return plugins_;
    }

    /// Collect all menu items from all plugins
    std::vector<PluginMenuItem> CollectMenuItems() const
    {
        std::vector<PluginMenuItem> items;
        for (auto& p : plugins_)
        {
            auto mi = p->GetMenuItems();
            items.insert(items.end(), mi.begin(), mi.end());
        }
        return items;
    }

    /// Collect all build hooks for a given phase
    std::vector<PluginBuildHook> CollectBuildHooks(BuildHookPhase phase) const
    {
        std::vector<PluginBuildHook> hooks;
        for (auto& p : plugins_)
        {
            for (auto& h : p->GetBuildHooks())
            {
                if (h.phase == phase)
                    hooks.push_back(h);
            }
        }
        return hooks;
    }

private:
    std::vector<std::unique_ptr<IEditorPlugin>> plugins_;
};

// ============================================================================
// C entry point for shared library plugins (future use)
// ============================================================================
// Plugins should export this function:
//   extern "C" AGSEditor::IEditorPlugin* ags_editor_plugin_create();
//   extern "C" void ags_editor_plugin_destroy(AGSEditor::IEditorPlugin* plugin);
// ============================================================================

} // namespace AGSEditor
