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
//
// Parent class for the WinAPI Dialog control.
//
//=============================================================================
#ifndef __AGS_EE_SETUP__WINDIALOG_H
#define __AGS_EE_SETUP__WINDIALOG_H

#include "platform/platform.h"

#if AGS_PLATFORM_OS_WINDOWS

#include "platform/windows/windows.h"
#include "util/geometry.h"
#include "util/string.h"

namespace AGS
{
namespace Engine
{

using namespace AGS::Common;

class WinDialog
{
public:
    WinDialog() = default;
    virtual ~WinDialog();

    static INT_PTR ShowModal(WinDialog *dialog, HWND parent_hwnd);
    void CreateModeless(HWND parent_hwnd);

    virtual String GetTitle() const { return ""; /* TODO? */ }

    bool IsCreated() const { return _hwnd != NULL; }
    HWND GetHandle() const { return _hwnd; }
    void Show();
    void Show(HWND insert_after, const Rect &pos);
    void Hide();

protected:
    virtual UINT GetTemplateID() const { return -1; }

    // Event handlers
    virtual INT_PTR OnDialogEvent(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual INT_PTR OnInitDialog() { return FALSE; /* do nothing */ }
    virtual INT_PTR OnDestroyDialog() { return FALSE; /* do nothing */ }
    virtual INT_PTR OnCommand(WORD id) { return FALSE; /* do nothing */ }
    virtual INT_PTR OnListSelection(WORD id) { return FALSE; /* do nothing */ }

    HWND _hwnd = NULL; // dialog handle; TODO: make private, but this will require to use getter

private:
    static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS

#endif // __AGS_EE_SETUP__WINDIALOG_H
