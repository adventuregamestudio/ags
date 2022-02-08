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
// Declarations of the utility functions used on native side.
//
//=============================================================================
#pragma once

#include "util/string.h"

typedef AGS::Common::String AGSString;

class TextConverter
{
public:
    TextConverter(bool unicode_mode = false) : _unicodeMode(unicode_mode) {}

    //-------------------------------------------------------------------------
    // Native to managed string convertions
    // Convert default kind which may contain human text (ANSI or UTF depending on mode)
    System::String^ ConvertAny(const AGS::Common::String &str);
    // Convert ASCII text to managed string;
    // this is for symbol names of variables, functions and alike
    System::String^ ConvertASCII(const AGS::Common::String &str);
    // Convert UTF-8 text to managed string;
    // this is for filepaths and error messages, etc
    System::String^ ConvertUTF8(const AGS::Common::String &str);

    //-------------------------------------------------------------------------
    // Managed to native string convertions
    // Convert default kind which may contain human text (ANSI or UTF depending on mode)
    AGSString ConvertAny(System::String^ clr_str);
    // Convert ASCII managed text to native string;
    // this is for symbol names of variables, functions and alike
    AGSString ConvertASCII(System::String^ clr_str);
    void ConvertASCIIToArray(System::String^ clr_str, char *buf, size_t buf_len);
    // Convert managed text to native string, forcing ASCII and testing for unknown chars
    // TODO: fix it uses and replace with ConvertUTF8ToArray
    void ConvertASCIIFilename(System::String^ clr_str, char *buf, size_t buf_len);
    // Convert UTF-8 managed text to native string;
    // this is for filepaths and error messages, etc
    AGSString ConvertUTF8(System::String^ clr_str);

private:
    bool _unicodeMode = false;
};

extern AGSString editorVersionNumber;
extern TextConverter tcv;
