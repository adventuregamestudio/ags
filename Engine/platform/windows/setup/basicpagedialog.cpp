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
    DataDirectory = ".";
    GameResType = kGameResolution_Undefined;
    GameColourDepth = 0;
    LetterboxByDesign = false;

    GfxFilterId = "StdScale";
    GfxDriverId = "D3D9";
    FsSetup = WindowSetup(kWnd_FullDesktop);
    WinSetup = WindowSetup(kWnd_Windowed);
    FsGameFrame = kFrame_Proportional;
    WinGameFrame = kFrame_Round;
    RefreshRate = 0;
    Windowed = false;
    VSync = false;
    RenderAtScreenRes = false;
    AntialiasSprites = false;

    MouseAutoLock = false;
    MouseSpeed = 1.f;

    AudioEnabled = true;
    UseVoicePack = true;

    SpriteCacheSize = 1024 * 128;
    TextureCacheSize = 1024 * 128;
    SoundCacheSize = 1024 * 32;
    DefaultLanguageName = "Game Default";

    Title = "Game Setup";
}

void WinConfig::Load(const ConfigTree &cfg)
{
    DataDirectory = CfgReadString(cfg, "misc", "datadir", DataDirectory);
    UserSaveDir = CfgReadString(cfg, "misc", "user_data_dir");
    AppDataDir = CfgReadString(cfg, "misc", "shared_data_dir");
    // Backward-compatible resolution type
    GameResType = (GameResolutionType)CfgReadInt(cfg, "gameproperties", "legacy_resolution", GameResType);
    if (GameResType < kGameResolution_Undefined || GameResType >= kNumGameResolutions)
        GameResType = kGameResolution_Undefined;
    GameResolution.Width = CfgReadInt(cfg, "gameproperties", "resolution_width", GameResolution.Width);
    GameResolution.Height = CfgReadInt(cfg, "gameproperties", "resolution_height", GameResolution.Height);
    GameColourDepth = CfgReadInt(cfg, "gameproperties", "resolution_bpp", GameColourDepth);
    LetterboxByDesign = CfgReadBoolInt(cfg, "gameproperties", "legacy_letterbox", false);

    GfxDriverId = CfgReadString(cfg, "graphics", "driver", GfxDriverId);
    GfxFilterId = CfgReadString(cfg, "graphics", "filter", GfxFilterId);
    FsSetup = parse_window_mode(CfgReadString(cfg, "graphics", "fullscreen", "default"), false);
    WinSetup = parse_window_mode(CfgReadString(cfg, "graphics", "window", "default"), true);

    FsGameFrame = parse_scaling_option(CfgReadString(cfg, "graphics", "game_scale_fs"), FsGameFrame);
    WinGameFrame = parse_scaling_option(CfgReadString(cfg, "graphics", "game_scale_win"), WinGameFrame);

    RefreshRate = CfgReadInt(cfg, "graphics", "refresh", RefreshRate);
    Windowed = CfgReadBoolInt(cfg, "graphics", "windowed", Windowed);
    VSync = CfgReadBoolInt(cfg, "graphics", "vsync", VSync);
    int locked_render_at_screenres = CfgReadInt(cfg, "gameproperties", "render_at_screenres", -1);
    if (locked_render_at_screenres < 0)
        RenderAtScreenRes = CfgReadInt(cfg, "graphics", "render_at_screenres", RenderAtScreenRes ? 1 : 0) != 0;
    else
        RenderAtScreenRes = locked_render_at_screenres != 0;
    AntialiasSprites = CfgReadInt(cfg, "graphics", "antialias", AntialiasSprites ? 1 : 0) != 0;

    AudioEnabled = CfgReadBoolInt(cfg, "sound", "enabled", AudioEnabled);
    AudioDriverId = CfgReadString(cfg, "sound", "driver", AudioDriverId);
    UseVoicePack = CfgReadBoolInt(cfg, "sound", "usespeech", UseVoicePack);

    MouseAutoLock = CfgReadBoolInt(cfg, "mouse", "auto_lock", MouseAutoLock);
    MouseSpeed = CfgReadFloat(cfg, "mouse", "speed", 1.f);
    if (MouseSpeed <= 0.f)
        MouseSpeed = 1.f;

    SpriteCacheSize = CfgReadInt(cfg, "graphics", "sprite_cache_size", SpriteCacheSize);
    TextureCacheSize = CfgReadInt(cfg, "graphics", "texture_cache_size", TextureCacheSize);
    SoundCacheSize = CfgReadInt(cfg, "sound", "cache_size", SoundCacheSize);
    Language = CfgReadString(cfg, "language", "translation", Language);
    DefaultLanguageName = CfgReadString(cfg, "language", "default_translation_name", DefaultLanguageName);

    // Accessibility settings
    SpeechSkipStyle = parse_speechskip_style(CfgReadString(cfg, "access", "speechskip"), SpeechSkipStyle);
    TextSkipStyle = parse_speechskip_style(CfgReadString(cfg, "access", "textskip"), TextSkipStyle);

    Title = CfgReadString(cfg, "misc", "titletext", Title);
}

void WinConfig::Save(ConfigTree &cfg, const Size &desktop_res)
{
    CfgWriteString(cfg, "misc", "user_data_dir", UserSaveDir);
    CfgWriteString(cfg, "misc", "shared_data_dir", AppDataDir);

    CfgWriteString(cfg, "graphics", "driver", GfxDriverId);
    CfgWriteString(cfg, "graphics", "filter", GfxFilterId);
    CfgWriteString(cfg, "graphics", "fullscreen", make_window_mode_option(FsSetup, GameResolution, desktop_res));
    CfgWriteString(cfg, "graphics", "window", make_window_mode_option(WinSetup, GameResolution, desktop_res));
    CfgWriteString(cfg, "graphics", "game_scale_fs", make_scaling_option(FsGameFrame));
    CfgWriteString(cfg, "graphics", "game_scale_win", make_scaling_option(WinGameFrame));
    CfgWriteInt(cfg, "graphics", "refresh", RefreshRate);
    CfgWriteInt(cfg, "graphics", "windowed", Windowed ? 1 : 0);
    CfgWriteInt(cfg, "graphics", "vsync", VSync ? 1 : 0);
    CfgWriteInt(cfg, "graphics", "render_at_screenres", RenderAtScreenRes ? 1 : 0);
    CfgWriteInt(cfg, "graphics", "antialias", AntialiasSprites ? 1 : 0);

    CfgWriteInt(cfg, "sound", "enabled", AudioEnabled ? 1 : 0);
    CfgWriteString(cfg, "sound", "driver", AudioDriverId);
    CfgWriteInt(cfg, "sound", "usespeech", UseVoicePack ? 1 : 0);

    CfgWriteInt(cfg, "mouse", "auto_lock", MouseAutoLock ? 1 : 0);
    CfgWriteFloat(cfg, "mouse", "speed", MouseSpeed, 1);

    CfgWriteInt(cfg, "graphics", "sprite_cache_size", SpriteCacheSize);
    CfgWriteInt(cfg, "graphics", "texture_cache_size", TextureCacheSize);
    CfgWriteInt(cfg, "sound", "cache_size", SoundCacheSize);
    CfgWriteString(cfg, "language", "translation", Language);

    CfgWriteString(cfg, "access", "speechskip", make_speechskip_option(SpeechSkipStyle));
    CfgWriteString(cfg, "access", "textskip", make_speechskip_option(TextSkipStyle));
}

//=============================================================================
//
// BasicPageDialog
//
//=============================================================================

