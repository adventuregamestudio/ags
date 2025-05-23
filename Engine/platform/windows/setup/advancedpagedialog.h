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
#ifndef __AGS_EE_SETUP__ADVANCEDPAGEDIALOG_H
#define __AGS_EE_SETUP__ADVANCEDPAGEDIALOG_H

#include "core/platform.h"

#if AGS_PLATFORM_OS_WINDOWS

#include "platform/windows/setup/basicpagedialog.h"

namespace AGS
{
namespace Engine
{

using namespace AGS::Common;

//=============================================================================
//
// AdvancedPageDialog
//
//=============================================================================
class AdvancedPageDialog : public WinSetupPageDialog
{
public:
    AdvancedPageDialog(WinConfig &win_cfg, const ConfigTree &cfg_in)
        : WinSetupPageDialog(win_cfg, cfg_in) {}

    String GetTitle() const override { return "Advanced"; }

    void ResetSetup(const ConfigTree &cfg_from) override;
    void SaveSetup() override;

protected:
    UINT GetTemplateID() const override { return IDD_PAGE_ADVANCED; }

    // Event handlers
    INT_PTR OnInitDialog() override;
    INT_PTR OnDialogEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    static const int MouseSpeedMin = 1;
    static const int MouseSpeedMax = 100;

    // Operations
    void FillAudioDriverList();
    void UpdateMouseSpeedText();

    // Dialog properties
    // Audio driver descriptions
    std::vector<std::pair<String, String>> _drvAudioList;

    // Dialog controls
    HWND _hSpriteCacheList = NULL;
    HWND _hTextureCacheList = NULL;
    HWND _hAudioDriverList = NULL;
    HWND _hSoundCacheList = NULL;
    HWND _hVSync = NULL;
    HWND _hRenderAtScreenRes = NULL;
    HWND _hAntialiasSprites = NULL;
    HWND _hUseVoicePack = NULL;
    HWND _hMouseLock = NULL;
    HWND _hMouseSpeed = NULL;
    HWND _hMouseSpeedText = NULL;
};

//=============================================================================
//
// GameFilesPageDialog
//
//=============================================================================
class GameFilesPageDialog : public WinSetupPageDialog
{
public:
    GameFilesPageDialog(WinConfig &win_cfg, const ConfigTree &cfg_in)
        : WinSetupPageDialog(win_cfg, cfg_in) {}

    String GetTitle() const override { return "Game Files"; }

    void ResetSetup(const ConfigTree &cfg_from) override;
    void SaveSetup() override;

protected:
    UINT GetTemplateID() const override { return IDD_PAGE_PATHS; }

    // Event handlers
    INT_PTR OnInitDialog() override;
    INT_PTR OnCommand(WORD id) override;

private:
    // Event handlers
    void OnCustomSaveDirBtn();
    void OnCustomSaveDirCheck();
    void OnCustomAppDataDirBtn();
    void OnCustomAppDataDirCheck();

    // Dialog controls
    HWND _hCustomSaveDir = NULL;
    HWND _hCustomSaveDirBtn = NULL;
    HWND _hCustomSaveDirCheck = NULL;
    HWND _hCustomAppDataDir = NULL;
    HWND _hCustomAppDataDirBtn = NULL;
    HWND _hCustomAppDataDirCheck = NULL;
    HWND _hCompressSavesCheck = NULL;
};

//=============================================================================
//
// AccessibilityPageDialog
//
//=============================================================================
class AccessibilityPageDialog : public WinSetupPageDialog
{
public:
    AccessibilityPageDialog(WinConfig &win_cfg, const ConfigTree &cfg_in, ConfigTree &cfg_out)
        : WinSetupPageDialog(win_cfg, cfg_in)
        , _cfgOut(cfg_out)
    {}

    String GetTitle() const override { return "Accessibility"; }

    void ResetSetup(const ConfigTree &cfg_from) override;
    void SaveSetup() override;

protected:
    UINT GetTemplateID() const override { return IDD_PAGE_ACCESS; }

    // Event handlers
    INT_PTR OnInitDialog() override;
    INT_PTR OnDialogEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    INT_PTR OnCommand(WORD id) override;

private:
    static const int TextReadSpeedMin = 0; // 0 means "use game defaults"
    static const int TextReadSpeedMax = 30; // double of the default AGS "15"

    // Event handlers
    void OnEnableAccessCheck();
    void UpdateTextReadSpeed();

    // Dialog controls
    HWND _hEnableAccess = NULL;
    HWND _hSpeechSkipStyle = NULL;
    HWND _hTextSkipStyle = NULL;
    HWND _hTextReadSpeed = NULL;
    HWND _hTextReadSpeedText = NULL;

    ConfigTree &_cfgOut;
    bool _disabledSkipStyle = false;
};

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS

#endif __AGS_EE_SETUP__ADVANCEDPAGEDIALOG_H
