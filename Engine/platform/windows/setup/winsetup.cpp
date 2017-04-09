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
#include "util/stdtr1compat.h"
#include TR1INCLUDE(memory)
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

extern "C"
{
    HWND win_get_window();
}

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
    Size   ScreenSize;
    FrameScaleDefinition ScaleDef;
    int    ScaleFactor;
    int    RefreshRate;
    bool   Windowed;
    bool   VSync;
    bool   RenderAtScreenRes;
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
    ScreenSize = get_desktop_size();
    ScaleDef = kFrame_MaxRound;
    ScaleFactor = 0;
    RefreshRate = 0;
    Windowed = false;
    VSync = false;
    RenderAtScreenRes = false;
    AntialiasSprites = false;
    Reduce32to16 = false;

    MouseAutoLock = false;
    MouseSpeed = 1.f;

    DigiWinIdx = 0;
    MidiWinIdx = 0;
    UseVoicePack = true;

    SpriteCacheSize = 1024 * 128;
    DefaultLanguageName = "Game Default";

    Title = "Game Setup";
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
    ScreenSize.Width = INIreadint(cfg, "graphics", "screen_width", ScreenSize.Width);
    ScreenSize.Height = INIreadint(cfg, "graphics", "screen_height", ScreenSize.Height);
    
    parse_scaling_option(INIreadstring(cfg, "graphics", "game_scale", make_scaling_option(ScaleDef, ScaleFactor)),
        ScaleDef, ScaleFactor);

    RefreshRate = INIreadint(cfg, "graphics", "refresh", RefreshRate);
    Windowed = INIreadint(cfg, "graphics", "windowed", Windowed ? 1 : 0) != 0;
    VSync = INIreadint(cfg, "graphics", "vsync", VSync ? 1 : 0) != 0;
    RenderAtScreenRes = INIreadint(cfg, "graphics", "render_at_screenres", RenderAtScreenRes ? 1 : 0) != 0;

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
    INIwritestring(cfg, "graphics", "screen_def", Windowed ? "scaling" : "explicit");
    INIwriteint(cfg, "graphics", "screen_width", ScreenSize.Width);
    INIwriteint(cfg, "graphics", "screen_height", ScreenSize.Height);
    INIwritestring(cfg, "graphics", "game_scale", make_scaling_option(ScaleDef, ScaleFactor));
    INIwriteint(cfg, "graphics", "refresh", RefreshRate);
    INIwriteint(cfg, "graphics", "windowed", Windowed ? 1 : 0);
    INIwriteint(cfg, "graphics", "vsync", VSync ? 1 : 0);
    INIwriteint(cfg, "graphics", "render_at_screenres", RenderAtScreenRes ? 1 : 0);

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

