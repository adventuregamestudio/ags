// AGS Build System implementation
// Implements the full game build pipeline:
//   1. Pre-build validation
//   2. Auto-generated script header creation
//   3. Script compilation pipeline (preprocessor + compiler)
//   4. Game data file generation (game28.dta)
//   5. Asset packaging (<GameFileName>.ags CLIB pack)
//   6. Engine file copying per platform
//   7. Config file generation (acsetup.cfg)

#include "build_system.h"
#include "game_data_writer.h"
#include "default_fonts.h"
#include "project.h"
#include "game_data.h"
#include "path_utils.h"
#include "io_utils.h"
#include "compiler_bridge.h"  // for kScriptAPIVersions, kScriptAPIHighest
#include "logger.h"

#include "script/cs_compiler.h"
#include "script/cc_common.h"
#include "script/cc_script.h"
#include "preproc/preprocessor.h"

#include "util/file.h"
#include "util/path.h"
#include "data/multifilelib.h"
#include "data/asset.h"
#include "data/dialogscriptconv.h"
#include "data/script_utils.h"
#include "data/scriptgen.h"
#include "data/tra_utils.h"
#include "data/mfl_utils.h"
#include "data/tra_file.h"

#include "ac/spritefile.h"
#include "gfx/image_file.h"
#include "gfx/bitmapdata.h"

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace AGSBuild
{

// =========================================================================
// BuildTarget helpers
// =========================================================================
const char* BuildTargetName(BuildTarget target)
{
    switch (target) {
        case BuildTarget::DataFile: return "Data File";
        case BuildTarget::Linux:    return "Linux";
        case BuildTarget::Windows:  return "Windows";
        case BuildTarget::MacOS:    return "macOS";
        case BuildTarget::Web:      return "Web";
        default: return "Unknown";
    }
}

const char* BuildTargetDirName(BuildTarget target)
{
    switch (target) {
        case BuildTarget::DataFile: return "Data";
        case BuildTarget::Linux:    return "Linux";
        case BuildTarget::Windows:  return "Windows";
        case BuildTarget::MacOS:    return "macOS";
        case BuildTarget::Web:      return "Web";
        default: return "Unknown";
    }
}

// =========================================================================
// BuildConfig
// =========================================================================
void BuildConfig::SetDefaults(const std::string& project_dir)
{
    output_base_dir = project_dir + "/Compiled";

    // Try to find the engine executable relative to the editor binary
    // or from the build directory
#ifdef _WIN32
    engine_windows_path = "acwin.exe";
#else
    // On Linux, try common locations
    engine_linux_path = "";
    // Check if 'ags' is in the same build directory as the editor
    // This will be resolved at runtime
#endif
}

// =========================================================================
// BuildResult
// =========================================================================
int BuildResult::ErrorCount() const
{
    int count = 0;
    for (const auto& m : messages)
        if (m.type == BuildMessageType::Error) count++;
    return count;
}

int BuildResult::WarningCount() const
{
    int count = 0;
    for (const auto& m : messages)
        if (m.type == BuildMessageType::Warning) count++;
    return count;
}

void BuildResult::AddError(const std::string& file, int line, const std::string& msg,
                            const std::string& module)
{
    messages.push_back({BuildMessageType::Error, file, line, 0, msg, module});
}

void BuildResult::AddWarning(const std::string& file, int line, const std::string& msg,
                              const std::string& module)
{
    messages.push_back({BuildMessageType::Warning, file, line, 0, msg, module});
}

void BuildResult::AddInfo(const std::string& msg)
{
    messages.push_back({BuildMessageType::Info, "", 0, 0, msg, ""});
}

// =========================================================================
// BuildSystem
// =========================================================================

// -------------------------------------------------------------------------
// Logging helper
// -------------------------------------------------------------------------
void BuildSystem::Log(const char* fmt, ...)
{
    if (!log_cb_) return;
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    log_cb_(std::string(buf));
}

// -------------------------------------------------------------------------
// Path helpers
// -------------------------------------------------------------------------
std::string BuildSystem::GetCompiledDir(const BuildConfig& config) const
{
    return config.output_base_dir;
}

std::string BuildSystem::GetDataDir(const BuildConfig& config) const
{
    return config.output_base_dir + "/Data";
}

std::string BuildSystem::GetTargetDir(const BuildConfig& config, BuildTarget target) const
{
    return config.output_base_dir + "/" + BuildTargetDirName(target);
}

static std::string FindEngineExecutable(const BuildConfig& config)
{
#ifndef _WIN32
    // Check explicit config path first
    if (!config.engine_linux_path.empty())
    {
        if (std::filesystem::is_regular_file(config.engine_linux_path))
            return config.engine_linux_path;
    }
    // Resolve paths relative to the editor binary's directory
    std::string editor_dir;
    {
        char exe_path[PATH_MAX] = {};
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (len > 0) {
            exe_path[len] = '\0';
            // Strip the binary name to get the directory
            char *last_slash = strrchr(exe_path, '/');
            if (last_slash) {
                *last_slash = '\0';
                editor_dir = exe_path;
            }
        }
    }

    // Try to find 'ags' in common locations
    std::vector<std::string> search_paths;
    if (!editor_dir.empty()) {
        // Same directory as the editor binary (typical build layout)
        search_paths.push_back(editor_dir + "/../../ags");
        search_paths.push_back(editor_dir + "/../ags");
        search_paths.push_back(editor_dir + "/ags");
    }
    // CWD-relative fallbacks
    search_paths.push_back("./ags");
    // System paths
    search_paths.push_back("/usr/bin/ags");
    search_paths.push_back("/usr/local/bin/ags");
    search_paths.push_back("/usr/games/ags");

    for (const auto& path : search_paths)
    {
        auto fst = std::filesystem::status(path);
        if (std::filesystem::is_regular_file(fst) &&
            (fst.permissions() & std::filesystem::perms::owner_exec) != std::filesystem::perms::none)
        {
            // Return absolute path via realpath
            char resolved[PATH_MAX] = {};
            if (realpath(path.c_str(), resolved))
                return std::string(resolved);
            return path;
        }
    }
#else
    if (!config.engine_windows_path.empty())
    {
        if (std::filesystem::exists(config.engine_windows_path))
            return config.engine_windows_path;
    }
#endif

    return "";
}

// -------------------------------------------------------------------------
// Packaging helpers (mirror BuildTargetDataFile.ConstructFileListForDataFile)
// -------------------------------------------------------------------------
static std::string GetBaseGameFileName(const Project &project, const GameData &gd)
{
    if (!gd.game_file_name.empty())
        return gd.game_file_name;
    std::filesystem::path p(project.GetProjectDir());
    std::string name = p.filename().string();
    if (name.empty())
        name = "game";
    return name;
}

static std::string ResolveAudioCacheRelPath(const GameData::AudioClipInfo &clip)
{
    if (!clip.filename.empty())
    {
        if (clip.filename.find('/') != std::string::npos ||
            clip.filename.find('\\') != std::string::npos)
            return clip.filename;
        if (clip.filename.size() >= 2 && clip.filename.compare(0, 2, "au") == 0)
            return "AudioCache/" + clip.filename;
    }

    int index = clip.fixed_index;
    if (index <= 0)
        index = clip.id + 1;
    if (index <= 0)
        index = 1;

    std::string src = !clip.source_filename.empty() ? clip.source_filename : clip.filename;
    std::string ext = ".ogg";
    const size_t dot = src.rfind('.');
    if (dot != std::string::npos)
    {
        ext = src.substr(dot);
        for (auto &c : ext)
            c = (char)tolower(c);
    }

    char buf[64];
    snprintf(buf, sizeof(buf), "AudioCache/au%06X%s", index, ext.c_str());
    return buf;
}

static std::string AssetBaseName(const std::string &path)
{
    const size_t pos = path.find_last_of("/\\");
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

static void TrimString(std::string &s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
        s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t'))
        s.pop_back();
}

static std::vector<std::string> SplitCommaList(const std::string &text)
{
    std::vector<std::string> parts;
    std::string cur;
    for (char c : text)
    {
        if (c == ',')
        {
            TrimString(cur);
            if (!cur.empty())
                parts.push_back(cur);
            cur.clear();
        }
        else
        {
            cur += c;
        }
    }
    TrimString(cur);
    if (!cur.empty())
        parts.push_back(cur);
    return parts;
}

static bool PathContainsDotDirs(const std::string &dir)
{
    std::string norm = dir;
    for (auto &c : norm)
    {
        if (c == '\\')
            c = '/';
    }
    size_t start = 0;
    while (start < norm.size())
    {
        size_t end = norm.find('/', start);
        if (end == std::string::npos)
            end = norm.size();
        std::string part = norm.substr(start, end - start);
        if (part == "." || part == "..")
            return true;
        start = end + 1;
    }
    return false;
}

static std::string NormalizeRelPathSlashes(std::string p)
{
    for (auto &c : p)
    {
        if (c == '\\')
            c = '/';
    }
    return p;
}

static bool PathIsSameOrNestedAmong(const std::string &test_dir,
                                   const std::vector<std::string> &roots)
{
    std::error_code ec;
    const auto test = std::filesystem::weakly_canonical(test_dir, ec);
    if (ec)
        return false;
    for (const auto &root : roots)
    {
        const auto base = std::filesystem::weakly_canonical(root, ec);
        if (ec)
            continue;
        if (test == base)
            return true;
        std::string test_str = test.string();
        std::string base_str = base.string();
        if (!base_str.empty() && test_str.size() > base_str.size() &&
            test_str.compare(0, base_str.size(), base_str) == 0 &&
            (test_str[base_str.size()] == '/' || test_str[base_str.size()] == '\\'))
        {
            return true;
        }
    }
    return false;
}

static bool MatchFlicWildcard(const std::string &fname)
{
    if (fname.size() < 9)
        return false;
    if (fname.compare(0, 4, "flic") != 0)
        return false;
    const size_t dot = fname.rfind('.');
    if (dot == std::string::npos || fname.size() != dot + 4)
        return false;
    return fname.compare(dot, 3, ".fl") == 0;
}

static bool MatchOgvExtension(const std::string &fname)
{
    if (fname.size() < 4)
        return false;
    std::string ext = fname.substr(fname.size() - 4);
    for (auto &c : ext)
        c = (char)tolower(c);
    return ext == ".ogv";
}

static void CollectProjectFiles(const std::string &proj_dir,
                                const std::function<bool(const std::string &)> &matcher,
                                std::vector<std::string> &out_names)
{
    CollectDirectoryFileNames(proj_dir, matcher, out_names);
}

static void CollectCustomDataAssets(Project &project, const GameData &gd,
                                    BuildResult &result,
                                    std::vector<std::pair<std::string, std::string>> &assets)
{
    if (gd.custom_data_dir.empty())
        return;

    const std::string proj_dir = project.GetProjectDir();
    const std::vector<std::string> restricted = {
        proj_dir + "/Compiled",
        proj_dir + "/_Debug",
        proj_dir + "/AudioCache",
        proj_dir + "/Speech",
    };
    std::vector<std::string> used_roots;

    for (const std::string &dir : SplitCommaList(gd.custom_data_dir))
    {
        if (dir.empty())
            continue;
        if (std::filesystem::path(dir).is_absolute())
        {
            result.AddWarning("", 0,
                "Cannot use absolute path as a custom data source: " + dir);
            continue;
        }
        if (PathContainsDotDirs(dir))
        {
            result.AddWarning("", 0,
                "Cannot use path containing '.' or '..' as a custom data source: " + dir);
            continue;
        }

        const std::string test_dir = proj_dir + "/" + dir;
        if (PathIsSameOrNestedAmong(test_dir, restricted))
        {
            result.AddWarning("", 0,
                "Cannot use restricted location as a custom data source: " + dir);
            continue;
        }
        if (PathIsSameOrNestedAmong(test_dir, used_roots))
            continue;

        if (!PathExists(test_dir))
            continue;

        used_roots.push_back(test_dir);
        std::error_code ec;
        for (const auto &entry :
             std::filesystem::recursive_directory_iterator(test_dir, ec))
        {
            if (!entry.is_regular_file())
                continue;
            std::string rel = NormalizeRelPathSlashes(
                std::filesystem::relative(entry.path(), proj_dir, ec).string());
            if (rel.empty())
                continue;
            assets.push_back({rel, entry.path().string()});
        }
    }
}

static void AddAssetIfExists(std::vector<std::pair<std::string, std::string>> &assets,
                             const std::string &archive_name,
                             const std::string &disk_path)
{
    if (PathExists(disk_path))
        assets.push_back({archive_name, disk_path});
}

static bool EnsureAudioCacheFromSources(Project &project, BuildResult &result)
{
    GameData *gd = project.GetGameData();
    if (!gd)
        return false;
    const std::string proj_dir = project.GetProjectDir();
    if (!PathExists(proj_dir + "/Assets/Sound"))
        return true;

    std::error_code ec;
    std::filesystem::create_directories(proj_dir + "/AudioCache", ec);

    bool did_any = false;
    for (const auto &clip : gd->audio_clips)
    {
        const std::string cache_rel = ResolveAudioCacheRelPath(clip);
        if (cache_rel.empty())
            continue;
        const std::string cache_path = proj_dir + "/" + cache_rel;
        if (PathExists(cache_path))
            continue;
        if (clip.source_filename.empty())
            continue;

        std::string src_rel = clip.source_filename;
        for (auto &c : src_rel) if (c == '\\') c = '/';
        std::string src_path = proj_dir + "/" + src_rel;
        if (!PathExists(src_path))
        {
            const std::string base = AssetBaseName(src_rel);
            const std::string fallback = proj_dir + "/Assets/Sound/" + base;
            if (PathExists(fallback))
                src_path = fallback;
        }
        if (!PathExists(src_path))
            continue;

        std::filesystem::create_directories(std::filesystem::path(cache_path).parent_path(), ec);
        if (CopyPath(src_path, cache_path))
        {
            did_any = true;
            result.AddInfo("Cached audio: " + src_rel + " -> " + cache_rel);
        }
    }

    if (did_any)
        result.AddInfo("AudioCache generated from Assets/Sound.");
    return true;
}

static void CollectSpriteSources(const GameData &gd,
                                 std::map<int, GameData::SpriteSourceInfo> &out)
{
    for (const auto &spr : gd.sprites)
    {
        if (spr.source.HasSource())
            out[spr.id] = spr.source;
    }
    for (const auto &kv : gd.pending_sprite_source_)
        out[kv.first] = kv.second;
}

static bool NeedsSpriteCrop(const GameData::SpriteSourceInfo &src, int full_w, int full_h)
{
    if (src.import_as_tile)
        return true;
    const int crop_w = src.import_width > 0 ? src.import_width : full_w;
    const int crop_h = src.import_height > 0 ? src.import_height : full_h;
    return src.offset_x != 0 || src.offset_y != 0 ||
           crop_w != full_w || crop_h != full_h;
}

static bool CropSpriteFromSource(const PixelBuffer &src_px,
                                 const GameData::SpriteSourceInfo &src,
                                 PixelBuffer &out)
{
    using namespace AGS::Common;

    const int full_w = src_px.GetWidth();
    const int full_h = src_px.GetHeight();
    if (full_w <= 0 || full_h <= 0 || !src_px.GetData())
        return false;

    int crop_x = src.offset_x;
    int crop_y = src.offset_y;
    int crop_w = src.import_width > 0 ? src.import_width : full_w;
    int crop_h = src.import_height > 0 ? src.import_height : full_h;

    if (crop_w <= 0 || crop_h <= 0 ||
        crop_x < 0 || crop_y < 0 ||
        crop_x + crop_w > full_w || crop_y + crop_h > full_h)
    {
        return false;
    }

    out = PixelBuffer(crop_w, crop_h, src_px.GetFormat());
    PixelOp::CopyPixelsRegion(
        src_px.GetData(), src_px.GetFormat(), src_px.GetStride(),
        (size_t)crop_x, crop_w, crop_h,
        out.GetData(), out.GetStride(), 0);
    return true;
}

static bool LoadSpritePixelFromSource(const std::string &proj_dir,
                                      const GameData::SpriteSourceInfo &src,
                                      PixelBuffer &out)
{
    using namespace AGS::Common;

    out = PixelBuffer();
    if (!src.HasSource())
        return false;

    std::string src_rel = src.source_file;
    for (auto &c : src_rel)
        if (c == '\\') c = '/';
    std::string src_path = proj_dir + "/" + src_rel;
    if (!PathExists(src_path))
    {
        const std::string base = AssetBaseName(src_rel);
        const std::string fallback = proj_dir + "/Assets/Sprites/" + base;
        if (PathExists(fallback))
            src_path = fallback;
    }
    if (!PathExists(src_path))
        return false;

    auto in = std::unique_ptr<Stream>(File::OpenFileRead(src_path.c_str()));
    if (!in)
        return false;

    std::string ext = std::filesystem::path(src_path).extension().string();
    if (!ext.empty() && ext[0] == '.')
        ext = ext.substr(1);

    PixelFormat fmt = kPxFmt_Undefined;
    PixelBuffer loaded = ImageFile::LoadImage(in.get(), ext.c_str(), &fmt, nullptr);
    if (!loaded.GetData() || loaded.GetWidth() <= 0 || loaded.GetHeight() <= 0)
        return false;

    // TODO: multi-frame sources (Frame > 0) when projects use GIF etc.
    if (!NeedsSpriteCrop(src, loaded.GetWidth(), loaded.GetHeight()))
    {
        out = std::move(loaded);
        return true;
    }
    return CropSpriteFromSource(loaded, src, out);
}

static bool EnsureSpriteFileFromSources(Project &project, const BuildConfig &config, BuildResult &result)
{
    GameData *gd = project.GetGameData();
    if (!gd)
        return false;

    const std::string proj_dir = project.GetProjectDir();
    const std::string spr_path = proj_dir + "/acsprset.spr";
    const std::string idx_path = proj_dir + "/sprindex.dat";

    std::map<int, GameData::SpriteSourceInfo> sources;
    CollectSpriteSources(*gd, sources);
    if (sources.empty())
        return true;

    if (!PathExists(proj_dir + "/Assets/Sprites"))
        return true;

    result.AddInfo("Generating acsprset.spr from Assets/Sprites (" +
                   std::to_string(sources.size()) + " source entries).");

    int topmost = -1;
    for (const auto &kv : sources)
        if (kv.first > topmost)
            topmost = kv.first;
    if (topmost < 0)
        return true;

    std::vector<std::pair<int, GameData::SpriteSourceInfo>> ordered;
    ordered.reserve(sources.size());
    for (const auto &kv : sources)
        ordered.push_back(kv);
    std::sort(ordered.begin(), ordered.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });

    using namespace AGS::Common;
    std::unique_ptr<Stream> out(File::CreateFile(spr_path.c_str()));
    if (!out)
    {
        result.AddWarning("", 0, "Failed to create sprite file: " + spr_path);
        return true;
    }

    SpriteFileWriter writer(std::move(out));
    int store_flags = 0;
    if (gd->optimize_sprite_storage)
        store_flags |= kSprStore_OptimizeForSize;
    const SpriteCompression compress = (SpriteCompression)gd->sprite_file_compression;
    writer.Begin(store_flags, compress, (sprkey_t)topmost);

    int next_id = 0;
    for (const auto &kv : ordered)
    {
        const int id = kv.first;
        const auto &src = kv.second;
        while (next_id < id)
        {
            writer.WriteEmptySlot();
            next_id++;
        }

        PixelBuffer px;
        if (!LoadSpritePixelFromSource(proj_dir, src, px) ||
            !px.GetData() || px.GetWidth() <= 0 || px.GetHeight() <= 0)
        {
            writer.WriteEmptySlot();
            next_id++;
            continue;
        }

        writer.WriteBitmap(px);
        next_id++;
    }

    while (next_id <= topmost)
    {
        writer.WriteEmptySlot();
        next_id++;
    }

    writer.Finalize();
    auto idx_err = SaveSpriteIndex(idx_path.c_str(), writer.GetIndex());
    if (!idx_err)
    {
        result.AddWarning("", 0,
            "Failed to write sprindex.dat: " + std::string(idx_err->FullMessage().GetCStr()));
        return true;
    }

    project.ReloadSpriteCache();
    result.AddInfo("Generated acsprset.spr + sprindex.dat up to slot " + std::to_string(topmost) + ".");
    (void)config;
    return true;
}

// -------------------------------------------------------------------------
// Create build output directories
// -------------------------------------------------------------------------
bool BuildSystem::CreateBuildDirectories(const BuildConfig& config, BuildResult& result)
{
    std::string base = GetCompiledDir(config);
    EnsureDirectory(base);
    EnsureDirectory(GetDataDir(config));

    if (!config.data_only)
    {
        for (int i = 0; i < (int)BuildTarget::Count; i++)
        {
            if (config.targets[i])
            {
                std::string dir = GetTargetDir(config, (BuildTarget)i);
                EnsureDirectory(dir);
            }
        }
    }

    // Temp directory for intermediate files
    EnsureDirectory(base + "/_temp");

    return true;
}

// -------------------------------------------------------------------------
// Ensure default WFN font files exist in the project directory.
// The C# editor does this at project load time (Tasks.AddFontIfNotAlreadyThere).
// Creates agsfntN.wfn for fonts 0-2 if neither .wfn nor .ttf exists.
// -------------------------------------------------------------------------
void BuildSystem::EnsureDefaultFonts(Project& project)
{
    std::string dir = project.GetProjectDir();
    for (int i = 0; i < AGSBuild::kDefaultFontCount; i++)
    {
        std::string ttf_path = dir + "/agsfnt" + std::to_string(i) + ".ttf";
        std::string wfn_path = dir + "/agsfnt" + std::to_string(i) + ".wfn";
        if (PathExists(ttf_path) || PathExists(wfn_path))
            continue;
        // Write default WFN
        std::ofstream out(wfn_path, std::ios::binary);
        if (out.is_open())
        {
            out.write(reinterpret_cast<const char*>(AGSBuild::kDefaultFonts[i].data),
                      (std::streamsize)AGSBuild::kDefaultFonts[i].size);
            out.close();
            Log("[Build] Created default font: agsfnt%d.wfn", i);
        }
    }
}

// -------------------------------------------------------------------------
// Pre-build validation
// -------------------------------------------------------------------------
bool BuildSystem::PreBuildChecks(Project& project, BuildResult& result)
{
    if (!project.IsLoaded())
    {
        result.AddError("", 0, "No project is loaded.");
        return false;
    }

    GameData* gd = project.GetGameData();
    if (!gd)
    {
        result.AddError("", 0, "Game data is null.");
        return false;
    }

    // Check that we have at least one script module
    if (gd->script_modules.empty())
    {
        result.AddError("", 0, "No script modules found in the project.");
        return false;
    }

    // Check for player character
    bool has_player = false;
    for (const auto& ch : gd->characters)
    {
        if (ch.id == 0)
        {
            has_player = true;
            break;
        }
    }
    if (!has_player && !gd->characters.empty())
        result.AddWarning("", 0, "No player character (ID 0) found.");

    // Check for at least one room
    if (gd->rooms.empty())
        result.AddWarning("", 0, "No rooms defined in the project.");

    // Ensure default font files exist in project directory
    // (the C# editor does this at project load time via SetDefaultGameContentIfMissing)
    EnsureDefaultFonts(project);

    // If the project was checked out without editor-generated caches, try to
    // generate what we can from source Assets/.
    EnsureAudioCacheFromSources(project, result);
    EnsureSpriteFileFromSources(project, config_, result);

    const std::string proj_dir = project.GetProjectDir();

    // Check that audio clip cache files exist
    for (const auto &clip : gd->audio_clips)
    {
        const std::string cache_rel = ResolveAudioCacheRelPath(clip);
        if (cache_rel.empty())
            continue;
        const std::string clip_path = proj_dir + "/" + cache_rel;
        if (!PathExists(clip_path))
        {
            result.AddWarning("", 0,
                "Audio clip '" + clip.name + "' references missing file: " + cache_rel);
        }
    }

    // Check that room files exist
    for (const auto& room : gd->rooms)
    {
        char room_name[64];
        snprintf(room_name, sizeof(room_name), "room%d.crm", room.number);
        std::string room_path = proj_dir + "/" + room_name;
        if (!PathExists(room_path))
        {
            result.AddWarning("", 0,
                "Room " + std::to_string(room.number) + " is defined but " +
                room_name + " was not found in the project directory.");
        }
    }

    // Check that font files exist
    for (const auto& font : gd->fonts)
    {
        char ttf_name[64], wfn_name[64];
        snprintf(ttf_name, sizeof(ttf_name), "agsfnt%d.ttf", font.id);
        snprintf(wfn_name, sizeof(wfn_name), "agsfnt%d.wfn", font.id);
        std::string ttf_path = proj_dir + "/" + ttf_name;
        std::string wfn_path = proj_dir + "/" + wfn_name;
        if (!PathExists(ttf_path) && !PathExists(wfn_path))
        {
            result.AddWarning("", 0,
                "Font " + std::to_string(font.id) + " has no file (" +
                ttf_name + " or " + wfn_name + ") in the project directory.");
        }
    }

    // Check room script-header dependencies (recompile stale rooms)
    // A room needs recompile if its .crm is older than its .asc or any .ash header.
    // Mirrors C# editor's RecompileAnyRoomsWhereTheScriptHasChanged().
    for (const auto& room : gd->rooms)
    {
        char crm_name[64], asc_name[64];
        snprintf(crm_name, sizeof(crm_name), "room%d.crm", room.number);
        snprintf(asc_name, sizeof(asc_name), "room%d.asc", room.number);
        std::string crm_path = proj_dir + "/" + crm_name;
        std::string asc_path = proj_dir + "/" + asc_name;

        if (!PathExists(crm_path))
            continue; // already warned above

        bool stale = false;
        std::string reason;

        // Check if room script is newer than compiled room
        if (PathExists(asc_path) && DoesFileNeedRecompile(asc_path, crm_path))
        {
            stale = true;
            reason = "room script changed";
        }

        // Check if any script header (.ash) is newer than compiled room
        if (!stale)
        {
            for (const auto& mod : gd->script_modules)
            {
                if (!mod.header_file.empty())
                {
                    std::string ash_path = proj_dir + "/" + mod.header_file;
                    if (DoesFileNeedRecompile(ash_path, crm_path))
                    {
                        stale = true;
                        reason = "script header '" + mod.header_file + "' changed";
                        break;
                    }
                }
            }
        }

        if (stale)
        {
            result.AddWarning("", 0,
                "Room " + std::to_string(room.number) + " may need recompilation (" +
                reason + "). Its .crm file is older than dependent sources.");
        }
    }

    return true;
}

// -------------------------------------------------------------------------
// Auto-generated script header (_AutoGenerated.ash)
// Mirrors the C# editor's Tasks.RegenerateScriptHeader()
// -------------------------------------------------------------------------

// Build a GameRef from the editor's GameData — provides the bridge between
// the editor's rich data model and the lightweight scriptgen structures.
static AGS::DataUtil::GameRef BuildGameRef(const GameData& gd)
{
    using namespace AGS::DataUtil;
    GameRef ref;

    // Helper: strip non-alphanumeric characters (except underscore) from names,
    // matching the sanitization in agfreader's ReadEntityRef.
    auto sanitize = [](const std::string& name) -> String {
        String result;
        for (char ch : name)
        {
            if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')
                result.AppendChar(ch);
        }
        return result;
    };

    // Audio clips
    for (const auto& ac : gd.audio_clips)
    {
        EntityRef e;
        e.TypeName = "AudioClip";
        e.ID = ac.id;
        e.ScriptName = sanitize(ac.name);
        ref.AudioClips.push_back(e);
    }

    // Audio clip types
    for (const auto& at : gd.audio_clip_types)
    {
        EntityRef e;
        e.ID = at.id;
        e.ScriptName = sanitize(at.name);
        ref.AudioTypes.push_back(e);
    }

    // Characters
    for (const auto& ch : gd.characters)
    {
        CharacterRef e;
        e.TypeName = "Character";
        e.ID = ch.id;
        e.ScriptName = sanitize(ch.script_name);
        ref.Characters.push_back(e);
    }

    // Cursors
    for (const auto& c : gd.cursors)
    {
        EntityRef e;
        e.ID = c.id;
        e.ScriptName = sanitize(c.name);
        ref.Cursors.push_back(e);
    }

    // Dialogs
    for (const auto& d : gd.dialogs)
    {
        DialogRef dr;
        dr.TypeName = "Dialog";
        dr.ID = d.id;
        dr.ScriptName = sanitize(d.script_name);
        dr.OptionCount = d.option_count;
        ref.Dialogs.push_back(dr);
    }

    // Fonts
    for (const auto& f : gd.fonts)
    {
        EntityRef e;
        e.ID = f.id;
        e.ScriptName = sanitize(f.name);
        ref.Fonts.push_back(e);
    }

    // GUIs (with controls)
    for (const auto& g : gd.guis)
    {
        GUIRef gr;
        gr.TypeName = "GUI";
        gr.ID = g.id;
        gr.ScriptName = sanitize(g.name);
        for (const auto& ctrl : g.controls)
        {
            EntityRef ce;
            ce.ID = ctrl.id;
            ce.ScriptName = sanitize(ctrl.name);
            // Map type_tag to script class type (matches C# GUIControl.ScriptClassType)
            if (ctrl.type_tag == "GUIButton")          ce.TypeName = "Button";
            else if (ctrl.type_tag == "GUILabel")      ce.TypeName = "Label";
            else if (ctrl.type_tag == "GUISlider")     ce.TypeName = "Slider";
            else if (ctrl.type_tag == "GUIListBox")    ce.TypeName = "ListBox";
            else if (ctrl.type_tag == "GUITextBox")    ce.TypeName = "TextBox";
            else if (ctrl.type_tag == "GUIInventory")  ce.TypeName = "InvWindow";
            else if (ctrl.type_tag == "GUITextWindowEdge") ce.TypeName = "Button";
            gr.Controls.push_back(ce);
        }
        ref.GUI.push_back(gr);
    }

    // Inventory items
    for (const auto& inv : gd.inventory_items)
    {
        EntityRef e;
        e.TypeName = "InventoryItem";
        e.ID = inv.id;
        e.ScriptName = sanitize(inv.script_name);
        ref.Inventory.push_back(e);
    }

    // Views
    for (const auto& v : gd.views)
    {
        EntityRef e;
        e.ID = v.id;
        e.ScriptName = sanitize(v.name);
        ref.Views.push_back(e);
    }

    // Global variables
    for (const auto& gv : gd.global_variables)
    {
        Variable var;
        var.Type = gv.type_name.c_str();
        var.Name = gv.name.c_str();
        var.Value = gv.default_value.c_str();
        ref.GlobalVars.push_back(var);
    }

    // Settings
    ref.Settings.SayFunction = gd.dialog_say_function.c_str();
    ref.Settings.NarrateFunction = gd.dialog_narrate_function.c_str();

    return ref;
}

static std::vector<AGS::DataUtil::Variable> BuildScriptGlobalVariables(const GameData& gd)
{
    std::vector<AGS::DataUtil::Variable> vars;
    vars.reserve(gd.global_variables.size());
    for (const auto& gv : gd.global_variables)
    {
        AGS::DataUtil::Variable var;
        var.Name = gv.name.c_str();
        var.Type = gv.type_name.c_str();
        if (gv.type_name == "bool" && !gv.default_value.empty())
        {
            AGS::Common::String value(gv.default_value.c_str());
            value.MakeLower();
            var.Value = value;
        }
        else
        {
            var.Value = gv.default_value.c_str();
        }
        vars.push_back(var);
    }
    return vars;
}

bool BuildSystem::GenerateAutoHeader(Project& project, std::string& out_header)
{
    GameData* gd = project.GetGameData();
    if (!gd) return false;

    AGS::DataUtil::GameRef game_ref = BuildGameRef(*gd);
    AGS::DataUtil::String header = AGS::DataUtil::MakeGameAutoScriptHeader(game_ref);
    out_header = header.GetCStr();
    return true;
}

// -------------------------------------------------------------------------
// Configure a fresh preprocessor with all macros driven by game settings.
// Matches the C# editor's DefineMacrosAccordingToGameSettings().
// -------------------------------------------------------------------------
static void ConfigurePreprocessor(AGS::Preprocessor::Preprocessor& pp,
                                   const GameData& gd,
                                   const BuildConfig& config)
{
    pp.SetAppVersion("3.6.3");

    pp.DefineMacro("AGS_SUPPORTS_IFVER", "1");
    if (gd.enforce_new_strings)
        pp.DefineMacro("AGS_NEW_STRINGS", "1");

    // STRICT macro: The modern agsdefns.sh from our repo moves more functions
    // (GetGlobalInt, SetGlobalInt, etc.) behind #ifndef STRICT than older versions
    // did. Old projects (pre-3.4.0) were created when those functions were still
    // available with STRICT. To avoid false compile errors with old projects when
    // using the modern agsdefns.sh, only define STRICT for projects saved with
    // AGS 3.4.0 or later.
    {
        bool is_old_project = false;
        if (!gd.saved_editor_version.empty())
        {
            int major = 0, minor = 0, patch = 0;
            sscanf(gd.saved_editor_version.c_str(), "%d.%d.%d", &major, &minor, &patch);
            if (major < 3 || (major == 3 && minor < 4))
                is_old_project = true;
        }
        if (gd.enforce_object_scripting && !is_old_project)
            pp.DefineMacro("STRICT", "1");
    }

    if (gd.left_to_right_precedence)
        pp.DefineMacro("LRPRECEDENCE", "1");
    if (gd.enforce_new_strings)
        pp.DefineMacro("STRICT_STRINGS", "1");
    if (gd.enforce_new_audio)
        pp.DefineMacro("STRICT_AUDIO", "1");
    if (!gd.use_old_custom_dialog_api)
        pp.DefineMacro("NEW_DIALOGOPTS_API", "1");
    if (!gd.use_old_keyboard_handling)
        pp.DefineMacro("NEW_KEYINPUT_API", "1");

    if (config.debug_mode)
        pp.DefineMacro("DEBUG", "1");

    int api_ver = gd.script_api_version;
    for (int i = 0; i < kScriptAPIVersionCount; i++)
    {
        std::string macro = "SCRIPT_API_" + std::string(kScriptAPIVersions[i].name);
        pp.DefineMacro(macro.c_str(), "1");
        if (api_ver != kScriptAPIHighest &&
            kScriptAPIVersions[i].value >= api_ver)
            break;
    }

    int compat_ver = gd.script_compat_level;
    if (compat_ver == kScriptAPIHighest)
        compat_ver = kScriptAPIVersions[kScriptAPIVersionCount - 1].value;
    for (int i = 0; i < kScriptAPIVersionCount; i++)
    {
        if (kScriptAPIVersions[i].value >= compat_ver)
        {
            std::string macro = "SCRIPT_COMPAT_" + std::string(kScriptAPIVersions[i].name);
            pp.DefineMacro(macro.c_str(), "1");
        }
    }
}

// -------------------------------------------------------------------------
// Preprocess all accumulated raw headers + a script body using a FRESH
// preprocessor, then set up ccDefaultHeaders and compile.
// Matches the C# editor's per-module CompileScript() pattern:
//   1. Create fresh Preprocessor (clean macro state)
//   2. Preprocess each raw header in order
//   3. Preprocess the script body
//   4. ccRemoveDefaultHeaders() + ccAddDefaultHeader() for each header
//   5. ccCompileText() with the preprocessed body
// This avoids macro state leaking between modules (e.g. agsdefns.sh
// #define/#undef cycles causing "macro already defined" errors).
// -------------------------------------------------------------------------
struct RawHeader
{
    std::string text;
    std::string name;
};

static ccScript* PreprocessAndCompile(
    const GameData& gd, const BuildConfig& config,
    const std::vector<RawHeader>& raw_headers,
    const std::string& script_text, const std::string& script_name,
    BuildResult& result, const std::string& module_name,
    const std::string& temp_dir)
{
    // Fresh preprocessor per module — no macro state carried over
    AGS::Preprocessor::Preprocessor pp;
    ConfigurePreprocessor(pp, gd, config);

    // Preprocess all accumulated headers from scratch
    std::vector<AGS::Common::String> pp_headers;
    pp_headers.reserve(raw_headers.size());
    for (const auto& hdr : raw_headers)
    {
        AGS::Common::String pp_hdr = pp.Preprocess(
            hdr.text.c_str(), hdr.name.c_str());
        const auto& pp_err = pp.GetLastError();
        if (pp_err.Type != AGS::Preprocessor::ErrorCode::None)
        {
            result.AddError(hdr.name, 0,
                std::string("Preprocessor error in header: ") +
                pp_err.Message.GetCStr(), module_name);
            return nullptr;
        }
        pp_headers.push_back(pp_hdr);
    }

    // Preprocess script body
    AGS::Common::String pp_script = pp.Preprocess(
        script_text.c_str(), script_name.c_str());
    {
        const auto& pp_err = pp.GetLastError();
        if (pp_err.Type != AGS::Preprocessor::ErrorCode::None)
        {
            result.AddError(script_name, 0,
                std::string("Preprocessor error: ") +
                pp_err.Message.GetCStr(), module_name);
            return nullptr;
        }
    }

    // DEBUG: dump preprocessed script
    if (!temp_dir.empty())
    {
        std::string dump_path = temp_dir + "/" + module_name + "_pp_script.txt";
        std::ofstream dump(dump_path);
        if (dump.is_open())
            dump << pp_script.GetCStr();
    }

    // Set up native compiler headers: clear all prior, re-add preprocessed
    ccRemoveDefaultHeaders();
    for (size_t i = 0; i < pp_headers.size(); i++)
    {
        ccAddDefaultHeader(pp_headers[i].GetCStr(), raw_headers[i].name.c_str());
    }

    // Compile
    cc_clear_error();
    ccScript* compiled = ccCompileText(pp_script.GetCStr(), module_name.c_str());

    if (!compiled || cc_has_error())
    {
        const ScriptError& err = cc_get_error();
        std::string error_msg = err.HasError
            ? err.ErrorString.GetCStr()
            : "Unknown compilation error";
        int error_line = err.HasError ? err.Line : 0;

        result.AddError(script_name, error_line, error_msg, module_name);
        return nullptr;
    }

    return compiled;
}

// -------------------------------------------------------------------------
// Post-process converted dialog script to escape bare double quotes in
// character speech string arguments.
// The C# editor's DialogScriptConverter.ProcessCharacterSpeech does:
//   textToSay = textToSay.Replace("\"", "\\\"");
// The C++ converter in Tools/data/ has this commented out (TODO marker).
// we apply the equivalent fix as a post-processing step on the converter
// output.
//
// The converter always formats speech as one of:
//   cChar.Say("speech text");          -- opening " right after (
//   cChar.SayAt(x, y, w, "text");     -- opening " after last ,<space>
//   Display("speech text");            -- opening " right after (
// We find the opening " (first " after '('), the closing " (last before ");"),
// and escape all bare " between them.
// -------------------------------------------------------------------------
static std::string FixDialogSpeechQuotes(const std::string& script)
{
    std::string result;
    result.reserve(script.size() + 64);
    size_t pos = 0;
    while (pos < script.size())
    {
        size_t eol = script.find('\n', pos);
        if (eol == std::string::npos)
            eol = script.size();

        size_t len = eol - pos;

        // Line must end with ");
        if (len >= 4
            && script[pos + len - 1] == ';'
            && script[pos + len - 2] == ')')
        {
            // Find closing " — the last " before ");"
            // It may be preceded by \" (escaped quote in text), so we need
            // the " that is NOT preceded by backslash, scanning from right.
            size_t closing = std::string::npos;
            for (size_t i = pos + len - 3; i > pos; --i)
            {
                if (script[i] == '"' && (i == 0 || script[i - 1] != '\\'))
                {
                    closing = i;
                    break;
                }
            }

            // Find opening " — search forward for first ("
            size_t opening = std::string::npos;
            if (closing != std::string::npos)
            {
                for (size_t i = pos; i < closing; ++i)
                {
                    if (script[i] == '(' && i + 1 < closing && script[i + 1] == '"')
                    {
                        opening = i + 1;
                        break;
                    }
                }
                // For SayAt(x, y, w, "text") — opening may be after ", "
                if (opening == std::string::npos)
                {
                    for (size_t i = pos; i < closing; ++i)
                    {
                        if (script[i] == ',' && i + 2 < closing
                            && script[i + 1] == ' ' && script[i + 2] == '"')
                        {
                            opening = i + 2;
                            // Don't break — take the LAST ", " before closing
                        }
                    }
                }
            }

            if (opening != std::string::npos && closing != std::string::npos
                && opening < closing)
            {
                // Check if there are bare quotes between delimiters
                bool needs_fix = false;
                for (size_t i = opening + 1; i < closing; ++i)
                {
                    if (script[i] == '"' && script[i - 1] != '\\')
                    {
                        needs_fix = true;
                        break;
                    }
                }

                if (needs_fix)
                {
                    // Copy everything up to and including opening "
                    result.append(script, pos, opening + 1 - pos);
                    // Escape bare quotes in the text content
                    for (size_t i = opening + 1; i < closing; ++i)
                    {
                        if (script[i] == '"' && script[i - 1] != '\\')
                        {
                            result += '\\';
                            result += '"';
                        }
                        else
                        {
                            result += script[i];
                        }
                    }
                    // Copy from closing " to end of line
                    result.append(script, closing, eol - closing);
                    if (eol < script.size())
                        result += '\n';
                    pos = eol + 1;
                    continue;
                }
            }
        }

        result.append(script, pos, eol - pos);
        if (eol < script.size())
            result += '\n';
        pos = eol + 1;
    }

    return result;
}

// -------------------------------------------------------------------------
// Fix import/export param-count mismatches across compiled scripts.
//
// Some AGS game projects have import declarations in headers that list
// fewer parameters than the real function definition in the implementing
// module (e.g.  import function options_Click();  vs.  function
// options_Click(GUIControl*, MouseButton)).
//
// The compiler stores imports with  name^argcount  (using '^') and exports
// with  name$argcount  (using '$').  At link-time the engine's
// ScriptSymbolsMap::GetIndexOfAny would need a fuzzy-match fallback to
// tolerate the discrepancy; rather than patching the engine we fix up the
// compiled .o files here so the param counts are consistent.
//
// Algorithm
//   1. Read every compiled .o, collect ALL exports  (base_name -> argcount).
//   2. For each .o, scan its imports: if  base^N  has no matching export
//      base$N  but  base$M  exists, rewrite the import to  base^M.
//   3. Rewrite only the .o files that were modified.
// -------------------------------------------------------------------------
static void FixImportExportMismatches(
    const std::vector<CompiledScriptInfo>& compiled_scripts)
{
    // Step 1 — collect every export's base name -> full export string
    //   Export format: "name$argcount"
    //   We also build a set of all full export names for quick exact-match.
    std::unordered_map<std::string, std::string> export_base_to_full;
    std::unordered_set<std::string> all_exports_as_imports; // with '^' separator

    for (const auto& cs : compiled_scripts)
    {
        if (cs.is_header) continue;
        auto stream = AGS::Common::File::OpenFileRead(cs.obj_file.c_str());
        if (!stream) continue;

        ccScript script;
        if (!script.Read(stream.get())) continue;

        for (const auto& exp : script.exports)
        {
            size_t dollar = exp.find('$');
            if (dollar != std::string::npos)
            {
                std::string base = exp.substr(0, dollar);
                std::string argcount = exp.substr(dollar + 1);
                export_base_to_full[base] = exp;
                // Store as import-style (^) for quick lookup
                all_exports_as_imports.insert(base + "^" + argcount);
            }
        }
    }

    // Step 2 — patch each script's imports where needed
    for (const auto& cs : compiled_scripts)
    {
        if (cs.is_header) continue;
        auto in_stream = AGS::Common::File::OpenFileRead(cs.obj_file.c_str());
        if (!in_stream) continue;

        ccScript script;
        if (!script.Read(in_stream.get())) continue;
        in_stream.reset();

        bool modified = false;
        for (auto& imp : script.imports)
        {
            if (imp.empty()) continue;

            size_t caret = imp.find('^');
            if (caret == std::string::npos) continue;

            // Already resolvable? skip
            if (all_exports_as_imports.count(imp)) continue;

            // Look for an export with the same base name
            std::string base = imp.substr(0, caret);
            auto it = export_base_to_full.find(base);
            if (it == export_base_to_full.end()) continue;

            // Found export with different param count — fix the import
            // Convert export "name$M" -> import "name^M"
            size_t dollar = it->second.find('$');
            if (dollar == std::string::npos) continue;
            std::string fixed_import = base + "^" + it->second.substr(dollar + 1);

            AGSBuild::Logger::Log("[Build] Fixing import mismatch in '%s': %s -> %s",
                cs.name.c_str(), imp.c_str(), fixed_import.c_str());
            imp = fixed_import;
            modified = true;
        }

        if (modified)
        {
            auto out_stream = AGS::Common::File::CreateFile(cs.obj_file.c_str());
            if (out_stream)
            {
                script.Write(out_stream.get());
            }
        }
    }
}

// -------------------------------------------------------------------------
// Full script compilation pipeline
// Mirrors the C# editor's AGSEditor.CompileScripts()
// Order: auto-header -> internal headers -> script module headers+bodies -> dialog scripts
//
// Key design (matching C# editor):
//   - Raw (unpreprocessed) header texts accumulate as modules are processed
//   - For each module, a FRESH preprocessor re-processes all accumulated
//     headers + the module body. This prevents macro state leaking between
//     modules and avoids the "macro already defined" error.
//   - ccRemoveDefaultHeaders() + ccAddDefaultHeader() are called per module
// -------------------------------------------------------------------------
bool BuildSystem::CompileAllScripts(Project& project, const BuildConfig& config,
                                     BuildResult& result)
{
    GameData* gd = project.GetGameData();
    if (!gd) return false;

    compiled_scripts_.clear();
    std::string temp_dir = GetCompiledDir(config) + "/_temp";

    // --- Setup compiler options (driven by game settings) ---
    ccSetSoftwareVersion("3.6.3");
    ccSetOption(SCOPT_EXPORTALL, 1);
    ccSetOption(SCOPT_LINENUMBERS, config.line_numbers ? 1 : 0);
    ccSetOption(SCOPT_LEFTTORIGHT, gd->left_to_right_precedence ? 1 : 0);
    ccSetOption(SCOPT_OLDSTRINGS, gd->enforce_new_strings ? 0 : 1);

    // Accumulate raw (unpreprocessed) header texts. Each module compilation
    // creates a fresh Preprocessor and re-preprocesses ALL headers from scratch.
    // This matches the C# editor pattern and avoids macro state leaking.
    std::vector<RawHeader> raw_headers;

    // --- 0. Load built-in header (agsdefns.sh) - store raw text ---
    {
        std::string builtin_header;
        std::vector<std::string> search_paths = {
#ifdef SOURCE_DIR
            std::string(SOURCE_DIR) + "/Editor/AGS.Editor/Resources/agsdefns.sh",
#endif
            "../Editor/AGS.Editor/Resources/agsdefns.sh",
            "Editor/AGS.Editor/Resources/agsdefns.sh",
            "/usr/share/ags/agsdefns.sh",
        };
        for (const auto& path : search_paths)
        {
            std::ifstream file(path);
            if (file.is_open())
            {
                std::ostringstream ss;
                ss << file.rdbuf();
                builtin_header = ss.str();
                Log("[Build] Loaded built-in header: %s (%d bytes)",
                    path.c_str(), (int)builtin_header.size());
                break;
            }
        }
        if (builtin_header.empty())
        {
            result.AddError("", 0,
                "Could not find agsdefns.sh (built-in script header). "
                "Scripts using AGS API types will fail to compile.");
            return false;
        }
        raw_headers.push_back({builtin_header, "_BuiltInScriptHeader"});
    }

    // --- Generate auto-header ---
    std::string auto_header;
    if (!GenerateAutoHeader(project, auto_header))
    {
        result.AddError("", 0, "Failed to generate auto-header.");
        return false;
    }

    Log("[Build] Generated _AutoGenerated.ash (%d bytes)", (int)auto_header.size());

    // Save auto-header for debugging
    {
        std::string auto_hdr_path = temp_dir + "/_AutoGenerated.ash";
        std::ofstream out(auto_hdr_path);
        if (out.is_open()) {
            out << auto_header;
            out.close();
        }
    }

    // --- Accumulate raw headers and compile each module ---
    // Raw auto-generated header
    raw_headers.push_back({auto_header, "_AutoGenerated"});

    // Step 1b: Generate and compile _GlobalVariables module
    if (!gd->global_variables.empty())
    {
        std::vector<AGS::DataUtil::Variable> gv_vars = BuildScriptGlobalVariables(*gd);
        const std::string gv_header =
            AGS::DataUtil::MakeVariablesScriptHeader(gv_vars).GetCStr();
        raw_headers.push_back({gv_header, "_GlobalVariables"});

        const std::string gv_body =
            AGS::DataUtil::MakeVariablesScriptBody(gv_vars).GetCStr();

        // Compile _GlobalVariables using fresh-preprocessor pattern
        ccScript* gv_compiled = PreprocessAndCompile(
            *gd, config, raw_headers, gv_body, "_GlobalVariables.asc",
            result, "_GlobalVariables", temp_dir);

        if (!gv_compiled)
            return false;

        // Write compiled script to temp file
        std::string gv_obj_path = temp_dir + "/_GlobalVariables.o";
        {
            std::unique_ptr<AGS::Common::Stream> out(
                AGS::Common::File::CreateFile(gv_obj_path.c_str()));
            if (out)
            {
                gv_compiled->Write(out.get());
            }
            else
            {
                result.AddError("", 0, "Failed to write compiled script: " + gv_obj_path);
                delete gv_compiled;
                return false;
            }
        }

        compiled_scripts_.push_back({"_GlobalVariables", gv_obj_path, false});
        delete gv_compiled;

        Log("[Build] _GlobalVariables compiled (%d variables).", (int)gd->global_variables.size());
    }

    // Step 2: Compile each script module
    int module_count = (int)gd->script_modules.size();
    int compiled_count = 0;

    for (int i = 0; i < module_count; i++)
    {
        const auto& mod = gd->script_modules[i];
        progress_.Advance("Compiling: " + mod.name);
        Log("[Build] Compiling module: %s", mod.name.c_str());

        // Read header file (if exists) and accumulate raw text
        // (matching C# pattern: headers.Add(scripts.Header) before CompileScript)
        if (!mod.header_file.empty())
        {
            std::string header_path = FindFileCaseInsensitive(
                project.GetProjectDir(), mod.header_file);
            if (PathExists(header_path))
            {
                std::string header_content;
                ReadTextFile(header_path, header_content);
                if (!header_content.empty())
                {
                    std::string hdr_name = mod.name + "_header";
                    raw_headers.push_back({header_content, hdr_name});
                }
            }
            else
            {
                Log("[Build]   Warning: header file not found: %s", header_path.c_str());
            }
        }

        // Read script body (case-insensitive lookup for Windows projects on Linux)
        std::string script_content;
        if (!mod.script_file.empty())
        {
            std::string script_path = FindFileCaseInsensitive(
                project.GetProjectDir(), mod.script_file);
            if (PathExists(script_path))
            {
                ReadTextFile(script_path, script_content);
            }
            else
            {
                Log("[Build]   Error: script file not found: %s", script_path.c_str());
                result.AddError(mod.script_file, 0,
                    "Script file not found: " + script_path, mod.name);
                return false;
            }
        }

        if (script_content.empty())
        {
            Log("[Build]   Skipping module '%s' (no script body)", mod.name.c_str());
            continue;
        }

        // Compile with fresh preprocessor (re-processes all headers from scratch)
        ccScript* compiled = PreprocessAndCompile(
            *gd, config, raw_headers, script_content, mod.script_file,
            result, mod.name, temp_dir);

        if (!compiled)
            return false;

        // Write compiled script object to temp file
        std::string obj_path = temp_dir + "/" + mod.name + ".o";
        {
            std::unique_ptr<AGS::Common::Stream> out(
                AGS::Common::File::CreateFile(obj_path.c_str()));
            if (out)
            {
                compiled->Write(out.get());
            }
            else
            {
                result.AddError("", 0, "Failed to write compiled script: " + obj_path);
                delete compiled;
                return false;
            }
        }

        compiled_scripts_.push_back({mod.name, obj_path, false});
        compiled_count++;
        delete compiled;

        Log("[Build]   %s compiled successfully.", mod.name.c_str());
    }

    Log("[Build] Script compilation complete: %d/%d modules compiled.",
        compiled_count, module_count);

    // --- Step 3: Compile dialog scripts ---
    // Dialog scripts use a special dialect that gets converted to regular
    // AGS script by DialogScriptConverter. All dialogs are merged into a
    // single "__DialogScripts" compilation unit, matching the C# editor.
    // Even with zero dialogs, we must produce an empty dialog script.
    {
        progress_.Advance("Compiling dialog scripts");
        Log("[Build] Compiling dialog scripts (%d dialogs)...",
            (int)gd->dialogs.size());

        // Build GameRef for the converter
        AGS::DataUtil::GameRef game_ref;
        game_ref.Settings.SayFunction =
            AGS::Common::String(gd->dialog_say_function.c_str());
        game_ref.Settings.NarrateFunction =
            AGS::Common::String(gd->dialog_narrate_function.c_str());

        for (const auto& ch : gd->characters)
        {
            AGS::DataUtil::CharacterRef cr;
            cr.ID = ch.id;
            cr.ScriptName = AGS::Common::String(ch.script_name.c_str());
            game_ref.Characters.push_back(cr);
        }

        // Populate GameRef.Dialogs so goto-dialog can look up by name
        for (const auto& d : gd->dialogs)
        {
            AGS::DataUtil::DialogRef dr;
            dr.ID = d.id;
            dr.ScriptName = AGS::Common::String(d.name.c_str());
            dr.OptionCount = d.option_count;
            game_ref.Dialogs.push_back(dr);
        }

        // Start with the default dialog script preamble
        std::string dialog_body = AGS::DataUtil::DialogScriptDefault;

        bool has_errors = false;
        for (const auto& d : gd->dialogs)
        {
            AGS::DataUtil::DialogRef dlg_ref;
            dlg_ref.ID = d.id;
            dlg_ref.ScriptName = AGS::Common::String(d.name.c_str());
            dlg_ref.OptionCount = d.option_count;

            AGS::DataUtil::DialogScriptConverter conv(
                AGS::Common::String(d.script.c_str()), game_ref, dlg_ref);
            AGS::Common::String converted = conv.Convert();

            // Check for conversion errors
            for (const auto& e : conv.GetErrors())
            {
                if (e.Error)
                {
                    std::string msg = "Dialog " + std::to_string(d.id) +
                        " (" + d.name + "): " + e.Message.GetCStr();
                    result.AddError("__DialogScripts", (int)e.LineNumber, msg);
                    Log("[Error] %s", msg.c_str());
                    has_errors = true;
                }
            }

            // Append the new-script marker + converted script
            dialog_body += AGS::DataUtil::NEW_SCRIPT_MARKER;
            dialog_body += "Dialog ";
            dialog_body += std::to_string(d.id);
            dialog_body += "\"\n";
            dialog_body += FixDialogSpeechQuotes(converted.GetCStr());
        }

        if (has_errors)
            return false;

        // DEBUG: dump merged dialog script
        {
            std::string dump_path = temp_dir + "/__DialogScripts_merged.txt";
            std::ofstream dump(dump_path);
            if (dump.is_open()) {
                dump << dialog_body;
                dump.close();
            }
        }

        // Compile dialog scripts with fresh preprocessor
        ccScript* dlg_compiled = PreprocessAndCompile(
            *gd, config, raw_headers, dialog_body, "__DialogScripts",
            result, "__DialogScripts", temp_dir);

        if (!dlg_compiled)
            return false;

        // Write compiled dialog script to temp file
        std::string dlg_obj_path = temp_dir + "/__DialogScripts.o";
        {
            std::unique_ptr<AGS::Common::Stream> out(
                AGS::Common::File::CreateFile(dlg_obj_path.c_str()));
            if (out)
            {
                dlg_compiled->Write(out.get());
            }
            else
            {
                result.AddError("", 0,
                    "Failed to write compiled dialog script: " + dlg_obj_path);
                delete dlg_compiled;
                return false;
            }
        }

        compiled_scripts_.push_back(
            {"__DialogScripts", dlg_obj_path, false});
        delete dlg_compiled;

        Log("[Build] Dialog scripts compiled successfully.");
    }

    // --- Step 4: Fix import/export param-count mismatches ---
    // Some games have buggy import declarations (e.g. 0-param import for a
    // 2-param function). The engine's fuzzy-match would accept them at
    // link-time, but we can fix them here so the vanilla engine works.
    FixImportExportMismatches(compiled_scripts_);

    return true;
}

// -------------------------------------------------------------------------
// Game data file generation (game28.dta)
// Delegates to the game_data_writer module which produces the exact binary
// format that the AGS engine expects.
// -------------------------------------------------------------------------
bool BuildSystem::WriteGameDataFile(Project& project, const BuildConfig& config,
                                     BuildResult& result, const std::string& output_path)
{
    GameData* gd = project.GetGameData();
    if (!gd) return false;

    progress_.Advance("Writing game data file");
    Log("[Build] Writing game data: %s", output_path.c_str());

    // Convert compiled script info to the writer's format.
    // The DTA format requires scripts in this exact order:
    //   [0]  = GlobalScript
    //   [1]  = __DialogScripts
    //   [2+] = script modules (_GlobalVariables first, then user modules)
    // This matches the C# editor's DataFileWriter.WriteCompiledScript order.
    std::vector<AGSBuild::CompiledScriptRef> scripts;

    // Classify compiled scripts into their DTA slots
    const CompiledScriptInfo* global_script = nullptr;
    const CompiledScriptInfo* dialog_script = nullptr;
    const CompiledScriptInfo* global_vars = nullptr;
    std::vector<const CompiledScriptInfo*> modules;

    for (const auto& cs : compiled_scripts_) {
        if (cs.is_header) continue;
        if (cs.name == "GlobalScript")
            global_script = &cs;
        else if (cs.name == "__DialogScripts")
            dialog_script = &cs;
        else if (cs.name == "_GlobalVariables")
            global_vars = &cs;
        else
            modules.push_back(&cs);
    }

    if (global_script)
        scripts.push_back({global_script->name, global_script->obj_file});
    if (dialog_script)
        scripts.push_back({dialog_script->name, dialog_script->obj_file});
    // _GlobalVariables is a script module that must come first (before user modules)
    if (global_vars)
        scripts.push_back({global_vars->name, global_vars->obj_file});
    for (const auto* m : modules)
        scripts.push_back({m->name, m->obj_file});

    if (!AGSBuild::WriteGameDataFile(*gd, scripts, output_path, result)) {
        return false;
    }

    Log("[Build] Game data written successfully.");
    result.output_files.push_back(output_path);
    return true;
}

// Helper: write a CLIB VOX file from a list of asset entries
static bool WriteVOXFile(const std::string& vox_path,
                         const std::vector<std::pair<std::string, std::string>>& entries)
{
    if (entries.empty())
        return true;

    std::vector<AGS::DataUtil::AssetFileEntry> mfl_entries;
    mfl_entries.reserve(entries.size());
    for (const auto &e : entries)
    {
        AGS::DataUtil::AssetFileEntry entry;
        entry.FileName = e.first.c_str();
        entry.SourcePath = e.second.c_str();
        mfl_entries.push_back(std::move(entry));
    }

    auto err = AGS::DataUtil::WriteLibraryFromPaths(
        AGS::Common::String(vox_path.c_str()), mfl_entries);
    return static_cast<bool>(err);
}

// -------------------------------------------------------------------------
// Asset packaging (<GameFileName>.ags)
// Creates a CLIB-format package containing all game assets.
// -------------------------------------------------------------------------
bool BuildSystem::PackageAssets(Project& project, const BuildConfig& config,
                                 BuildResult& result, const std::string& data_dir)
{
    progress_.Advance("Packaging assets");
    Log("[Build] Packaging assets...");

    GameData* gd = project.GetGameData();
    const std::string base_name = gd ? GetBaseGameFileName(project, *gd) : "game";
    std::string pak_path = data_dir + "/" + base_name + ".ags";
    const std::string proj_dir = project.GetProjectDir();

    using AssetEntry = std::pair<std::string, std::string>;
    std::vector<AssetEntry> assets;

    // preload.pcx
    AddAssetIfExists(assets, "preload.pcx", proj_dir + "/preload.pcx");

    // sprindex.dat
    AddAssetIfExists(assets, "sprindex.dat", proj_dir + "/sprindex.dat");

    // InGameEXE audio clips (AudioCache/au*.ext)
    if (gd)
    {
        for (const auto& clip : gd->audio_clips)
        {
            if (clip.bundling_type != 0)
                continue;
            const std::string cache_rel = ResolveAudioCacheRelPath(clip);
            if (cache_rel.empty())
                continue;
            const std::string clip_path = proj_dir + "/" + cache_rel;
            AddAssetIfExists(assets, AssetBaseName(cache_rel), clip_path);
        }
    }

    // flic*.fl?
    {
        std::vector<std::string> flic_files;
        CollectProjectFiles(proj_dir, MatchFlicWildcard, flic_files);
        for (const auto& fname : flic_files)
            AddAssetIfExists(assets, fname, proj_dir + "/" + fname);
    }

    // game28.dta (temporary build artifact)
    const std::string dta_path =
        GetCompiledDir(config) + "/_temp/" + kCompiledDtaFileName;
    AddAssetIfExists(assets, kCompiledDtaFileName, dta_path);

    // agsfnt*.ttf / agsfnt*.wfn
    if (gd)
    {
        for (const auto& font : gd->fonts)
        {
            char ttf_name[64], wfn_name[64];
            snprintf(ttf_name, sizeof(ttf_name), "agsfnt%d.ttf", font.id);
            snprintf(wfn_name, sizeof(wfn_name), "agsfnt%d.wfn", font.id);

            const std::string ttf_path = proj_dir + "/" + ttf_name;
            const std::string wfn_path = proj_dir + "/" + wfn_name;

            if (PathExists(ttf_path))
                assets.push_back({ttf_name, ttf_path});
            else if (PathExists(wfn_path))
                assets.push_back({wfn_name, wfn_path});
            else
            {
                Log("[Build] Warning: font file not found for font %d (%s / %s)",
                    font.id, ttf_name, wfn_name);
            }
        }
    }

    // acsprset.spr
    AddAssetIfExists(assets, "acsprset.spr", proj_dir + "/acsprset.spr");

    // room*.crm
    if (gd)
    {
        for (const auto& room : gd->rooms)
        {
            char room_name[64];
            snprintf(room_name, sizeof(room_name), "room%d.crm", room.number);
            AddAssetIfExists(assets, room_name, proj_dir + "/" + room_name);
        }
    }

    // *.ogv
    {
        std::vector<std::string> ogv_files;
        CollectProjectFiles(proj_dir, MatchOgvExtension, ogv_files);
        for (const auto& fname : ogv_files)
            AddAssetIfExists(assets, fname, proj_dir + "/" + fname);
    }

    // Custom data folders (Settings/CustomDataDir)
    if (gd)
        CollectCustomDataAssets(project, *gd, result, assets);

    if (assets.empty())
        result.AddWarning("", 0, "No assets found to package.");

    std::vector<AGS::DataUtil::AssetFileEntry> mfl_entries;
    mfl_entries.reserve(assets.size());
    for (const auto &e : assets)
    {
        AGS::DataUtil::AssetFileEntry entry;
        entry.FileName = e.first.c_str();
        entry.SourcePath = e.second.c_str();
        mfl_entries.push_back(std::move(entry));
    }

    auto mfl_err = AGS::DataUtil::WriteLibraryFromPaths(
        AGS::Common::String(pak_path.c_str()), mfl_entries);
    if (!mfl_err)
    {
        result.AddError("", 0,
            "Failed to write package: " + std::string(mfl_err->FullMessage().GetCStr()));
        return false;
    }

    Log("[Build] Package created: %s (%d assets)", pak_path.c_str(), (int)assets.size());
    result.output_files.push_back(pak_path);

    // Delete temporary DTA (matches Editor BuildTargetDataFile)
    if (PathExists(dta_path))
        std::remove(dta_path.c_str());

    return true;
}

// -------------------------------------------------------------------------
// Compile registered translations to Compiled/Data/*.tra (not packed in .ags)
// -------------------------------------------------------------------------
bool BuildSystem::CompileRegisteredTranslations(Project& project,
                                                 const BuildConfig& /*config*/,
                                                 BuildResult& result,
                                                 const std::string& data_dir)
{
    GameData* gd = project.GetGameData();
    if (!gd || gd->translations.empty())
        return true;

    progress_.Advance("Compiling translations");
    Log("[Build] Compiling %d registered translation(s)...", (int)gd->translations.size());

    const std::string proj_dir = project.GetProjectDir();
    for (const auto& tr : gd->translations)
    {
        const std::string trs_name = tr.name + ".trs";
        const std::string trs_path = proj_dir + "/" + trs_name;
        const std::string tra_name = tr.name + ".tra";
        const std::string tra_path = data_dir + "/" + tra_name;

        if (!PathExists(trs_path))
        {
            result.AddWarning("", 0, "Translation source not found: " + trs_name);
            continue;
        }

        auto trs_in = std::unique_ptr<AGS::Common::Stream>(
            AGS::Common::File::OpenFileRead(AGS::Common::String(trs_path.c_str())));
        if (!trs_in)
        {
            result.AddWarning("", 0, "Failed to open translation: " + trs_name);
            continue;
        }

        AGS::Common::Translation tra;
        tra.GameUid = gd->unique_id;
        tra.GameName = AGS::Common::String(gd->game_title.c_str());

        AGS::Common::HError err = AGS::DataUtil::ReadTRS(tra, std::move(trs_in));
        if (!err)
        {
            result.AddWarning("", 0,
                "Failed to parse " + trs_name + ": " + err->FullMessage().GetCStr());
            continue;
        }

        auto tra_out = std::unique_ptr<AGS::Common::Stream>(
            AGS::Common::File::CreateFile(AGS::Common::String(tra_path.c_str())));
        if (!tra_out)
        {
            result.AddError("", 0, "Failed to create translation: " + tra_path);
            return false;
        }

        err = AGS::DataUtil::WriteTRA(tra, std::move(tra_out));
        if (!err)
        {
            result.AddError("", 0,
                "Failed to write " + tra_name + ": " + err->FullMessage().GetCStr());
            return false;
        }

        Log("[Build] Compiled translation: %s -> %s (%d entries)",
            trs_name.c_str(), tra_name.c_str(), (int)tra.Dict.size());
        result.output_files.push_back(tra_path);
    }

    return true;
}

// -------------------------------------------------------------------------
// Create audio.vox and speech.vox files
// -------------------------------------------------------------------------
bool BuildSystem::CreateVOXFiles(Project& project, const BuildConfig& config,
                                  BuildResult& result, const std::string& data_dir)
{
    progress_.Advance("Creating VOX files");
    Log("[Build] Creating VOX files...");

    GameData* gd = project.GetGameData();

    // --- audio.vox ---
    // Collect audio clips with bundling_type == 1 (InSeparateVOX)
    {
        std::vector<std::pair<std::string, std::string>> vox_entries; // {asset_name, source_path}
        if (gd)
        {
            for (const auto& clip : gd->audio_clips)
            {
                if (clip.bundling_type != 1)
                    continue;
                const std::string cache_rel = ResolveAudioCacheRelPath(clip);
                if (cache_rel.empty())
                    continue;
                const std::string clip_path = project.GetProjectDir() + "/" + cache_rel;
                if (PathExists(clip_path))
                    vox_entries.push_back({AssetBaseName(cache_rel), clip_path});
            }
        }

        std::string audio_vox = data_dir + "/audio.vox";
        if (!vox_entries.empty())
        {
            if (WriteVOXFile(audio_vox, vox_entries))
            {
                Log("[Build] Created audio.vox with %d clips.", (int)vox_entries.size());
                result.output_files.push_back(audio_vox);
            }
            else
            {
                result.AddWarning("", 0, "Failed to create audio.vox");
            }
        }
        else
        {
            // Remove stale audio.vox if no clips need separate VOX
            if (PathExists(audio_vox))
                std::remove(audio_vox.c_str());
        }
    }

    // --- speech.vox ---
    // Collect speech files from the Speech/ directory
    {
        std::string speech_dir = project.GetProjectDir() + "/Speech";
        std::vector<std::string> speech_extensions = {".mp3", ".ogg", ".wav", ".dat"};

        auto buildVox = [&](const std::string& src_dir, const std::string& vox_name) {
            std::vector<std::string> files;
            CollectFilesByExtension(src_dir, speech_extensions, files);

            // Also include syncdata.dat if present
            std::string sync_path = src_dir + "/syncdata.dat";
            bool sync_already = false;
            for (const auto& f : files)
            {
                if (f == sync_path) { sync_already = true; break; }
            }
            // syncdata.dat matches .dat, so it's already included — but keep the check
            (void)sync_already;

            if (files.empty()) return;

            std::vector<std::pair<std::string, std::string>> vox_entries;
            for (const auto& f : files)
            {
                // Asset name = path relative to speech_dir (just filename for top-level)
                std::string relative;
                if (f.size() > src_dir.size() + 1)
                    relative = f.substr(src_dir.size() + 1);
                else
                    relative = f;
                vox_entries.push_back({relative, f});
            }

            std::string vox_path = data_dir + "/" + vox_name;
            if (WriteVOXFile(vox_path, vox_entries))
            {
                Log("[Build] Created %s with %d files.", vox_name.c_str(), (int)vox_entries.size());
                result.output_files.push_back(vox_path);
            }
            else
            {
                result.AddWarning("", 0, "Failed to create " + vox_name);
            }
        };

        if (PathExists(speech_dir))
        {
            buildVox(speech_dir, "speech.vox");

            // Create sp_*.vox for each subdirectory (multilingual support)
            std::error_code ec;
            for (const auto& sub_entry : std::filesystem::directory_iterator(speech_dir, ec))
            {
                if (!sub_entry.is_directory()) continue;
                std::string subdir_name = sub_entry.path().filename().string();
                std::string subdir_path = speech_dir + "/" + subdir_name;
                // Lowercase the folder name for the VOX filename
                std::string lower_name = subdir_name;
                for (auto& c : lower_name) c = (char)tolower(c);
                buildVox(subdir_path, "sp_" + lower_name + ".vox");
            }
        }
        else
        {
            // Remove stale speech.vox if Speech/ directory doesn't exist
            std::string speech_vox = data_dir + "/speech.vox";
            if (PathExists(speech_vox))
                std::remove(speech_vox.c_str());
        }
    }

    return true;
}

// -------------------------------------------------------------------------
// Copy engine files for a specific platform
// -------------------------------------------------------------------------
bool BuildSystem::CopyEngineFiles(const BuildConfig& config, BuildTarget target,
                                   BuildResult& result, const std::string& target_dir)
{
    progress_.Advance("Copying engine files for " + std::string(BuildTargetName(target)));
    Log("[Build] Copying engine files for target: %s", BuildTargetName(target));

    // Copy data files to target directory
    std::string data_dir = GetDataDir(config);

    // Copy game.ags
    std::string pak_src = data_dir + "/game.ags";
    std::string pak_dst = target_dir + "/game.ags";
    if (PathExists(pak_src))
    {
        if (!CopyPath(pak_src, pak_dst))
        {
            result.AddWarning("", 0, "Failed to copy game.ags to " + target_dir);
        }
    }

    // Copy VOX files (audio.vox, speech.vox, sp_*.vox)
    auto copyVoxFiles = [&](const std::string& dst_dir) {
        // Copy specific named VOX files
        for (const auto& vox_name : {"audio.vox", "speech.vox"})
        {
            std::string vox_src = data_dir + "/" + vox_name;
            if (PathExists(vox_src))
            {
                CopyPath(vox_src, dst_dir + "/" + vox_name);
                Log("[Build]   Copied %s", vox_name);
            }
        }
        // Copy sp_*.vox multilingual speech files
        {
            std::error_code ec;
            for (const auto& entry : std::filesystem::directory_iterator(data_dir, ec))
            {
                if (!entry.is_regular_file()) continue;
                std::string fn = entry.path().filename().string();
                if (fn.size() > 7 && fn.substr(0, 3) == "sp_" &&
                    fn.substr(fn.size() - 4) == ".vox")
                {
                    CopyPath(data_dir + "/" + fn, dst_dir + "/" + fn);
                    Log("[Build]   Copied %s", fn.c_str());
                }
            }
        }
    };

    // Copy VOX files to the same directory as game.ags
    copyVoxFiles(target_dir);

    switch (target)
    {
    case BuildTarget::Linux:
    {
        // Copy the AGS engine binary
        std::string engine = FindEngineExecutable(config);
        if (!engine.empty())
        {
            std::string dst = target_dir + "/ags";
            CopyPath(engine, dst);
            // Make executable
#ifndef _WIN32
            std::filesystem::permissions(dst, std::filesystem::perms::owner_all | std::filesystem::perms::group_read | std::filesystem::perms::group_exec | std::filesystem::perms::others_read | std::filesystem::perms::others_exec);
#endif
            Log("[Build]   Copied engine: %s", engine.c_str());
        }
        else
        {
            result.AddWarning("", 0, "Linux engine binary not found. Game data packaged but no engine copied.");
        }

        // Create launcher script
        std::string launcher = target_dir + "/run.sh";
        std::ofstream sh(launcher);
        if (sh.is_open())
        {
            sh << "#!/bin/bash\n";
            sh << "# AGS Game Launcher\n";
            sh << "DIR=\"$(cd \"$(dirname \"$0\")\" && pwd)\"\n";
            sh << "cd \"$DIR\"\n";
            sh << "exec ./ags game.ags \"$@\"\n";
            sh.close();
#ifndef _WIN32
            std::filesystem::permissions(launcher, std::filesystem::perms::owner_all | std::filesystem::perms::group_read | std::filesystem::perms::group_exec | std::filesystem::perms::others_read | std::filesystem::perms::others_exec);
#endif
        }
        break;
    }

    case BuildTarget::Windows:
    {
        if (!config.engine_windows_path.empty() && PathExists(config.engine_windows_path))
        {
            std::string dst = target_dir + "/game.exe";
            CopyPath(config.engine_windows_path, dst);
            Log("[Build]   Copied Windows engine: %s", config.engine_windows_path.c_str());
        }
        else
        {
            result.AddWarning("", 0, "Windows engine (acwin.exe) not found. Set path in Build Settings.");
        }
        break;
    }

    case BuildTarget::MacOS:
    {
        // Create a macOS .app bundle structure
        // GameName.app/
        //   Contents/
        //     Info.plist
        //     MacOS/
        //       ags          (engine binary)
        //     Resources/
        //       game.ags     (game data)
        //       acsetup.cfg  (config)
        std::string game_name = "AGSGame";
        if (!build_game_title_.empty())
        {
            game_name = build_game_title_;
            // Sanitize for filesystem
            for (auto& ch : game_name)
                if (ch == '/' || ch == '\\' || ch == ':') ch = '_';
        }

        std::string bundle_dir = target_dir + "/" + game_name + ".app";
        std::string contents_dir = bundle_dir + "/Contents";
        std::string macos_dir = contents_dir + "/MacOS";
        std::string resources_dir = contents_dir + "/Resources";

        EnsureDirectory(macos_dir);
        EnsureDirectory(resources_dir);

        // Write Info.plist
        {
            std::string plist_path = contents_dir + "/Info.plist";
            std::ofstream plist(plist_path);
            if (plist.is_open())
            {
                plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
                plist << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
                         "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
                plist << "<plist version=\"1.0\">\n";
                plist << "<dict>\n";
                plist << "  <key>CFBundleName</key>\n";
                plist << "  <string>" << game_name << "</string>\n";
                plist << "  <key>CFBundleIdentifier</key>\n";
                plist << "  <string>com.ags.game." << game_name << "</string>\n";
                plist << "  <key>CFBundleVersion</key>\n";
                plist << "  <string>1.0</string>\n";
                plist << "  <key>CFBundleExecutable</key>\n";
                plist << "  <string>ags</string>\n";
                plist << "  <key>CFBundlePackageType</key>\n";
                plist << "  <string>APPL</string>\n";
                plist << "  <key>NSHighResolutionCapable</key>\n";
                plist << "  <true/>\n";
                plist << "</dict>\n";
                plist << "</plist>\n";
                plist.close();
                Log("[Build]   Created Info.plist");
            }
        }

        // Copy macOS engine binary if available
        if (!config.engine_macos_path.empty() && PathExists(config.engine_macos_path))
        {
            std::string dst = macos_dir + "/ags";
            CopyPath(config.engine_macos_path, dst);
#ifndef _WIN32
            std::filesystem::permissions(dst, std::filesystem::perms::owner_all | std::filesystem::perms::group_read | std::filesystem::perms::group_exec | std::filesystem::perms::others_read | std::filesystem::perms::others_exec);
#endif
            Log("[Build]   Copied macOS engine: %s", config.engine_macos_path.c_str());
        }
        else
        {
            result.AddWarning("", 0,
                "macOS engine binary not found. Set engine_macos_path in Build Settings. "
                "Bundle structure created but no engine copied.");
        }

        // Copy game data into Resources
        std::string pak_src2 = data_dir + "/game.ags";
        if (PathExists(pak_src2))
        {
            CopyPath(pak_src2, resources_dir + "/game.ags");
            Log("[Build]   Copied game.ags to Resources/");
        }
        // Copy VOX files into Resources
        copyVoxFiles(resources_dir);

        Log("[Build]   macOS bundle created: %s", bundle_dir.c_str());
        break;
    }

    case BuildTarget::Web:
    {
        // Create a web deployment directory with:
        //   index.html         (HTML shell page)
        //   game.ags           (game data)
        //   ags.js / ags.wasm  (engine files, if available)
        Log("[Build]   Preparing Web (Emscripten) build...");

        // Copy game data
        std::string pak_src3 = data_dir + "/game.ags";
        if (PathExists(pak_src3))
        {
            CopyPath(pak_src3, target_dir + "/game.ags");
            Log("[Build]   Copied game.ags to web output");
        }
        // Copy VOX files for web target
        copyVoxFiles(target_dir);

        // Copy Emscripten engine files if configured
        if (!config.engine_web_path.empty())
        {
            // engine_web_path should point to a directory containing ags.js and ags.wasm
            std::string js_path = config.engine_web_path + "/ags.js";
            std::string wasm_path = config.engine_web_path + "/ags.wasm";
            std::string data_path = config.engine_web_path + "/ags.data";

            if (PathExists(js_path))
            {
                CopyPath(js_path, target_dir + "/ags.js");
                Log("[Build]   Copied ags.js");
            }
            if (PathExists(wasm_path))
            {
                CopyPath(wasm_path, target_dir + "/ags.wasm");
                Log("[Build]   Copied ags.wasm");
            }
            if (PathExists(data_path))
            {
                CopyPath(data_path, target_dir + "/ags.data");
                Log("[Build]   Copied ags.data");
            }
        }
        else
        {
            result.AddWarning("", 0,
                "Emscripten engine path not set. Set engine_web_path in Build Settings "
                "to a directory containing ags.js and ags.wasm.");
        }

        // Generate index.html shell
        {
            std::string game_name = build_game_title_;
            std::string html_path = target_dir + "/index.html";
            std::ofstream html(html_path);
            if (html.is_open())
            {
                html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
                html << "  <meta charset=\"UTF-8\">\n";
                html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
                html << "  <title>" << game_name << "</title>\n";
                html << "  <style>\n";
                html << "    body { margin: 0; background: #000; display: flex; ";
                html << "justify-content: center; align-items: center; min-height: 100vh; }\n";
                html << "    canvas { border: 1px solid #333; }\n";
                html << "    #status { color: #fff; font-family: monospace; text-align: center; ";
                html << "padding: 20px; }\n";
                html << "  </style>\n</head>\n<body>\n";
                html << "  <div id=\"status\">Loading " << game_name << "...</div>\n";
                html << "  <canvas id=\"canvas\" oncontextmenu=\"event.preventDefault()\" ";
                html << "tabindex=\"-1\"></canvas>\n";
                html << "  <script>\n";
                html << "    var Module = {\n";
                html << "      canvas: document.getElementById('canvas'),\n";
                html << "      arguments: ['game.ags'],\n";
                html << "      setStatus: function(text) {\n";
                html << "        document.getElementById('status').innerText = text;\n";
                html << "        if (!text) document.getElementById('status').style.display = 'none';\n";
                html << "      },\n";
                html << "      onRuntimeInitialized: function() {\n";
                html << "        document.getElementById('status').style.display = 'none';\n";
                html << "      }\n";
                html << "    };\n";
                html << "  </script>\n";
                html << "  <script src=\"ags.js\"></script>\n";
                html << "</body>\n</html>\n";
                html.close();
                Log("[Build]   Generated index.html");
            }
        }

        Log("[Build]   Web build output: %s", target_dir.c_str());
        break;
    }

    default:
        break;
    }

    return true;
}

// -------------------------------------------------------------------------
// Generate acsetup.cfg configuration file
// -------------------------------------------------------------------------
bool BuildSystem::GenerateConfigFile(Project& project, const BuildConfig& config,
                                      const std::string& target_dir)
{
    GameData* gd = project.GetGameData();
    if (!gd) return false;

    std::string cfg_path = target_dir + "/acsetup.cfg";
    std::ofstream cfg(cfg_path);
    if (!cfg.is_open()) return false;

    cfg << "[misc]\n";
    cfg << "gamecolordepth=" << gd->color_depth << "\n";
    cfg << "defaultres=" << gd->resolution_width << "x" << gd->resolution_height << "\n";
    cfg << "titletext=" << gd->game_title << "\n";
    cfg << "\n";

    cfg << "[graphics]\n";
    cfg << "driver=auto\n";
    cfg << "windowed=1\n";
    cfg << "screen_def=scaling\n";
    cfg << "game_scale_fs=proportional\n";
    cfg << "game_scale_win=round\n";
    cfg << "filter=stdscale\n";
    cfg << "refresh=0\n";
    cfg << "render_at_screenres=0\n";
    cfg << "supersampling=1\n";
    cfg << "vsync=0\n";
    cfg << "\n";

    cfg << "[sound]\n";
    cfg << "enabled=1\n";
    cfg << "digiid=auto\n";
    cfg << "midiid=auto\n";
    cfg << "threaded=1\n";
    cfg << "\n";

    if (config.debug_mode)
    {
        cfg << "[debug]\n";
        cfg << "enabled=1\n";
        cfg << "debugger=1\n";
        cfg << "\n";
    }

    cfg.close();
    return true;
}

// =========================================================================
// Main build pipeline
// =========================================================================
BuildResult BuildSystem::BuildGame(Project& project, const BuildConfig& config)
{
    BuildResult result;
    building_ = true;
    config_ = config;

    auto start_time = std::chrono::steady_clock::now();

    // Calculate total steps
    int total_steps = 3; // pre-check + compile + data file
    total_steps += 1;    // packaging
    total_steps += 1;    // VOX files
    total_steps += 1;    // translations
    if (!config.data_only)
    {
        for (int i = 0; i < (int)BuildTarget::Count; i++)
            if (config.targets[i])
                total_steps++;
    }
    total_steps += 1;    // config file

    progress_.Reset(total_steps);

    Log("[Build] ========================================");
    Log("[Build] Starting game build...");
    Log("[Build] ========================================");

    // Cache game title for platform-specific builds
    {
        GameData* gd = project.GetGameData();
        build_game_title_ = (gd && !gd->game_title.empty()) ? gd->game_title : "AGSGame";
    }

    // Step 1: Create directories
    progress_.Advance("Creating build directories");
    if (!config.output_base_dir.empty())
        config_.output_base_dir = config.output_base_dir;
    else
        config_.SetDefaults(project.GetProjectDir());

    CreateBuildDirectories(config_, result);

    // Step 2: Pre-build checks
    progress_.Advance("Validating project");
    Log("[Build] Validating project...");
    if (!PreBuildChecks(project, result))
    {
        Log("[Error] Pre-build validation failed.");
        result.success = false;
        building_ = false;
        auto end_time = std::chrono::steady_clock::now();
        result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
        last_result_ = result;
        return result;
    }
    Log("[Build] Project validated OK.");

    // Step 3: Compile all scripts
    Log("[Build] Compiling scripts...");
    if (!CompileAllScripts(project, config_, result))
    {
        Log("[Error] Script compilation failed with %d error(s).", result.ErrorCount());
        result.success = false;
        building_ = false;
        auto end_time = std::chrono::steady_clock::now();
        result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
        last_result_ = result;
        return result;
    }

    // Step 4: Write game data file
    std::string dta_path =
        GetCompiledDir(config_) + "/_temp/" + kCompiledDtaFileName;
    if (!WriteGameDataFile(project, config_, result, dta_path))
    {
        Log("[Error] Failed to write game data file.");
        result.success = false;
        building_ = false;
        auto end_time = std::chrono::steady_clock::now();
        result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
        last_result_ = result;
        return result;
    }

    // Step 5: Package assets
    std::string data_dir = GetDataDir(config_);
    if (!PackageAssets(project, config_, result, data_dir))
    {
        Log("[Error] Asset packaging failed.");
        result.success = false;
        building_ = false;
        auto end_time = std::chrono::steady_clock::now();
        result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
        last_result_ = result;
        return result;
    }

    // Step 5b: Create VOX files (audio.vox, speech.vox)
    CreateVOXFiles(project, config_, result, data_dir);

    // Step 5c: Compile registered translations to Compiled/Data/*.tra
    if (!CompileRegisteredTranslations(project, config_, result, data_dir))
    {
        Log("[Error] Translation compilation failed.");
        result.success = false;
        building_ = false;
        auto end_time = std::chrono::steady_clock::now();
        result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
        last_result_ = result;
        return result;
    }

    // Step 6: Copy engine files per target (skipped in data-only / agsbuild mode)
    if (!config_.data_only)
    {
        for (int i = 0; i < (int)BuildTarget::Count; i++)
        {
            if (!config_.targets[i])
                continue;
            BuildTarget target = (BuildTarget)i;
            if (target == BuildTarget::DataFile)
                continue;

            std::string target_dir = GetTargetDir(config_, target);
            CopyEngineFiles(config_, target, result, target_dir);
            GenerateConfigFile(project, config_, target_dir);
        }
    }

    // Generate config in data dir
    GenerateConfigFile(project, config_, data_dir);

    // Done!
    auto end_time = std::chrono::steady_clock::now();
    result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
    result.success = (result.ErrorCount() == 0);
    building_ = false;
    last_result_ = result;

    if (result.success)
    {
        Log("[Build] ========================================");
        Log("[Build] Build succeeded in %.2f seconds.", result.elapsed_seconds);
        Log("[Build]   %d warning(s)", result.WarningCount());
        Log("[Build]   Output: %s", data_dir.c_str());
        Log("[Build] ========================================");
    }
    else
    {
        Log("[Build] ========================================");
        Log("[Error] Build FAILED: %d error(s), %d warning(s) (%.2f s)",
            result.ErrorCount(), result.WarningCount(), result.elapsed_seconds);
        Log("[Build] ========================================");
    }

    return result;
}

} // namespace AGSBuild
