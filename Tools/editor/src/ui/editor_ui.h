// AGS Editor ImGui - Main Editor UI
#pragma once

#include <memory>
#include <vector>
#include <string>

class AGSEditorApp;

namespace AGSEditor
{

class MenuBar;
class ProjectPanel;
class PropertiesPanel;
class LogPanel;
class Project;

// Editor panes (tabbed)
class ScriptEditor;
class RoomEditor;
class SpriteManager;
class SettingsPane;
class WelcomePane;
class CharacterEditor;
class DialogEditor;
class ViewEditor;
class GUIEditor;
class FontEditor;
class AudioManager;
class InventoryEditor;
class CursorEditor;
class PaletteEditor;
class BuildPane;
class PreferencesPane;
class TranslationEditor;
class GlobalVariablesEditor;
class CustomPropertiesEditor;
class TextParserEditor;
class LipSyncEditor;
class DebugLogPane;

// Base class for editor panes that appear in the tabbed center area
class EditorPane
{
public:
    EditorPane() : pane_id_(NextPaneId()) {}
    virtual ~EditorPane() = default;
    virtual void Draw() = 0;
    virtual const char* GetTitle() const = 0;
    virtual bool IsOpen() const { return open_; }
    virtual void Close() { open_ = false; }
    virtual void Open() { open_ = true; }
    virtual bool IsModified() const { return false; }
    virtual std::string GetHelpKeyword() const { return ""; }
    virtual void FocusEvents() {}  // F4: scroll to events section
    bool* GetOpenPtr() { return &open_; }
    int GetPaneId() const { return pane_id_; }
protected:
    bool open_ = true;
private:
    int pane_id_;
    static int NextPaneId() { static int s_next = 0; return ++s_next; }
};

class EditorUI
{
public:
    explicit EditorUI(AGSEditorApp& app);
    ~EditorUI();

    void Draw();

    // Pane management
    void OpenPane(std::unique_ptr<EditorPane> pane);
    void OpenWelcomePane();
    void NotifyProjectLoaded(); // Rebuild project tree after loading
    void NotifyProjectClosed(); // Reset UI after closing project

    // Open or focus an existing pane by type (prevents duplicates)
    template <typename T, typename... Args>
    T* OpenOrFocusPane(Args&&... args)
    {
        // Check if a pane of this type is already open
        for (size_t i = 0; i < panes_.size(); i++)
        {
            T* existing = dynamic_cast<T*>(panes_[i].get());
            if (existing)
            {
                active_pane_index_ = (int)i;
                pending_focus_pane_id_ = panes_[i]->GetPaneId();
                return existing;
            }
        }
        // Not found — create a new one
        auto pane = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = pane.get();
        OpenPane(std::move(pane));
        return ptr;
    }

    // Open or focus a script file (creates ScriptEditor if not already open)
    void OpenScriptFile(const std::string& filename);

    // Open or focus a room editor for a specific room number
    RoomEditor* OpenRoomEditor(int room_number);

    // Close a room editor pane by room number (if open)
    void ClosePaneByRoomNumber(int room_number);
    void ClosePaneByScriptFilename(const std::string& filename);

    // Access
    AGSEditorApp& GetApp() { return app_; }
    LogPanel& GetLogPanel() { return *log_panel_; }
    PropertiesPanel& GetPropertiesPanel() { return *properties_panel_; }
    ProjectPanel& GetProjectPanel() { return *project_panel_; }
    class Project* GetProject() const;

    // Find existing pane by type
    template <typename T>
    T* FindPane() const
    {
        for (auto& p : panes_)
        {
            T* cast = dynamic_cast<T*>(p.get());
            if (cast) return cast;
        }
        return nullptr;
    }

    // Get the currently active/focused pane index
    int GetActivePaneIndex() const { return active_pane_index_; }
    std::vector<std::unique_ptr<EditorPane>>& GetPanes() { return panes_; }
    const std::vector<std::unique_ptr<EditorPane>>& GetPanes() const { return panes_; }

private:
    void DrawCenterPane();

    AGSEditorApp& app_;

    std::unique_ptr<MenuBar> menu_bar_;
    std::unique_ptr<ProjectPanel> project_panel_;
    std::unique_ptr<PropertiesPanel> properties_panel_;
    std::unique_ptr<LogPanel> log_panel_;

    // Tabbed panes in the center
    std::vector<std::unique_ptr<EditorPane>> panes_;
    int active_pane_index_ = -1;
    int pending_focus_pane_id_ = -1;  // Pane ID to focus on next frame
};

} // namespace AGSEditor
