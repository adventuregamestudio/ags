// AGS Editor ImGui - File Dialog implementation
#include "file_dialog.h"
#include "ImGuiFileDialog.h"
#include "imgui.h"
#include "core/dpi_helper.h"

namespace AGSEditor
{

std::function<void(const std::string&)> FileDialog::callback_;
std::function<void(const std::vector<std::string>&)> FileDialog::multi_callback_;
std::string FileDialog::current_key_;
bool FileDialog::multi_select_ = false;

void FileDialog::Open(FileDialogType type,
                      const std::string& title,
                      const std::string& filters,
                      const std::string& default_path,
                      std::function<void(const std::string&)> callback)
{
    callback_ = callback;
    multi_callback_ = nullptr;
    multi_select_ = false;
    current_key_ = "FileDialog_" + title;

    IGFD::FileDialogConfig config;
    config.path = default_path;
    config.flags = ImGuiFileDialogFlags_Modal;

    switch (type)
    {
    case FileDialogType::OpenFile:
        config.flags |= ImGuiFileDialogFlags_ReadOnlyFileNameField;
        break;
    case FileDialogType::SaveFile:
        config.flags |= ImGuiFileDialogFlags_ConfirmOverwrite;
        break;
    case FileDialogType::SelectFolder:
        break;
    }

    ImGuiFileDialog::Instance()->OpenDialog(
        current_key_, title, filters.empty() ? nullptr : filters.c_str(), config);
}

void FileDialog::OpenMulti(const std::string& title,
                           const std::string& filters,
                           const std::string& default_path,
                           std::function<void(const std::vector<std::string>&)> callback)
{
    callback_ = nullptr;
    multi_callback_ = callback;
    multi_select_ = true;
    current_key_ = "FileDialog_" + title;

    IGFD::FileDialogConfig config;
    config.path = default_path;
    config.flags = ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ReadOnlyFileNameField;
    config.countSelectionMax = 0; // 0 = unlimited selection

    ImGuiFileDialog::Instance()->OpenDialog(
        current_key_, title, filters.empty() ? nullptr : filters.c_str(), config);
}

void FileDialog::Draw()
{
    if (current_key_.empty())
        return;

    ImVec2 max_size = DpiVec(800, 600);
    ImVec2 min_size = DpiVec(500, 350);

    if (ImGuiFileDialog::Instance()->Display(current_key_, ImGuiWindowFlags_NoCollapse, min_size, max_size))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            if (multi_select_ && multi_callback_)
            {
                auto selection = ImGuiFileDialog::Instance()->GetSelection();
                std::vector<std::string> paths;
                paths.reserve(selection.size());
                for (const auto& pair : selection)
                    paths.push_back(pair.second); // second = full path
                multi_callback_(paths);
            }
            else if (callback_)
            {
                std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
                callback_(path);
            }
        }

        ImGuiFileDialog::Instance()->Close();
        current_key_.clear();
        callback_ = nullptr;
        multi_callback_ = nullptr;
        multi_select_ = false;
    }
}

bool FileDialog::IsOpen()
{
    return !current_key_.empty();
}

} // namespace AGSEditor
