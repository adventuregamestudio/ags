//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <cstring>
#include <memory>

#include "data/mfl_utils.h"
#include "data/multifilelib.h"
#include "gtest/gtest.h"
#include "util/file.h"
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

namespace
{

void WriteTextFile(const String &path, const char *text)
{
    std::unique_ptr<Stream> out(File::CreateFile(path));
    out->Write(text, strlen(text));
}

// Packs two small assets into lib_file, appending the library if requested
HError PackTwoAssets(const String &lib_file, bool append)
{
    WriteTextFile("mfl_test_a.txt", "first");
    WriteTextFile("mfl_test_b.txt", "second");
    StringIMap asset_map;
    asset_map["a.txt"] = "mfl_test_a.txt";
    asset_map["b.txt"] = "mfl_test_b.txt";
    AssetLibInfo lib;
    HError err = MakeAssetLib(lib, lib_file, asset_map, 0, append);
    if (err)
        err = WriteLibrary(lib, asset_map, ".", MFLUtil::kMFLVersion_MultiV30, append, false);
    File::DeleteFile("mfl_test_a.txt");
    File::DeleteFile("mfl_test_b.txt");
    return err;
}

bool FileHasEnder(const String &path)
{
    std::unique_ptr<Stream> in(File::OpenFileRead(path));
    return MFLUtil::HasEnderSig(in.get());
}

} // namespace

TEST(MflUtils, StandaloneLibraryHasNoEnder)
{
    // A file of its own begins with the library, so it needs no ender, as the editor writes it
    const String lib_file = "mfl_test_standalone.vox";
    ASSERT_TRUE(PackTwoAssets(lib_file, false));
    EXPECT_FALSE(FileHasEnder(lib_file));
    EXPECT_TRUE(TestLibraryFile(lib_file, nullptr));
    File::DeleteFile(lib_file);
}

TEST(MflUtils, AppendedLibraryHasEnder)
{
    // A library appended to another file is found from the ender at the file's end
    const String lib_file = "mfl_test_appended.exe";
    WriteTextFile(lib_file, "an executable");
    ASSERT_TRUE(PackTwoAssets(lib_file, true));
    EXPECT_TRUE(FileHasEnder(lib_file));
    std::unique_ptr<Stream> in(File::OpenFileRead(lib_file));
    soff_t lib_offset = 0;
    ASSERT_EQ(MFLUtil::kMFLNoError, MFLUtil::ReadOffset(in.get(), lib_offset));
    EXPECT_EQ(static_cast<soff_t>(strlen("an executable")), lib_offset);
    in.reset();
    File::DeleteFile(lib_file);
}
