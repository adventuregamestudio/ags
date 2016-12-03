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

#include <string.h>
#include "debug/logfile.h"
#include "util/file.h"
#include "util/stream.h"

namespace AGS
{
namespace Engine
{

using namespace Common;

//
// TODO: filepath parameter here may be actually used as a pattern
// or prefix, while the actual filename could be made by combining
// this prefix with current date, game name, and similar additional
// useful information. Whether this is to be determined here or on
// high-level side remains a question.
//
LogFile::LogFile(size_t buffer_limit)
    : _bufferLimit(buffer_limit)
    , _openMode(kLogFile_OpenOverwrite)
    , _file(NULL)
    , _buffering(true)
    , _msgLost(0)
{
}

LogFile::LogFile(const String &file_path, LogFileOpenMode open_mode, bool open_at_start, size_t buffer_limit)
    : _bufferLimit(buffer_limit)
    , _filePath(file_path)
    , _openMode(open_mode)
    , _file(NULL)
    , _buffering(true)
    , _msgLost(0)
{
    if (open_at_start)
    {
        OpenFile();
    }
}

LogFile::~LogFile()
{
    CloseFile();
}

void LogFile::Write(const DebugMessage &msg)
{
    if (!msg.GroupName.IsEmpty())
    {
        _file->Write(msg.GroupName, msg.GroupName.GetLength());
        _file->Write(" : ", 3);
    }
    _file->Write(msg.Text, msg.Text.GetLength());
    _file->WriteInt8('\n');
}

void LogFile::PrintMessage(const DebugMessage &msg)
{
    if (_file /* && !buffering, but (_file != NULL) condition is sufficient here */)
    {
        Write(msg);
        // We should flush after every write to the log; this will make writing
        // bit slower, but will increase the chances that all latest output
        // will get to the disk in case of program crash.
        _file->Flush();
    }
    else
    {
        if (_buffer.size() < _bufferLimit)
            _buffer.push_back(msg);
        else
            _msgLost++;
    }
}

bool LogFile::OpenFile()
{
    return OpenFile(_filePath, _openMode);
}

bool LogFile::OpenFile(const String &file_path, LogFileOpenMode open_mode)
{
    CloseFile();

    _filePath = file_path;
    _openMode = open_mode;
    _file = File::OpenFile(file_path,
                           open_mode == kLogFile_OpenAppend ? Common::kFile_Create : Common::kFile_CreateAlways,
                           Common::kFile_Write);
    if (_file)
    {
        _buffering = false;
        FlushBuffer();
        return true;
    }
    return false;
}

void LogFile::CloseFile()
{
    if (_file)
    {
        delete _file;
        _file = NULL;
        _buffering = true;
    }
}

void LogFile::FlushBuffer()
{
    if (_file && !_buffer.empty())
    {
        for (std::vector<DebugMessage>::const_iterator it = _buffer.begin(); it != _buffer.end(); ++it)
        {
            Write(*it);
        }
        if (_msgLost > 0)
        {
            String warning = String::FromFormat("WARNING: output lost exceeding buffer: %u debug messages\n", (unsigned)_msgLost);
            _file->Write(warning.GetCStr(), warning.GetLength());
        }
        _file->Flush();
        _buffer.clear();
        _msgLost = 0;
    }
}

} // namespace Engine
} // namespace AGS