int InsertString(HWND hwnd, LPCTSTR text, int at_index, DWORD_PTR data = 0L)
{
    int index = SendMessage(hwnd, CB_INSERTSTRING, at_index, (LPARAM)text);
    if (index >= 0)
        SendMessage(hwnd, CB_SETITEMDATA, index, data);
    return index;
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
        kGfxMode_None = -1,
        kGfxMode_Desktop,
        kGfxMode_GameRes,
        kNumGfxModeSpecials
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
    void OnGfxDriverUpdate();
    void OnGfxFilterUpdate();
    void OnGfxModeUpdate();
    void OnScalingUpdate();
    void OnWindowedUpdate();
    void ShowAdvancedOptions();

    // Helper structs
    typedef std::vector<DisplayMode> VDispModes;
    struct GfxModes : public IGfxModeList
    {
        VDispModes Modes;

        virtual int  GetModeCount() const;
        virtual bool GetMode(int index, DisplayMode &mode) const;
    };

    typedef std::vector<GfxFilterInfo> VFilters;
    struct DriverDesc
    {
        String      Id;
        String      UserName;
        GfxModes    GfxModeList;
        VFilters    FilterList;
    };

    // Operations
    void AddScalingString(int scaling_factor);
    void FillGfxFilterList();
    void FillGfxModeList();
    void FillLanguageList();
    void FillScalingList();
    void InitGfxModes();
    void InitDriverDescFromFactory(const String &id);
    void SaveSetup();
    void SelectNearestGfxMode(const Size screen_size);
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
    typedef stdtr1compat::shared_ptr<DriverDesc> PDriverDesc;
    typedef std::map<String, PDriverDesc> DriverDescMap;
    DriverDescMap _drvDescMap;
    PDriverDesc _drvDesc;
    GfxFilterInfo _gfxFilterInfo;
    // Resolution limits
    Size _desktopSize;
    Size _maxWindowSize;
    Size _minGameSize;
    int _maxGameScale;
    int _minGameScale;

    // Dialog controls
    HWND _hVersionText;
    HWND _hCustomSaveDir;
    HWND _hCustomSaveDirBtn;
    HWND _hCustomSaveDirCheck;
    HWND _hGfxDriverList;
    HWND _hGfxModeList;
    HWND _hGfxFilterList;
    HWND _hGameScalingList;
    HWND _hDigiDriverList;
    HWND _hMidiDriverList;
    HWND _hLanguageList;
    HWND _hSpriteCacheList;
    HWND _hWindowed;
    HWND _hVSync;
    HWND _hRenderAtScreenRes;
    HWND _hRefresh85Hz;
    HWND _hAntialiasSprites;
    HWND _hReduce32to16;
    HWND _hUseVoicePack;
    HWND _hAdvanced;
    HWND _hGameResolutionText;
    HWND _hGfxModeText;
    HWND _hMouseLock;
    HWND _hMouseSpeed;
    HWND _hMouseSpeedText;
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
    INT_PTR dlg_res = DialogBoxParam(GetModuleHandle(NULL), (LPCTSTR)IDD_SETUP, win_get_window(),
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
    _hGameScalingList       = GetDlgItem(_hwnd, IDC_GAMESCALING);
    _hDigiDriverList        = GetDlgItem(_hwnd, IDC_DIGISOUND);
    _hMidiDriverList        = GetDlgItem(_hwnd, IDC_MIDIMUSIC);
    _hLanguageList          = GetDlgItem(_hwnd, IDC_LANGUAGE);
    _hSpriteCacheList       = GetDlgItem(_hwnd, IDC_SPRITECACHE);
    _hWindowed              = GetDlgItem(_hwnd, IDC_WINDOWED);
    _hVSync                 = GetDlgItem(_hwnd, IDC_VSYNC);
    _hRenderAtScreenRes     = GetDlgItem(_hwnd, IDC_RENDERATSCREENRES);
    _hRefresh85Hz           = GetDlgItem(_hwnd, IDC_REFRESH_85HZ);
    _hAntialiasSprites      = GetDlgItem(_hwnd, IDC_ANTIALIAS);
    _hReduce32to16          = GetDlgItem(_hwnd, IDC_REDUCE32TO16);
    _hUseVoicePack          = GetDlgItem(_hwnd, IDC_VOICEPACK);
    _hAdvanced              = GetDlgItem(_hwnd, IDC_ADVANCED);
    _hGameResolutionText    = GetDlgItem(_hwnd, IDC_RESOLUTION);
    _hGfxModeText           = GetDlgItem(_hwnd, IDC_GFXMODETEXT);
    _hMouseLock             = GetDlgItem(_hwnd, IDC_MOUSE_AUTOLOCK);
    _hMouseSpeed            = GetDlgItem(_hwnd, IDC_MOUSESPEED);
    _hMouseSpeedText        = GetDlgItem(_hwnd, IDC_MOUSESPEED_TEXT);

    _desktopSize = get_desktop_size();
    _maxWindowSize = _desktopSize;
    AGSPlatformDriver::GetDriver()->ValidateWindowSize(_maxWindowSize.Width, _maxWindowSize.Height, false);
    _minGameSize = Size(320, 200);
    _maxGameScale = 1;
    _minGameScale = 1;

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

    SetText(_hwnd, _winCfg.Title);
    SetText(win_get_window(), _winCfg.Title);
    SetText(_hGameResolutionText, String::FromFormat("Native game resolution: %d x %d x %d",
        _winCfg.GameResolution.Width, _winCfg.GameResolution.Height, _winCfg.GameColourDepth));

    SetText(_hVersionText, _winCfg.VersionString);

    InitGfxModes();

    for (DriverDescMap::const_iterator it = _drvDescMap.begin(); it != _drvDescMap.end(); ++it)
    {
        if (it->second->GfxModeList.GetModeCount() > 0)
            AddString(_hGfxDriverList, it->second->UserName, (DWORD_PTR)it->second->Id.GetCStr());
    }
    SetCurSelToItemDataStr(_hGfxDriverList, _winCfg.GfxDriverId.GetCStr(), 0);
    OnGfxDriverUpdate();

    SetCheck(_hWindowed, _winCfg.Windowed);
    OnWindowedUpdate();

    SetCheck(_hVSync, _winCfg.VSync);

    SetCheck(_hRenderAtScreenRes, _winCfg.RenderAtScreenRes);

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

    AddString(_hSpriteCacheList, "16 MB", 16);
    AddString(_hSpriteCacheList, "32 MB", 32);
    AddString(_hSpriteCacheList, "64 MB", 64);
    AddString(_hSpriteCacheList, "128 MB (default)", 128);
    AddString(_hSpriteCacheList, "256 MB", 256);
    AddString(_hSpriteCacheList, "384 MB", 384);
    AddString(_hSpriteCacheList, "512 MB", 512);
    SetCurSelToItemData(_hSpriteCacheList, _winCfg.SpriteCacheSize / 1024, NULL, 3);

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

    if (INIreadint(_cfgIn, "disabled", "render_at_screenres", 0) != 0)
        EnableWindow(_hRenderAtScreenRes, FALSE);

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
    case IDC_GAMESCALING: OnScalingUpdate(); break;
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

void WinSetupDialog::OnGfxDriverUpdate()
{
    _winCfg.GfxDriverId = (LPCTSTR)GetCurItemData(_hGfxDriverList);

    DriverDescMap::const_iterator it = _drvDescMap.find(_winCfg.GfxDriverId);
    if (it != _drvDescMap.end())
        _drvDesc = it->second;

    FillGfxModeList();
    FillGfxFilterList();
}

void WinSetupDialog::OnGfxFilterUpdate()
{
    _winCfg.GfxFilterId = (LPCTSTR)GetCurItemData(_hGfxFilterList);

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
    DWORD_PTR sel = GetCurItemData(_hGfxModeList);
    if (sel == kGfxMode_Desktop)
        _winCfg.ScreenSize = _desktopSize;
    else if (sel == kGfxMode_GameRes)
        _winCfg.ScreenSize = _winCfg.GameResolution;
    else
    {
        const DisplayMode &mode = *(const DisplayMode*)sel;
        _winCfg.ScreenSize = Size(mode.Width, mode.Height);
    }
}

void WinSetupDialog::OnScalingUpdate()
{
    int scale = GetCurItemData(_hGameScalingList);
    if (scale >= 0 && scale < kNumFrameScaleDef)
    {
        _winCfg.ScaleDef = (FrameScaleDefinition)scale;
        _winCfg.ScaleFactor = 0;
    }
    else
    {
        _winCfg.ScaleDef = kFrame_IntScale;
        _winCfg.ScaleFactor = scale >= 0 ? scale - kNumFrameScaleDef : scale;
    }

    SetGfxModeText();
}

void WinSetupDialog::OnWindowedUpdate()
{
    _winCfg.Windowed = GetCheck(_hWindowed);

    if (_winCfg.Windowed)
    {
        ShowWindow(_hGfxModeList, SW_HIDE);
        ShowWindow(_hGfxModeText, SW_SHOW);
        SetGfxModeText();
    }
    else
    {
        ShowWindow(_hGfxModeList, SW_SHOW);
        ShowWindow(_hGfxModeText, SW_HIDE);
    }

    SelectNearestGfxMode(_winCfg.ScreenSize);
    FillScalingList();
}

void WinSetupDialog::ShowAdvancedOptions()
{
    // Reveal the advanced bit of the window
    ShowWindow(_hAdvanced, SW_HIDE);

    RECT win_rect;
    GetWindowRect(_hwnd, &win_rect);
    MoveWindow(_hwnd, max(0, win_rect.left + (_baseSize.Width - _winSize.Width) / 2),
                      max(0, win_rect.top + (_baseSize.Height - _winSize.Height) / 2),
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
    if (scaling_factor >= 0)
        s = String::FromFormat("x%d", scaling_factor);
    else
        s = String::FromFormat("1/%d", -scaling_factor);
    AddString(_hGameScalingList, s, (DWORD_PTR)(scaling_factor >= 0 ? scaling_factor + kNumFrameScaleDef : scaling_factor));
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
        if (INIreadint(_cfgIn, "disabled", info.Id, 0) == 0)
            AddString(_hGfxFilterList, info.Name, (DWORD_PTR)info.Id.GetCStr());
    }

    SetCurSelToItemDataStr(_hGfxFilterList, _winCfg.GfxFilterId, 0);
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
    const int use_colour_depth = _winCfg.GameColourDepth ? _winCfg.GameColourDepth : 32;
    String buf;
    GraphicResolution prev_mode;
    bool has_desktop_mode = false;
    bool has_native_mode = false;
    for (size_t i = 0; i < modes.size(); ++i)
    {
        const GraphicResolution &mode = modes[i];
        if (use_colour_depth == mode.ColorDepth &&
            // Sort of hack to hide different refresh rate modes (for now)
            prev_mode.Width != mode.Width && prev_mode.Height != mode.Height)
        {
            if (mode.Width == _desktopSize.Width && mode.Height == _desktopSize.Height)
            {
                has_desktop_mode = true;
                continue;
            }
            else if (mode.Width == _winCfg.GameResolution.Width && mode.Height == _winCfg.GameResolution.Height)
            {
                has_native_mode = true;
                continue;
            }

            buf.Format("%d x %d", mode.Width, mode.Height);
            AddString(_hGfxModeList, buf, (DWORD_PTR)&mode);
            prev_mode = mode;
        }
    }

    int spec_mode_idx = 0;
    if (has_desktop_mode)
        InsertString(_hGfxModeList, String::FromFormat("Desktop resolution (%d x %d)",
            _desktopSize.Width, _desktopSize.Height), spec_mode_idx++, (DWORD_PTR)kGfxMode_Desktop);
    if (has_native_mode)
        InsertString(_hGfxModeList, String::FromFormat("Native game resolution (%d x %d)",
            _winCfg.GameResolution.Width, _winCfg.GameResolution.Height), spec_mode_idx++, (DWORD_PTR)kGfxMode_GameRes);

    SelectNearestGfxMode(_winCfg.ScreenSize);
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
    ResetContent(_hGameScalingList);

    const int min_scale = min(_winCfg.GameResolution.Width / _minGameSize.Width, _winCfg.GameResolution.Height / _minGameSize.Height);
    const Size max_size = _winCfg.Windowed ? _maxWindowSize : _winCfg.ScreenSize;
    const int max_scale = min(max_size.Width / _winCfg.GameResolution.Width, max_size.Height / _winCfg.GameResolution.Height);
    _maxGameScale = max(1, max_scale);
    _minGameScale = -max(1, min_scale);

    if (_winCfg.Windowed)
        AddString(_hGameScalingList, "None", 1 + kNumFrameScaleDef);

    AddString(_hGameScalingList, "Max round multiplier", kFrame_MaxRound);
    AddString(_hGameScalingList, "Stretch to fit screen", kFrame_MaxStretch);
    AddString(_hGameScalingList, "Stretch to fit screen (preserve aspect ratio)", kFrame_MaxProportional);

    if (_winCfg.Windowed && !_winCfg.GameResolution.IsNull())
    {
        // Add integer multipliers
        for (int scale = 2; scale <= _maxGameScale; ++scale)
            AddScalingString(scale);
    }

    SetCurSelToItemData(_hGameScalingList,
        _winCfg.ScaleDef == kFrame_IntScale ? _winCfg.ScaleFactor + kNumFrameScaleDef : _winCfg.ScaleDef, NULL, 0);

    EnableWindow(_hGameScalingList, SendMessage(_hGameScalingList, CB_GETCOUNT, 0, 0) > 1 ? TRUE : FALSE);
    OnScalingUpdate();
}

void WinSetupDialog::InitGfxModes()
{
    InitDriverDescFromFactory("D3D9");
    InitDriverDescFromFactory("DX5");
    InitDriverDescFromFactory("OGL");

    size_t total_modes = 0;
    for (DriverDescMap::const_iterator it = _drvDescMap.begin(); it != _drvDescMap.end(); ++it)
        total_modes += it->second->GfxModeList.Modes.size();
    if (total_modes == 0)
        MessageBox(_hwnd, "Unable to query available graphic modes.", "Initialization error", MB_OK | MB_ICONERROR);
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

    const int use_colour_depth = _winCfg.GameColourDepth ? _winCfg.GameColourDepth : 32;
    IGfxModeList *gfxm_list = gfx_driver->GetSupportedModeList(use_colour_depth);
    VDispModes &modes = drv_desc->GfxModeList.Modes;
    if (gfxm_list)
    {
        modes.resize(gfxm_list->GetModeCount());
        for (size_t i = 0; i < modes.size(); ++i)
            gfxm_list->GetMode(i, modes[i]);
        delete gfxm_list;
    }

    drv_desc->FilterList.resize(gfx_factory->GetFilterCount());
    for (size_t i = 0; i < drv_desc->FilterList.size(); ++i)
    {
        drv_desc->FilterList[i] = *gfx_factory->GetFilterInfo(i);
    }

    gfx_factory->Shutdown();
    _drvDescMap[drv_desc->Id] = drv_desc;
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
    _winCfg.RenderAtScreenRes = GetCheck(_hRenderAtScreenRes);
    _winCfg.AntialiasSprites = GetCheck(_hAntialiasSprites);
    _winCfg.RefreshRate = GetCheck(_hRefresh85Hz) ? 85 : 0;
    _winCfg.Reduce32to16 = GetCheck(_hReduce32to16);
    _winCfg.GfxFilterId = (LPCTSTR)GetCurItemData(_hGfxFilterList);

    _winCfg.MouseAutoLock = GetCheck(_hMouseLock);
    int slider_pos = GetSliderPos(_hMouseSpeed);
    _winCfg.MouseSpeed = (float)slider_pos / 10.f;

    _winCfg.Save(_cfgOut);
}

void WinSetupDialog::SelectNearestGfxMode(const Size screen_size)
{
    if (!_drvDesc)
    {
        OnGfxModeUpdate();
        return;
    }

    // First check two special modes
    if (screen_size == _desktopSize)
    {
        SetCurSel(_hGfxModeList, kGfxMode_Desktop);
    }
    else if (screen_size == _winCfg.GameResolution)
    {
        SetCurSel(_hGfxModeList, kGfxMode_GameRes);
    }
    else
    {
        // Look up for the nearest supported mode
        int index = -1;
        DisplayMode dm;
        if (find_nearest_supported_mode(_drvDesc->GfxModeList, screen_size, _winCfg.GameColourDepth != 0 ? _winCfg.GameColourDepth : 32,
                                        NULL, NULL, dm, &index))
        {
            SetCurSelToItemData(_hGfxModeList, (DWORD_PTR)&_drvDesc->GfxModeList.Modes[index], NULL, kGfxMode_Desktop);
        }
        else
            SetCurSel(_hGfxModeList, kGfxMode_Desktop);
    }
    OnGfxModeUpdate();
}

void WinSetupDialog::SetGfxModeText()
{
    Size sz;
    if (_winCfg.ScaleDef == kFrame_MaxStretch)
    {
        sz = _maxWindowSize;
    }
    else if (_winCfg.ScaleDef == kFrame_MaxProportional)
    {
        sz = ProportionalStretch(_maxWindowSize, _winCfg.GameResolution);
    }
    else
    {
        int scale = 0;
        if (_winCfg.ScaleDef == kFrame_MaxRound)
            scale = _maxGameScale;
        else
            scale = _winCfg.ScaleFactor;

        if (scale >= 0)
        {
            sz.Width  = _winCfg.GameResolution.Width * scale;
            sz.Height = _winCfg.GameResolution.Height * scale;
        }
        else
        {
            sz.Width  = _winCfg.GameResolution.Width / (-scale);
            sz.Height = _winCfg.GameResolution.Height / (-scale);
        }
    }
    String text = String::FromFormat("%d x %d", sz.Width, sz.Height);
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
    SetClassLong(win_get_window(),GCL_HICON,
        (LONG) LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON))); 
}

SetupReturnValue WinSetup(const ConfigTree &cfg_in, ConfigTree &cfg_out,
                          const String &game_data_dir, const String &version_str)
{
    return WinSetupDialog::ShowModal(cfg_in, cfg_out, game_data_dir, version_str);
}

} // namespace Engine
} // namespace AGS
