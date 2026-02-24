// AGS Editor ImGui - AGS Script auto-complete data and language definition
#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace AGSEditor
{

// Holds a single auto-complete suggestion
struct AutoCompleteEntry
{
    std::string name;        // e.g. "Display"
    std::string declaration; // e.g. "void Display(const string message, ...)"
    int type = 0;            // 0 = function, 1 = property, 2 = type, 3 = keyword, 4 = constant
};

// Collects AGS script keywords, built-in types, and API functions
// for use in auto-complete and syntax highlighting
class ScriptAPIData
{
public:
    ScriptAPIData();

    // Get keyword sets for syntax highlighting
    const std::unordered_set<std::string>& GetKeywords() const { return keywords_; }
    const std::unordered_set<std::string>& GetBuiltinTypes() const { return builtin_types_; }
    const std::unordered_set<std::string>& GetBuiltinFunctions() const { return builtin_functions_; }
    const std::unordered_set<std::string>& GetConstants() const { return constants_; }

    // Get all auto-complete entries
    const std::vector<AutoCompleteEntry>& GetAllEntries() const { return entries_; }

    // Find auto-complete matches for a prefix
    std::vector<const AutoCompleteEntry*> FindMatches(const std::string& prefix,
                                                       int max_results = 20) const;

    // Load additional identifiers from game scripts (user-defined functions etc.)
    void AddUserIdentifier(const std::string& name, const std::string& declaration);

private:
    void InitKeywords();
    void InitBuiltinTypes();
    void InitBuiltinFunctions();
    void InitConstants();

    std::unordered_set<std::string> keywords_;
    std::unordered_set<std::string> builtin_types_;
    std::unordered_set<std::string> builtin_functions_;
    std::unordered_set<std::string> constants_;
    std::vector<AutoCompleteEntry> entries_;
};

} // namespace AGSEditor
