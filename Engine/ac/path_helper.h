//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Functions related to constructing game and script paths.
//
// TODO: We need some kind of a "file manager" which deals with opening files
// in defined set of directories. To ensure that rest of the engine code does
// not work with explicit paths or creates directories on its own.
//
//=============================================================================
#ifndef __AGS_EE_AC__PATHHELPER_H
#define __AGS_EE_AC__PATHHELPER_H

#include "util/path.h"

using AGS::Common::String;

// Filepath tokens, which are replaced by platform-specific directory names
extern const String UserSavedgamesRootToken;
extern const String GameSavedgamesDirToken;
extern const String GameDataDirToken;
extern const String DefaultConfigFileName;

// Subsitutes illegal characters with '_'. This function uses illegal chars array
// specific to current platform.
void FixupFilename(char *filename);

// FSLocation describes a file system location defined by two parts:
// a secure path that engine does not own, and sub-path that it owns.
// The meaning of this is that engine is only allowed to create
// sub-path subdirectories, and only if secure path exists.
struct FSLocation
{
    String BaseDir; // base directory, which we assume already exists; not our responsibility
    String SubDir;  // sub-directory, relative to BaseDir
    String FullDir; // full path to location
    FSLocation() = default;
    FSLocation(const String &base) : BaseDir(base), FullDir(base) {}
    FSLocation(const String &base, const String &subdir)
        : BaseDir(base), SubDir(subdir), FullDir(AGS::Common::Path::ConcatPaths(base, subdir)) {}
    inline bool IsValid() const { return !FullDir.IsEmpty(); }
    // Concats the given path to the existing full dir
    inline FSLocation Concat(const String &path) const
        { return FSLocation(BaseDir, AGS::Common::Path::ConcatPaths(SubDir, path)); }
};
// Tests the input path, if it's an absolute path then returns it unchanged;
// if it's a relative path then resolves it into absolute, using install dir as a base.
String PathFromInstallDir(const String &path);
FSLocation PathFromInstallDir(const FSLocation &fsloc);
// Makes sure that given system location is available, makes directories if have to (and if it's allowed to)
// Returns full file path on success, empty string on failure.
String PreparePathForWriting(const FSLocation& fsloc, const String &filename);

// Following functions calculate paths to directories according to game setup
// Returns the directory where global user config is to be found
FSLocation GetGlobalUserConfigDir();
// Returns the directory where this game's user config is to be found
FSLocation GetGameUserConfigDir();
// Returns the directory where this game's shared app files are to be found
FSLocation GetGameAppDataDir();
// Returns the directory where this game's saves and user data are to be found
FSLocation GetGameUserDataDir();

// ResolvedPath describes an actual location pointed by a user path (e.g. from script)
struct ResolvedPath
{
    FSLocation Loc;  // location (directory)
    String FullPath; // full path, including filename
    String AltPath;  // alternative read-only full path, for backwards compatibility
    bool   AssetMgr = false; // file is to be accessed through the asset manager
    ResolvedPath() = default;
    ResolvedPath(const String &file, const String &alt = "")
        : FullPath(file), AltPath(alt) {}
    ResolvedPath(const FSLocation &loc, const String &file, const String &alt = "")
        : Loc(loc), FullPath(AGS::Common::Path::ConcatPaths(loc.FullDir, file)), AltPath(alt) {}
};
// Resolves a file path provided by user (e.g. script) into actual file path,
// by substituting special keywords with actual platform-specific directory names.
// Fills in ResolvedPath object on success.
// Returns 'true' on success, and 'false' if either path is impossible to resolve
// or if the file path is forbidden to be accessed in current situation.
bool ResolveScriptPath(const String &sc_path, bool read_only, ResolvedPath &rp);
// Resolves a user file path for writing, and makes sure all the sub-directories are
// created along the actual path.
// Returns 'true' on success, and 'false' if either path is impossible to resolve,
// forbidden for writing, or if failed to create any subdirectories.
bool ResolveWritePathAndCreateDirs(const String &sc_path, ResolvedPath &rp);
// Creates all necessary subdirectories inside the safe parent location.
bool CreateFSDirs(const FSLocation &fs);

#endif // __AGS_EE_AC__PATHHELPER_H
