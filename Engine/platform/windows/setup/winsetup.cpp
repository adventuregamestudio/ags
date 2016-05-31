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

#if !defined (WINDOWS_VERSION)
#error This file should only be included on the Windows build
#endif

#include <windows.h>
#include <commctrl.h>
#include <crtdbg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <vector>
#include "ac/gamestructdefines.h"
#undef RGB
#undef PALETTE
#define RGB void*
#define PALETTE void*
#include "gfx/gfxdriverfactory.h"
#include "gfx/gfxfilter.h"
#include "gfx/graphicsdriver.h"
#include "main/config.h"
#include "main/graphics_mode.h"
#include "platform/base/agsplatformdriver.h"
#include "resource/resource.h"
#include "util/file.h"
#include "util/string_utils.h"

#define AL_ID(a,b,c,d)     (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))

#define DIGI_DIRECTAMX(n)        AL_ID('A','X','A'+(n),' ')
// This DirectX hardware mixer is crap, it crashes the program
// when two sound effects are played at once
#define DIGI_DIRECTX(n)          AL_ID('D','X','A'+(n),' ')
#define DIGI_WAVOUTID(n)         AL_ID('W','O','A'+(n),' ')
#define DIGI_NONE  0
#define MIDI_AUTODETECT       -1 
#define MIDI_NONE             0 
#define MIDI_WIN32MAPPER         AL_ID('W','3','2','M')

extern "C" HWND allegro_wnd;

namespace AGS
{
namespace Engine
{

using namespace AGS::Common;

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
    GameResolutionType GameResType;
    Size   GameResolution;
    int    GameColourDepth;
    bool   LetterboxByDesign;

    String GfxDriverId;
    String GfxFilterId;
    bool   ScreenSizeFromScaling;
    bool   MatchDeviceAspectRatio;
    Size   ScreenSize;
    int    FilterScaling;
    String FramePlacement;
    int    RefreshRate;
    bool   Windowed;
    bool   VSync;
    bool   Reduce32to16;
    bool   AntialiasSprites;

    int    DigiWinIdx;
    int    MidiWinIdx;
    bool   UseVoicePack;

    bool   MouseAutoLock;
    float  MouseSpeed;

    int    SpriteCacheSize;
    String DefaultLanguageName;
    String Language;

    WinConfig();
    void SetDefaults();
    void Load(const ConfigTree &cfg);
    void Save(ConfigTree &cfg);
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
    LetterboxByDesign = false;

    GfxFilterId = "StdScale";
    GfxDriverId = "D3D9";
    ScreenSizeFromScaling = false;
    MatchDeviceAspectRatio = false;
    ScreenSize = get_desktop_size();
    FilterScaling = 0;
    FramePlacement = "center";
    RefreshRate = 0;
    Windowed = false;
    VSync = false;
    AntialiasSprites = false;
    Reduce32to16 = false;

    MouseAutoLock = false;
    MouseSpeed = 1.f;

    DigiWinIdx = 0;
    MidiWinIdx = 0;
    UseVoicePack = true;

    SpriteCacheSize = 1024 * 20;
    DefaultLanguageName = "Game Default";

    Title = "Game Setup";
}

String MakeScalingFactorString(int scaling)
{
    return scaling == 0 ? "max" : String::FromFormat("%d", scaling);
}

int ParseScalingFactor(const String &s)
{
    if (s.CompareNoCase("max") == 0)
        return 0;
    return StrUtil::StringToInt(s);
}

void WinConfig::Load(const ConfigTree &cfg)
{
    DataDirectory = INIreadstring(cfg, "misc", "datadir", DataDirectory);
    UserSaveDir = INIreadstring(cfg, "misc", "user_data_dir");
    // Backward-compatible resolution type
    GameResType = (GameResolutionType)INIreadint(cfg, "misc", "defaultres", GameResType);
    if (GameResType < kGameResolution_Undefined || GameResType >= kNumGameResolutions)
        GameResType = kGameResolution_Undefined;
    GameResolution.Width = INIreadint(cfg, "misc", "game_width", GameResolution.Width);
    GameResolution.Height = INIreadint(cfg, "misc", "game_height", GameResolution.Height);
    GameColourDepth = INIreadint(cfg, "misc", "gamecolordepth", GameColourDepth);
    LetterboxByDesign = INIreadint(cfg, "misc", "letterbox", 0) != 0;

    GfxDriverId = INIreadstring(cfg, "graphics", "driver", GfxDriverId);
    GfxFilterId = INIreadstring(cfg, "graphics", "filter", GfxFilterId);
    String screen_def = INIreadstring(cfg, "graphics", "screen_def", "max");
    ScreenSizeFromScaling = screen_def.CompareNoCase("scaling") == 0;
    MatchDeviceAspectRatio = INIreadint(cfg, "graphics", "match_device_ratio", 1) != 0;
    ScreenSize.Width = INIreadint(cfg, "graphics", "screen_width", ScreenSize.Width);
    ScreenSize.Height = INIreadint(cfg, "graphics", "screen_height", ScreenSize.Height);
    FilterScaling = ParseScalingFactor(INIreadstring(cfg, "graphics", "filter_scaling", MakeScalingFactorString(FilterScaling)));
    FramePlacement = INIreadstring(cfg, "graphics", "game_frame", FramePlacement);
    RefreshRate = INIreadint(cfg, "graphics", "refresh", RefreshRate);
    Windowed = INIreadint(cfg, "graphics", "windowed", Windowed ? 1 : 0) != 0;
    VSync = INIreadint(cfg, "graphics", "vsync", VSync ? 1 : 0) != 0;

    Reduce32to16 = INIreadint(cfg, "misc","notruecolor", Reduce32to16 ? 1 : 0) != 0;
    AntialiasSprites = INIreadint(cfg, "misc", "antialias", AntialiasSprites ? 1 : 0) != 0;

    DigiWinIdx = INIreadint(cfg, "sound", "digiwinindx", DigiWinIdx);
    MidiWinIdx = INIreadint(cfg, "sound", "midiwinindx", MidiWinIdx);
    UseVoicePack = INIreadint(cfg, "sound", "usespeech", UseVoicePack ? 1 : 0) != 0;

    MouseAutoLock = INIreadint(cfg, "mouse", "auto_lock", MouseAutoLock ? 1 : 0) != 0;
    MouseSpeed = INIreadfloat(cfg, "mouse", "speed", 1.f);
    if (MouseSpeed <= 0.f)
        MouseSpeed = 1.f;

    SpriteCacheSize = INIreadint(cfg, "misc", "cachemax", SpriteCacheSize);
    Language = INIreadstring(cfg, "language", "translation", Language);
    DefaultLanguageName = INIreadstring(cfg, "language", "default_translation_name", DefaultLanguageName);

    Title = INIreadstring(cfg, "misc", "titletext", Title);
}

void WinConfig::Save(ConfigTree &cfg)
{
    INIwritestring(cfg, "misc", "user_data_dir", UserSaveDir);

    INIwritestring(cfg, "graphics", "driver", GfxDriverId);
    INIwritestring(cfg, "graphics", "filter", GfxFilterId);
    INIwritestring(cfg, "graphics", "screen_def", ScreenSizeFromScaling ? "scaling" : "explicit");
    INIwriteint(cfg, "graphics", "match_device_ratio", MatchDeviceAspectRatio ? 1 : 0);
    INIwriteint(cfg, "graphics", "screen_width", ScreenSize.Width);
    INIwriteint(cfg, "graphics", "screen_height", ScreenSize.Height);
    INIwritestring(cfg, "graphics", "filter_scaling", MakeScalingFactorString(FilterScaling));
    INIwritestring(cfg, "graphics", "game_frame", FramePlacement);
    INIwriteint(cfg, "graphics", "refresh", RefreshRate);
    INIwriteint(cfg, "graphics", "windowed", Windowed ? 1 : 0);
    INIwriteint(cfg, "graphics", "vsync", VSync ? 1 : 0);

    INIwriteint(cfg, "misc", "notruecolor", Reduce32to16 ? 1 : 0);
    INIwriteint(cfg, "misc", "antialias", AntialiasSprites ? 1 : 0);

    INIwriteint(cfg, "sound", "digiwinindx", DigiWinIdx);
    INIwriteint(cfg, "sound", "midiwinindx", MidiWinIdx);
    INIwriteint(cfg, "sound", "usespeech", UseVoicePack ? 1 : 0);

    INIwriteint(cfg, "mouse", "auto_lock", MouseAutoLock ? 1 : 0);
    INIwritestring(cfg, "mouse", "speed", String::FromFormat("%0.1f", MouseSpeed));

    INIwriteint(cfg, "misc", "cachemax", SpriteCacheSize);
    INIwritestring(cfg, "language", "translation", Language);
}


//=============================================================================
//
// WinAPI interaction helpers
//
//=============================================================================

int AddString(HWND hwnd, LPCTSTR text, DWORD_PTR data = 0L)
{
    int index = SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)text);
    if (index >= 0)
        SendMessage(hwnd, CB_SETITEMDATA, index, data);
    return index;
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
    LPCTSTR text_ptr1 = (LPCTSTR)data1;
    LPCTSTR text_ptr2 = (LPCTSTR)data2;
    return text_ptr1 && text_ptr2 && StrCmpI(text_ptr1, text_ptr2) == 0 || !text_ptr1 && !text_ptr2;
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

int SetCurSelToItemDataStr(HWND hwnd, LPCTSTR text, int def_sel = -1)
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
    TCHAR short_buf[MAX_PATH + 1];
    int len = SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
    if (len > 0)
    {
        TCHAR *buf = len >= sizeof(short_buf) ? new TCHAR[len + 1] : short_buf;
        SendMessage(hwnd, WM_GETTEXT, len + 1, (LPARAM)buf);
        String s = buf;
        if (buf != short_buf)
            delete [] buf;
        return s;
    }
    return "";
}

void SetText(HWND hwnd, LPCTSTR text)
{
    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)text);
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

void MakeFullLongPath(const char *path, char *out_buf, int buf_len)
{
    GetFullPathName(path, buf_len, out_buf, NULL);
    GetLongPathName(out_buf, out_buf, buf_len);
}


