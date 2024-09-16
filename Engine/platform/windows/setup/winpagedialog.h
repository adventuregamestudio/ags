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
// Page control implementation in WinAPI.
//
//=============================================================================
#ifndef __AGS_EE_SETUP__WINPAGEDIALOG_H
#define __AGS_EE_SETUP__WINPAGEDIALOG_H

#include "core/platform.h"

#if AGS_PLATFORM_OS_WINDOWS

#include <memory>
#include <vector>
#include "platform/windows/setup/windialog.h"
#include "util/geometry.h"
#include "util/string.h"

namespace AGS
{
namespace Engine
{

using namespace AGS::Common;

class WinPageDialog : public WinDialog
{
public:
    WinPageDialog() = default;
    ~WinPageDialog() override = default;
};

class GenericPageDialog : public WinPageDialog
{
public:
    GenericPageDialog(UINT template_id)
        : _templateID(template_id) {}

protected:
    virtual UINT GetTemplateID() const { return _templateID; }

    UINT _templateID;
};

class PageControl
{
public:
    PageControl(HWND control_hwnd);
    ~PageControl() = default;

    void AddPage(std::shared_ptr<WinPageDialog> dlg);
    void AddPage(std::shared_ptr<WinPageDialog> dlg, const String &title);
    size_t GetPageCount() const { return _pages.size(); }
    std::shared_ptr<WinPageDialog> GetPage(size_t index) const { return index < _pages.size() ? _pages[index] : nullptr; }
    void SelectPage(size_t index);

private:
    HWND _hwnd = NULL;
    Rect _parentRect;
    std::vector<std::shared_ptr<WinPageDialog>> _pages;
    WinPageDialog *_curPage = nullptr;
};

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS

#endif __AGS_EE_SETUP__WINPAGEDIALOG_H
