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
// Tests of the agsbuild task manager: a failing task, and cancelling a build
// (docs/refactor/PLAN.md, 7.1b and 7.1h).
#include <chrono>
#include <future>
#include <thread>
#include "gtest/gtest.h"
#include "agsbuild/diagnostics.h"
#include "agsbuild/pipedprocess.h"
#include "agsbuild/taskmanager.h"

using namespace AGS::Common;
using namespace AGSBuild;

namespace
{

// A command that runs for a minute
PipedProcessParams SleepCommand()
{
#if AGS_PLATFORM_OS_WINDOWS
    return PipedProcessParams("ping", "-n 60 127.0.0.1");
#else
    return PipedProcessParams("sleep", "60");
#endif
}

// A command that prints an error in the form of the tools, and fails
PipedProcessParams FailingCommand()
{
#if AGS_PLATFORM_OS_WINDOWS
    return PipedProcessParams("cmd", "/c echo Error: compile failed at a.asc, line 3 : broken& exit 1");
#else
    return PipedProcessParams("sh", "-c 'echo \"Error: compile failed at a.asc, line 3 : broken\"; exit 1'");
#endif
}

PipedProcessParams QuickCommand()
{
#if AGS_PLATFORM_OS_WINDOWS
    return PipedProcessParams("cmd", "/c echo done");
#else
    return PipedProcessParams("sh", "-c 'echo done'");
#endif
}

template <class T>
T WaitOrAbort(std::future<T> &f, int seconds, const char *what)
{
    if (f.wait_for(std::chrono::seconds(seconds)) != std::future_status::ready)
    {
        // A blocked thread can't be stopped, so end the test run instead of hanging it
        std::fprintf(stderr, "%s: did not finish in time\n", what);
        std::abort();
    }
    return f.get();
}

} // namespace

// The tasks which depend on a failed one are cancelled, the others finish, and RunAll returns
TEST(TaskManager, FailedTaskCancelsDependentsOnly)
{
    const int errors_before = Diag::ErrorCount();
    TaskManager mgr(2);
    mgr.AddTask(std::make_unique<TaskPipedProcess>("fail", &mgr, FailingCommand()));
    mgr.AddTask(
        std::make_unique<TaskPipedProcess>("after-fail", &mgr, SleepCommand(), std::vector<TaskInput>{{"fail"}}));
    mgr.AddTask(std::make_unique<TaskPipedProcess>("independent", &mgr, QuickCommand()));

    auto run = std::async(std::launch::async, [&mgr]() { return mgr.RunAll(); });
    HError err = WaitOrAbort(run, 60, "FailedTaskCancelsDependentsOnly");
    EXPECT_FALSE(err);
    // The error the tool printed was read from its output
    EXPECT_EQ(errors_before + 1, Diag::ErrorCount());
}

// The running tools are killed and the waiting tasks do not start: RunAll returns long before
// the minute that the tools would run
TEST(TaskManager, CancelStopsRunningAndWaitingTasks)
{
    Diag::ResetCancel();
    TaskManager mgr(2);
    mgr.AddTask(std::make_unique<TaskPipedProcess>("first", &mgr, SleepCommand()));
    mgr.AddTask(std::make_unique<TaskPipedProcess>("second", &mgr, SleepCommand()));
    mgr.AddTask(std::make_unique<TaskPipedProcess>("waiting", &mgr, SleepCommand(),
                                                   std::vector<TaskInput>{{"first"}, {"second"}}));

    const auto start = std::chrono::steady_clock::now();
    auto run = std::async(std::launch::async, [&mgr]() { return mgr.RunAll(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(700)); // let the processes start
    Diag::RequestCancel();
    HError err = WaitOrAbort(run, 40, "CancelStopsRunningAndWaitingTasks");
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start);
    Diag::ResetCancel();

    EXPECT_FALSE(err);
    EXPECT_LT(seconds.count(), 30);
}

// A build cancelled before any task has started runs nothing
TEST(TaskManager, CancelBeforeStart)
{
    Diag::ResetCancel();
    Diag::RequestCancel();
    TaskManager mgr(2);
    mgr.AddTask(std::make_unique<TaskPipedProcess>("only", &mgr, SleepCommand()));
    auto run = std::async(std::launch::async, [&mgr]() { return mgr.RunAll(); });
    HError err = WaitOrAbort(run, 40, "CancelBeforeStart");
    Diag::ResetCancel();
    EXPECT_FALSE(err);
}
