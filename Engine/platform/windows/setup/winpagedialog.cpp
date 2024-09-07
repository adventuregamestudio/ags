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
#include "winpagedialog.h"

#if AGS_PLATFORM_OS_WINDOWS

#include "debug/assert.h"
#include "platform/windows/setup/winapihelpers.h"

namespace AGS
{
namespace Engine
{

PageControl::PageControl(HWND control_hwnd)
    : _hwnd(control_hwnd)
{
    assert(_hwnd);
    _parentRect = GetTabControlDisplayRect(_hwnd);
}

void PageControl::AddPage(std::shared_ptr<WinPageDialog> dlg)
{
    AddPage(dlg, dlg->GetTitle());
}

void PageControl::AddPage(std::shared_ptr<WinPageDialog> dlg, const String &title)
{
    _pages.push_back(dlg);
    InsertTabButton(_hwnd, _pages.size() - 1, title);
    _parentRect = GetTabControlDisplayRect(_hwnd);
}

void PageControl::SelectPage(size_t index)
{
    assert(index < _pages.size());
    // Hide the previous page
    if (_curPage)
    {
        _curPage->Hide();
        _curPage = nullptr;
    }

    if (index < _pages.size())
    {
        _curPage = _pages[index].get();
        if (!_curPage->IsCreated())
            _curPage->CreateModeless(_hwnd);
        _curPage->Show(HWND_TOP, _parentRect);
    }
}

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS
