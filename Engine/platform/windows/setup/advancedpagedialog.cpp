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
#include "core/platform.h"

#if AGS_PLATFORM_OS_WINDOWS

#include <array>
#include <shlwapi.h>
#include "ac/speech.h"
#include "platform/windows/setup/advancedpagedialog.h"
#include "platform/windows/setup/winapihelpers.h"
#include "resource/resource.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stdio_compat.h"

namespace AGS
{
namespace Engine
{

//=============================================================================
//
// AdvancedPageDialog
//
//=============================================================================

INT_PTR AdvancedPageDialog::OnInitDialog()
{
    _hSpriteCacheList       = GetDlgItem(_hwnd, IDC_SPRITECACHE);
    _hTextureCacheList      = GetDlgItem(_hwnd, IDC_TEXTURECACHE);
    _hVSync                 = GetDlgItem(_hwnd, IDC_VSYNC);
    _hRenderAtScreenRes     = GetDlgItem(_hwnd, IDC_RENDERATSCREENRES);
    _hAntialiasSprites      = GetDlgItem(_hwnd, IDC_ANTIALIAS);
    _hAudioDriverList       = GetDlgItem(_hwnd, IDC_DIGISOUND);
    _hSoundCacheList        = GetDlgItem(_hwnd, IDC_SOUNDCACHE);
    _hUseVoicePack          = GetDlgItem(_hwnd, IDC_VOICEPACK);
    _hMouseLock             = GetDlgItem(_hwnd, IDC_MOUSE_AUTOLOCK);
    _hMouseSpeed            = GetDlgItem(_hwnd, IDC_MOUSESPEED);
    _hMouseSpeedText        = GetDlgItem(_hwnd, IDC_MOUSESPEED_TEXT);

    // Init sprite cache and texture cache lists
    // 32-bit programs have accessible RAM limit of 2-3 GB (may be less in practice),
    // and engine will need RAM for other things too, keep that in mind
    const std::array<std::pair<const char*, int>, 11> spr_cache_vals = { {
        { "16 MB", 16 }, { "32 MB", 32 }, { "64 MB", 64 }, { "128 MB", 128 },
        { "256 MB", 256}, { "384 MB", 384}, { "512 MB", 512 }, { "640 MB", 640 },
        { "768 MB", 768 }, { "896 MB", 896 }, { "1 GB", 1024 },
    }};
    ResetContent(_hSpriteCacheList);
    for (const auto &val : spr_cache_vals)
        AddString(_hSpriteCacheList, val.first, val.second);
    ResetContent(_hTextureCacheList);
    for (const auto &val : spr_cache_vals)
        AddString(_hTextureCacheList, val.first, val.second);
#if AGS_PLATFORM_64BIT
    const std::array<std::pair<const char*, int>, 4> spr_cache_vals64 = { {
        { "1.25 GB ", 1280 }, { "1.5 GB ", 1536 }, { "1.75 GB ", 1792 }, { "2 GB ", 2048 }
    }};
    for (const auto &val : spr_cache_vals64)
        AddString(_hSpriteCacheList, val.first, val.second);
    for (const auto &val : spr_cache_vals64)
        AddString(_hTextureCacheList, val.first, val.second);
#endif

    // Init sound cache list (keep in mind: currently meant only for small sounds)
    const std::array<std::pair<const char*, int>, 5> sound_cache_vals = { {
        { "Off", 0 }, { "16 MB", 16 }, { "32 MB", 32 }, { "64 MB", 64 }, { "128 MB", 128 }
    }};
    ResetContent(_hSoundCacheList);
    for (const auto &val : sound_cache_vals)
        AddString(_hSoundCacheList, val.first, val.second);

    FillAudioDriverList();
    if (!File::IsFile("speech.vox"))
        EnableWindow(_hUseVoicePack, FALSE);

    SetSliderRange(_hMouseSpeed, MouseSpeedMin, MouseSpeedMax);

    if (CfgReadBoolInt(_cfgIn, "disabled", "speechvox"))
        EnableWindow(_hUseVoicePack, FALSE);
    if (CfgReadBoolInt(_cfgIn, "disabled", "antialias"))
        EnableWindow(_hAntialiasSprites, FALSE);
    if (CfgReadBoolInt(_cfgIn, "disabled", "render_at_screenres") ||
        CfgReadInt(_cfgIn, "gameproperties", "render_at_screenres", -1) >= 0)
        EnableWindow(_hRenderAtScreenRes, FALSE);

    ResetSetup(_cfgIn);

    _isInit = true;
    return FALSE; // notify WinAPI that we set focus ourselves
}

INT_PTR AdvancedPageDialog::OnDialogEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // First try the generic handlers in the base class
    if (WinDialog::OnDialogEvent(uMsg, wParam, lParam) == TRUE)
        return TRUE;

