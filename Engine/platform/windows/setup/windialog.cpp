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
    assert(dialog->_hwnd == NULL);
    if (dialog->_hwnd != NULL)
        return FALSE;
    return DialogBoxParamW(GetModuleHandleW(NULL), (LPCWSTR)dialog->GetTemplateID(), parent_hwnd,
        (DLGPROC)WinDialog::DialogProc, (LONG_PTR)dialog);
}

// DoLockDlgRes - loads and locks a dialog template resource.
// Returns a pointer to the locked resource.
// lpszResName - name of the resource
DLGTEMPLATE *DoLockDlgRes(HINSTANCE hinstance, LPCWSTR lpszResName)
{
    HRSRC hrsrc = FindResourceW(NULL, lpszResName, (LPCWSTR)RT_DIALOG);
    HGLOBAL hglb = LoadResource(hinstance, hrsrc);
    return (DLGTEMPLATE *)LockResource(hglb);
}

void WinDialog::CreateModeless(HWND parent_hwnd)
{
    assert(_hwnd == NULL);
    if (_hwnd != NULL)
        return;

    HINSTANCE hinst = GetModuleHandleW(NULL);
    DLGTEMPLATE *dlgtempl = DoLockDlgRes(hinst, (LPCWSTR)GetTemplateID());
    CreateDialogIndirectParamW(hinst, dlgtempl, parent_hwnd,
        (DLGPROC)WinDialog::DialogProc, (LONG_PTR)this);
}

void WinDialog::Show()
{
    ShowWindow(_hwnd, SW_SHOW);
}

void WinDialog::Show(HWND insert_after, const Rect &pos)
{
    ShowWindow(_hwnd, SW_SHOW);
    SetWindowPos(_hwnd, insert_after, pos.Left, pos.Top, 0, 0, SWP_NOSIZE);
}

void WinDialog::Hide()
{
    ShowWindow(_hwnd, SW_HIDE);
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