INT_PTR BasicPageDialog::OnInitDialog()
{
    _hGameResolutionText    = GetDlgItem(_hwnd, IDC_RESOLUTION);
    _hWindowed              = GetDlgItem(_hwnd, IDC_WINDOWED);
    _hGfxDriverList         = GetDlgItem(_hwnd, IDC_GFXDRIVER);
    _hGfxModeList           = GetDlgItem(_hwnd, IDC_GFXMODE);
    _hFullscreenDesktop     = GetDlgItem(_hwnd, IDC_FULLSCREENDESKTOP);
    _hFsScalingList         = GetDlgItem(_hwnd, IDC_FSSCALING);
    _hWinScalingList        = GetDlgItem(_hwnd, IDC_WINDOWSCALING);
    _hGfxFilterList         = GetDlgItem(_hwnd, IDC_GFXFILTER);
    _hLanguageList          = GetDlgItem(_hwnd, IDC_LANGUAGE);

    _desktopSize = get_desktop_size();
    _maxWindowSize = AGSPlatformDriver::GetDriver()->ValidateWindowSize(_desktopSize, false);

    // Resolution controls
    if (_winCfg.GameResolution.IsNull())
        _winCfg.GameResolution = ResolutionTypeToSize(_winCfg.GameResType, _winCfg.LetterboxByDesign);

    SetText(_hGameResolutionText, STR(String::FromFormat("Native game resolution: %d x %d x %d",
        _winCfg.GameResolution.Width, _winCfg.GameResolution.Height, _winCfg.GameColourDepth)));

    FillGfxDriverList();
    SetCheck(_hFullscreenDesktop, _winCfg.FsSetup.Mode == kWnd_FullDesktop);
    EnableWindow(_hGfxModeList, _winCfg.FsSetup.Mode == kWnd_Fullscreen);
    OnGfxDriverUpdate();

    SetCheck(_hWindowed, _winCfg.Windowed);

    FillScalingList(_hFsScalingList, _winCfg.FsSetup, _winCfg.FsGameFrame, false);
    FillScalingList(_hWinScalingList, _winCfg.WinSetup, _winCfg.WinGameFrame, true);
    OnFullScalingUpdate();
    OnWinScalingUpdate();

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

void BasicPageDialog::OnGfxDriverUpdate()
{
    _winCfg.GfxDriverId = (LPCSTR)GetCurItemData(_hGfxDriverList);

    DriverDescMap::const_iterator it = _drvDescMap.find(_winCfg.GfxDriverId);
    if (it != _drvDescMap.end())
        _drvDesc = it->second;
    else
        _drvDesc.reset();

    FillGfxModeList();
    FillGfxFilterList();
}

void BasicPageDialog::OnGfxFilterUpdate()
{
    _winCfg.GfxFilterId = (LPCSTR)GetCurItemData(_hGfxFilterList);

    _gfxFilterInfo = GfxFilterInfo();
    for (size_t i = 0; i < _drvDesc->FilterList.size(); ++i)
    {
        if (_drvDesc->FilterList[i].Id.CompareNoCase(_winCfg.GfxFilterId) == 0)
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
        _winCfg.FsSetup = WindowSetup(_desktopSize, kWnd_Fullscreen); break;
    case static_cast<DWORD_PTR>(kGfxMode_GameRes):
        _winCfg.FsSetup = WindowSetup(_winCfg.GameResolution, kWnd_Fullscreen); break;
    default:
        {
            const DisplayMode &mode = _drvDesc->GfxModeList.Modes[sel];
            _winCfg.FsSetup = WindowSetup(Size(mode.Width, mode.Height), kWnd_Fullscreen);
        }
    }
}

void BasicPageDialog::OnFullScalingUpdate()
{
    _winCfg.FsGameFrame = (FrameScaleDef)GetCurItemData(_hFsScalingList);
}

void BasicPageDialog::OnWinScalingUpdate()
{
    int scale_type = GetCurItemData(_hWinScalingList);
    if (scale_type < kNumFrameScaleDef)
    { // if one of three main scaling types, then set up max window with that scaling
        _winCfg.WinGameFrame = (FrameScaleDef)scale_type;
        _winCfg.WinSetup = WindowSetup(kWnd_Windowed);
    }
    else
    { // ...otherwise, it's round scaling multiplier (shifted by kNumFrameScaleDef)
        _winCfg.WinGameFrame = kFrame_Round;
        _winCfg.WinSetup = WindowSetup(scale_type - kNumFrameScaleDef, kWnd_Windowed);
    }
}

void BasicPageDialog::OnWindowedUpdate()
{
    _winCfg.Windowed = GetCheck(_hWindowed);
}

void BasicPageDialog::OnFullscreenDesktop()
{
    if (GetCheck(_hFullscreenDesktop))
    {
        EnableWindow(_hGfxModeList, FALSE);
        _winCfg.FsSetup = WindowSetup(kWnd_FullDesktop);
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

    for (DriverDescMap::const_iterator it = _drvDescMap.begin(); it != _drvDescMap.end(); ++it)
        AddString(_hGfxDriverList, STR(it->second->UserName), (DWORD_PTR)it->second->Id.GetCStr());
    SetCurSelToItemDataStr(_hGfxDriverList, _winCfg.GfxDriverId.GetCStr(), 0);
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

    SetCurSelToItemDataStr(_hGfxFilterList, STR(_winCfg.GfxFilterId), 0);
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
        if (mode->Width == _desktopSize.Width && mode->Height == _desktopSize.Height)
            has_desktop_mode = true;
        else if (mode->Width == _winCfg.GameResolution.Width && mode->Height == _winCfg.GameResolution.Height)
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

    SelectNearestGfxMode(_winCfg.FsSetup);
}

void BasicPageDialog::FillLanguageList()
{
    ResetContent(_hLanguageList);
    AddString(_hLanguageList, _winCfg.DefaultLanguageName.GetCStr());
    SetCurSel(_hLanguageList, 0);

    bool found_sel = false;
    for (FindFile ff = FindFile::OpenFiles(_winCfg.DataDirectory, "*.tra"); !ff.AtEnd(); ff.Next())
    {
        String filename = Path::RemoveExtension(Path::GetFilename(ff.Current()));
        filename.SetAt(0, toupper(filename[0]));
        int index = AddString(_hLanguageList, STR(filename));
        if (!found_sel && _winCfg.Language.CompareNoCase(filename) == 0)
        {
            SetCurSel(_hLanguageList, index);
            found_sel = true;
        }
    }
}

void BasicPageDialog::FillScalingList(HWND hlist, const WindowSetup &ws, const FrameScaleDef frame, bool windowed)
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

    if (windowed && ws.Scale > 0)
        SetCurSelToItemData(hlist, ws.Scale + kNumFrameScaleDef, NULL, 0);
    else
        SetCurSelToItemData(hlist, frame, NULL, 0);

    EnableWindow(hlist, SendMessage(hlist, CB_GETCOUNT, 0, 0) > 1 ? TRUE : FALSE);
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

    IGfxModeList *gfxm_list = gfx_driver->GetSupportedModeList(drv_desc->UseColorDepth);
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
    if (ws.IsDefaultSize())
    {
        SetCurSelToItemData(_hGfxModeList, static_cast<DWORD_PTR>(kGfxMode_Desktop));
    }
    else if (ws.Size == _winCfg.GameResolution || ws.Scale == 1)
    {
        SetCurSelToItemData(_hGfxModeList, static_cast<DWORD_PTR>(kGfxMode_GameRes));
    }
    else
    {
        // Look up for the nearest supported mode
        const Size screen_size = !ws.Size.IsNull() ? ws.Size : (_winCfg.GameResolution * ws.Scale);
        int index = -1;
        DisplayMode dm;
        if (find_nearest_supported_mode(_drvDesc->GfxModeList, screen_size, _drvDesc->UseColorDepth,
                                        NULL, NULL, dm, &index))
        {
            SetCurSelToItemData(_hGfxModeList, index, NULL, kGfxMode_Desktop);
        }
        else
            SetCurSelToItemData(_hGfxModeList, static_cast<DWORD_PTR>(kGfxMode_Desktop));
    }
    OnGfxModeUpdate();
}

void BasicPageDialog::SaveSetup()
{
    if (GetCurSel(_hLanguageList) == 0)
        _winCfg.Language.Empty();
    else
        _winCfg.Language = GetText(_hLanguageList);
    _winCfg.GfxFilterId = (LPCSTR)GetCurItemData(_hGfxFilterList);
}

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS
