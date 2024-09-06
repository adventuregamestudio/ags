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
#include "windialog.h"

#if AGS_PLATFORM_OS_WINDOWS

#include "debug/assert.h"

namespace AGS
{
namespace Engine
{

WinDialog::~WinDialog()
{
    if (_hwnd)
        DestroyWindow(_hwnd);
}

INT_PTR WinDialog::ShowModal(WinDialog *dialog, HWND parent_hwnd)
{
    return DialogBoxParamW(GetModuleHandleW(NULL), (LPCWSTR)dialog->GetTemplateID(), parent_hwnd,
        (DLGPROC)WinDialog::DialogProc, (LONG_PTR)dialog);
}

INT_PTR CALLBACK WinDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WinDialog *dlg;
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // Bind a dialog object pointer to HWND (we assume it was passed as lParam)
        assert(lParam != NULL);
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, lParam);
        if (lParam != NULL)
        {
            dlg = (WinDialog*)lParam;
            dlg->_hwnd = hwndDlg;
            return dlg->OnDialogEvent(uMsg, wParam, lParam);
        }
        return FALSE;
    default:
        // Retrieve a bound dialog object from HWND and call OnDialogEvent()
        dlg = (WinDialog*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);
        if (dlg)
            return dlg->OnDialogEvent(uMsg, wParam, lParam);
        return FALSE;
    }
}

INT_PTR WinDialog::OnDialogEvent(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return OnInitDialog();
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE)
            return OnListSelection(LOWORD(wParam));
        return OnCommand(LOWORD(wParam));
    default:
        return FALSE;
    }
}

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS
