//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#include "core/platform.h"

#if AGS_PLATFORM_OS_WINDOWS
#include "platform/windows/windows.h"
#include <commctrl.h>
#include <crtdbg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <memory>
#include <algorithm>
#include <array>
#include <set>
#include <vector>
#include "ac/gamestructdefines.h"
#include "gfx/gfxdriverfactory.h"
#include "gfx/gfxfilter.h"
#include "gfx/graphicsdriver.h"
#include "main/config.h"
#include "main/graphics_mode.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
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

using namespace AGS::Common;

inline LPCSTR STR(const String &str) { return str.GetCStr(); }

//=============================================================================
//
// WinConfig struct, keeps all configurable data.
//
//=============================================================================
struct WinConfig
{
    String Title;
    String VersionString;

    String DataDirectory;
    String UserSaveDir;
    String AppDataDir;
    GameResolutionType GameResType;
    Size   GameResolution;
    int    GameColourDepth;

    String GfxDriverId;
    String GfxFilterId;
    WindowSetup FsSetup;
    WindowSetup WinSetup;
    FrameScaleDef FsGameFrame;
    FrameScaleDef WinGameFrame;
    int    RefreshRate;
    bool   Windowed;
    bool   VSync;
    bool   RenderAtScreenRes;
    bool   AntialiasSprites;

    bool   AudioEnabled;
    String AudioDriverId;
    bool   UseVoicePack;

    bool   MouseAutoLock;
    float  MouseSpeed;

    int    SpriteCacheSize;
    int    TextureCacheSize;
    int    SoundCacheSize;
    String DefaultLanguageName;
    String Language;

    WinConfig();
    void SetDefaults();
    void Load(const ConfigTree &cfg);
    void Save(ConfigTree &cfg, const Size &desktop_res);
};

WinConfig::WinConfig()
{
    SetDefaults();
}

void WinConfig::SetDefaults()
{
    DataDirectory = ".";
    GameResType = kGameResolution_Undefined;
    GameColourDepth = 0;

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
    GameResolution.Width = CfgReadInt(cfg, "gameproperties", "resolution_width", GameResolution.Width);
    GameResolution.Height = CfgReadInt(cfg, "gameproperties", "resolution_height", GameResolution.Height);
    GameColourDepth = CfgReadInt(cfg, "gameproperties", "resolution_bpp", GameColourDepth);

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
}


//=============================================================================
//
// WinAPI interaction helpers
//
//=============================================================================

int AddString(HWND hwnd, LPCWSTR text, DWORD_PTR data = 0L)
{
    int index = SendMessageW(hwnd, CB_ADDSTRING, 0, (LPARAM)text);
    if (index >= 0)
        SendMessageW(hwnd, CB_SETITEMDATA, index, data);
    return index;
}

int AddString(HWND hwnd, LPCSTR text, DWORD_PTR data = 0L)
{
    WCHAR wstr[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wstr, MAX_PATH_SZ);
    return AddString(hwnd, wstr, data);
}

int InsertString(HWND hwnd, LPCWSTR text, int at_index, DWORD_PTR data = 0L)
{
    int index = SendMessageW(hwnd, CB_INSERTSTRING, at_index, (LPARAM)text);
    if (index >= 0)
        SendMessageW(hwnd, CB_SETITEMDATA, index, data);
    return index;
}

int InsertString(HWND hwnd, LPCSTR text, int at_index, DWORD_PTR data = 0L)
{
    WCHAR wstr[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wstr, MAX_PATH_SZ);
    return InsertString(hwnd, wstr, at_index, data);
}

int GetItemCount(HWND hwnd)
{
    return SendMessage(hwnd, CB_GETCOUNT, 0, 0L);
}

bool GetCheck(HWND hwnd)
{
    return SendMessage(hwnd, BM_GETCHECK, 0, 0) != FALSE;
}

void SetCheck(HWND hwnd, bool check)
{
    SendMessage(hwnd, BM_SETCHECK, check ? BST_CHECKED : BST_UNCHECKED, 0);
}

int GetCurSel(HWND hwnd)
{
    return SendMessage(hwnd, CB_GETCURSEL, 0, 0);
}

void SetCurSel(HWND hwnd, int cur_sel)
{
    SendMessage(hwnd, CB_SETCURSEL, cur_sel, 0);
}

typedef bool (*PfnCompareCBItemData)(DWORD_PTR data1, DWORD_PTR data2);

bool CmpICBItemDataAsStr(DWORD_PTR data1, DWORD_PTR data2)
{
    LPCSTR text_ptr1 = (LPCSTR)data1;
    LPCSTR text_ptr2 = (LPCSTR)data2;
    return text_ptr1 && text_ptr2 && StrCmpA(text_ptr1, text_ptr2) == 0 || !text_ptr1 && !text_ptr2;
}

int SetCurSelToItemData(HWND hwnd, DWORD_PTR data, PfnCompareCBItemData pfn_cmp = NULL, int def_sel = -1)
{
    int count = SendMessage(hwnd, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; ++i)
    {
        DWORD_PTR item_data = SendMessage(hwnd, CB_GETITEMDATA, i, 0);
        if (pfn_cmp && pfn_cmp(item_data, data) || !pfn_cmp && item_data == data)
        {
            LRESULT res = SendMessage(hwnd, CB_SETCURSEL, i, 0);
            if (res != CB_ERR)
                return res;
            break;
        }
    }
    return SendMessage(hwnd, CB_SETCURSEL, def_sel, 0);
}

int SetCurSelToItemDataStr(HWND hwnd, LPCSTR text, int def_sel = -1)
{
    return SetCurSelToItemData(hwnd, (DWORD_PTR)text, CmpICBItemDataAsStr, def_sel);
}

DWORD_PTR GetCurItemData(HWND hwnd, DWORD_PTR def_value = 0)
{
    int index = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    if (index >= 0)
        return SendMessage(hwnd, CB_GETITEMDATA, index, 0);
    return def_value;
}

String GetText(HWND hwnd)
{
    WCHAR buf[MAX_PATH_SZ];
    int len = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);
    if (len > 0)
    {
        WCHAR *pbuf = len >= MAX_PATH_SZ ? new WCHAR[len + 1] : buf;
        SendMessageW(hwnd, WM_GETTEXT, len + 1, (LPARAM)buf);
        String s = Path::WidePathToUTF8(pbuf);
        if (pbuf != buf)
            delete [] pbuf;
        return s;
    }
    return "";
}

void SetText(HWND hwnd, LPCSTR text)
{
    WCHAR wstr[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wstr, MAX_PATH_SZ);
    SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)wstr);
}

void SetText(HWND hwnd, LPCWSTR wtext)
{
    SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)wtext);
}

void ResetContent(HWND hwnd)
{
    SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
}

void SetSliderRange(HWND hwnd, int min, int max)
{
    SendMessage(hwnd, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(min, max));
}

int GetSliderPos(HWND hwnd)
{
    return SendMessage(hwnd, TBM_GETPOS, 0, 0);
}

void SetSliderPos(HWND hwnd, int pos)
{
    SendMessage(hwnd, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pos);
}

void MakeFullLongPath(const char *path, WCHAR *out_buf, int buf_len)
{
    WCHAR wbuf[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, out_buf, buf_len);
    GetFullPathNameW(out_buf, MAX_PATH_SZ, wbuf, NULL);
    GetLongPathNameW(wbuf, out_buf, buf_len);
}


//=============================================================================
//
// Browse-for-folder dialog
//
//=============================================================================

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lParam*/, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED)
    {
        // Set initial selection
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpData);
    }
    return 0;
}

bool BrowseForFolder(String &dir_buf)
{
    bool res = false;
    CoInitialize(NULL);

    BROWSEINFO bi = { 0 };
    bi.lpszTitle = "Select location for game saves and custom data files";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM)dir_buf.GetCStr();
    LPITEMIDLIST pidl = SHBrowseForFolder ( &bi );
    if (pidl)
    {
        WCHAR path[MAX_PATH_SZ];
        if (SHGetPathFromIDListW(pidl, path) != FALSE)
        {
            dir_buf = Path::WidePathToUTF8(path);
            res = true;
        }
        CoTaskMemFree(pidl);
    }

    CoUninitialize();
    return res;
}


//=============================================================================
//
// WinSetupDialog, handles the dialog UI.
//
//=============================================================================
class WinSetupDialog
{
public:
    enum GfxModeSpecial
    {
        kGfxMode_None    = -1,
        kGfxMode_Desktop = -2,
        kGfxMode_GameRes = -3,
    };

    static const int MouseSpeedMin = 1;
    static const int MouseSpeedMax = 100;

public:
    WinSetupDialog(const ConfigTree &cfg_in, ConfigTree &cfg_out, const String &data_dir, const String &version_str);
    ~WinSetupDialog();
    static SetupReturnValue ShowModal(const ConfigTree &cfg_in, ConfigTree &cfg_out,
                                      const String &data_dir, const String &version_str);

private:
    static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Event handlers
    INT_PTR OnInitDialog(HWND hwnd);
    INT_PTR OnCommand(WORD id);
    INT_PTR OnListSelection(WORD id);
    void OnCustomSaveDirBtn();
    void OnCustomSaveDirCheck();
    void OnCustomAppDataDirBtn();
    void OnCustomAppDataDirCheck();
    void OnGfxDriverUpdate();
    void OnGfxFilterUpdate();
    void OnGfxModeUpdate();
    void OnFullScalingUpdate();
    void OnWinScalingUpdate();
    void OnWindowedUpdate();
    void OnFullscreenDesktop();
    void ShowAdvancedOptions();

    // Helper structs
    typedef std::vector<DisplayMode> VDispModes;
    // NOTE: we have to implement IGfxModeList for now because we are using
    // few engine functions that take IGfxModeList as parameter
    struct GfxModes : public IGfxModeList
    {
        VDispModes Modes;

        int  GetModeCount() const override;
        bool GetMode(int index, DisplayMode &mode) const override;
    };

    typedef std::vector<GfxFilterInfo> VFilters;
    struct DriverDesc
    {
        String      Id;            // internal id
        String      UserName;      // human-friendly driver name
        GfxModes    GfxModeList;   // list of supported modes
        VFilters    FilterList;    // list of supported filters
        int         UseColorDepth; // recommended display depth
    };

    // Operations
    void AddScalingString(HWND hlist, int scaling_factor);
    void FillAudioDriverList();
    void FillGfxFilterList();
    void FillGfxModeList();
    void FillLanguageList();
    void FillScalingList(HWND hlist, const WindowSetup &ws, const FrameScaleDef frame, bool windowed);
    void InitGfxModes();
    void InitDriverDescFromFactory(const String &id);
    void SaveSetup();
    void SelectNearestGfxMode(const WindowSetup &ws);
    void UpdateMouseSpeedText();

    // Dialog singleton and properties
    static WinSetupDialog *_dlg;
    HWND _hwnd;
    WinConfig _winCfg;
    const ConfigTree &_cfgIn;
    ConfigTree &_cfgOut;
    // Window size
    Size _winSize;
    Size _baseSize;
    // Graphics driver descriptions
    typedef std::shared_ptr<DriverDesc> PDriverDesc;
    typedef std::map<String, PDriverDesc> DriverDescMap;
    DriverDescMap _drvDescMap;
    PDriverDesc _drvDesc;
    GfxFilterInfo _gfxFilterInfo;
    // Audio driver descriptions
    std::vector<std::pair<String, String>> _drvAudioList;
    // Resolution limits
    Size _desktopSize;
    Size _maxWindowSize;

    // Dialog controls
    HWND _hVersionText = NULL;
    HWND _hCustomSaveDir = NULL;
    HWND _hCustomSaveDirBtn = NULL;
    HWND _hCustomSaveDirCheck = NULL;
    HWND _hCustomAppDataDir = NULL;
    HWND _hCustomAppDataDirBtn = NULL;
    HWND _hCustomAppDataDirCheck = NULL;
    HWND _hGfxDriverList = NULL;
    HWND _hGfxModeList = NULL;
    HWND _hFullscreenDesktop = NULL;
    HWND _hFsScalingList = NULL;
    HWND _hWinScalingList = NULL;
    HWND _hGfxFilterList = NULL;
    HWND _hAudioDriverList = NULL;
    HWND _hLanguageList = NULL;
    HWND _hSpriteCacheList = NULL;
    HWND _hTextureCacheList = NULL;
    HWND _hSoundCacheList = NULL;
    HWND _hWindowed = NULL;
    HWND _hVSync = NULL;
    HWND _hRenderAtScreenRes = NULL;
    HWND _hRefresh85Hz = NULL;
    HWND _hAntialiasSprites = NULL;
    HWND _hUseVoicePack = NULL;
    HWND _hAdvanced = NULL;
    HWND _hGameResolutionText = NULL;
    HWND _hMouseLock = NULL;
    HWND _hMouseSpeed = NULL;
    HWND _hMouseSpeedText = NULL;
};

WinSetupDialog *WinSetupDialog::_dlg = NULL;

WinSetupDialog::WinSetupDialog(const ConfigTree &cfg_in, ConfigTree &cfg_out, const String &data_dir, const String &version_str)
    : _hwnd(NULL)
    , _cfgIn(cfg_in)
    , _cfgOut(cfg_out)
{
    _winCfg.DataDirectory = data_dir;
    _winCfg.VersionString = version_str;
}

WinSetupDialog::~WinSetupDialog()
{
}

SetupReturnValue WinSetupDialog::ShowModal(const ConfigTree &cfg_in, ConfigTree &cfg_out,
                                           const String &data_dir, const String &version_str)
{
    _dlg = new WinSetupDialog(cfg_in, cfg_out, data_dir, version_str);
    INT_PTR dlg_res = DialogBoxParamW(GetModuleHandleW(NULL), (LPCWSTR)IDD_SETUP, (HWND)sys_win_get_window(),
        (DLGPROC)WinSetupDialog::DialogProc, 0L);
    delete _dlg;
    _dlg = NULL;

    switch (dlg_res)
    {
    case IDOKRUN: return kSetup_RunGame;
    case IDOK: return kSetup_Done;
    default: return kSetup_Cancel;
    }
}

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

INT_PTR WinSetupDialog::OnInitDialog(HWND hwnd)
{
    _hwnd                   = hwnd;
    _hVersionText           = GetDlgItem(_hwnd, IDC_VERSION);
    _hCustomSaveDir         = GetDlgItem(_hwnd, IDC_CUSTOMSAVEDIR);
    _hCustomSaveDirBtn      = GetDlgItem(_hwnd, IDC_CUSTOMSAVEDIRBTN);
    _hCustomSaveDirCheck    = GetDlgItem(_hwnd, IDC_CUSTOMSAVEDIRCHECK);
    _hCustomAppDataDir      = GetDlgItem(_hwnd, IDC_CUSTOMAPPDATADIR);
    _hCustomAppDataDirBtn   = GetDlgItem(_hwnd, IDC_CUSTOMAPPDATADIRBTN);
    _hCustomAppDataDirCheck = GetDlgItem(_hwnd, IDC_CUSTOMAPPDATADIRCHECK);
    _hGfxDriverList         = GetDlgItem(_hwnd, IDC_GFXDRIVER);
    _hGfxModeList           = GetDlgItem(_hwnd, IDC_GFXMODE);
    _hFullscreenDesktop     = GetDlgItem(_hwnd, IDC_FULLSCREENDESKTOP);
    _hFsScalingList         = GetDlgItem(_hwnd, IDC_FSSCALING);
    _hWinScalingList        = GetDlgItem(_hwnd, IDC_WINDOWSCALING);
    _hGfxFilterList         = GetDlgItem(_hwnd, IDC_GFXFILTER);
    _hAudioDriverList       = GetDlgItem(_hwnd, IDC_DIGISOUND);
    _hLanguageList          = GetDlgItem(_hwnd, IDC_LANGUAGE);
    _hSpriteCacheList       = GetDlgItem(_hwnd, IDC_SPRITECACHE);
    _hTextureCacheList      = GetDlgItem(_hwnd, IDC_TEXTURECACHE);
    _hSoundCacheList        = GetDlgItem(_hwnd, IDC_SOUNDCACHE);
    _hWindowed              = GetDlgItem(_hwnd, IDC_WINDOWED);
    _hVSync                 = GetDlgItem(_hwnd, IDC_VSYNC);
    _hRenderAtScreenRes     = GetDlgItem(_hwnd, IDC_RENDERATSCREENRES);
    _hRefresh85Hz           = GetDlgItem(_hwnd, IDC_REFRESH_85HZ);
    _hAntialiasSprites      = GetDlgItem(_hwnd, IDC_ANTIALIAS);
    _hUseVoicePack          = GetDlgItem(_hwnd, IDC_VOICEPACK);
    _hAdvanced              = GetDlgItem(_hwnd, IDC_ADVANCED);
    _hGameResolutionText    = GetDlgItem(_hwnd, IDC_RESOLUTION);
    _hMouseLock             = GetDlgItem(_hwnd, IDC_MOUSE_AUTOLOCK);
    _hMouseSpeed            = GetDlgItem(_hwnd, IDC_MOUSESPEED);
    _hMouseSpeedText        = GetDlgItem(_hwnd, IDC_MOUSESPEED_TEXT);

    _desktopSize = get_desktop_size();
    _maxWindowSize = AGSPlatformDriver::GetDriver()->ValidateWindowSize(_desktopSize, false);

    _winCfg.Load(_cfgIn);

    // Custom save dir controls
    SetupCustomDirCtrl(_winCfg.UserSaveDir, _winCfg.DataDirectory,
        _hCustomSaveDirCheck, _hCustomSaveDir, _hCustomSaveDirBtn);
    SetupCustomDirCtrl(_winCfg.AppDataDir, _winCfg.DataDirectory,
        _hCustomAppDataDirCheck, _hCustomAppDataDir, _hCustomAppDataDirBtn);

    // Resolution controls
    if (_winCfg.GameResolution.IsNull() &&
          (_winCfg.GameResType == kGameResolution_Undefined || _winCfg.GameResType == kGameResolution_Custom) ||
          _winCfg.GameColourDepth == 0)
        MessageBox(_hwnd, "Essential information about the game is missing in the configuration file. Setup program may be unable to deduce graphic modes properly.", "Initialization error", MB_OK | MB_ICONWARNING);

    SetText(_hwnd, STR(_winCfg.Title));
    SetText((HWND)sys_win_get_window(), STR(_winCfg.Title));
    SetText(_hGameResolutionText, STR(String::FromFormat("Native game resolution: %d x %d x %d",
        _winCfg.GameResolution.Width, _winCfg.GameResolution.Height, _winCfg.GameColourDepth)));

    SetText(_hVersionText, STR(_winCfg.VersionString));

    InitGfxModes();
    for (DriverDescMap::const_iterator it = _drvDescMap.begin(); it != _drvDescMap.end(); ++it)
        AddString(_hGfxDriverList, STR(it->second->UserName), (DWORD_PTR)it->second->Id.GetCStr());
    SetCurSelToItemDataStr(_hGfxDriverList, _winCfg.GfxDriverId.GetCStr(), 0);
    SetCheck(_hFullscreenDesktop, _winCfg.FsSetup.Mode == kWnd_FullDesktop);
    EnableWindow(_hGfxModeList, _winCfg.FsSetup.Mode == kWnd_Fullscreen);
    OnGfxDriverUpdate();

    SetCheck(_hWindowed, _winCfg.Windowed);

    FillScalingList(_hFsScalingList, _winCfg.FsSetup, _winCfg.FsGameFrame, false);
    FillScalingList(_hWinScalingList, _winCfg.WinSetup, _winCfg.WinGameFrame, true);
    OnFullScalingUpdate();
    OnWinScalingUpdate();

    SetCheck(_hVSync, _winCfg.VSync);

    SetCheck(_hRenderAtScreenRes, _winCfg.RenderAtScreenRes);

    FillLanguageList();

    SetCheck(_hMouseLock, _winCfg.MouseAutoLock);

    SetSliderRange(_hMouseSpeed, MouseSpeedMin, MouseSpeedMax);
    int slider_pos = (int)(_winCfg.MouseSpeed * 10.f + .5f);
    SetSliderPos(_hMouseSpeed, slider_pos);
    UpdateMouseSpeedText();

    // Init sprite cache list
#if AGS_PLATFORM_64BIT
    const std::array<std::pair<const char*, int>, 11> spr_cache_vals = { {
        { "16 MB", 16 },  { "32 MB", 32 },  { "64 MB", 64 },   { "128 MB (default)", 128 },
        { "256 MB", 256}, { "384 MB", 384}, { "512 MB", 512 }, { "768 MB", 768 },
        { "1 GB ", 1024 }, { "1.5 GB ", 1536 }, { "2 GB ", 2048 }
    }};
#else
    // 32-bit programs have accessible RAM limit of ~2GB (may be less in practice),
    // and engine will need RAM for other things than spritecache, keep that in mind
    const std::array<std::pair<const char*, int>, 7> spr_cache_vals = { {
        { "16 MB", 16 }, { "32 MB", 32 }, { "64 MB", 64 }, { "128 MB (default)", 128 },
        { "256 MB", 256}, { "384 MB", 384}, { "512 MB", 512 }
    }};
#endif
    for (const auto &val : spr_cache_vals)
        AddString(_hSpriteCacheList, val.first, val.second);
    SetCurSelToItemData(_hSpriteCacheList, _winCfg.SpriteCacheSize / 1024, NULL, 3);

    // Init texture cache list
    const std::array<std::pair<const char*, int>, 10> tx_cache_vals = { {
        { "Off (not recommended)", 0 },
        { "16 MB", 16 }, { "32 MB", 32 }, { "64 MB", 64 }, { "128 MB (default)", 128 },
        { "256 MB", 256}, { "384 MB", 384}, { "512 MB", 512 }, { "768 MB", 768 },
        { "1 GB ", 1024 }
    }};
    for (const auto &val : tx_cache_vals)
        AddString(_hTextureCacheList, val.first, val.second);
    SetCurSelToItemData(_hTextureCacheList, _winCfg.TextureCacheSize / 1024, NULL, 4);

    // Init sound cache list (keep in mind: currently meant only for small sounds)
    const std::array<std::pair<const char*, int>, 5> sound_cache_vals = { {
        { "Off", 0 }, { "16 MB", 16 }, { "32 MB (default)", 32 }, { "64 MB", 64 },
        { "128 MB", 128 }
    }};
    for (const auto &val : sound_cache_vals)
        AddString(_hSoundCacheList, val.first, val.second);
    SetCurSelToItemData(_hSoundCacheList, _winCfg.SoundCacheSize / 1024, NULL, 2);


    SetCheck(_hRefresh85Hz, _winCfg.RefreshRate == 85);
    SetCheck(_hAntialiasSprites, _winCfg.AntialiasSprites);

    FillAudioDriverList();
    if (_winCfg.AudioEnabled)
    {
        if (_winCfg.AudioDriverId.IsEmpty())
            SetCurSelToItemDataStr(_hAudioDriverList, "default");
        else
            SetCurSelToItemDataStr(_hAudioDriverList, _winCfg.AudioDriverId.GetCStr());
    }
    else
    {
        SetCurSelToItemDataStr(_hAudioDriverList, "none");
    }
    SetCheck(_hUseVoicePack, _winCfg.UseVoicePack);
    if (!File::IsFile("speech.vox"))
        EnableWindow(_hUseVoicePack, FALSE);

    if (CfgReadBoolInt(_cfgIn, "disabled", "speechvox"))
        EnableWindow(_hUseVoicePack, FALSE);
    if (CfgReadBoolInt(_cfgIn, "disabled", "filters"))
        EnableWindow(_hGfxFilterList, FALSE);
    if (CfgReadBoolInt(_cfgIn, "disabled", "antialias"))
        EnableWindow(_hAntialiasSprites, FALSE);
    if (CfgReadBoolInt(_cfgIn, "disabled", "render_at_screenres") ||
        CfgReadInt(_cfgIn, "gameproperties", "render_at_screenres", -1) >= 0)
        EnableWindow(_hRenderAtScreenRes, FALSE);
    if (CfgReadBoolInt(_cfgIn, "disabled", "translation"))
        EnableWindow(_hLanguageList, FALSE);

    RECT win_rect, gfx_rect, adv_rect, border;
    GetWindowRect(_hwnd, &win_rect);
    GetWindowRect(GetDlgItem(_hwnd, IDC_GFXOPTIONS), &gfx_rect);
    _winSize.Width = win_rect.right - win_rect.left;
    _winSize.Height = win_rect.bottom - win_rect.top;
    GetWindowRect(_hAdvanced, &adv_rect);
    border.left = border.top = border.right = border.bottom = 9;
    MapDialogRect(_hwnd, &border);
    _baseSize.Width = (adv_rect.right + (gfx_rect.left - win_rect.left)) - win_rect.left;
    _baseSize.Height = adv_rect.bottom - win_rect.top + border.bottom;

    MoveWindow(_hwnd, std::max<int>(0, win_rect.left + (_winSize.Width - _baseSize.Width) / 2),
                      std::max<int>(0, win_rect.top + (_winSize.Height - _baseSize.Height) / 2),
                      _baseSize.Width, _baseSize.Height, TRUE);
    SetFocus(GetDlgItem(_hwnd, IDOK));
    return FALSE; // notify WinAPI that we set focus ourselves
}

INT_PTR WinSetupDialog::OnCommand(WORD id)
{
    switch (id)
    {
    case IDC_ADVANCED:  ShowAdvancedOptions(); break;
    case IDC_WINDOWED:  OnWindowedUpdate(); break;
    case IDC_FULLSCREENDESKTOP: OnFullscreenDesktop(); break;
    case IDC_CUSTOMSAVEDIRBTN: OnCustomSaveDirBtn(); break;
    case IDC_CUSTOMSAVEDIRCHECK: OnCustomSaveDirCheck(); break;
    case IDC_CUSTOMAPPDATADIRBTN: OnCustomAppDataDirBtn(); break;
    case IDC_CUSTOMAPPDATADIRCHECK: OnCustomAppDataDirCheck(); break;
    case IDOK:
    case IDOKRUN:
        SaveSetup();
        // fall-through intended
    case IDCANCEL:
        EndDialog(_hwnd, id);
        return TRUE;
    default:
        return FALSE;
    }
    return TRUE;
}

INT_PTR WinSetupDialog::OnListSelection(WORD id)
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

void WinSetupDialog::OnCustomSaveDirBtn()
{
    String save_dir = GetText(_hCustomSaveDir);
    if (BrowseForFolder(save_dir))
    {
        SetText(_hCustomSaveDir, STR(save_dir));
    }
}

