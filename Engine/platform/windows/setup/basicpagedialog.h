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
#ifndef __AGS_EE_SETUP__BASICPAGEDIALOG_H
#define __AGS_EE_SETUP__BASICPAGEDIALOG_H

#include "core/platform.h"

#if AGS_PLATFORM_OS_WINDOWS

#include <map>
#include <memory>
#include <vector>
#include "ac/gamestructdefines.h"
#include "ac/gamesetup.h"
#include "gfx/gfxfilter.h"
#include "gfx/gfxmodelist.h"
#include "main/graphics_mode.h"
#include "resource/resource.h"
#include "platform/windows/setup/winpagedialog.h"
#include "util/ini_util.h"

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
struct WinConfig : public GameConfig
{
    // Meta properties
    String Title;
    String VersionString;
    String DataDirectory;
    GameResolutionType GameResType;
    Size   GameResolution;
    int    GameColourDepth;
    bool   LetterboxByDesign;
    String DefaultLanguageName; // human-readable title for the "default language" selection

    WinConfig();
    void SetDefaults();
    void LoadMeta(const ConfigTree &cfg);
    void LoadCommon(const ConfigTree &cfg, const Size &desktop_res);
    void Save(ConfigTree &cfg, const Size &desktop_res) const;
};

//=============================================================================
//
// WinSetupPageDialog is a parent class for a page dialog
// with setup controls.
//
//=============================================================================
class WinSetupPageDialog : public WinPageDialog
{
public:
    WinSetupPageDialog(WinConfig &win_cfg, const ConfigTree &cfg_in)
        : _winCfg(win_cfg)
        , _cfgIn(cfg_in)
    {}

    virtual void ResetSetup() { /* do nothing */ };
    virtual void SaveSetup() { /* do nothing */ };

protected:
    WinConfig &_winCfg;
    const ConfigTree &_cfgIn;
    bool _isInit = false;
};

//=============================================================================
//
// BasicPageDialog
//
//=============================================================================
class BasicPageDialog : public WinSetupPageDialog
{
public:
    BasicPageDialog(WinConfig &win_cfg, const ConfigTree &cfg_in)
        : WinSetupPageDialog(win_cfg, cfg_in) {}

    String GetTitle() const override { return "Basic"; }

    void ResetSetup() override;
    void SaveSetup() override;

protected:
    UINT GetTemplateID() const override { return IDD_PAGE_BASIC; }

    // Event handlers
    INT_PTR OnInitDialog() override;
    INT_PTR OnCommand(WORD id) override;
    INT_PTR OnListSelection(WORD id) override;

private:
    enum GfxModeSpecial
    {
        kGfxMode_None    = -1,
        kGfxMode_Desktop = -2,
        kGfxMode_GameRes = -3,
    };

    // Event handlers
    void OnGfxDriverUpdate();
    void OnGfxFilterUpdate();
    void OnGfxModeUpdate();
    void OnFullScalingUpdate();
    void OnWinScalingUpdate();
    void OnWindowedUpdate();
    void OnFullscreenDesktop();

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
    void FillGfxDriverList();
    void FillGfxFilterList();
    void FillGfxModeList();
    void FillLanguageList();
    void FillScalingList(HWND hlist, bool windowed);
    void SetScalingSelection();
    void InitDriverDescFromFactory(const String &id);
    void SelectNearestGfxMode(const WindowSetup &ws);

    // Dialog properties
    // Graphics driver descriptions
    typedef std::shared_ptr<DriverDesc> PDriverDesc;
    typedef std::map<String, PDriverDesc> DriverDescMap;
    DriverDescMap _drvDescMap;
    PDriverDesc _drvDesc;
    GfxFilterInfo _gfxFilterInfo;
    // Resolution limits
    Size _desktopSize;
    Size _maxWindowSize;

    // Dialog controls
    HWND _hGameResolutionText = NULL;
    HWND _hWindowed = NULL;
    HWND _hGfxDriverList = NULL;
    HWND _hGfxModeList = NULL;
    HWND _hFullscreenDesktop = NULL;
    HWND _hFsScalingList = NULL;
    HWND _hWinScalingList = NULL;
    HWND _hGfxFilterList = NULL;
    HWND _hLanguageList = NULL;
};

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS

#endif __AGS_EE_SETUP__BASICPAGEDIALOG_H
