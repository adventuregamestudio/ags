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
#include "platform/platform.h"
#if (AGS_PLATFORM_OS_WINDOWS)
#include "platform/windows/windows.h"
#else
#include <unistd.h>
#endif
#include "util/error.h"
#include "util/string.h"
#include "util/textstreamwriter.h"

namespace AGSBuild
{

using String = AGS::Common::String;
using HError = AGS::Common::HError;

class PipedProcess
{
public:
    PipedProcess(const String &exe_path, const String &params, AGS::Common::TextStreamWriter *log_writer = nullptr);

    HError RunSync(std::vector<uint8_t> *stdout_data = nullptr);

    int    GetExitCode() const;

private:
    HError CreateProcess();
    HError RunProcessSync(std::vector<uint8_t> *stdout_data = nullptr);
    void   WaitForProcessSync();
    void   CloseProcess();

    bool   IsPipeCreated() const;

#if (AGS_PLATFORM_OS_WINDOWS)
    void   ReadFromPipe(HANDLE pipe, std::vector<uint8_t> *output_data, bool print_to_buffer);
#else
    void   ReadFromPipe(int pipe_fd, std::vector<uint8_t> *output_data, bool print_to_buffer);
#endif

    String _exePath;
    String _params;
    String _command;
    AGS::Common::TextStreamWriter *_logWriter = nullptr;
    int    _exitCode = -1;
#if (AGS_PLATFORM_OS_WINDOWS)
    HANDLE _hStdOutPipeRead = INVALID_HANDLE_VALUE;
    HANDLE _hStdOutPipeWrite = INVALID_HANDLE_VALUE;
    HANDLE _hStdErrPipeRead = INVALID_HANDLE_VALUE;
    HANDLE _hStdErrPipeWrite = INVALID_HANDLE_VALUE;
    PROCESS_INFORMATION _pi;
#else
    typedef int PipeFD[2];
    PipeFD _stdOutPipe = {0, 0};
    PipeFD _stdErrPipe = {0, 0};
    pid_t  _cpid = -1;
#endif
};

} // namespace AGSBuild
