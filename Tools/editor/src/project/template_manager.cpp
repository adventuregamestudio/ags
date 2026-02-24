// AGS Editor ImGui - Template Manager implementation
// Uses AGS Common AssetManager to read CLIB-format .agt template files.
#include "template_manager.h"

#include "core/assetmanager.h"
#include "util/multifilelib.h"
#include "util/stream.h"
#include "util/file.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/string.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>

namespace fs = std::filesystem;
using namespace AGS::Common;

namespace AGSEditor
{

// Files that indicate a valid game template
static const char* kNewEditorDataFile = "game.agf";     // Game.agf (case insensitive match below)
static const char* kOldEditorDataFile = "editor.dat";

// Files excluded from extraction
static const char* kTemplateLockFile = "template.dta";
static const char* kTemplateIconFile = "template.ico";

// Template metadata file
static const char* kTemplateDescFile = "template.txt";

// --------------------------------------------------------------------------
// TemplateManager
// --------------------------------------------------------------------------

TemplateManager::TemplateManager() = default;
TemplateManager::~TemplateManager() = default;

void TemplateManager::AddSearchPath(const std::string& path)
{
    search_paths_.push_back(path);
}

void TemplateManager::DiscoverTemplates(const std::string& editor_dir)
{
    templates_.clear();

    // 1. Editor's Templates/ directory
    std::string editor_templates = editor_dir + "/Templates";
    ScanDirectory(editor_templates);

    // 1b. Common development locations (relative to ags source tree)
    //     editor_dir is often the EditorImGui source dir
    ScanDirectory(editor_dir + "/../ags-templates/Templates");
    ScanDirectory(editor_dir + "/../../ags-templates/Templates");

    // 2. User templates directory
    std::string user_templates;
#ifdef _WIN32
    const char* local_appdata = getenv("LOCALAPPDATA");
    if (local_appdata)
        user_templates = std::string(local_appdata) + "/AGS/Templates";
#else
    const char* home = getenv("HOME");
    if (home)
        user_templates = std::string(home) + "/.local/share/ags/Templates";
#endif
    if (!user_templates.empty())
    {
        // Create if it doesn't exist
        try { fs::create_directories(user_templates); } catch (...) {}
        ScanDirectory(user_templates);
    }

    // 3. Additional search paths
    for (const auto& path : search_paths_)
        ScanDirectory(path);

    // Deduplicate by friendly name (keep first found)
    {
        std::vector<GameTemplate> deduped;
        for (auto& t : templates_)
        {
            bool dupe = false;
            for (const auto& d : deduped)
                if (d.friendly_name == t.friendly_name) { dupe = true; break; }
            if (!dupe)
                deduped.push_back(std::move(t));
        }
        templates_ = std::move(deduped);
    }

    // Sort by friendly name
    std::sort(templates_.begin(), templates_.end(),
        [](const GameTemplate& a, const GameTemplate& b) {
            // "Empty Game" should always be first
            if (a.friendly_name == "Empty Game") return true;
            if (b.friendly_name == "Empty Game") return false;
            return a.friendly_name < b.friendly_name;
        });

    fprintf(stderr, "[TemplateManager] Discovered %zu templates\n", templates_.size());
    for (const auto& t : templates_)
        fprintf(stderr, "  - %s (%s)\n", t.friendly_name.c_str(), t.file_path.c_str());
}

void TemplateManager::ScanDirectory(const std::string& dir)
{
    if (!fs::exists(dir) || !fs::is_directory(dir))
        return;

    try
    {
        for (const auto& entry : fs::directory_iterator(dir))
        {
            if (!entry.is_regular_file())
                continue;

            std::string ext = entry.path().extension().string();
            // Case-insensitive extension check
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext != ".agt")
                continue;

            GameTemplate tmpl = LoadTemplateMetadata(entry.path().string());
            if (tmpl.valid)
                templates_.push_back(std::move(tmpl));
        }
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "[TemplateManager] Error scanning '%s': %s\n", dir.c_str(), e.what());
    }
}

GameTemplate TemplateManager::LoadTemplateMetadata(const std::string& agt_path)
{
    GameTemplate tmpl;
    tmpl.file_path = agt_path;
    tmpl.file_name = fs::path(agt_path).filename().string();
    tmpl.friendly_name = fs::path(agt_path).stem().string();
    tmpl.valid = false;

    // Open as CLIB library
    AssetManager mgr;
    const AssetLibInfo* lib = nullptr;
    AssetError err = mgr.AddLibrary(String(agt_path.c_str()), &lib);
    if (err != kAssetNoError)
    {
        fprintf(stderr, "[TemplateManager] Cannot open '%s': %s\n",
                agt_path.c_str(), GetAssetErrorText(err).GetCStr());
        return tmpl;
    }

    if (!lib)
        return tmpl;

    // Check for required files (game.agf or editor.dat)
    bool has_game_file = false;
    for (const auto& asset : lib->AssetInfos)
    {
        String lower = asset.FileName.Lower();
        if (lower == kNewEditorDataFile || lower == kOldEditorDataFile)
        {
            has_game_file = true;
            break;
        }
    }

    if (!has_game_file)
    {
        fprintf(stderr, "[TemplateManager] '%s' does not contain game data, skipping.\n",
                agt_path.c_str());
        return tmpl;
    }

    // Read description from template.txt if present
    auto desc_stream = mgr.OpenAsset(String(kTemplateDescFile));
    if (desc_stream)
    {
        soff_t len = desc_stream->GetLength();
        if (len > 0 && len < 65536)
        {
            std::vector<char> buf(len + 1, 0);
            desc_stream->Read(buf.data(), len);
            tmpl.description = buf.data();
        }
    }

    tmpl.valid = true;
    return tmpl;
}

bool TemplateManager::ExtractTemplate(const GameTemplate& tmpl, const std::string& target_dir)
{
    if (!tmpl.valid || tmpl.file_path.empty())
        return false;

    // Create target directory
    try { fs::create_directories(target_dir); } catch (...) {
        fprintf(stderr, "[TemplateManager] Failed to create directory '%s'\n", target_dir.c_str());
        return false;
    }

    // Open as CLIB library
    AssetManager mgr;
    const AssetLibInfo* lib = nullptr;
    AssetError err = mgr.AddLibrary(String(tmpl.file_path.c_str()), &lib);
    if (err != kAssetNoError)
    {
        fprintf(stderr, "[TemplateManager] Cannot open '%s' for extraction: %s\n",
                tmpl.file_path.c_str(), GetAssetErrorText(err).GetCStr());
        return false;
    }

    if (!lib)
        return false;

    // Validate template contains required data file
    bool has_game_file = false;
    for (const auto& asset : lib->AssetInfos)
    {
        String lower = asset.FileName.Lower();
        if (lower == kNewEditorDataFile || lower == kOldEditorDataFile)
        {
            has_game_file = true;
            break;
        }
    }

    if (!has_game_file)
    {
        fprintf(stderr, "[TemplateManager] Template does not contain main project data.\n");
        return false;
    }

    // Extract all assets except excluded files
    int extracted = 0;
    for (const auto& asset : lib->AssetInfos)
    {
        if (asset.FileName.IsEmpty())
            continue;

        // Skip excluded files
        String lower = asset.FileName.Lower();
        if (lower == kTemplateLockFile)
            continue;

        // Open asset stream
        auto in = mgr.OpenAsset(asset.FileName);
        if (!in)
        {
            fprintf(stderr, "[TemplateManager] Cannot read asset '%s'\n",
                    asset.FileName.GetCStr());
            continue;
        }

        // Determine output path
        std::string out_path = target_dir + "/" + std::string(asset.FileName.GetCStr());

        // Create subdirectories if needed
        fs::path parent = fs::path(out_path).parent_path();
        if (!parent.empty())
        {
            try { fs::create_directories(parent); } catch (...) {}
        }

        // Write to disk
        auto out = File::CreateFile(String(out_path.c_str()));
        if (!out)
        {
            fprintf(stderr, "[TemplateManager] Cannot write '%s'\n", out_path.c_str());
            continue;
        }

        soff_t length = in->GetLength();
        CopyStream(in.get(), out.get(), length);
        extracted++;
    }

    fprintf(stderr, "[TemplateManager] Extracted %d files from '%s' to '%s'\n",
            extracted, tmpl.friendly_name.c_str(), target_dir.c_str());

    // Create standard subdirectories
    EnsureStandardSubFolders(target_dir);

    return true;
}

