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

#include <set>
#include "gfx/gfxdriverfactory.h"
#include "gfx/gfxfilter.h"
#include "gfx/graphicsdriver.h"
#include "main/config.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "platform/windows/setup/basicpagedialog.h"
#include "platform/windows/setup/winapihelpers.h"
#include "resource/resource.h"
#include "util/directory.h"

namespace AGS
{
namespace Engine
{

//=============================================================================
//
// WinConfig struct, keeps all configurable data.
//
//=============================================================================

WinConfig::WinConfig()
{
    SetDefaults();
}

void WinConfig::SetDefaults()
{
    Title = "Game Setup";
    VersionString = "";

    DataDirectory = ".";
    GameResType = kGameResolution_Undefined;
    GameResolution = Size();
    GameColourDepth = 0;
    LetterboxByDesign = false;

    Display.Filter.ID = "StdScale";
    Display.DriverID = "D3D9";
    Display.FsSetup = WindowSetup(kWnd_FullDesktop);
    Display.WinSetup = WindowSetup(kWnd_Windowed);
    Display.FsGameFrame = kFrame_Proportional;
    Display.WinGameFrame = kFrame_Round;

    AudioEnabled = true;
    UseVoicePack = true;

    DefaultLanguageName = "Game Default";
    Translation = "";

    UserSaveDir = ".";
    AppDataDir = ".";
}

void WinConfig::LoadMeta(const ConfigTree &cfg)
{
    // Backward-compatible resolution type
    GameResType = (GameResolutionType)CfgReadInt(cfg, "gameproperties", "legacy_resolution", GameResType);
    if (GameResType < kGameResolution_Undefined || GameResType >= kNumGameResolutions)
        GameResType = kGameResolution_Undefined;
    GameResolution.Width = CfgReadInt(cfg, "gameproperties", "resolution_width", GameResolution.Width);
    GameResolution.Height = CfgReadInt(cfg, "gameproperties", "resolution_height", GameResolution.Height);
    GameColourDepth = CfgReadInt(cfg, "gameproperties", "resolution_bpp", GameColourDepth);
    LetterboxByDesign = CfgReadBoolInt(cfg, "gameproperties", "legacy_letterbox", false);

    // Setup program meta
    Title = CfgReadString(cfg, "misc", "titletext", Title);
    DefaultLanguageName = CfgReadString(cfg, "language", "default_translation_name", DefaultLanguageName);
}

void WinConfig::LoadCommon(const ConfigTree &cfg)
{
    load_common_config(cfg, *this);
}

void WinConfig::Save(ConfigTree &cfg) const
{
    save_common_config(*this, cfg);
}

//=============================================================================
//
// BasicPageDialog
//
//=============================================================================

INT_PTR BasicPageDialog::OnInitDialog()
{
    _hGameResolutionText    = GetDlgItem(_hwnd, IDC_RESOLUTION);
    _hDisplayList           = GetDlgItem(_hwnd, IDC_DISPLAYINDEX);
    _hWindowed              = GetDlgItem(_hwnd, IDC_WINDOWED);
    _hGfxDriverList         = GetDlgItem(_hwnd, IDC_GFXDRIVER);
    _hGfxModeList           = GetDlgItem(_hwnd, IDC_GFXMODE);
    _hFullscreenDesktop     = GetDlgItem(_hwnd, IDC_FULLSCREENDESKTOP);
    _hFsScalingList         = GetDlgItem(_hwnd, IDC_FSSCALING);
    _hWinScalingList        = GetDlgItem(_hwnd, IDC_WINDOWSCALING);
    _hGfxFilterList         = GetDlgItem(_hwnd, IDC_GFXFILTER);
    _hLanguageList          = GetDlgItem(_hwnd, IDC_LANGUAGE);

    _displayIndex = _winCfg.Display.DisplayIndex;
    _desktopSize = get_desktop_size(_displayIndex);
    _maxWindowSize = AGSPlatformDriver::GetDriver()->ValidateWindowSize(_displayIndex, _desktopSize, false);

    // Resolution controls
    if (_winCfg.GameResolution.IsNull())
        _winCfg.GameResolution = ResolutionTypeToSize(_winCfg.GameResType, _winCfg.LetterboxByDesign);

    SetText(_hGameResolutionText, STR(String::FromFormat("Native game resolution: %d x %d x %d",
        _winCfg.GameResolution.Width, _winCfg.GameResolution.Height, _winCfg.GameColourDepth)));

    FillDisplayList();
    FillGfxDriverList();
    FillScalingList(_hFsScalingList, false);
    FillScalingList(_hWinScalingList, true);

    FillLanguageList();

    if (CfgReadBoolInt(_cfgIn, "disabled", "gfxdrivers"))
        EnableWindow(_hGfxDriverList, FALSE);
    if (CfgReadBoolInt(_cfgIn, "disabled", "filters"))
        EnableWindow(_hGfxFilterList, FALSE);
    if (CfgReadBoolInt(_cfgIn, "disabled", "translation"))
        EnableWindow(_hLanguageList, FALSE);
    if (CfgReadBoolInt(_cfgIn, "disabled", "fullscreen"))
    {
        EnableWindow(_hFullscreenDesktop, FALSE);
        EnableWindow(_hGfxModeList, FALSE);
    }

    ResetSetup(_cfgIn);

    _isInit = true;
    return FALSE; // notify WinAPI that we set focus ourselves
}

INT_PTR BasicPageDialog::OnCommand(WORD id)
{
    switch (id)
    {
    case IDC_WINDOWED:  OnWindowedUpdate(); break;
    case IDC_FULLSCREENDESKTOP: OnFullscreenDesktop(); break;
    default:
        return FALSE;
    }
    return TRUE;
}

INT_PTR BasicPageDialog::OnListSelection(WORD id)
{
    switch (id)
    {
    case IDC_DISPLAYINDEX: OnDisplayUpdate(); break;
    case IDC_GFXDRIVER: OnGfxDriverUpdate(); break;
    case IDC_GFXFILTER: OnGfxFilterUpdate(); break;
    case IDC_GFXMODE:   OnGfxModeUpdate(); break;
    case IDC_FSSCALING: OnFullScalingUpdate(); break;
    case IDC_WINDOWSCALING: OnWinScalingUpdate(); break;
    default:
        return FALSE;
    }
    return TRUE;
}

void BasicPageDialog::OnDisplayUpdate()
{
    _displayIndex = GetRealDisplayIndex();
    FillGfxModeList();
}

void BasicPageDialog::OnGfxDriverUpdate()
{
    _winCfg.Display.DriverID = (LPCSTR)GetCurItemData(_hGfxDriverList);

    DriverDescMap::const_iterator it = _drvDescMap.find(_winCfg.Display.DriverID);
    if (it != _drvDescMap.end())
        _drvDesc = it->second;
    else
        _drvDesc.reset();

    FillGfxModeList();
    FillGfxFilterList();
}

void BasicPageDialog::OnGfxFilterUpdate()
{
    _winCfg.Display.Filter.ID = (LPCSTR)GetCurItemData(_hGfxFilterList);

    _gfxFilterInfo = GfxFilterInfo();
    for (size_t i = 0; i < _drvDesc->FilterList.size(); ++i)
    {
        if (_drvDesc->FilterList[i].Id.CompareNoCase(_winCfg.Display.Filter.ID) == 0)
        {
            _gfxFilterInfo = _drvDesc->FilterList[i];
            break;
        }
    }
}

void BasicPageDialog::OnGfxModeUpdate()
{
    if (GetCheck(_hFullscreenDesktop))
        return;

    DWORD_PTR sel = GetCurItemData(_hGfxModeList);
    switch (sel)
    {
    case static_cast<DWORD_PTR>(kGfxMode_Desktop):
        _winCfg.Display.FsSetup = WindowSetup(kWndSizeHint_Desktop, _desktopSize, kWnd_Fullscreen); break;
    case static_cast<DWORD_PTR>(kGfxMode_GameRes):
        _winCfg.Display.FsSetup = WindowSetup(kWndSizeHint_GameNative, _winCfg.GameResolution, kWnd_Fullscreen); break;
    default:
        {
            const DisplayMode &mode = _drvDesc->GfxModeList.Modes[sel];
            _winCfg.Display.FsSetup = WindowSetup(Size(mode.Width, mode.Height), kWnd_Fullscreen);
        }
    }
}

void BasicPageDialog::OnFullScalingUpdate()
{
    _winCfg.Display.FsGameFrame = (FrameScaleDef)GetCurItemData(_hFsScalingList);
}

void BasicPageDialog::OnWinScalingUpdate()
{
    int scale_type = GetCurItemData(_hWinScalingList);
    if (scale_type < kNumFrameScaleDef)
    { // if one of three main scaling types, then set up max window with that scaling
        _winCfg.Display.WinGameFrame = (FrameScaleDef)scale_type;
        _winCfg.Display.WinSetup = WindowSetup(kWnd_Windowed);
    }
    else
    { // ...otherwise, it's round scaling multiplier (shifted by kNumFrameScaleDef)
        _winCfg.Display.WinGameFrame = kFrame_Round;
        _winCfg.Display.WinSetup = WindowSetup(scale_type - kNumFrameScaleDef, kWnd_Windowed);
    }
}

void BasicPageDialog::OnWindowedUpdate()
{
    _winCfg.Display.Windowed = GetCheck(_hWindowed);
}

void BasicPageDialog::OnFullscreenDesktop()
{
    if (GetCheck(_hFullscreenDesktop))
    {
        EnableWindow(_hGfxModeList, FALSE);
        _winCfg.Display.FsSetup = WindowSetup(kWnd_FullDesktop);
    }
    else
    {
        EnableWindow(_hGfxModeList, TRUE);
        OnGfxModeUpdate();
    }
}

int BasicPageDialog::GfxModes::GetModeCount() const
{
    return Modes.size();
}

bool BasicPageDialog::GfxModes::GetMode(int index, DisplayMode &mode) const
{
    if (index >= 0 && (size_t)index < Modes.size())
    {
        mode = Modes[index];
        return true;
    }
    return false;
}

void BasicPageDialog::FillDisplayList()
{
    ResetContent(_hDisplayList);
    AddString(_hDisplayList, "Default");
    int display_count = SDL_GetNumVideoDisplays();
    for (int i = 0; i < display_count; ++i)
    {
        const char *disp_name = SDL_GetDisplayName(i);
        String item_name = String::FromFormat("%d : %s", i + 1, disp_name ? disp_name : "(unknown)");
        AddString(_hDisplayList, STR(item_name));
    }

    if (display_count <= 1)
    {
        EnableWindow(_hDisplayList, FALSE);
    }
}

void BasicPageDialog::AddScalingString(HWND hlist, int scaling_factor)
{
    String s;
    if (scaling_factor >= 0)
        s = String::FromFormat("x%d", scaling_factor);
    else
        s = String::FromFormat("1/%d", -scaling_factor);
    AddString(hlist, STR(s), (DWORD_PTR)(scaling_factor >= 0 ? scaling_factor + kNumFrameScaleDef : scaling_factor));
}

void BasicPageDialog::FillGfxDriverList()
{
    std::vector<String> gfx_drv_names;
    GetGfxDriverFactoryNames(gfx_drv_names);
    for (const auto &drvname : gfx_drv_names)
    {
        String item = CfgFindKey(_cfgIn, "disabled", drvname, true);
        if (item.IsEmpty() || !CfgReadBoolInt(_cfgIn, "disabled", item))
            InitDriverDescFromFactory(drvname);
    }

    if (_drvDescMap.size() == 0)
        MessageBox(_hwnd, "Unable to detect any supported graphic drivers!", "Initialization error", MB_OK | MB_ICONERROR);

    ResetContent(_hGfxDriverList);
    for (DriverDescMap::const_iterator it = _drvDescMap.begin(); it != _drvDescMap.end(); ++it)
        AddString(_hGfxDriverList, STR(it->second->UserName), (DWORD_PTR)it->second->Id.GetCStr());
}

void BasicPageDialog::FillGfxFilterList()
{
    ResetContent(_hGfxFilterList);

    if (!_drvDesc)
    {
        _gfxFilterInfo = GfxFilterInfo();
        return;
    }

    for (size_t i = 0; i < _drvDesc->FilterList.size(); ++i)
    {
        const GfxFilterInfo &info = _drvDesc->FilterList[i];
        String item = CfgFindKey(_cfgIn, "disabled", info.Id, true);
        if (item.IsEmpty() || !CfgReadBoolInt(_cfgIn, "disabled", item))
            AddString(_hGfxFilterList, STR(info.Name), (DWORD_PTR)info.Id.GetCStr());
    }

    SetCurSelToItemDataStr(_hGfxFilterList, STR(_winCfg.Display.Filter.ID), 0);
    OnGfxFilterUpdate();
}

void BasicPageDialog::FillGfxModeList()
{
    ResetContent(_hGfxModeList);

    if (!_drvDesc)
    {
        OnGfxModeUpdate();
        return;
    }

    const VDispModes &modes = _drvDesc->GfxModeList.Modes;
    bool has_desktop_mode = false;
    bool has_native_mode = false;
    String buf;
    for (VDispModes::const_iterator mode = modes.begin(); mode != modes.end(); ++mode)
    {
        if (*mode == _desktopSize)
            has_desktop_mode = true;
        if (*mode == _winCfg.GameResolution)
            has_native_mode = true;
        buf.Format("%d x %d", mode->Width, mode->Height);
        AddString(_hGfxModeList, STR(buf), (DWORD_PTR)(mode - modes.begin()));
    }

    int spec_mode_idx = 0;
    if (has_desktop_mode)
        InsertString(_hGfxModeList, STR(String::FromFormat("Desktop resolution (%d x %d)",
            _desktopSize.Width, _desktopSize.Height)), spec_mode_idx++, (DWORD_PTR)kGfxMode_Desktop);
    if (has_native_mode)
        InsertString(_hGfxModeList, STR(String::FromFormat("Native game resolution (%d x %d)",
            _winCfg.GameResolution.Width, _winCfg.GameResolution.Height)), spec_mode_idx++, (DWORD_PTR)kGfxMode_GameRes);

    SelectNearestGfxMode(_winCfg.Display.FsSetup);
}

void BasicPageDialog::FillLanguageList()
{
    ResetContent(_hLanguageList);
    AddString(_hLanguageList, _winCfg.DefaultLanguageName.GetCStr());

    for (FindFile ff = FindFile::OpenFiles(_winCfg.DataDirectory, "*.tra"); !ff.AtEnd(); ff.Next())
    {
        String filename = Path::RemoveExtension(Path::GetFilename(ff.Current()));
        filename.SetAt(0, toupper(filename[0]));
        int index = AddString(_hLanguageList, STR(filename));
    }

    SetCurSel(_hLanguageList, 0);
}

void BasicPageDialog::FillScalingList(HWND hlist, bool windowed)
{
    ResetContent(hlist);

    if (windowed)
        AddString(hlist, "None (original game size)", 1 + kNumFrameScaleDef);

    AddString(hlist, "Max round multiplier", kFrame_Round);
    AddString(hlist, "Fill whole screen", kFrame_Stretch);
    AddString(hlist, "Stretch, preserving aspect ratio", kFrame_Proportional);

    // Add integer multipliers for the windowed scaling list
    if (windowed && !_winCfg.GameResolution.IsNull())
    {
        const Size max_size = _maxWindowSize;
        const int max_scale = std::max(1, 
            _winCfg.GameResolution.IsNull() ? 1 :
            (std::min(max_size.Width / _winCfg.GameResolution.Width, max_size.Height / _winCfg.GameResolution.Height)));
        for (int scale = 2; scale <= max_scale; ++scale)
            AddScalingString(hlist, scale);
    }
}

int BasicPageDialog::GetRealDisplayIndex()
{
    const int user_display_sel = GetCurSel(_hDisplayList);
    return user_display_sel == 0 ? 0 : user_display_sel - 1;
}

void BasicPageDialog::SetScalingSelection()
{
    SetCurSelToItemData(_hFsScalingList, _winCfg.Display.FsGameFrame, NULL, 0);

    if (_winCfg.Display.WinSetup.Scale > 0)
        SetCurSelToItemData(_hWinScalingList, _winCfg.Display.WinSetup.Scale + kNumFrameScaleDef, NULL, 0);
    else
        SetCurSelToItemData(_hWinScalingList, _winCfg.Display.WinGameFrame, NULL, 0);

    OnFullScalingUpdate();
    OnWinScalingUpdate();
}

// "Less" predicate that compares two display modes only by their screen metrics
bool SizeLess(const DisplayMode &first, const DisplayMode &second)
{
    return Size(first.Width, first.Height) < Size(second.Width, second.Height);
}

void BasicPageDialog::InitDriverDescFromFactory(const String &id)
{
    IGfxDriverFactory *gfx_factory = GetGfxDriverFactory(id);
    if (!gfx_factory)
        return;
    IGraphicsDriver *gfx_driver = gfx_factory->GetDriver();
    if (!gfx_driver)
    {
        gfx_factory->Shutdown();
        return;
    }

    PDriverDesc drv_desc(new DriverDesc());
    drv_desc->Id = gfx_driver->GetDriverID();
    drv_desc->UserName = gfx_driver->GetDriverName();
    drv_desc->UseColorDepth =
        gfx_driver->GetDisplayDepthForNativeDepth(_winCfg.GameColourDepth ? _winCfg.GameColourDepth : 32);

    IGfxModeList *gfxm_list = gfx_driver->GetSupportedModeList(_displayIndex, drv_desc->UseColorDepth);
    VDispModes &modes = drv_desc->GfxModeList.Modes;
    if (gfxm_list)
    {
        std::set<Size> unique_sizes; // trying to hide modes which only have different refresh rates
        for (int i = 0; i < gfxm_list->GetModeCount(); ++i)
        {
            DisplayMode mode;
            gfxm_list->GetMode(i, mode);
            if (mode.ColorDepth != drv_desc->UseColorDepth || unique_sizes.count(Size(mode.Width, mode.Height)) != 0)
                continue;
            unique_sizes.insert(Size(mode.Width, mode.Height));
            modes.push_back(mode);
        }
        std::sort(modes.begin(), modes.end(), SizeLess);
        delete gfxm_list;
    }
    if (modes.size() == 0)
    {
        // Add two default modes in hope that engine will be able to handle them (or fallbacks to something else)
        modes.push_back(DisplayMode(GraphicResolution(_desktopSize.Width, _desktopSize.Height, drv_desc->UseColorDepth)));
        modes.push_back(DisplayMode(GraphicResolution(_winCfg.GameResolution.Width, _winCfg.GameResolution.Height, drv_desc->UseColorDepth)));
    }

    drv_desc->FilterList.resize(gfx_factory->GetFilterCount());
    for (size_t i = 0; i < drv_desc->FilterList.size(); ++i)
    {
        drv_desc->FilterList[i] = *gfx_factory->GetFilterInfo(i);
    }

    gfx_factory->Shutdown();
    _drvDescMap[drv_desc->Id] = drv_desc;
}

void BasicPageDialog::SelectNearestGfxMode(const WindowSetup &ws)
{
    if (!_drvDesc)
    {
        OnGfxModeUpdate();
        return;
    }

    // First check two special modes
    int res = -1;
    if (ws.IsDefaultSize() || ws.SizeHint == kWndSizeHint_Desktop)
    {
        res = SetCurSelToItemData(_hGfxModeList, static_cast<DWORD_PTR>(kGfxMode_Desktop));
    }
    else if (ws.SizeHint == kWndSizeHint_GameNative || ws.Scale == 1)
    {
        res = SetCurSelToItemData(_hGfxModeList, static_cast<DWORD_PTR>(kGfxMode_GameRes));
    }

    // If above failed, then look up for the nearest supported mode
    if (res < 0)
    {
        const Size screen_size = !ws.Size.IsNull() ? ws.Size : (_winCfg.GameResolution * ws.Scale);
        int index = -1;
        DisplayMode dm;
        if (find_nearest_supported_mode(_drvDesc->GfxModeList, screen_size, _drvDesc->UseColorDepth,
                                        NULL, NULL, dm, &index))
        {
            SetCurSelToItemData(_hGfxModeList, index, NULL, kGfxMode_Desktop);
        }
        else
        {
            SetCurSelToItemData(_hGfxModeList, static_cast<DWORD_PTR>(kGfxMode_Desktop));
        }
    }

    OnGfxModeUpdate();
}

void BasicPageDialog::ResetSetup(const ConfigTree & /*cfg_from*/)
{
    SetCurSelToItemDataStr(_hGfxDriverList, _winCfg.Display.DriverID.GetCStr(), 0);
    SetCurSel(_hDisplayList, _winCfg.Display.UseDefaultDisplay ? 0 : _winCfg.Display.DisplayIndex + 1);
    SetCheck(_hFullscreenDesktop, _winCfg.Display.FsSetup.Mode == kWnd_FullDesktop);
    EnableWindow(_hGfxModeList, _winCfg.Display.FsSetup.Mode == kWnd_Fullscreen);
    OnGfxDriverUpdate();
    SetCheck(_hWindowed, _winCfg.Display.Windowed);
    SetScalingSelection();
    SetCurSelToItemDataStr(_hLanguageList, STR(_winCfg.Translation), 0);
}

void BasicPageDialog::SaveSetup()
{
    if (!_isInit)
        return; // was not init, don't apply settings

    const int user_display_index = GetCurSel(_hDisplayList);
    _winCfg.Display.UseDefaultDisplay = user_display_index == 0;
    _winCfg.Display.DisplayIndex = user_display_index == 0 ? 0 : user_display_index - 1;
    if (GetCurSel(_hLanguageList) == 0)
        _winCfg.Translation.Empty();
    else
        _winCfg.Translation = GetText(_hLanguageList);
    _winCfg.Display.Filter.ID = (LPCSTR)GetCurItemData(_hGfxFilterList);
}

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS
