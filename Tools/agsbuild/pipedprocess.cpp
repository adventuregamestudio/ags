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
#if (AGS_PLATFORM_OS_WINDOWS)
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#endif
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
#if (AGS_PLATFORM_OS_WINDOWS)
    _command = String::FromFormat("%s.exe %s", exe_path.GetCStr(), params.GetCStr());
    memset(&_pi, 0, sizeof(_pi));
#else
    _command = String::FromFormat("%s %s", exe_path.GetCStr(), params.GetCStr());
#endif
}

HError PipedProcess::RunSync(std::vector<uint8_t> *stdout_data)
{
    HError err = CreateProcess();
    if (!err)
        return err;

    if (IsPipeCreated())
    {
        err = RunProcessSync(stdout_data);
    }
    else
    {
        if (stdout_data)
            err = new Error("Failed to create pipe(s), won't be able to read out process output");
        else
            _logWriter->WriteString("Failed to create pipe(s), won't be able to read out process output\n");
        WaitForProcessSync();
    }

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
    const bool stdout_pipe = pipe(_stdOutPipe) >= 0;
    const bool stderr_pipe = pipe(_stdErrPipe) >= 0;

    _cpid = fork();
    if (_cpid < 0)
    {
        return new Error(String::FromFormat("Process fork failed with err code: %d\n", errno));
    }

    if (_cpid == 0)
    {
        // Setup child process to write into the pipe
        // Route write pipe's end to stdout and release the old handle
        dup2(_stdOutPipe[1], STDOUT_FILENO);
        // Route write pipe's end to stderr and release the old handle
        dup2(_stdErrPipe[1], STDERR_FILENO);
        // Close our pipe instances
        // TODO: smart pointer for the pipe handle
        close(_stdOutPipe[0]);       
        close(_stdOutPipe[1]);
        close(_stdErrPipe[0]);
        close(_stdErrPipe[1]);
        _stdOutPipe[0] = _stdOutPipe[1] = 0;
        _stdErrPipe[0] = _stdErrPipe[1] = 0;
        // TODO: in order to call the program directly, instead of using /bin/sh method,
        // we need a params parser, which splits params string into separate elements,
        // but it must properly handle args in doublequotes!
        execl("/bin/sh", "/bin/sh", "-c", _command.GetCStr(), nullptr);
        _exit(EXIT_FAILURE); // exec* does not return on success, if we got here then it failed
    }
    else
    {
        // Setup parent process to read from the pipe
        // Close unused write end
        close(_stdOutPipe[1]);
        close(_stdErrPipe[1]);
        _stdOutPipe[1] = 0;
        _stdErrPipe[1] = 0;
    }
#endif
    return HError::None();
}

#if (AGS_PLATFORM_OS_WINDOWS)
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
#else
void PipedProcess::ReadFromPipe(int pipe_fd, std::vector<uint8_t> *output_data, bool print_to_buffer)
{
    uint8_t buffer[1024];
    ssize_t bytes_read = 0;
    do
    {
        bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0)
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
        }
    }
    while ((bytes_read > 0) || (bytes_read == EAGAIN || bytes_read == EWOULDBLOCK));
}
#endif

HError PipedProcess::RunProcessSync(std::vector<uint8_t> *stdout_data)
{
#if (AGS_PLATFORM_OS_WINDOWS)
    if (_pi.hProcess == NULL)
        return new Error("Internal error: external process was not initialized");

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
    if (_cpid == 0)
        return new Error("Internal error: external process was not initialized");

    pid_t wait_pid = 0;
    int wstatus = 0;
    do
    {
        wait_pid = waitpid(_cpid, &wstatus, WNOHANG);
        //printf("waitpid -- wstatus = %d\n", wstatus);
        if (_stdOutPipe[0] != 0)
            ReadFromPipe(_stdOutPipe[0], stdout_data, true);
        if (_stdErrPipe[0] != 0)
            ReadFromPipe(_stdErrPipe[0], nullptr, true);
    }
    while ((wait_pid == 0) || ((wait_pid == _cpid) && !WIFEXITED(wstatus) && !WIFSIGNALED(wstatus)));
    
    if (WIFEXITED(wstatus))
        _exitCode = static_cast<int8_t>(WEXITSTATUS(wstatus));
    else if (WIFSIGNALED(wstatus))
        _exitCode = WTERMSIG(wstatus); // FIXME: separate code variable
#endif
    return HError::None();
}

void PipedProcess::WaitForProcessSync()
{
#if (AGS_PLATFORM_OS_WINDOWS)
    WaitForSingleObject(_pi.hProcess, INFINITE);
#else
    
    int wstatus = 0;
    waitpid(_cpid, &wstatus, 0);
    if (WIFEXITED(wstatus))
        _exitCode = static_cast<int8_t>(WEXITSTATUS(wstatus));
    else if (WIFSIGNALED(wstatus))
        _exitCode = WTERMSIG(wstatus); // FIXME: separate code variable
#endif
}

void PipedProcess::CloseProcess()
{
#if (AGS_PLATFORM_OS_WINDOWS)
    // TODO: smart pointer for the pipe handle
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

    DWORD proc_code;
    GetExitCodeProcess(_pi.hProcess, &proc_code);
    CloseHandle(_pi.hThread);
    CloseHandle(_pi.hProcess);
    _exitCode = static_cast<int>(proc_code);
#else
    if (_cpid == 0)
        return;

    // TODO: smart pointer for the pipe handle
    if (_stdOutPipe[0])
        close(_stdOutPipe[0]);
    if (_stdOutPipe[1])
        close(_stdOutPipe[1]);
    if (_stdErrPipe[0])
        close(_stdErrPipe[0]);
    if (_stdErrPipe[1])
        close(_stdErrPipe[1]);
    _stdOutPipe[0] = _stdOutPipe[1] = 0;
    _stdErrPipe[0] = _stdErrPipe[1] = 0;
#endif
    _logWriter->WriteFormat("\tExit code: %d\n\n", _exitCode);
}

bool PipedProcess::IsPipeCreated() const
{
#if (AGS_PLATFORM_OS_WINDOWS)
    return _hStdOutPipeRead != NULL && _hStdOutPipeWrite != NULL;
#else
    return _stdOutPipe[0] >= 0;
#endif
}

int PipedProcess::GetExitCode() const
{
    return static_cast<int>(_exitCode);
}

} // namespace AGSBuild