bool TemplateManager::MakeTemplate(const std::string& project_dir, const std::string& output_agt_path)
{
    // Collect files to include in the template, matching the C# editor's logic
    struct AssetEntry {
        std::string name;     // asset name inside the pack (relative path)
        std::string source;   // full path on disk
        soff_t size;
    };
    std::vector<AssetEntry> assets;

    auto add_file = [&](const std::string& relative_name) {
        std::string full_path = project_dir + "/" + relative_name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISREG(st.st_mode))
        {
            // Normalize slashes to forward slashes
            std::string name = relative_name;
            std::replace(name.begin(), name.end(), '\\', '/');
            assets.push_back({name, full_path, (soff_t)st.st_size});
        }
    };

    auto add_glob = [&](const std::string& dir_rel, const std::string& pattern) {
        std::string search_dir = project_dir;
        if (!dir_rel.empty())
            search_dir += "/" + dir_rel;
        if (!fs::exists(search_dir) || !fs::is_directory(search_dir))
            return;
        try {
            for (const auto& entry : fs::recursive_directory_iterator(search_dir))
            {
                if (!entry.is_regular_file()) continue;
                std::string fname = entry.path().filename().string();
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                std::string fname_lower = fname;
                std::transform(fname_lower.begin(), fname_lower.end(), fname_lower.begin(), ::tolower);

                bool match = false;
                if (pattern == "*.*")
                    match = true;
                else if (pattern.size() > 1 && pattern[0] == '*')
                {
                    std::string suffix = pattern.substr(1);
                    std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
                    // Handle patterns like "*.crm", "*.asc", etc.
                    if (suffix[0] == '.')
                        match = (ext == suffix);
                    else
                        match = (fname_lower.size() >= suffix.size() &&
                                 fname_lower.substr(fname_lower.size() - suffix.size()) == suffix);
                }
                else
                {
                    // Prefix pattern like "agsfnt*.ttf"
                    // Split at * into prefix and suffix
                    size_t star = pattern.find('*');
                    if (star != std::string::npos)
                    {
                        std::string prefix = pattern.substr(0, star);
                        std::string suffix = pattern.substr(star + 1);
                        std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);
                        std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
                        match = (fname_lower.size() >= prefix.size() + suffix.size() &&
                                 fname_lower.substr(0, prefix.size()) == prefix &&
                                 fname_lower.substr(fname_lower.size() - suffix.size()) == suffix);
                    }
                }

                if (match)
                {
                    // Compute relative path from project_dir
                    std::string rel = fs::relative(entry.path(), project_dir).string();
                    std::replace(rel.begin(), rel.end(), '\\', '/');
                    assets.push_back({rel, entry.path().string(), (soff_t)entry.file_size()});
                }
            }
        } catch (...) {}
    };

    // Check for template_includes.txt (equivalent to C# TEMPLATE_INCLUDE_FILE)
    std::string include_file = project_dir + "/template_includes.txt";
    if (fs::exists(include_file))
    {
        // Use includes file to determine which files to package
        std::ifstream inc(include_file);
        if (inc.is_open())
        {
            std::string line;
            while (std::getline(inc, line))
            {
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                if (line.empty() || line[0] == '#')
                    continue;
                // Each line is a glob pattern
                add_glob("", line);
            }
        }
    }
    else
    {
        // Default file list (matches C# ConstructBasicFileListForTemplate)
        add_file("Game.agf");
        add_file("acsprset.spr");
        add_file("sprindex.dat");
        add_file("preload.pcx");
        add_glob("", "*.ico");
        add_glob("", "*.crm");
        add_glob("", "*.asc");
        add_glob("", "*.ash");
        add_glob("", "*.txt");
        add_glob("", "*.trs");
        add_glob("", "*.pdf");
        add_glob("", "*.ogv");
        add_glob("", "agsfnt*.ttf");
        add_glob("", "agsfnt*.wfn");
        add_glob("", "flic*.fl?");
        add_glob("AudioCache", "*.*");
        add_glob("Speech", "*.*");
    }

    if (assets.empty())
    {
        fprintf(stderr, "[TemplateManager] No files found to include in template.\n");
        return false;
    }

    // Deduplicate by asset name (keep first)
    {
        std::vector<AssetEntry> deduped;
        for (auto& a : assets)
        {
            bool found = false;
            for (const auto& d : deduped)
                if (d.name == a.name) { found = true; break; }
            if (!found)
                deduped.push_back(std::move(a));
        }
        assets = std::move(deduped);
    }

    // Build the CLIB pack file (same approach as build_system.cpp PackageAssets)
    std::unique_ptr<AGS::Common::Stream> out(
        AGS::Common::File::CreateFile(AGS::Common::String(output_agt_path.c_str())));
    if (!out)
    {
        fprintf(stderr, "[TemplateManager] Cannot create output file: %s\n", output_agt_path.c_str());
        return false;
    }

    AGS::Common::AssetLibInfo lib;
    lib.LibFileNames.push_back(AGS::Common::String(fs::path(output_agt_path).filename().string().c_str()));

    for (const auto& a : assets)
    {
        AGS::Common::AssetInfo ai;
        ai.FileName = AGS::Common::String(a.name.c_str());
        ai.LibUid = 0;
        ai.Size = a.size;
        ai.Offset = 0;
        lib.AssetInfos.push_back(ai);
    }

    // Write header (placeholder offsets)
    soff_t header_start = out->GetPosition();
    AGS::Common::MFLUtil::WriteHeader(lib, AGS::Common::MFLUtil::kMFLVersion_MultiV30, 0, out.get());
    soff_t data_start = out->GetPosition();

    // Write asset data and record offsets
    for (size_t i = 0; i < assets.size(); i++)
    {
        lib.AssetInfos[i].Offset = out->GetPosition();

        std::ifstream src(assets[i].source, std::ios::binary);
        if (src.is_open())
        {
            std::vector<char> buf(65536);
            soff_t remaining = assets[i].size;
            while (remaining > 0)
            {
                size_t to_read = (size_t)std::min((soff_t)buf.size(), remaining);
                src.read(buf.data(), (std::streamsize)to_read);
                size_t nread = (size_t)src.gcount();
                if (nread == 0) break;
                out->Write(buf.data(), nread);
                remaining -= (soff_t)nread;
            }
        }
    }

    // Rewrite header with correct offsets
    out->Seek(header_start, AGS::Common::kSeekBegin);
    AGS::Common::MFLUtil::WriteHeader(lib, AGS::Common::MFLUtil::kMFLVersion_MultiV30, 0, out.get());

    // Write footer
    out->Seek(0, AGS::Common::kSeekEnd);
    AGS::Common::MFLUtil::WriteEnder(header_start, AGS::Common::MFLUtil::kMFLVersion_MultiV30, out.get());

    fprintf(stderr, "[TemplateManager] Created template: %s (%zu assets)\n",
            output_agt_path.c_str(), assets.size());
    return true;
}

void TemplateManager::EnsureStandardSubFolders(const std::string& game_dir)
{
    const char* subdirs[] = {
        "Speech",
        "AudioCache",
        "Compiled",
        "Compiled/Data",
        nullptr
    };

    for (int i = 0; subdirs[i]; i++)
    {
        std::string path = game_dir + "/" + subdirs[i];
        try { fs::create_directories(path); } catch (...) {}
    }
}

} // namespace AGSEditor
