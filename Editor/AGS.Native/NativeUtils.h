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
// Declarations of the utility functions used on native side.
//
//=============================================================================
#pragma once

#include "util/string.h"

typedef AGS::Common::String AGSString;

// TextConverter provides methods for converting between managed
// System::String and native AGSString using provided Encoding.
public ref class TextConverter
{
public:
    TextConverter(System::Text::Encoding^ enc);
    System::Text::Encoding^ GetEncoding();
    // Convert native string to managed using current encoding
    System::String^ Convert(const AGS::Common::String &str);
    // Convert managed string to native using current encoding;
    // this is meant for strings containing human-readable texts
    AGSString Convert(System::String^ clr_str);
    // Converts a textual property from managed string to native using current encoding;
    // Does additional transformation for human-readable text:
    // - unescapes special sequences ("\\n" to '\n')
    AGSString ConvertTextProperty(System::String^ clr_str);
    // Convert managed string to native std::string using current encoding;
    // this is meant for strings containing human-readable texts
    std::string ConvertToStd(System::String^ clr_str);

private:
    System::Text::Encoding^ _encoding = nullptr;
};

// TextHelper namespace groups several text conversion functions
// that always use explicit encoding either by definition or as an argument.
namespace TextHelper
{
    // Gets a global game text converter, dependent on the game settings.
    TextConverter^ GetGameTextConverter();

    //-------------------------------------------------------------------------
    // Native to managed string convertions
    // Convert ASCII text to managed string;
    // this is for symbol names of variables, functions and alike
    System::String^ ConvertASCII(const AGS::Common::String &str);
    // Convert UTF-8 text to managed string;
    // this is for filepaths and error messages, etc
    System::String^ ConvertUTF8(const AGS::Common::String &str);

    //-------------------------------------------------------------------------
    // Managed to native string convertions
    // Convert ASCII managed text to native string;
    // this is for symbol names of variables, functions and alike
    AGSString ConvertASCII(System::String^ clr_str);
    std::string ConvertASCIIToStd(System::String^ clr_str);
    void ConvertASCIIToArray(System::String^ clr_str, char *buf, size_t buf_len);
    // Convert UTF-8 managed text to native string;
    // this is for filepaths and error messages, etc
    AGSString ConvertUTF8(System::String^ clr_str);

    // Convert native to managed using given encoder
    System::String^ Convert(const AGS::Common::String &str, System::Text::Encoding^ enc);
    // Convert managed to native using given encoder
    AGSString Convert(System::String^ clr_str, System::Text::Encoding^ enc);
    // Convert managed to native std::string using given encoder
    std::string ConvertToStd(System::String^ clr_str, System::Text::Encoding^ enc);
};

namespace WinAPIHelper
{
    // Returns a message describing given WinAPI error code.
    // Error text is returned as a UTF-8 string.
    // Uses GetLastError if passed errcode is 0.
    AGSString GetErrorUTF8(uint32_t errcode = 0);
    AGSString MakeErrorUTF8(const AGSString &error_title, uint32_t errcode = 0);
    System::String^ MakeErrorManaged(const AGSString &error_title, uint32_t errcode = 0);
}

extern AGSString editorVersionNumber;
