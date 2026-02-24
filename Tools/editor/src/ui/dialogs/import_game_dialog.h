// AGS Editor ImGui - Import Old Game Dialog
#pragma once

#include <string>
#include <functional>
#include "project/old_game_importer.h"

namespace AGSEditor
{

class ImportGameDialog
{
public:
    using Callback = std::function<void(const ImportOptions& options)>;

    // Open the dialog
    static void Open(Callback on_import = nullptr);

    // Draw (call every frame)
    static void Draw();

    // Check if open
    static bool IsOpen() { return open_; }

    // Show import results (call after import completes)
    static void ShowResults(const ImportResult& result);

private:
    static void DrawPageSelectFile();
    static void DrawPageOptions();
    static void DrawPageResult();

    static bool open_;
    static int current_page_;
    static ImportOptions options_;
    static Callback callback_;

    // Result display
    static bool show_results_;
    static ImportResult last_result_;
};

} // namespace AGSEditor
