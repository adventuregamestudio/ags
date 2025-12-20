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
#include "core/platform.h"

#if AGS_PLATFORM_OS_WINDOWS
#include <memory>
#include <algorithm>
#include <array>
#include <set>
#include <vector>
#include "platform/windows/windows.h"
#include <commctrl.h>
#include "main/config.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "platform/windows/setup/basicpagedialog.h"
#include "platform/windows/setup/advancedpagedialog.h"
#include "platform/windows/setup/winapihelpers.h"
#include "platform/windows/setup/windialog.h"
#include "platform/windows/setup/winpagedialog.h"
#include "resource/resource.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stdio_compat.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Engine
{

//=============================================================================
//
// WinSetupDialog, handles the dialog UI.
//
//=============================================================================
class WinSetupDialog : public WinDialog
{
public:
    WinSetupDialog(const ConfigTree &cfg_in, const ConfigTree &def_cfg_in, ConfigTree &cfg_out, const String &data_dir, const String &version_str);
    ~WinSetupDialog() override;

    static SetupReturnValue ShowModal(const ConfigTree &cfg_in, const ConfigTree &def_cfg_in,
        ConfigTree &cfg_out, const String &data_dir, const String &version_str);

protected:
    UINT GetTemplateID() const override { return IDD_SETUP; }

    // Event handlers
    INT_PTR OnDialogEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    INT_PTR OnInitDialog() override;
    INT_PTR OnCommand(WORD id) override;

private:
    // Event handlers
    void OnResetToDefaults();

    void SaveSetup();

    // Dialog properties
    WinConfig _winCfg;
    const ConfigTree &_cfgIn;
    const ConfigTree &_defCfgIn; // game defaults, not including player's setup
    ConfigTree &_cfgOut;

    // Dialog controls
    HWND _hTabber = NULL;
    HWND _hVersionText = NULL;
    std::unique_ptr<PageControl> _pager;
    std::vector<std::shared_ptr<WinSetupPageDialog>> _pages;
};

WinSetupDialog::WinSetupDialog(const ConfigTree &cfg_in, const ConfigTree &def_cfg_in,
        ConfigTree &cfg_out, const String &data_dir, const String &version_str)
    : _cfgIn(cfg_in)
    , _defCfgIn(def_cfg_in)
    , _cfgOut(cfg_out)
{
    _winCfg.DataDirectory = data_dir;
    _winCfg.VersionString = version_str;
}

WinSetupDialog::~WinSetupDialog()
{
}

SetupReturnValue WinSetupDialog::ShowModal(const ConfigTree &cfg_in, const ConfigTree &def_cfg_in, ConfigTree &cfg_out,
                                           const String &data_dir, const String &version_str)
{
    std::unique_ptr<WinSetupDialog> dlg(new WinSetupDialog(cfg_in, def_cfg_in, cfg_out, data_dir, version_str));
    INT_PTR dlg_res = WinDialog::ShowModal(dlg.get(), (HWND)sys_win_get_window());
    dlg.reset();

    switch (dlg_res)
    {
    case IDOKRUN: return kSetup_RunGame;
    case IDOK: return kSetup_Done;
    default: return kSetup_Cancel;
    }
}

INT_PTR WinSetupDialog::OnInitDialog()
{
    _hVersionText = GetDlgItem(_hwnd, IDC_VERSION);
    _hTabber      = GetDlgItem(_hwnd, IDC_TABPANEL);

    _winCfg.LoadMeta(_defCfgIn);
    _winCfg.LoadCommon(_cfgIn);

    // Resolution controls
    if (_winCfg.GameResolution.IsNull() &&
          (_winCfg.GameResType == kGameResolution_Undefined || _winCfg.GameResType == kGameResolution_Custom) ||
          _winCfg.GameColourDepth == 0)
        MessageBox(_hwnd, "Essential information about the game is missing in the configuration file. Setup program may be unable to deduce graphic modes properly.", "Initialization error", MB_OK | MB_ICONWARNING);

    SetText(_hwnd, STR(_winCfg.Title));
    SetText((HWND)sys_win_get_window(), STR(_winCfg.Title));
    SetText(_hVersionText, STR(_winCfg.VersionString));

    _pages.emplace_back(new BasicPageDialog(_winCfg, _cfgIn));
    _pages.emplace_back(new AdvancedPageDialog(_winCfg, _cfgIn));
    _pages.emplace_back(new CustomPathsPageDialog(_winCfg, _cfgIn));
    if (AccessibilityPageDialog::ShouldDisplayPage(_cfgIn))
        _pages.emplace_back(new AccessibilityPageDialog(_winCfg, _cfgIn, _cfgOut));
    _pager.reset(new PageControl(_hTabber));
    for (auto &p : _pages)
        _pager->AddPage(p);
    _pager->SelectPage(0);

    SetFocus(GetDlgItem(_hwnd, IDOK));
    return FALSE; // notify WinAPI that we set focus ourselves
}

INT_PTR WinSetupDialog::OnDialogEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // First try the generic handlers in the base class
    if (WinDialog::OnDialogEvent(uMsg, wParam, lParam) == TRUE)
        return TRUE;

    // Handle any uncommon messages that do not have corresponding
    // methods in the WinDialog class
    switch (uMsg)
    {
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case TCN_SELCHANGE:
            // Select another page on the tab control
            _pager->SelectPage(GetSelectedTab(_hTabber));
            return TRUE;
        default:
            return FALSE;
        }
    default:
        return FALSE;
    }
}

INT_PTR WinSetupDialog::OnCommand(WORD id)
{
    switch (id)
    {
    case IDOK:
    case IDOKRUN:
        SaveSetup();
        // fall-through intended
    case IDCANCEL:
        EndDialog(_hwnd, id);
        return TRUE;
    case IDRESETTODEFAULTS:
        OnResetToDefaults();
        return TRUE;
    default:
        return FALSE;
    }
    return TRUE;
}

void WinSetupDialog::OnResetToDefaults()
{
    if (MessageBox(_hwnd, "This will reset ALL the game configuration to defaults.\nWould you like to continue?", "Confirmation", MB_OKCANCEL | MB_ICONWARNING)
            == IDCANCEL)
        return;

    _winCfg.SetDefaults();
    _winCfg.LoadCommon(_defCfgIn);
    for (auto &p : _pages)
        p->ResetSetup(_defCfgIn);
}

void WinSetupDialog::SaveSetup()
{
    // Save all the setup pages
    for (auto &p : _pages)
        p->SaveSetup();

    _winCfg.Save(_cfgOut);
}

//=============================================================================
//
// Windows setup entry point.
//
//=============================================================================
SetupReturnValue WinSetup(const ConfigTree &cfg_in, const ConfigTree &def_cfg_in,
    ConfigTree &cfg_out, const String &game_data_dir, const String &version_str)
{
    return WinSetupDialog::ShowModal(cfg_in, def_cfg_in, cfg_out, game_data_dir, version_str);
}

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS
