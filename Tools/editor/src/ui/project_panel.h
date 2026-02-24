// AGS Editor ImGui - Project Panel (tree view)
#pragma once

#include <string>
#include <vector>

namespace AGSEditor
{

class EditorUI;

// Represents a node in the project tree
struct ProjectTreeNode
{
    std::string name;
    std::string icon; // Text icon prefix
    std::string category; // Parent category name (e.g. "Characters")
    std::vector<ProjectTreeNode> children;
    bool is_leaf = false;
    int id = 0;
    int user_data = -1; // Generic int payload (e.g. room number)

    ProjectTreeNode() = default;
    ProjectTreeNode(const std::string& n, const std::string& ico = "", bool leaf = false)
        : name(n), icon(ico), is_leaf(leaf) {}
};

class ProjectPanel
{
public:
    explicit ProjectPanel(EditorUI& editor);

    void Draw();
    void RebuildTree(); // Rebuild tree from current GameData
    void MarkTreeDirty() { tree_dirty_ = true; }

private:
    void BuildDefaultTree();
    void DrawTreeNode(ProjectTreeNode& node);

    // Drag-and-drop: reorder children within a parent node
    void HandleDragDrop(ProjectTreeNode& node, ProjectTreeNode& parent);

    // Context menu helpers
    void DrawCategoryContextMenu(ProjectTreeNode& node);
    void DrawLeafContextMenu(ProjectTreeNode& node);

    // "Go to..." navigation dialog helpers
    void DrawGoToDialogs();

    // Find all usages of a script name across all project scripts
    void FindAllUsagesOf(const std::string& script_name);

    // Import/export helpers
    void ImportScriptModule();
    void ExportScriptModule(int module_index);
    void ImportRoom();

    EditorUI& editor_;
    ProjectTreeNode root_;
    int selected_node_id_ = -1;
    int next_id_ = 0;

    // Inline rename state
    int renaming_node_id_ = -1;
    char rename_buffer_[256] = {};
    bool rename_focus_set_ = false;

    // Drag-and-drop state
    int pending_drag_source_id_ = -1;
    int pending_drag_target_id_ = -1;

    // Room deletion state
    bool confirm_delete_room_ = false;
    int delete_room_number_ = -1;

    // Script module deletion state
    bool confirm_delete_module_ = false;
    int delete_module_index_ = -1;

    // "Go to..." dialog state
    bool show_goto_dialog_ = false;
    std::string goto_category_; // Which category: "Room", "View", etc.
    char goto_filter_[128] = {};
    int goto_selected_id_ = -1;

    // Auto-rebuild when tree is marked dirty
    bool tree_dirty_ = false;
};

} // namespace AGSEditor
