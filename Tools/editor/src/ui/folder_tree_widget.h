// Reusable ImGui folder tree widget for FolderInfo hierarchies
#pragma once

#include "project/game_data.h"
#include "imgui.h"
#include <algorithm>
#include <functional>
#include <set>

namespace AGSEditor {

// DnD payload type for item IDs being dragged between folders
static constexpr const char* kFolderItemDnDType = "FOLDER_ITEM_ID";

// Remove an item ID from any folder in the tree. Returns true if found and removed.
inline bool RemoveItemFromFolder(FolderInfo& folder, int item_id)
{
    auto it = std::find(folder.item_ids.begin(), folder.item_ids.end(), item_id);
    if (it != folder.item_ids.end())
    {
        folder.item_ids.erase(it);
        return true;
    }
    for (auto& sub : folder.subfolders)
        if (RemoveItemFromFolder(sub, item_id)) return true;
    return false;
}

// Move an item ID from its current folder to the target folder.
// Returns true if the item was found and moved.
inline bool MoveItemToFolder(FolderInfo& root, int item_id, FolderInfo& target)
{
    RemoveItemFromFolder(root, item_id);
    target.item_ids.push_back(item_id);
    return true;
}

// Find the FolderInfo pointer matching a const pointer in a mutable tree
inline FolderInfo* FindMutableFolder(FolderInfo& root, const FolderInfo* target)
{
    if (&root == target) return &root;
    for (auto& sub : root.subfolders)
    {
        auto* found = FindMutableFolder(sub, target);
        if (found) return found;
    }
    return nullptr;
}

// Draw a folder tree using FolderInfo. Returns the currently selected folder pointer.
// - selected_folder: pointer to the currently selected FolderInfo (nullptr = "All")
// - root: the root FolderInfo node
// - all_label: label for the "All" node (e.g., "All Sprites", "All Characters")
// - mutable_root: optional mutable root for drag-and-drop support (nullptr disables DnD targets)
// Returns: the newly selected folder pointer (or nullptr for "All")
inline const FolderInfo* DrawFolderTreeWidget(
    const FolderInfo* selected_folder,
    const FolderInfo& root,
    const char* all_label,
    FolderInfo* mutable_root = nullptr)
{
    const FolderInfo* result = selected_folder;

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Folders");
    ImGui::Separator();

    // "All" item at the top
    bool all_selected = (selected_folder == nullptr);
    ImGuiTreeNodeFlags all_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (all_selected)
        all_flags |= ImGuiTreeNodeFlags_Selected;
    ImGui::TreeNodeEx(all_label, all_flags);
    if (ImGui::IsItemClicked())
        result = nullptr;

    // Accept drop on "All" — moves item to root folder
    if (mutable_root && ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kFolderItemDnDType))
        {
            int item_id = *(const int*)payload->Data;
            MoveItemToFolder(*mutable_root, item_id, *mutable_root);
        }
        ImGui::EndDragDropTarget();
    }

    // Recursive lambda to draw folder tree
    std::function<void(const FolderInfo&)> draw_folder =
        [&](const FolderInfo& folder) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                       ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                       ImGuiTreeNodeFlags_SpanAvailWidth;

            bool is_leaf = folder.subfolders.empty();
            if (is_leaf)
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (selected_folder == &folder)
                flags |= ImGuiTreeNodeFlags_Selected;

            int direct_count = (int)folder.item_ids.size();

            char label[256];
            snprintf(label, sizeof(label), "%s (%d)", folder.name.c_str(), direct_count);

            bool open = ImGui::TreeNodeEx(&folder, flags, "%s", label);
            if (ImGui::IsItemClicked())
                result = &folder;

            // Accept drop on folder node
            if (mutable_root && ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kFolderItemDnDType))
                {
                    int item_id = *(const int*)payload->Data;
                    FolderInfo* target = FindMutableFolder(*mutable_root, &folder);
                    if (target)
                        MoveItemToFolder(*mutable_root, item_id, *target);
                }
                ImGui::EndDragDropTarget();
            }

            if (open && !is_leaf)
            {
                for (const auto& sub : folder.subfolders)
                    draw_folder(sub);
                ImGui::TreePop();
            }
        };

    // Draw root folder's contents
    if (!root.subfolders.empty() || !root.item_ids.empty())
    {
        for (const auto& sub : root.subfolders)
            draw_folder(sub);

        // If root has direct items alongside subfolders, show root as a node
        if (!root.item_ids.empty() && !root.subfolders.empty())
        {
            ImGuiTreeNodeFlags rf = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (selected_folder == &root)
                rf |= ImGuiTreeNodeFlags_Selected;
            char rl[256];
            snprintf(rl, sizeof(rl), "%s (%d)", root.name.c_str(), (int)root.item_ids.size());
            ImGui::TreeNodeEx(&root, rf, "%s", rl);
            if (ImGui::IsItemClicked())
                result = &root;

            // Accept drop on root node
            if (mutable_root && ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kFolderItemDnDType))
                {
                    int item_id = *(const int*)payload->Data;
                    MoveItemToFolder(*mutable_root, item_id, *mutable_root);
                }
                ImGui::EndDragDropTarget();
            }
        }
    }

    return result;
}

// Collect all item IDs from a folder (including subfolders recursively)
inline void CollectFolderItemIds(const FolderInfo& folder, std::set<int>& out)
{
    for (int id : folder.item_ids)
        out.insert(id);
    for (const auto& sub : folder.subfolders)
        CollectFolderItemIds(sub, out);
}

// Helper: begin a drag source for an item with the given ID.
// Call this after rendering a Selectable/Image/etc. for an item.
inline void BeginItemDragSource(int item_id, const char* preview_text)
{
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        ImGui::SetDragDropPayload(kFolderItemDnDType, &item_id, sizeof(int));
        ImGui::Text("%s", preview_text);
        ImGui::EndDragDropSource();
    }
}

} // namespace AGSEditor