//=============================================================================
//
// Browse-for-folder dialog
//
//=============================================================================

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
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
        char path[MAX_PATH];
        if (SHGetPathFromIDList(pidl, path) != FALSE)
        {
            dir_buf = path;
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
        kGfxMode_NoSpecial      = -1,
        kGfxMode_Desktop        =  0,
        kGfxMode_GameRes,
        kGfxMode_FromScaling,
        kGfxMode_FromScaling_KeepDeviceRatio,
        kGfxMode_FirstSpecial   = kGfxMode_Desktop,
        kGfxMode_LastSpecial    = kGfxMode_FromScaling_KeepDeviceRatio
    };

    enum FilterScalingSpecial
    {
        kFilterScaling_None,
        kFilterScaling_MaxFit,
        kNumFilterScalingSpecials
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
    void OnFramePlacement();
    void OnGfxDriverUpdate();
    void OnGfxFilterUpdate();
    void OnGfxModeUpdate();
    void OnScalingUpdate();
    void OnWindowedUpdate();
    void ShowAdvancedOptions();

    // Helper structs
    struct GfxModes : public IGfxModeList
    {
        std::vector<DisplayMode> Modes;

        virtual int  GetModeCount() const;
        virtual bool GetMode(int index, DisplayMode &mode) const;
    };

    struct DriverDesc
    {
        GfxModes                   GfxModeList;
        std::vector<GfxFilterInfo> FilterList;
    };

    // Operations
    void AddScalingString(int scaling_factor);
    void FillGfxFilterList();
    void FillGfxModeList();
    void FillLanguageList();
    void FillScalingList();
    void InitGfxModes();
    void InitDriverDescFromFactory(const String &id, DriverDesc &drv_desc);
    void SaveSetup();
    void SelectNearestGfxMode(const Size screen_size, GfxModeSpecial gfx_mode_spec);
    void SetFramePlacement(const String &frame_place);
    void SetGfxModeText();
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
    // Driver descriptions
    DriverDesc _dx5;
    DriverDesc _d3d9;
    DriverDesc *_drvDesc;
    GfxFilterInfo *_gfxFilterInfo;
    // Resolution limits
    Size _desktopSize;
    Size _maxWindowSize;
    Size _minGameSize;
    int _maxGameScale;
    GfxModeSpecial _specialGfxMode; // remember last selected special gfx mode

    // Dialog controls
    HWND _hVersionText;
    HWND _hCustomSaveDir;
    HWND _hCustomSaveDirBtn;
    HWND _hCustomSaveDirCheck;
    HWND _hGfxDriverList;
    HWND _hGfxModeList;
    HWND _hGfxFilterList;
    HWND _hGfxFilterScalingList;
    HWND _hDigiDriverList;
    HWND _hMidiDriverList;
    HWND _hLanguageList;
    HWND _hSpriteCacheList;
    HWND _hWindowed;
    HWND _hVSync;
    HWND _hRefresh85Hz;
    HWND _hAntialiasSprites;
    HWND _hReduce32to16;
    HWND _hUseVoicePack;
    HWND _hAdvanced;
    HWND _hGameResolutionText;
    HWND _hGfxModeText;
    HWND _hStretchToScreen;
    HWND _hKeepAspectRatio;
    HWND _hMouseLock;
    HWND _hMouseSpeed;
    HWND _hMouseSpeedText;
};

WinSetupDialog *WinSetupDialog::_dlg = NULL;

WinSetupDialog::WinSetupDialog(const ConfigTree &cfg_in, ConfigTree &cfg_out, const String &data_dir, const String &version_str)
    : _hwnd(NULL)
    , _cfgIn(cfg_in)
    , _cfgOut(cfg_out)
    , _drvDesc(NULL)
    , _gfxFilterInfo(NULL)
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
    INT_PTR dlg_res = DialogBoxParam(GetModuleHandle(NULL), (LPCTSTR)IDD_SETUP, allegro_wnd,
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

INT_PTR WinSetupDialog::OnInitDialog(HWND hwnd)
{
    _hwnd                   = hwnd;
    _hVersionText           = GetDlgItem(_hwnd, IDC_VERSION);
    _hCustomSaveDir         = GetDlgItem(_hwnd, IDC_CUSTOMSAVEDIR);
    _hCustomSaveDirBtn      = GetDlgItem(_hwnd, IDC_CUSTOMSAVEDIRBTN);
    _hCustomSaveDirCheck    = GetDlgItem(_hwnd, IDC_CUSTOMSAVEDIRCHECK);
    _hGfxDriverList         = GetDlgItem(_hwnd, IDC_GFXDRIVER);
    _hGfxModeList           = GetDlgItem(_hwnd, IDC_GFXMODE);
    _hGfxFilterList         = GetDlgItem(_hwnd, IDC_GFXFILTER);
    _hGfxFilterScalingList  = GetDlgItem(_hwnd, IDC_GFXFILTERSCALING);
    _hDigiDriverList        = GetDlgItem(_hwnd, IDC_DIGISOUND);
    _hMidiDriverList        = GetDlgItem(_hwnd, IDC_MIDIMUSIC);
    _hLanguageList          = GetDlgItem(_hwnd, IDC_LANGUAGE);
    _hSpriteCacheList       = GetDlgItem(_hwnd, IDC_SPRITECACHE);
    _hWindowed              = GetDlgItem(_hwnd, IDC_WINDOWED);
    _hVSync                 = GetDlgItem(_hwnd, IDC_VSYNC);
    _hRefresh85Hz           = GetDlgItem(_hwnd, IDC_REFRESH_85HZ);
    _hAntialiasSprites      = GetDlgItem(_hwnd, IDC_ANTIALIAS);
    _hReduce32to16          = GetDlgItem(_hwnd, IDC_REDUCE32TO16);
    _hUseVoicePack          = GetDlgItem(_hwnd, IDC_VOICEPACK);
    _hAdvanced              = GetDlgItem(_hwnd, IDC_ADVANCED);
    _hGameResolutionText    = GetDlgItem(_hwnd, IDC_RESOLUTION);
    _hGfxModeText           = GetDlgItem(_hwnd, IDC_GFXMODETEXT);
    _hStretchToScreen       = GetDlgItem(_hwnd, IDC_STRETCHTOSCREEN);
    _hKeepAspectRatio       = GetDlgItem(_hwnd, IDC_ASPECTRATIO);
    _hMouseLock             = GetDlgItem(_hwnd, IDC_MOUSE_AUTOLOCK);
    _hMouseSpeed            = GetDlgItem(_hwnd, IDC_MOUSESPEED);
    _hMouseSpeedText        = GetDlgItem(_hwnd, IDC_MOUSESPEED_TEXT);

    _desktopSize = get_desktop_size();
    _maxWindowSize = _desktopSize;
    AGSPlatformDriver::GetDriver()->ValidateWindowSize(_maxWindowSize.Width, _maxWindowSize.Height, false);
    _minGameSize = Size(320, 200);
    _maxGameScale = 1;

    _winCfg.Load(_cfgIn);

    // Custom save dir controls
    String custom_save_dir = _winCfg.UserSaveDir;
    bool has_save_dir = !custom_save_dir.IsEmpty();
    if (!has_save_dir)
        custom_save_dir = _winCfg.DataDirectory;
    SetCheck(_hCustomSaveDirCheck, has_save_dir);
    char full_save_dir[MAX_PATH] = {0};
    MakeFullLongPath(custom_save_dir, full_save_dir, MAX_PATH);
    SetText(_hCustomSaveDir, full_save_dir);
    EnableWindow(_hCustomSaveDir, has_save_dir ? TRUE : FALSE);
    EnableWindow(_hCustomSaveDirBtn, has_save_dir ? TRUE : FALSE);

    // Resolution controls
    if (_winCfg.GameResolution.IsNull() &&
          (_winCfg.GameResType == kGameResolution_Undefined || _winCfg.GameResType == kGameResolution_Custom) ||
          _winCfg.GameColourDepth == 0)
        MessageBox(_hwnd, "Essential information about the game is missing in the configuration file. Setup program may be unable to deduce graphic modes properly.", "Initialization error", MB_OK | MB_ICONWARNING);

    if (_winCfg.GameResolution.IsNull())
        _winCfg.GameResolution = ResolutionTypeToSize(_winCfg.GameResType, _winCfg.LetterboxByDesign);

    if (!_winCfg.Windowed && _winCfg.ScreenSizeFromScaling)
        _specialGfxMode = _winCfg.MatchDeviceAspectRatio ? kGfxMode_FromScaling_KeepDeviceRatio : kGfxMode_FromScaling;
    else
        _specialGfxMode = kGfxMode_NoSpecial;

    SetText(_hwnd, _winCfg.Title);
    SetText(allegro_wnd, _winCfg.Title);
    SetText(_hGameResolutionText, String::FromFormat("Native game resolution: %d x %d x %d",
        _winCfg.GameResolution.Width, _winCfg.GameResolution.Height, _winCfg.GameColourDepth));

    SetText(_hVersionText, _winCfg.VersionString);

    InitGfxModes();

    if (_dx5.GfxModeList.GetModeCount() > 0)
        AddString(_hGfxDriverList, "DirectDraw 5", (DWORD_PTR)"DX5");
    if (_d3d9.GfxModeList.GetModeCount() > 0)
        AddString(_hGfxDriverList, "Direct3D 9", (DWORD_PTR)"D3D9");
    SetCurSelToItemDataStr(_hGfxDriverList, _winCfg.GfxDriverId.GetCStr(), 0);
    OnGfxDriverUpdate();

    SetCheck(_hWindowed, _winCfg.Windowed);
    OnWindowedUpdate();

    SetCheck(_hVSync, _winCfg.VSync);

    AddString(_hDigiDriverList, "No Digital Sound", DIGI_NONE);
    AddString(_hDigiDriverList, "Default DirectSound Device", DIGI_DIRECTAMX(0));
    AddString(_hDigiDriverList, "Default WaveOut Device", DIGI_WAVOUTID(0));
    AddString(_hDigiDriverList, "DirectSound (Hardware mixer)", DIGI_DIRECTX(0));
    // converting from legacy hard-coded indexes
    int digiwin_drv;
    switch (_winCfg.DigiWinIdx)
    {
    case 0:  digiwin_drv = DIGI_DIRECTAMX(0); break;
    case 1:  digiwin_drv = DIGI_WAVOUTID(0); break;
    case 3:  digiwin_drv = DIGI_DIRECTX(0); break;
    default: digiwin_drv = DIGI_NONE; break;
    }
    SetCurSelToItemData(_hDigiDriverList, digiwin_drv);

    AddString(_hMidiDriverList, "Disable MIDI music", MIDI_NONE);
    AddString(_hMidiDriverList, "Default MCI Music Device", MIDI_AUTODETECT);
    AddString(_hMidiDriverList, "Win32 MIDI Mapper", MIDI_WIN32MAPPER);
    // converting from legacy hard-coded indexes
    int midiwin_drv;
    switch (_winCfg.MidiWinIdx)
    {
    case 1:  midiwin_drv = MIDI_NONE; break;
    case 2:  midiwin_drv = MIDI_WIN32MAPPER; break;
    default: midiwin_drv = MIDI_AUTODETECT; break;
    }
    SetCurSelToItemData(_hMidiDriverList, midiwin_drv);

    FillLanguageList();

    SetCheck(_hMouseLock, _winCfg.MouseAutoLock);

    SetSliderRange(_hMouseSpeed, MouseSpeedMin, MouseSpeedMax);
    int slider_pos = (int)(_winCfg.MouseSpeed * 10.f + .5f);
    SetSliderPos(_hMouseSpeed, slider_pos);
    UpdateMouseSpeedText();

    AddString(_hSpriteCacheList, "10 MB", 10);
    AddString(_hSpriteCacheList, "20 MB (default)", 20);
    AddString(_hSpriteCacheList, "50 MB", 50);
    AddString(_hSpriteCacheList, "100 MB", 100);
    SetCurSelToItemData(_hSpriteCacheList, _winCfg.SpriteCacheSize / 1024);

    SetCheck(_hRefresh85Hz, _winCfg.RefreshRate == 85);
    SetCheck(_hAntialiasSprites, _winCfg.AntialiasSprites);
    SetCheck(_hUseVoicePack, _winCfg.UseVoicePack);

    if (_winCfg.GameColourDepth < 32)
        EnableWindow(_hReduce32to16, FALSE);
    else
        SetCheck(_hReduce32to16, _winCfg.Reduce32to16);

    if (!File::TestReadFile("speech.vox"))
        EnableWindow(_hUseVoicePack, FALSE);
    else
        SetCheck(_hUseVoicePack, _winCfg.UseVoicePack);

    if (INIreadint(_cfgIn, "disabled", "speechvox", 0) != 0)
        EnableWindow(_hUseVoicePack, FALSE);
    if (INIreadint(_cfgIn, "disabled", "16bit", 0) != 0)
        EnableWindow(_hReduce32to16, FALSE);
    if (INIreadint(_cfgIn, "disabled", "filters", 0) != 0)
        EnableWindow(_hGfxFilterList, FALSE);

    RECT win_rect, gfx_rect, adv_rect;
    GetWindowRect(_hwnd, &win_rect);
    GetWindowRect(GetDlgItem(_hwnd, IDC_GFXOPTIONS), &gfx_rect);
    _winSize.Width = win_rect.right - win_rect.left;
    _winSize.Height = win_rect.bottom - win_rect.top;
    GetWindowRect(_hAdvanced, &adv_rect);
    _baseSize.Width = (adv_rect.right + (gfx_rect.left - win_rect.left)) - win_rect.left;
    _baseSize.Height = win_rect.bottom - win_rect.top;

    MoveWindow(_hwnd, max(0, win_rect.left + (_winSize.Width - _baseSize.Width) / 2),
                      max(0, win_rect.top + (_winSize.Height - _baseSize.Height) / 2),
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
    case IDC_STRETCHTOSCREEN: OnFramePlacement(); break;
    case IDC_ASPECTRATIO: OnFramePlacement(); break;
    case IDC_CUSTOMSAVEDIRBTN: OnCustomSaveDirBtn(); break;
    case IDC_CUSTOMSAVEDIRCHECK: OnCustomSaveDirCheck(); break;
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
    case IDC_GFXFILTERSCALING: OnScalingUpdate(); break;
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
        SetText(_hCustomSaveDir, save_dir);
    }
}

void WinSetupDialog::OnCustomSaveDirCheck()
{
    bool custom_save_dir = GetCheck(_hCustomSaveDirCheck);
    EnableWindow(_hCustomSaveDir, custom_save_dir ? TRUE : FALSE);
    EnableWindow(_hCustomSaveDirBtn, custom_save_dir ? TRUE : FALSE);
}

void WinSetupDialog::OnFramePlacement()
{
    const bool stretch = GetCheck(_hStretchToScreen);

    if (stretch)
        EnableWindow(_hKeepAspectRatio, TRUE);
    else
    {
        EnableWindow(_hKeepAspectRatio, FALSE);
        SetCheck(_hKeepAspectRatio, false);
    }

    const bool keep_ratio = GetCheck(_hKeepAspectRatio);
    if (stretch)
        _winCfg.FramePlacement = keep_ratio ? "proportional": "stretch";
    else
        _winCfg.FramePlacement = "center";    
}

void WinSetupDialog::OnGfxDriverUpdate()
{
    _winCfg.GfxDriverId = (LPCTSTR)GetCurItemData(_hGfxDriverList);

    if (_winCfg.GfxDriverId.CompareNoCase("D3D9") == 0)
        _drvDesc = &_d3d9;
    else if (_winCfg.GfxDriverId.CompareNoCase("DX5") == 0)
        _drvDesc = &_dx5;

    FillGfxModeList();
    FillGfxFilterList();
}

void WinSetupDialog::OnGfxFilterUpdate()
{
    _winCfg.GfxFilterId = (LPCTSTR)GetCurItemData(_hGfxFilterList);

    _gfxFilterInfo = NULL;
    for (size_t i = 0; i < _drvDesc->FilterList.size(); ++i)
    {
        if (_drvDesc->FilterList[i].Id.CompareNoCase(_winCfg.GfxFilterId) == 0)
        {
            _gfxFilterInfo = &_drvDesc->FilterList[i];
            break;
        }
    }

    FillScalingList();
}

void WinSetupDialog::OnGfxModeUpdate()
{
    DWORD_PTR sel = GetCurItemData(_hGfxModeList);

    _winCfg.ScreenSizeFromScaling = _winCfg.Windowed;
    _winCfg.MatchDeviceAspectRatio = false;
    if (sel >= kGfxMode_FirstSpecial && sel <= kGfxMode_LastSpecial)
    {
        _specialGfxMode = (GfxModeSpecial)sel;
        if (sel == kGfxMode_Desktop)
            _winCfg.ScreenSize = _desktopSize;
        else if (sel == kGfxMode_GameRes)
            _winCfg.ScreenSize = _winCfg.GameResolution;
        else if (sel == kGfxMode_FromScaling || sel == kGfxMode_FromScaling_KeepDeviceRatio)
        {
            _winCfg.ScreenSize = _desktopSize;
            _winCfg.ScreenSizeFromScaling = true;
            _winCfg.MatchDeviceAspectRatio = sel == kGfxMode_FromScaling_KeepDeviceRatio;
        }
    }
    else
    {
        _specialGfxMode = kGfxMode_NoSpecial;
        if (sel == NULL)
            _winCfg.ScreenSize = Size();
        else
        {
            const DisplayMode &mode = *(const DisplayMode*)sel;
            _winCfg.ScreenSize = Size(mode.Width, mode.Height);
        }
    }
    
    if (_gfxFilterInfo)
        FillScalingList();
}

void WinSetupDialog::OnScalingUpdate()
{
    _winCfg.FilterScaling = GetCurItemData(_hGfxFilterScalingList);
    SetGfxModeText();
}

void WinSetupDialog::OnWindowedUpdate()
{
    _winCfg.Windowed = GetCheck(_hWindowed);
    if (_winCfg.Windowed)
    {
        ShowWindow(_hGfxModeList, SW_HIDE);
        ShowWindow(_hGfxModeText, SW_SHOW);
        EnableWindow(_hStretchToScreen, FALSE);
        EnableWindow(_hKeepAspectRatio, FALSE);
        SetCheck(_hStretchToScreen, true);
        SetCheck(_hKeepAspectRatio, true);
        SetGfxModeText();
        SelectNearestGfxMode(_winCfg.ScreenSize, _specialGfxMode);
    }
    else
    {
        ShowWindow(_hGfxModeList, SW_SHOW);
        ShowWindow(_hGfxModeText, SW_HIDE);
        SetFramePlacement(_winCfg.FramePlacement);
        SelectNearestGfxMode(_winCfg.ScreenSize, _specialGfxMode);
    }
}

void WinSetupDialog::ShowAdvancedOptions()
{
    // Reveal the advanced bit of the window
    ShowWindow(_hAdvanced, SW_HIDE);
    ShowWindow(_hDigiDriverList, SW_SHOW);
    ShowWindow(_hMidiDriverList, SW_SHOW);
    ShowWindow(_hUseVoicePack, SW_SHOW);
    ShowWindow(_hVSync, SW_SHOW);
    ShowWindow(_hRefresh85Hz, SW_SHOW);
    ShowWindow(_hAntialiasSprites, SW_SHOW);
    ShowWindow(_hReduce32to16, SW_SHOW);
    ShowWindow(_hSpriteCacheList, SW_SHOW);

    RECT win_rect;
    GetWindowRect(_hwnd, &win_rect);
    MoveWindow(_hwnd, max(0, win_rect.left + (_baseSize.Width - _winSize.Width) / 2),
                      max(0, win_rect.top + (_baseSize.Height - _winSize.Height) / 2),
                      _winSize.Width, _winSize.Height, TRUE);
}

INT_PTR CALLBACK WinSetupDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

void WinSetupDialog::AddScalingString(int scaling_factor)
{
    String s;
    if (scaling_factor > 1)
        s = String::FromFormat("x%d", scaling_factor);
    else if (scaling_factor < -1)
        s = String::FromFormat("1/%d", -scaling_factor);
    else if (scaling_factor == 0)
        s = "Max fit";
    else
        s = "None";
    AddString(_hGfxFilterScalingList, s, (DWORD_PTR)scaling_factor);
}

void WinSetupDialog::FillGfxFilterList()
{
    ResetContent(_hGfxFilterList);

    if (!_drvDesc)
    {
        OnGfxFilterUpdate();
        return;
    }

    for (size_t i = 0; i < _drvDesc->FilterList.size(); ++i)
    {
        const GfxFilterInfo &info = _drvDesc->FilterList[i];
        if (INIreadint(_cfgIn, "disabled", info.Id, 0) == 0)
            AddString(_hGfxFilterList, info.Name, (DWORD_PTR)info.Id.GetCStr());
    }

    SetCurSelToItemDataStr(_hGfxFilterList, _winCfg.GfxFilterId, 0);
    OnGfxFilterUpdate();
}

void WinSetupDialog::FillGfxModeList()
{
    DWORD_PTR old_sel = GetCurItemData(_hGfxModeList, kGfxMode_NoSpecial);
    // special case for initial UI setup
    if (old_sel == kGfxMode_NoSpecial && _specialGfxMode != kGfxMode_NoSpecial)
        old_sel = _specialGfxMode;
    ResetContent(_hGfxModeList);

    if (!_drvDesc)
    {
        OnGfxModeUpdate();
        return;
    }

    AddString(_hGfxModeList, String::FromFormat("Desktop resolution (%d x %d)",
        _desktopSize.Width, _desktopSize.Height), (DWORD_PTR)kGfxMode_Desktop);
    AddString(_hGfxModeList, String::FromFormat("Game resolution (%d x %d)",
        _winCfg.GameResolution.Width, _winCfg.GameResolution.Height), (DWORD_PTR)kGfxMode_GameRes);
    AddString(_hGfxModeList, "Bind to game scaling", (DWORD_PTR)kGfxMode_FromScaling);
    AddString(_hGfxModeList, "Bind to game scaling (force desktop ratio)", (DWORD_PTR)kGfxMode_FromScaling_KeepDeviceRatio);

    const std::vector<DisplayMode> &modes = _drvDesc->GfxModeList.Modes;
    const int use_colour_depth = _winCfg.GameColourDepth ? _winCfg.GameColourDepth : 32;
    String buf;
    GraphicResolution prev_mode;
    for (size_t i = 0; i < modes.size(); ++i)
    {
        const GraphicResolution &mode = modes[i];
        if (use_colour_depth == mode.ColorDepth &&
            !(mode.Width == _desktopSize.Width && mode.Height == _desktopSize.Height) &&
            !(mode.Width == _winCfg.GameResolution.Width && mode.Height == _winCfg.GameResolution.Height) &&
            // Sort of hack to hide different refresh rate modes (for now)
            prev_mode.Width != mode.Width && prev_mode.Height != mode.Height)
        {
            buf.Format("%d x %d", mode.Width, mode.Height);
            AddString(_hGfxModeList, buf, (DWORD_PTR)&mode);
            prev_mode = mode;
        }
    }

    SelectNearestGfxMode(_winCfg.ScreenSize, (GfxModeSpecial)old_sel);
}

void WinSetupDialog::FillLanguageList()
{
    ResetContent(_hLanguageList);
    AddString(_hLanguageList, _winCfg.DefaultLanguageName.GetCStr());
    SetCurSel(_hLanguageList, 0);

    String path_mask = String::FromFormat("%s\\*.tra", _winCfg.DataDirectory.GetCStr());
    WIN32_FIND_DATAA file_data;
    HANDLE find_handle = FindFirstFile(path_mask, &file_data);
    if (find_handle != INVALID_HANDLE_VALUE)
    {
        bool found_sel = false;
        do
        {
            LPTSTR ext = PathFindExtension(file_data.cFileName);
            if (ext && StrCmpI(ext, ".tra") == 0)
            {
                file_data.cFileName[0] = toupper(file_data.cFileName[0]);
                *ext = 0;
                int index = AddString(_hLanguageList, file_data.cFileName);
                if (!found_sel && _winCfg.Language.CompareNoCase(file_data.cFileName) == 0)
                {
                    SetCurSel(_hLanguageList, index);
                    found_sel = true;
                }
            }
        }
        while (FindNextFileA(find_handle, &file_data) != FALSE);
        FindClose(find_handle);
    }
}

void WinSetupDialog::FillScalingList()
{
    ResetContent(_hGfxFilterScalingList);

    if (!_gfxFilterInfo)
    {
        OnScalingUpdate();
        return;
    }

    {
        AddString(_hGfxFilterScalingList, "None", 1);
        AddString(_hGfxFilterScalingList, "Max fit", 0);

        if (!_winCfg.GameResolution.IsNull())
        {
            const int min_scale = min(_winCfg.GameResolution.Width / _minGameSize.Width, _winCfg.GameResolution.Height / _minGameSize.Height);
            const Size max_size = _winCfg.Windowed ? _maxWindowSize : _winCfg.ScreenSize;
            const int max_scale = min(max_size.Width / _winCfg.GameResolution.Width, max_size.Height / _winCfg.GameResolution.Height);

            // upscales
            const int upscale_from = _gfxFilterInfo && _gfxFilterInfo->MinScale > 2 ? _gfxFilterInfo->MinScale : 2;
            for (int scale = upscale_from; scale <= max_scale; ++scale)
                AddScalingString(scale);
            // downscales
            for (int scale = 2; scale <= min_scale; ++scale)
                AddScalingString(-scale);

            _maxGameScale = max(1, max_scale);
        }

        SetCurSelToItemData(_hGfxFilterScalingList, _winCfg.FilterScaling, NULL, kFilterScaling_MaxFit);
    }

    EnableWindow(_hGfxFilterScalingList, SendMessage(_hGfxFilterScalingList, CB_GETCOUNT, 0, 0) > 1 ? TRUE : FALSE);
    OnScalingUpdate();
}

void WinSetupDialog::InitGfxModes()
{
    InitDriverDescFromFactory("D3D9", _d3d9);
    InitDriverDescFromFactory("DX5", _dx5);

    if (_d3d9.GfxModeList.GetModeCount() == 0 && _dx5.GfxModeList.GetModeCount() == 0)
        MessageBox(_hwnd, "Unable to query available graphic modes.", "Initialization error", MB_OK | MB_ICONERROR);
}

void WinSetupDialog::InitDriverDescFromFactory(const String &id, DriverDesc &drv_desc)
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

    const int use_colour_depth = _winCfg.GameColourDepth ? _winCfg.GameColourDepth : 32;
    IGfxModeList *gfxm_list = gfx_driver->GetSupportedModeList(use_colour_depth);
    GfxModes &modes = drv_desc.GfxModeList;
    if (gfxm_list)
    {
        modes.Modes.resize(gfxm_list->GetModeCount());
        for (size_t i = 0; i < modes.Modes.size(); ++i)
            gfxm_list->GetMode(i, modes.Modes[i]);
        delete gfxm_list;
    }

    drv_desc.FilterList.resize(gfx_factory->GetFilterCount());
    for (size_t i = 0; i < drv_desc.FilterList.size(); ++i)
    {
        drv_desc.FilterList[i] = *gfx_factory->GetFilterInfo(i);
    }

    gfx_factory->Shutdown();
}

void WinSetupDialog::SaveSetup()
{
    const bool custom_save_dir = GetCheck(_hCustomSaveDirCheck);
    if (custom_save_dir)
    {
        // Compare user path with the game data directory. If user chose
        // path pointing inside game's directory, then store relative
        // path instead; thus the path will keep pointing at game's
        // directory if user moves game elsewhere.
        String save_dir;
        save_dir = GetText(_hCustomSaveDir);
        char full_data_dir[MAX_PATH] = {0};
        char full_save_dir[MAX_PATH] = {0};
        MakeFullLongPath(_winCfg.DataDirectory, full_data_dir, MAX_PATH);
        MakeFullLongPath(save_dir, full_save_dir, MAX_PATH);
        char rel_save_dir[MAX_PATH] = {0};
        if (PathRelativePathTo(rel_save_dir, full_data_dir, FILE_ATTRIBUTE_DIRECTORY, full_save_dir, FILE_ATTRIBUTE_DIRECTORY) &&
            strstr(rel_save_dir, "..") == NULL)
        {
            _winCfg.UserSaveDir = rel_save_dir;
        }
        else
        {
            _winCfg.UserSaveDir = save_dir;
        }
    }
    else
    {
        _winCfg.UserSaveDir = "";
    }

    int digiwin_drv = GetCurItemData(_hDigiDriverList);
    // converting to legacy hard-coded indexes
    switch (digiwin_drv)
    {
    case DIGI_DIRECTAMX(0): _winCfg.DigiWinIdx = 0; break;
    case DIGI_WAVOUTID(0):  _winCfg.DigiWinIdx = 1; break;
    case DIGI_DIRECTX(0):   _winCfg.DigiWinIdx = 3; break;
    default:                _winCfg.DigiWinIdx = 2; break;
    }

    int midiwin_drv = GetCurItemData(_hMidiDriverList);
    // converting to legacy hard-coded indexes
    switch (midiwin_drv)
    {
    case MIDI_NONE:        _winCfg.MidiWinIdx = 1; break;
    case MIDI_WIN32MAPPER: _winCfg.MidiWinIdx = 2; break;
    default:               _winCfg.MidiWinIdx = 0; break;
    }

    if (GetCurSel(_hLanguageList) == 0)
        _winCfg.Language.Empty();
    else
        _winCfg.Language = GetText(_hLanguageList);
    _winCfg.SpriteCacheSize = GetCurItemData(_hSpriteCacheList) * 1024;
    _winCfg.UseVoicePack = GetCheck(_hUseVoicePack);
    _winCfg.VSync = GetCheck(_hVSync);
    _winCfg.AntialiasSprites = GetCheck(_hAntialiasSprites);
    _winCfg.RefreshRate = GetCheck(_hRefresh85Hz) ? 85 : 0;
    _winCfg.Reduce32to16 = GetCheck(_hReduce32to16);
    _winCfg.GfxFilterId = (LPCTSTR)GetCurItemData(_hGfxFilterList);

    _winCfg.MouseAutoLock = GetCheck(_hMouseLock);
    int slider_pos = GetSliderPos(_hMouseSpeed);
    _winCfg.MouseSpeed = (float)slider_pos / 10.f;

    _winCfg.Save(_cfgOut);
}

void WinSetupDialog::SelectNearestGfxMode(const Size screen_size, GfxModeSpecial gfx_mode_spec)
{
    if (!_drvDesc)
    {
        OnGfxModeUpdate();
        return;
    }

    if (gfx_mode_spec >= kGfxMode_FirstSpecial && gfx_mode_spec <= kGfxMode_LastSpecial)
        SetCurSelToItemData(_hGfxModeList, gfx_mode_spec, NULL, kGfxMode_Desktop);
    else
    {
        // Look up for the nearest supported mode
        Size found_size = screen_size;
        int index = -1;
        if (find_nearest_supported_mode(_drvDesc->GfxModeList, found_size, &index, _winCfg.GameColourDepth != 0 ? _winCfg.GameColourDepth : 32))
        {
            SetCurSelToItemData(_hGfxModeList, (DWORD_PTR)&_drvDesc->GfxModeList.Modes[index], NULL, kGfxMode_Desktop);
        }
        else
            SetCurSelToItemData(_hGfxModeList, kGfxMode_Desktop);
    }
    OnGfxModeUpdate();
}

void WinSetupDialog::SetFramePlacement(const String &frame_place)
{
    EnableWindow(_hStretchToScreen, TRUE);
    if (frame_place.CompareNoCase("stretch") == 0)
    {
        EnableWindow(_hKeepAspectRatio, TRUE);
        SetCheck(_hStretchToScreen, true);
        SetCheck(_hKeepAspectRatio, false);
    }
    else if (frame_place.CompareNoCase("proportional") == 0)
    {
        EnableWindow(_hKeepAspectRatio, TRUE);
        SetCheck(_hStretchToScreen, true);
        SetCheck(_hKeepAspectRatio, true);
    }
    else
    {
        EnableWindow(_hKeepAspectRatio, FALSE);
        SetCheck(_hStretchToScreen, false);
        SetCheck(_hKeepAspectRatio, false);
    }
}

void WinSetupDialog::SetGfxModeText()
{
    int width, height, scaling;
    if (_winCfg.FilterScaling == 0)
        scaling = _maxGameScale;
    else
        scaling = _winCfg.FilterScaling;

    if (scaling >= 0)
    {
        width = _winCfg.GameResolution.Width * scaling;
        height = _winCfg.GameResolution.Height * scaling;
    }
    else
    {
        width = _winCfg.GameResolution.Width / (-scaling);
        height = _winCfg.GameResolution.Height / (-scaling);
    }
    String text = String::FromFormat("%d x %d", width, height);
    SetText(_hGfxModeText, text);
}

void WinSetupDialog::UpdateMouseSpeedText()
{
    int slider_pos = GetSliderPos(_hMouseSpeed);
    float mouse_speed = (float)slider_pos / 10.f;
    String text = mouse_speed == 1.f ? "Mouse speed: x 1.0 (Default)" : String::FromFormat("Mouse speed: x %0.1f", mouse_speed);
    SetText(_hMouseSpeedText, text);
}

//=============================================================================
//
// Windows setup entry point.
//
//=============================================================================
void SetWinIcon()
{
    SetClassLong(allegro_wnd,GCL_HICON,
        (LONG) LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON))); 
}

SetupReturnValue WinSetup(const ConfigTree &cfg_in, ConfigTree &cfg_out,
                          const String &game_data_dir, const String &version_str)
{
    return WinSetupDialog::ShowModal(cfg_in, cfg_out, game_data_dir, version_str);
}

} // namespace Engine
} // namespace AGS