void WinSetupDialog::OnCustomSaveDirCheck()
{
    bool custom_save_dir = GetCheck(_hCustomSaveDirCheck);
    EnableWindow(_hCustomSaveDir, custom_save_dir ? TRUE : FALSE);
    EnableWindow(_hCustomSaveDirBtn, custom_save_dir ? TRUE : FALSE);
}

void WinSetupDialog::OnCustomAppDataDirBtn()
{
    String data_dir = GetText(_hCustomAppDataDir);
    if (BrowseForFolder(data_dir))
    {
        SetText(_hCustomAppDataDir, STR(data_dir));
    }
}

void WinSetupDialog::OnCustomAppDataDirCheck()
{
    bool custom_data_dir = GetCheck(_hCustomAppDataDirCheck);
    EnableWindow(_hCustomAppDataDir, custom_data_dir ? TRUE : FALSE);
    EnableWindow(_hCustomAppDataDirBtn, custom_data_dir ? TRUE : FALSE);
}

void WinSetupDialog::OnGfxDriverUpdate()
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

void WinSetupDialog::OnGfxFilterUpdate()
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

void WinSetupDialog::OnGfxModeUpdate()
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

void WinSetupDialog::OnFullScalingUpdate()
{
    _winCfg.FsGameFrame = (FrameScaleDef)GetCurItemData(_hFsScalingList);
}

void WinSetupDialog::OnWinScalingUpdate()
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

void WinSetupDialog::OnWindowedUpdate()
{
    _winCfg.Windowed = GetCheck(_hWindowed);
}

void WinSetupDialog::OnFullscreenDesktop()
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

void WinSetupDialog::ShowAdvancedOptions()
{
    // Reveal the advanced bit of the window
    ShowWindow(_hAdvanced, SW_HIDE);

    RECT win_rect;
    GetWindowRect(_hwnd, &win_rect);
    MoveWindow(_hwnd, std::max<int>(0, win_rect.left + (_baseSize.Width - _winSize.Width) / 2),
                      std::max<int>(0, win_rect.top + (_baseSize.Height - _winSize.Height) / 2),
                      _winSize.Width, _winSize.Height, TRUE);

    int offset = _winSize.Height - _baseSize.Height;
    RECT rc;
    int ctrl_ids[] = { IDC_VERSION, IDOK, IDOKRUN, IDCANCEL, 0 };
    for (int i = 0; ctrl_ids[i]; ++i)
    {
        HWND hctrl = GetDlgItem(_hwnd, ctrl_ids[i]);
        GetWindowRect(hctrl, &rc);
        ScreenToClient(_hwnd, (POINT*)&rc);
        ScreenToClient(_hwnd, (POINT*)&rc.right);
        MoveWindow(hctrl, rc.left, rc.top + offset, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    }
}

INT_PTR CALLBACK WinSetupDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        _ASSERT(_dlg != NULL && _dlg->_hwnd == NULL);
        return _dlg->OnInitDialog(hwndDlg);
    case WM_COMMAND:
        _ASSERT(_dlg != NULL && _dlg->_hwnd != NULL);
        if (HIWORD(wParam) == CBN_SELCHANGE)
            return _dlg->OnListSelection(LOWORD(wParam));
        return _dlg->OnCommand(LOWORD(wParam));
    case WM_HSCROLL:
        _ASSERT(_dlg != NULL && _dlg->_hwnd != NULL);
        _dlg->UpdateMouseSpeedText();
        return TRUE;
    default:
        return FALSE;
    }
}

int WinSetupDialog::GfxModes::GetModeCount() const
{
    return Modes.size();
}

bool WinSetupDialog::GfxModes::GetMode(int index, DisplayMode &mode) const
{
    if (index >= 0 && (size_t)index < Modes.size())
    {
        mode = Modes[index];
        return true;
    }
    return false;
}

void WinSetupDialog::AddScalingString(HWND hlist, int scaling_factor)
{
    String s;
    if (scaling_factor >= 0)
        s = String::FromFormat("x%d", scaling_factor);
    else
        s = String::FromFormat("1/%d", -scaling_factor);
    AddString(hlist, STR(s), (DWORD_PTR)(scaling_factor >= 0 ? scaling_factor + kNumFrameScaleDef : scaling_factor));
}

void WinSetupDialog::FillAudioDriverList()
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
    for (const auto &desc : _drvAudioList)
        AddString(_hAudioDriverList, STR(desc.second), (DWORD_PTR)desc.first.GetCStr());
}

void WinSetupDialog::FillGfxFilterList()
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

void WinSetupDialog::FillGfxModeList()
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

void WinSetupDialog::FillLanguageList()
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

void WinSetupDialog::FillScalingList(HWND hlist, const WindowSetup &ws, const FrameScaleDef frame, bool windowed)
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

void WinSetupDialog::InitGfxModes()
{
    InitDriverDescFromFactory("D3D9");
    InitDriverDescFromFactory("OGL");
    InitDriverDescFromFactory("Software");

    if (_drvDescMap.size() == 0)
        MessageBox(_hwnd, "Unable to detect any supported graphic drivers!", "Initialization error", MB_OK | MB_ICONERROR);
}

// "Less" predicate that compares two display modes only by their screen metrics
bool SizeLess(const DisplayMode &first, const DisplayMode &second)
{
    return Size(first.Width, first.Height) < Size(second.Width, second.Height);
}

void WinSetupDialog::InitDriverDescFromFactory(const String &id)
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

void WinSetupDialog::SaveSetup()
{
    _winCfg.UserSaveDir = SaveCustomDirSetup(_winCfg.DataDirectory, _hCustomSaveDirCheck, _hCustomSaveDir);
    _winCfg.AppDataDir = SaveCustomDirSetup(_winCfg.DataDirectory, _hCustomAppDataDirCheck, _hCustomAppDataDir);

    if (GetCurSel(_hLanguageList) == 0)
        _winCfg.Language.Empty();
    else
        _winCfg.Language = GetText(_hLanguageList);
    _winCfg.SpriteCacheSize = GetCurItemData(_hSpriteCacheList) * 1024;
    _winCfg.TextureCacheSize = GetCurItemData(_hTextureCacheList) * 1024;
    _winCfg.SoundCacheSize = GetCurItemData(_hSoundCacheList) * 1024;
    if (GetCurSel(_hAudioDriverList) == 0)
    {
        _winCfg.AudioEnabled = false;
        _winCfg.AudioDriverId = "";
    }
    else
    {
        _winCfg.AudioEnabled = true;
        if (GetCurSel(_hAudioDriverList) == 1)
            _winCfg.AudioDriverId = ""; // use default
        else
            _winCfg.AudioDriverId = (LPCSTR)GetCurItemData(_hAudioDriverList);
    }
    _winCfg.UseVoicePack = GetCheck(_hUseVoicePack);
    _winCfg.VSync = GetCheck(_hVSync);
    _winCfg.RenderAtScreenRes = GetCheck(_hRenderAtScreenRes);
    _winCfg.AntialiasSprites = GetCheck(_hAntialiasSprites);
    _winCfg.RefreshRate = GetCheck(_hRefresh85Hz) ? 85 : 0;
    _winCfg.GfxFilterId = (LPCSTR)GetCurItemData(_hGfxFilterList);

    _winCfg.MouseAutoLock = GetCheck(_hMouseLock);
    int slider_pos = GetSliderPos(_hMouseSpeed);
    _winCfg.MouseSpeed = (float)slider_pos / 10.f;

    _winCfg.Save(_cfgOut, _desktopSize);
}

void WinSetupDialog::SelectNearestGfxMode(const WindowSetup &ws)
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

void WinSetupDialog::UpdateMouseSpeedText()
{
    int slider_pos = GetSliderPos(_hMouseSpeed);
    float mouse_speed = (float)slider_pos / 10.f;
    String text = mouse_speed == 1.f ? "Mouse speed: x 1.0 (Default)" : String::FromFormat("Mouse speed: x %0.1f", mouse_speed);
    SetText(_hMouseSpeedText, STR(text));
}

//=============================================================================
//
// Windows setup entry point.
//
//=============================================================================
SetupReturnValue WinSetup(const ConfigTree &cfg_in, ConfigTree &cfg_out,
                          const String &game_data_dir, const String &version_str)
{
    return WinSetupDialog::ShowModal(cfg_in, cfg_out, game_data_dir, version_str);
}

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS
