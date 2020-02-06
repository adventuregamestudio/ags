using System;
using System.ComponentModel;

namespace AGS.Editor.Preferences
{
    public interface IAppSettings
    {
        int BackupWarningInterval { get; set; }
        string ColorTheme { get; set; }
        string DefaultImportPath { get; set; }
        bool DialogOnMultipleTabsClose { get; set; }
        bool IndentUseTabs { get; set; }
        bool KeepHelpOnTop { get; set; }
        DateTime LastBackupWarning { get; set; }
        int MainWinHeight { get; set; }
        bool MainWinMaximize { get; set; }
        int MainWinWidth { get; set; }
        int MainWinX { get; set; }
        int MainWinY { get; set; }
        MessageBoxOnCompile MessageBoxOnCompile { get; set; }
        bool MigratedSettings { get; set; }
        string NewGamePath { get; set; }
        string PaintProgramPath { get; set; }
        BindingList<RecentGame> RecentGames { get; }
        BindingList<string> RecentSearches { get; }
        ReloadScriptOnExternalChange ReloadScriptOnExternalChange { get; set; }
        bool RemapPalettizedBackgrounds { get; set; }
        bool SendAnonymousStats { get; set; }
        bool ShowViewPreviewByDefault { get; set; }
        SpriteImportMethod SpriteImportMethod { get; set; }
        StartupPane StartupPane { get; set; }
        DateTime StatsLastSent { get; set; }
        int TabSize { get; set; }
        TestGameWindowStyle TestGameWindowStyle { get; set; }
        bool UpgradedSettings { get; set; }
        void Save();
    }
}