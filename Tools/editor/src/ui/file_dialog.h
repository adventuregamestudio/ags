// AGS Editor ImGui - File Dialog wrapper using ImGuiFileDialog
#pragma once

#include <string>
#include <functional>
#include <vector>

namespace AGSEditor
{

// File dialog types
enum class FileDialogType
{
    OpenFile,
    SaveFile,
    SelectFolder
};

// Wrapper around ImGuiFileDialog for consistent usage
class FileDialog
{
public:
    // Open a file dialog with the given parameters
    // The callback receives the selected path when confirmed
    static void Open(FileDialogType type,
                     const std::string& title,
                     const std::string& filters,
                     const std::string& default_path = ".",
                     std::function<void(const std::string&)> callback = nullptr);

    // Open a file dialog that allows selecting multiple files
    // The callback receives a vector of selected paths
    static void OpenMulti(const std::string& title,
                          const std::string& filters,
                          const std::string& default_path = ".",
                          std::function<void(const std::vector<std::string>&)> callback = nullptr);

    // Must be called every frame to render active dialogs
    static void Draw();

    // Check if a dialog is currently active
    static bool IsOpen();

private:
    static std::function<void(const std::string&)> callback_;
    static std::function<void(const std::vector<std::string>&)> multi_callback_;
    static std::string current_key_;
    static bool multi_select_;
};

} // namespace AGSEditor
