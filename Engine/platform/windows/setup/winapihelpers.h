//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// WinAPI interaction helpers.
//
//=============================================================================
#ifndef __AGS_EE_SETUP__WINAPIHELPERS_H
#define __AGS_EE_SETUP__WINAPIHELPERS_H

#include "core/platform.h"

#if AGS_PLATFORM_OS_WINDOWS

#include "platform/windows/windows.h"
#include "util/string.h"

namespace AGS
{
namespace Engine
{

using namespace AGS::Common;

// Cast AGS::Common::String to LPCSTR
inline LPCSTR STR(const String &str) { return str.GetCStr(); }

//
// Common controls
//
String GetText(HWND hwnd);
void SetText(HWND hwnd, LPCSTR text);
void SetText(HWND hwnd, LPCWSTR wtext);
//
// Button controls
//
bool GetCheck(HWND hwnd);
void SetCheck(HWND hwnd, bool check);
//
// List controls
//
int AddString(HWND hwnd, LPCWSTR text, DWORD_PTR data = 0L);
int AddString(HWND hwnd, LPCSTR text, DWORD_PTR data = 0L);
int InsertString(HWND hwnd, LPCWSTR text, int at_index, DWORD_PTR data = 0L);
int InsertString(HWND hwnd, LPCSTR text, int at_index, DWORD_PTR data = 0L);
int GetItemCount(HWND hwnd);
int GetCurSel(HWND hwnd);
void SetCurSel(HWND hwnd, int cur_sel);
typedef bool (*PfnCompareCBItemData)(DWORD_PTR data1, DWORD_PTR data2);
int SetCurSelToItemData(HWND hwnd, DWORD_PTR data, PfnCompareCBItemData pfn_cmp = NULL, int def_sel = -1);
int SetCurSelToItemDataStr(HWND hwnd, LPCSTR text, int def_sel = -1);
DWORD_PTR GetCurItemData(HWND hwnd, DWORD_PTR def_value = 0);
void ResetContent(HWND hwnd);
//
// Slider controls
//
void SetSliderRange(HWND hwnd, int min, int max);
int GetSliderPos(HWND hwnd);
void SetSliderPos(HWND hwnd, int pos);
//
// Standard Dialogs
//
// Opens a "Browse for folder" standard dialog
bool BrowseForFolder(String &dir_buf);
//
// Miscellaneous
//
void MakeFullLongPath(const char *path, WCHAR *out_buf, int buf_len);

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS

#endif // __AGS_EE_SETUP__WINAPIHELPERS_H
