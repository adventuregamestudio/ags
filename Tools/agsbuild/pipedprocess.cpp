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
#include "pipedprocess.h"
#include "util/memorystream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

namespace AGSBuild
{

PipedProcess::PipedProcess(const String &exe_path, const String &params, TextStreamWriter *log_writer)
    : _exePath(exe_path)
    , _params(params)
    , _logWriter(log_writer)
{
    _command = String::FromFormat("%s.exe %s", exe_path.GetCStr(), params.GetCStr());

#if (AGS_PLATFORM_OS_WINDOWS)
    memset(&_pi, 0, sizeof(_pi));
#else
#endif
}

HError PipedProcess::RunSync(std::vector<uint8_t> *stdout_data)
{
    HError err = CreateProcess();
    if (!err)
        return err;

    if (IsPipeCreated())
        err = RunProcessSync(stdout_data);
    else
        WaitForProcessSync();

    CloseProcess();
    return err;
}

HError PipedProcess::CreateProcess()
{
#if (AGS_PLATFORM_OS_WINDOWS)
    STARTUPINFOW si = { sizeof(si), NULL, NULL, NULL, NULL, CREATE_NO_WINDOW, NULL };

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
    sa.bInheritHandle = TRUE; 
    sa.lpSecurityDescriptor = NULL; 
    const bool stdout_pipe = CreatePipe(&_hStdOutPipeRead, &_hStdOutPipeWrite, &sa, 1024u) == TRUE;
    const bool stderr_pipe = CreatePipe(&_hStdErrPipeRead, &_hStdErrPipeWrite, &sa, 1024u) == TRUE;
    // NOTE: we only check the pipe result to know if we should dispose of it.
    // if pipe failed, we still can run the process, although we won't see it's output
    if (stdout_pipe || stderr_pipe)
    {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = INVALID_HANDLE_VALUE;
        si.hStdOutput = _hStdOutPipeWrite;
        si.hStdError = _hStdErrPipeWrite;
    }
    
    std::vector<wchar_t> cmd(_command.GetLength() + 1);
    StrUtil::ConvertUtf8ToWstr(_command.GetCStr(), cmd.data(), cmd.size());
    if (CreateProcessW(NULL, cmd.data(), NULL, NULL, TRUE /* bInheritHandles */, CREATE_NO_WINDOW, NULL, NULL, &si, &_pi) != TRUE)
    {
        return new Error(String::FromFormat("Process failed with err code: %u\n", GetLastError()));
    }
#else
    // TODO
#endif
    return HError::None();
}

void PipedProcess::ReadFromPipe(HANDLE pipe, std::vector<uint8_t> *output_data, bool print_to_buffer)
{
    uint8_t buffer[1024];
    DWORD pipe_has_bytes = 0;
    if ((PeekNamedPipe(pipe, NULL, 0, NULL, &pipe_has_bytes, NULL) == TRUE) && (pipe_has_bytes > 0))
    {
        DWORD bytes_read = 0;
        while (pipe_has_bytes > 0
            && ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytes_read, NULL) == TRUE && bytes_read > 0 && bytes_read != MAXDWORD)
        {
            if (output_data)
            {
                std::copy(buffer, buffer + bytes_read, std::back_inserter(*output_data));
            }
            if (print_to_buffer)
            {
                buffer[bytes_read] = 0;
                _logWriter->WriteString(reinterpret_cast<const char*>(buffer));
            }
            pipe_has_bytes -= bytes_read;
        }
    }
}

HError PipedProcess::RunProcessSync(std::vector<uint8_t> *stdout_data)
{
#if (AGS_PLATFORM_OS_WINDOWS)
    DWORD proc_result = 0;

    do
    {
        proc_result = WaitForSingleObject(_pi.hProcess, 1);
        if (_hStdOutPipeRead)
            ReadFromPipe(_hStdOutPipeRead, stdout_data, true);
        if (_hStdErrPipeRead)
            ReadFromPipe(_hStdErrPipeRead, nullptr, true);
    }
    while (proc_result == WAIT_TIMEOUT);
#else
    // TODO
#endif
    return HError::None();
}

void PipedProcess::WaitForProcessSync()
{
#if (AGS_PLATFORM_OS_WINDOWS)
    WaitForSingleObject(_pi.hProcess, INFINITE);
#else
    // TODO
#endif
}

void PipedProcess::CloseProcess()
{
#if (AGS_PLATFORM_OS_WINDOWS)
    if (_hStdOutPipeRead != INVALID_HANDLE_VALUE)
        CloseHandle(_hStdOutPipeRead);
    if (_hStdOutPipeWrite != INVALID_HANDLE_VALUE)
        CloseHandle(_hStdOutPipeWrite);
    if (_hStdErrPipeRead != INVALID_HANDLE_VALUE)
        CloseHandle(_hStdErrPipeRead);
    if (_hStdErrPipeWrite != INVALID_HANDLE_VALUE)
        CloseHandle(_hStdErrPipeWrite);

    _hStdOutPipeRead = INVALID_HANDLE_VALUE;
    _hStdOutPipeWrite = INVALID_HANDLE_VALUE;
    _hStdErrPipeRead = INVALID_HANDLE_VALUE;
    _hStdErrPipeWrite = INVALID_HANDLE_VALUE;

    GetExitCodeProcess(_pi.hProcess, &_exitCode);
    CloseHandle(_pi.hThread);
    CloseHandle(_pi.hProcess);
    _logWriter->WriteFormat("\tExit code: %d\n\n", _exitCode);
#else
    // TODO
#endif
}

bool PipedProcess::IsPipeCreated() const
{
#if (AGS_PLATFORM_OS_WINDOWS)
    return _hStdOutPipeRead != NULL && _hStdOutPipeWrite != NULL;
#else
    return false;
#endif
}

int PipedProcess::GetExitCode() const
{
#if (AGS_PLATFORM_OS_WINDOWS)
    return static_cast<int>(_exitCode);
#else
    return false;
#endif
}

} // namespace AGSBuild