    // Handle any uncommon messages that do not have corresponding
    // methods in the WinDialog class
    switch (uMsg)
    {
    case WM_HSCROLL:
        if ((HWND)lParam == _hMouseSpeed)
            UpdateMouseSpeedText();
        return TRUE;
    default:
        return FALSE;
    }
}

void AdvancedPageDialog::FillAudioDriverList()
{
    _drvAudioList.clear();
    _drvAudioList.push_back(std::make_pair("none", "None"));
    _drvAudioList.push_back(std::make_pair("default", "Default"));
    // TODO: unfortunately, SDL atm does not expose user-friendly device names;
    // so we have to hardcode this for the time being
    const std::array<std::pair<const char*, const char*>, 5> drv_names =
    { {
        { "wasapi", "WASAPI" },
        { "directsound", "DirectSound" },
        { "winmm", "Windows Waveform Audio" },
        { "disk", "Direct-to-disk Audio" },
        { "dummy", "Dummy audio driver" },
    } };
    for (const auto &names : drv_names)
        _drvAudioList.push_back(std::make_pair(names.first, names.second));
    // Fill driver data into UI list
    ResetContent(_hAudioDriverList);
    for (const auto &desc : _drvAudioList)
        AddString(_hAudioDriverList, STR(desc.second), (DWORD_PTR)desc.first.GetCStr());
}

void AdvancedPageDialog::UpdateMouseSpeedText()
{
    int slider_pos = GetSliderPos(_hMouseSpeed);
    float mouse_speed = (float)slider_pos / 10.f;
    String text = mouse_speed == 1.f ? "Mouse speed: x 1.0 (Default)" : String::FromFormat("Mouse speed: x %0.1f", mouse_speed);
    SetText(_hMouseSpeedText, STR(text));
}

void AdvancedPageDialog::ResetSetup(const ConfigTree & /*cfg_from*/)
{
    SetCheck(_hVSync, _winCfg.Display.VSync);
    SetCheck(_hRenderAtScreenRes, _winCfg.RenderAtScreenRes);
    SetCheck(_hAntialiasSprites, _winCfg.AntialiasSprites);

    SetCheck(_hMouseLock, _winCfg.MouseAutoLock);
    int slider_pos = (int)(_winCfg.MouseSpeed * 10.f + .5f);
    SetSliderPos(_hMouseSpeed, slider_pos);
    UpdateMouseSpeedText();

    SetCurSelToNearestItemData(_hSpriteCacheList, _winCfg.SpriteCacheSize / 1024);
    SetCurSelToNearestItemData(_hTextureCacheList, _winCfg.TextureCacheSize / 1024);
    SetCurSelToNearestItemData(_hSoundCacheList, _winCfg.SoundCacheSize / 1024);

    if (_winCfg.AudioEnabled)
    {
        if (_winCfg.AudioDriverID.IsEmpty())
            SetCurSelToItemDataStr(_hAudioDriverList, "default");
        else
            SetCurSelToItemDataStr(_hAudioDriverList, _winCfg.AudioDriverID.GetCStr());
    }
    else
    {
        SetCurSelToItemDataStr(_hAudioDriverList, "none");
    }
    SetCheck(_hUseVoicePack, _winCfg.UseVoicePack);
}

void AdvancedPageDialog::SaveSetup()
{
    if (!_isInit)
        return; // was not init, don't apply settings

    _winCfg.SpriteCacheSize = GetCurItemData(_hSpriteCacheList) * 1024;
    _winCfg.TextureCacheSize = GetCurItemData(_hTextureCacheList) * 1024;
    _winCfg.SoundCacheSize = GetCurItemData(_hSoundCacheList) * 1024;
    if (GetCurSel(_hAudioDriverList) == 0)
    {
        _winCfg.AudioEnabled = false;
        _winCfg.AudioDriverID = "";
    }
    else
    {
        _winCfg.AudioEnabled = true;
        if (GetCurSel(_hAudioDriverList) == 1)
            _winCfg.AudioDriverID = ""; // use default
        else
            _winCfg.AudioDriverID = (LPCSTR)GetCurItemData(_hAudioDriverList);
    }
    _winCfg.UseVoicePack = GetCheck(_hUseVoicePack);
    _winCfg.Display.VSync = GetCheck(_hVSync);
    _winCfg.RenderAtScreenRes = GetCheck(_hRenderAtScreenRes);
    _winCfg.AntialiasSprites = GetCheck(_hAntialiasSprites);

    _winCfg.MouseAutoLock = GetCheck(_hMouseLock);
    int slider_pos = GetSliderPos(_hMouseSpeed);
    _winCfg.MouseSpeed = (float)slider_pos / 10.f;
}

//=============================================================================
//
// CustomPathsPageDialog
//
//=============================================================================

static void SetupCustomDirCtrl(const String &save_dir_opt, const String &def_dir,
    HWND dir_check, HWND dir_text, HWND dir_btn)
{
    String custom_save_dir = save_dir_opt;
    bool has_save_dir = !custom_save_dir.IsEmpty();
    if (!has_save_dir)
        custom_save_dir = def_dir;
    SetCheck(dir_check, has_save_dir);
    WCHAR full_save_dir[MAX_PATH_SZ] = { 0 };
    MakeFullLongPath(STR(custom_save_dir), full_save_dir, MAX_PATH_SZ);
    SetText(dir_text, full_save_dir);
    EnableWindow(dir_text, has_save_dir ? TRUE : FALSE);
    EnableWindow(dir_btn, has_save_dir ? TRUE : FALSE);
}

INT_PTR CustomPathsPageDialog::OnInitDialog()
{
    _hCustomSaveDir         = GetDlgItem(_hwnd, IDC_CUSTOMSAVEDIR);
    _hCustomSaveDirBtn      = GetDlgItem(_hwnd, IDC_CUSTOMSAVEDIRBTN);
    _hCustomSaveDirCheck    = GetDlgItem(_hwnd, IDC_CUSTOMSAVEDIRCHECK);
    _hCustomAppDataDir      = GetDlgItem(_hwnd, IDC_CUSTOMAPPDATADIR);
    _hCustomAppDataDirBtn   = GetDlgItem(_hwnd, IDC_CUSTOMAPPDATADIRBTN);
    _hCustomAppDataDirCheck = GetDlgItem(_hwnd, IDC_CUSTOMAPPDATADIRCHECK);

    ResetSetup(_cfgIn);

    _isInit = true;
    return FALSE; // notify WinAPI that we set focus ourselves
}

INT_PTR CustomPathsPageDialog::OnCommand(WORD id)
{
    switch (id)
    {
    case IDC_CUSTOMSAVEDIRBTN: OnCustomSaveDirBtn(); break;
    case IDC_CUSTOMSAVEDIRCHECK: OnCustomSaveDirCheck(); break;
    case IDC_CUSTOMAPPDATADIRBTN: OnCustomAppDataDirBtn(); break;
    case IDC_CUSTOMAPPDATADIRCHECK: OnCustomAppDataDirCheck(); break;
    default:
        return FALSE;
    }
    return TRUE;
}

void CustomPathsPageDialog::OnCustomSaveDirBtn()
{
    String save_dir = GetText(_hCustomSaveDir);
    if (BrowseForFolder(save_dir))
    {
        SetText(_hCustomSaveDir, STR(save_dir));
    }
}

void CustomPathsPageDialog::OnCustomSaveDirCheck()
{
    bool custom_save_dir = GetCheck(_hCustomSaveDirCheck);
    EnableWindow(_hCustomSaveDir, custom_save_dir ? TRUE : FALSE);
    EnableWindow(_hCustomSaveDirBtn, custom_save_dir ? TRUE : FALSE);
}

void CustomPathsPageDialog::OnCustomAppDataDirBtn()
{
    String data_dir = GetText(_hCustomAppDataDir);
    if (BrowseForFolder(data_dir))
    {
        SetText(_hCustomAppDataDir, STR(data_dir));
    }
}

void CustomPathsPageDialog::OnCustomAppDataDirCheck()
{
    bool custom_data_dir = GetCheck(_hCustomAppDataDirCheck);
    EnableWindow(_hCustomAppDataDir, custom_data_dir ? TRUE : FALSE);
    EnableWindow(_hCustomAppDataDirBtn, custom_data_dir ? TRUE : FALSE);
}

static String SaveCustomDirSetup(const String &def_dir, HWND dir_check, HWND dir_text)
{
    if (!GetCheck(dir_check))
        return "";
    // Compare user path with the game data directory. If user chose
    // path pointing inside game's directory, then store relative
    // path instead; thus the path will keep pointing at game's
    // directory if user moves game elsewhere.
    String custom_dir = GetText(dir_text);
    WCHAR full_data_dir[MAX_PATH_SZ] = { 0 };
    WCHAR full_custom_dir[MAX_PATH_SZ] = { 0 };
    MakeFullLongPath(STR(def_dir), full_data_dir, MAX_PATH_SZ);
    MakeFullLongPath(STR(custom_dir), full_custom_dir, MAX_PATH_SZ);
    WCHAR rel_dir[MAX_PATH_SZ] = { 0 };
    if (PathRelativePathToW(rel_dir, full_data_dir, FILE_ATTRIBUTE_DIRECTORY, full_custom_dir, FILE_ATTRIBUTE_DIRECTORY) &&
        wcsstr(rel_dir, L"..") == NULL)
    {
        return Path::WidePathToUTF8(rel_dir);
    }
    else
    {
        return custom_dir;
    }
}

void CustomPathsPageDialog::ResetSetup(const ConfigTree & /*cfg_from*/)
{
    // Custom save dir controls
    SetupCustomDirCtrl(_winCfg.UserSaveDir, _winCfg.DataDirectory,
        _hCustomSaveDirCheck, _hCustomSaveDir, _hCustomSaveDirBtn);
    SetupCustomDirCtrl(_winCfg.AppDataDir, _winCfg.DataDirectory,
        _hCustomAppDataDirCheck, _hCustomAppDataDir, _hCustomAppDataDirBtn);
}

void CustomPathsPageDialog::SaveSetup()
{
    if (!_isInit)
        return; // was not init, don't apply settings

    _winCfg.UserSaveDir = SaveCustomDirSetup(_winCfg.DataDirectory, _hCustomSaveDirCheck, _hCustomSaveDir);
    _winCfg.AppDataDir = SaveCustomDirSetup(_winCfg.DataDirectory, _hCustomAppDataDirCheck, _hCustomAppDataDir);
}

//=============================================================================
//
// AccessibilityPageDialog
//
//=============================================================================

bool AccessibilityPageDialog::ShouldDisplayPage(const ConfigTree &cfg_in)
{
    // Test if at least one accessibility groups was not disabled in default config
    return !CfgReadBoolInt(cfg_in, "disabled", "access_skipstyle")
        ;
}

INT_PTR AccessibilityPageDialog::OnInitDialog()
{
    _hEnableAccess          = GetDlgItem(_hwnd, IDC_ACCESSENABLECHECK);
    _hSpeechSkipStyle       = GetDlgItem(_hwnd, IDC_SPEECHSKIPSTYLE);
    _hTextSkipStyle         = GetDlgItem(_hwnd, IDC_TEXTSKIPSTYLE);
    _hTextReadSpeed         = GetDlgItem(_hwnd, IDC_TEXTREADSPEED);
    _hTextReadSpeedText     = GetDlgItem(_hwnd, IDC_TEXTREADSPEED_TEXT);

    SetSliderRange(_hTextReadSpeed, TextReadSpeedMin, TextReadSpeedMax);

    const std::array<std::pair<const char*, SkipSpeechStyle>, 4> skip_vals = { {
        { "Game Default", kSkipSpeechNone }, { "Player Input", kSkipSpeech_AnyInput }, { "Auto (by time)", kSkipSpeechTime }, { "Any", kSkipSpeech_AnyInputOrTime }
    }};
    ResetContent(_hSpeechSkipStyle);
    ResetContent(_hTextSkipStyle);
    for (const auto &val : skip_vals)
    {
        AddString(_hSpeechSkipStyle, val.first, val.second);
        AddString(_hTextSkipStyle, val.first, val.second);
    }

    _disabledSkipStyle = CfgReadBoolInt(_cfgIn, "disabled", "access_skipstyle");
    // If all Accessibility options are disabled, then disable the "enable" checkbox too
    if (_disabledSkipStyle)
    {
        EnableWindow(_hEnableAccess, FALSE);
    }

    ResetSetup(_cfgIn);

    _isInit = true;
    return FALSE; // notify WinAPI that we set focus ourselves
}

INT_PTR AccessibilityPageDialog::OnDialogEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // First try the generic handlers in the base class
    if (WinDialog::OnDialogEvent(uMsg, wParam, lParam) == TRUE)
        return TRUE;

    // Handle any uncommon messages that do not have corresponding
    // methods in the WinDialog class
    switch (uMsg)
    {
    case WM_HSCROLL:
        if ((HWND)lParam == _hTextReadSpeed)
            UpdateTextReadSpeed();
        return TRUE;
    default:
        return FALSE;
    }
}

INT_PTR AccessibilityPageDialog::OnCommand(WORD id)
{
    switch (id)
    {
    case IDC_ACCESSENABLECHECK: OnEnableAccessCheck(); break;
    default:
        return FALSE;
    }
    return TRUE;
}

void AccessibilityPageDialog::OnEnableAccessCheck()
{
    const bool enable = GetCheck(_hEnableAccess);
    const bool enable_skipstyles = !_disabledSkipStyle && enable;
    EnableWindow(GetDlgItem(_hwnd, IDC_LABEL_SPEECHSKIPSTYLE), enable_skipstyles ? TRUE : FALSE);
    EnableWindow(GetDlgItem(_hwnd, IDC_LABEL_TEXTSKIPSTYLE), enable_skipstyles ? TRUE : FALSE);
    EnableWindow(GetDlgItem(_hwnd, IDC_LABEL_TEXTREADSPEED), enable_skipstyles ? TRUE : FALSE);
    EnableWindow(_hSpeechSkipStyle, enable_skipstyles ? TRUE : FALSE);
    EnableWindow(_hTextSkipStyle, enable_skipstyles ? TRUE : FALSE);
    EnableWindow(_hTextReadSpeed, enable_skipstyles ? TRUE : FALSE);
    EnableWindow(_hTextReadSpeedText, enable_skipstyles ? TRUE : FALSE);
}

void AccessibilityPageDialog::UpdateTextReadSpeed()
{
    int slider_pos = GetSliderPos(_hTextReadSpeed);
    String text = slider_pos == 0 ? "Game Default" : String::FromFormat("%d chars per sec", slider_pos);
    SetText(_hTextReadSpeedText, STR(text));
}

void AccessibilityPageDialog::ResetSetup(const ConfigTree &cfg_from)
{
    bool enable_access = CfgReadBoolInt(cfg_from, "winsetup", "access_page_on")
    // Also test if there's any option with non-default value
        || (_winCfg.Access.SpeechSkipStyle != kSkipSpeechNone)
        || (_winCfg.Access.TextSkipStyle != kSkipSpeechNone)
        || (_winCfg.Access.TextReadSpeed > 0)
        ;

    SetCheck(_hEnableAccess, enable_access ? TRUE : FALSE);
    OnEnableAccessCheck();

    SetCurSelToItemData(_hSpeechSkipStyle, _winCfg.Access.SpeechSkipStyle);
    SetCurSelToItemData(_hTextSkipStyle, _winCfg.Access.TextSkipStyle);
    int slider_pos = Math::Clamp(_winCfg.Access.TextReadSpeed, TextReadSpeedMin, TextReadSpeedMax);
    SetSliderPos(_hTextReadSpeed, slider_pos);
    UpdateTextReadSpeed();
}

void AccessibilityPageDialog::SaveSetup()
{
    if (!_isInit)
        return; // was not init, don't apply settings

    const bool enable = GetCheck(_hEnableAccess);
    CfgWriteBoolInt(_cfgOut, "winsetup", "access_page_on", enable);

    if (enable)
    {
        _winCfg.Access.SpeechSkipStyle = (SkipSpeechStyle)GetCurItemData(_hSpeechSkipStyle, kSkipSpeechNone);
        _winCfg.Access.TextSkipStyle = (SkipSpeechStyle)GetCurItemData(_hTextSkipStyle, kSkipSpeechNone);  
        _winCfg.Access.TextReadSpeed = GetSliderPos(_hTextReadSpeed);
    }
    else
    {
        _winCfg.Access.SpeechSkipStyle = kSkipSpeechNone;
        _winCfg.Access.TextSkipStyle = kSkipSpeechNone;
        _winCfg.Access.TextReadSpeed = 0;
    }
}

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS
