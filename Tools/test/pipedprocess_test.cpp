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
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <memory>
#include <vector>
#include "gtest/gtest.h"
#include "agsbuild/pipedprocess.h"
#include "util/memorystream.h"

using namespace AGS::Common;
using namespace AGSBuild;

namespace
{

// A command that prints `lines` lines of 40+ characters each, so the output is much larger
// than the pipe buffer (1 KB)
void GetChattyCommand(int lines, String &exe, String &params)
{
#if AGS_PLATFORM_OS_WINDOWS
    exe = "cmd";
    params = String::FromFormat("/c for /L %%i in (1,1,%d) do @echo 0123456789abcdef0123456789abcdef line %%i", lines);
#else
    exe = "sh";
    params = String::FromFormat("-c 'i=0; while [ $i -lt %d ]; do echo 0123456789abcdef0123456789abcdef line $i; "
                                "i=$((i+1)); done'",
                                lines);
#endif
}

size_t RunChatty(int lines)
{
    String exe, params;
    GetChattyCommand(lines, exe, params);
    std::vector<uint8_t> log_buf;
    TextStreamWriter log(std::make_unique<Stream>(std::make_unique<VectorStream>(log_buf, kStream_Write)));
    std::vector<uint8_t> out;
    PipedProcess proc(exe, params, &log);
    HError err = proc.RunSync(&out);
    return err ? out.size() : 0;
}

} // namespace

// Several processes with large output, started from parallel threads as agsbuild does. This used
// to hang on Windows: a read asked for more bytes than the pipe had, and blocked forever because
// agsbuild still held the pipe's write end.
TEST(PipedProcess, ParallelLargeOutputCompletes)
{
    const int line_count = 2000, process_count = 6;
    std::vector<std::future<size_t>> runs;
    for (int i = 0; i < process_count; ++i)
        runs.push_back(std::async(std::launch::async, RunChatty, line_count));

    for (auto &run : runs)
    {
        if (run.wait_for(std::chrono::seconds(120)) != std::future_status::ready)
        {
            // A blocked thread can't be cancelled, so stop here instead of hanging the test run
            std::fprintf(stderr, "PipedProcess.ParallelLargeOutputCompletes: a process run did not finish\n");
            std::abort();
        }
        EXPECT_GE(run.get(), static_cast<size_t>(line_count) * 40u) << "process output was cut short";
    }
}
