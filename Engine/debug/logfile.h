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
//
// LogFile, the IOutputTarget implementation that writes to file.
//
// When created LogFile may open file right away or delay doing this.
// In the latter case it will buffer output up to certain size limit.
// When told to open the file, it will first flush its buffer. This allows to
// log events even before the log path is decided (for example, before or
// during reading configuration and/or parsing command line).
//
//=============================================================================
#ifndef __AGS_EE_DEBUG__RAWFILEOUTPUTTARGET_H
#define __AGS_EE_DEBUG__RAWFILEOUTPUTTARGET_H

#include "debug/outputtarget.h"
#include "util/file.h"

namespace AGS
{

    namespace Common { class Stream; }

    namespace Engine
    {

        using Common::Stream;
        using Common::String;

        namespace Out
        {
            class LogFile : public AGS::Common::Out::IOutputTarget
            {
            public:
                enum LogFileOpenMode
                {
                    kLogFile_OpenOverwrite,
                    kLogFile_OpenAppend
                };

            public:
                // Initialize LogFile object without predefining file path
                LogFile(size_t buffer_limit = 4096);
                // Initialize LogFile object, optionally opening the file right away
                LogFile(const String &file_path, LogFileOpenMode open_mode = kLogFile_OpenOverwrite,
                    bool open_at_start = true, size_t buffer_limit = 4096);
                virtual ~LogFile();

                virtual void Out(const char *sz_fullmsg);

                // Open file using predefined file path and open mode
                bool         OpenFile();
                // Open file using given file path, optionally appending if one exists
                bool         OpenFile(const String &file_path, LogFileOpenMode open_mode = kLogFile_OpenOverwrite);
                // Close file and begin buffering output
                void         CloseFile();

            private:
                // Flush buffered output to the file
                void         FlushBuffer();

                const size_t    _bufferLimit;
                String          _filePath;
                LogFileOpenMode _openMode;

                Stream         *_file;
                String          _buffer;
                bool            _buffering;
                size_t          _charsLost;
            };

        }   // namespace Out

    }   // namespace Engine
}   // namespace AGS

#endif // __AGS_EE_DEBUG__RAWFILEOUTPUTTARGET_H
