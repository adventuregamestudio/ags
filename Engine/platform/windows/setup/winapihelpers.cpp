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
#include "winapihelpers.h"

#if AGS_PLATFORM_OS_WINDOWS

#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include "util/path.h"
#include "util/stdio_compat.h"

namespace AGS
{
namespace Engine
{

//=============================================================================
//
// Common controls
//
//=============================================================================

String GetText(HWND hwnd)
{
    WCHAR buf[MAX_PATH_SZ];
    int len = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);
    if (len > 0)
    {
        WCHAR *pbuf = len >= MAX_PATH_SZ ? new WCHAR[len + 1] : buf;
        SendMessageW(hwnd, WM_GETTEXT, len + 1, (LPARAM)buf);
        String s = Path::WidePathToUTF8(pbuf);
        if (pbuf != buf)
            delete [] pbuf;
        return s;
    }
    return "";
}

void SetText(HWND hwnd, LPCSTR text)
{
    WCHAR wstr[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wstr, MAX_PATH_SZ);
    SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)wstr);
}

void SetText(HWND hwnd, LPCWSTR wtext)
{
    SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)wtext);
}

//=============================================================================
//
// Button controls
//
//=============================================================================

bool GetCheck(HWND hwnd)
{
    return SendMessage(hwnd, BM_GETCHECK, 0, 0) != FALSE;
}

void SetCheck(HWND hwnd, bool check)
{
    SendMessage(hwnd, BM_SETCHECK, check ? BST_CHECKED : BST_UNCHECKED, 0);
}

//=============================================================================
//
// ListBox controls
//
//=============================================================================

int AddString(HWND hwnd, LPCWSTR text, DWORD_PTR data)
{
    int index = SendMessageW(hwnd, CB_ADDSTRING, 0, (LPARAM)text);
    if (index >= 0)
        SendMessageW(hwnd, CB_SETITEMDATA, index, data);
    return index;
}

int AddString(HWND hwnd, LPCSTR text, DWORD_PTR data)
{
    WCHAR wstr[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wstr, MAX_PATH_SZ);
    return AddString(hwnd, wstr, data);
}

int InsertString(HWND hwnd, LPCWSTR text, int at_index, DWORD_PTR data)
{
    int index = SendMessageW(hwnd, CB_INSERTSTRING, at_index, (LPARAM)text);
    if (index >= 0)
        SendMessageW(hwnd, CB_SETITEMDATA, index, data);
    return index;
}

int InsertString(HWND hwnd, LPCSTR text, int at_index, DWORD_PTR data)
{
    WCHAR wstr[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wstr, MAX_PATH_SZ);
    return InsertString(hwnd, wstr, at_index, data);
}

//=============================================================================
//
// List controls
//
//=============================================================================

int GetItemCount(HWND hwnd)
{
    return SendMessage(hwnd, CB_GETCOUNT, 0, 0L);
}

int GetCurSel(HWND hwnd)
{
    return SendMessage(hwnd, CB_GETCURSEL, 0, 0);
}

void SetCurSel(HWND hwnd, int cur_sel)
{
    SendMessage(hwnd, CB_SETCURSEL, cur_sel, 0);
}

bool CmpICBItemDataAsStr(DWORD_PTR data1, DWORD_PTR data2)
{
    LPCSTR text_ptr1 = (LPCSTR)data1;
    LPCSTR text_ptr2 = (LPCSTR)data2;
    return text_ptr1 && text_ptr2 && StrCmpIA(text_ptr1, text_ptr2) == 0 || !text_ptr1 && !text_ptr2;
}

int SetCurSelToItemData(HWND hwnd, DWORD_PTR data, PfnCompareCBItemData pfn_cmp, int def_sel)
{
    int count = SendMessage(hwnd, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; ++i)
    {
        DWORD_PTR item_data = SendMessage(hwnd, CB_GETITEMDATA, i, 0);
        if (pfn_cmp && pfn_cmp(item_data, data) || !pfn_cmp && item_data == data)
        {
            LRESULT res = SendMessage(hwnd, CB_SETCURSEL, i, 0);
            if (res != CB_ERR)
                return res;
            break;
        }
    }
    return SendMessage(hwnd, CB_SETCURSEL, def_sel, 0);
}

int SetCurSelToItemDataStr(HWND hwnd, LPCSTR text, int def_sel)
{
    return SetCurSelToItemData(hwnd, (DWORD_PTR)text, CmpICBItemDataAsStr, def_sel);
}

DWORD_PTR GetCurItemData(HWND hwnd, DWORD_PTR def_value)
{
    int index = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    if (index >= 0)
        return SendMessage(hwnd, CB_GETITEMDATA, index, 0);
    return def_value;
}

void ResetContent(HWND hwnd)
{
    SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
}

//=============================================================================
//
// Slider controls
//
//=============================================================================

void SetSliderRange(HWND hwnd, int min, int max)
{
    SendMessage(hwnd, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(min, max));
}

int GetSliderPos(HWND hwnd)
{
    return SendMessage(hwnd, TBM_GETPOS, 0, 0);
}

void SetSliderPos(HWND hwnd, int pos)
{
    SendMessage(hwnd, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pos);
}

//=============================================================================
//
// Tab controls
//
//=============================================================================

Rect GetTabControlDisplayRect(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    SendMessage(hwnd, TCM_ADJUSTRECT, FALSE, (LPARAM)&rc);
    return Rect(rc.left, rc.top, rc.right - 1, rc.bottom - 1);
}

void InsertTabButton(HWND hwnd, int index, const String &text)
{
    TC_ITEM tie;
    tie.mask = TCIF_TEXT | TCIF_IMAGE;
    tie.iImage = -1;
    tie.pszText = (LPSTR)text.GetCStr();
    SendMessage(hwnd, TCM_INSERTITEM, (WPARAM)index, (LPARAM)&tie);
}

int GetSelectedTab(HWND hwnd)
{
    return SendMessage(hwnd, TCM_GETCURSEL, 0, 0);   
}

//=============================================================================
//
// Browse-for-folder dialog
//
//=============================================================================

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lParam*/, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED)
    {
        // Set initial selection
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpData);
    }
    return 0;
}

bool BrowseForFolder(String &dir_buf)
{
    bool res = false;
    CoInitialize(NULL);

    BROWSEINFO bi = { 0 };
    bi.lpszTitle = "Select location for game saves and custom data files";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM)dir_buf.GetCStr();
    LPITEMIDLIST pidl = SHBrowseForFolder ( &bi );
    if (pidl)
    {
        WCHAR path[MAX_PATH_SZ];
        if (SHGetPathFromIDListW(pidl, path) != FALSE)
        {
            dir_buf = Path::WidePathToUTF8(path);
            res = true;
        }
        CoTaskMemFree(pidl);
    }

    CoUninitialize();
    return res;
}

//=============================================================================
//
// Miscellaneous
//
//=============================================================================

void MakeFullLongPath(const char *path, WCHAR *out_buf, int buf_len)
{
    WCHAR wbuf[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, out_buf, buf_len);
    GetFullPathNameW(out_buf, MAX_PATH_SZ, wbuf, NULL);
    GetLongPathNameW(wbuf, out_buf, buf_len);
}

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS
