// AGS Editor ImGui - Find All Usages Results Pane
// Displays cross-file search results with clickable navigation.
#pragma once

#include "ui/editor_ui.h"
#include <string>
#include <vector>
#include <functional>

namespace AGSEditor
{

struct FindResult
{
    std::string filename;   // Script filename (e.g. "GlobalScript.asc")
    int line;               // 1-based line number
    std::string line_text;  // Full text of the matching line
    int char_index;         // Character offset within the line
};

class FindResultsPane : public EditorPane
{
public:
    using NavigateCallback = std::function<void(const std::string& file, int line)>;

    explicit FindResultsPane(EditorUI& editor);
    ~FindResultsPane() = default;

    void Draw() override;
    const char* GetTitle() const override { return title_.c_str(); }

    // Set the search token and results to display
    void SetResults(const std::string& token,
                    std::vector<FindResult>&& results);

    // Set callback for navigating to a result
    void SetNavigateCallback(NavigateCallback cb) { navigate_cb_ = std::move(cb); }

private:
    EditorUI& editor_;
    std::string title_;
    std::string search_token_;
    std::vector<FindResult> results_;
    NavigateCallback navigate_cb_;
};

} // namespace AGSEditor
