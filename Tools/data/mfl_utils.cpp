//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#include "data/mfl_utils.h"
#include <memory>
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stream.h"

using namespace AGS::Common;

namespace AGS
{
namespace DataUtil
{

// TODO: might replace "printf" with the logging functions,
// but then we'd also need to make sure they are initialized in tools

HError UnpackLibrary(const AssetLibInfo &lib, const String &lib_dir,
    const String &lib_basefile, const String &dst_dir)
{
    for (size_t i = 0; i < lib.LibFileNames.size(); ++i)
    {
        String lib_f = lib.LibFileNames[i];
        String path = Path::ConcatPaths(lib_dir, lib_f);
        std::unique_ptr<Stream> lib_in(File::OpenFileRead(path));
        if (!lib_in && i == 0)
        {
            lib_f = lib_basefile;
            path = Path::ConcatPaths(lib_dir, lib_f);
            lib_in.reset(File::OpenFileRead(path));
        }
        if (!lib_in)
        {
            return new Error(String::FromFormat("Failed to open a library file for reading: %s",
                lib_f.GetCStr()));
        }
        printf("Extracting %s:\n", lib_f.GetCStr());
        for (const auto &asset : lib.AssetInfos)
        {
            if (asset.LibUid != i) continue;
            String dst_f = Path::ConcatPaths(dst_dir, asset.FileName);
            String sub_dir = Path::GetParent(asset.FileName);
            if (!sub_dir.IsEmpty() && sub_dir != "." &&
                !Directory::CreateAllDirectories(dst_dir, sub_dir))
            {
                printf("Error: unable to create a subdirectory: %s\n", sub_dir.GetCStr());
                continue;
            }
            std::unique_ptr<Stream> out(File::CreateFile(dst_f));
            if (!out)
            {
                printf("Error: unable to open a file for writing: %s\n", asset.FileName.GetCStr());
                continue;
            }
            lib_in->Seek(asset.Offset, kSeekBegin);
            soff_t wrote = CopyStream(lib_in.get(), out.get(), asset.Size);
            if (wrote == asset.Size)
                printf("+ %s\n", asset.FileName.GetCStr());
            else
                printf("Error: file was not written correctly: %s\n Expected: %lld, wrote: %lld bytes\n",
                    asset.FileName.GetCStr(), asset.Size, wrote);
        }
    }
    return HError::None();
}

} // namespace DataUtil
} // namespace AGS
