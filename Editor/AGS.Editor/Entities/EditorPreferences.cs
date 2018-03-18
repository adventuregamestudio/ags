using System;
using System.Configuration;
using System.IO;
using Microsoft.Win32;

namespace AGS.Editor.Preferences
{
    public enum EditorStartupPane
    {
        StartPage = 0,
        GeneralSettings = 1,
        None = 2
    }

    public enum MessageBoxOnCompile
    {
        WarningsAndErrors = 0,
        OnlyErrors = 1,
        Never = 2
    }

    public enum SpriteImportMethod
    {
        Pixel0 = 0,
        TopLeft = 1,
        BottomLeft = 2,
        TopRight = 3,
        BottomRight = 4,
        LeaveAsIs = 5,
        NoTransparency = 6
    }

    public enum TestGameWindowStyle
    {
        UseGameSetup = 0,
        FullScreen = 1,
        Windowed = 2
    }

    public sealed class AppSettings : ApplicationSettingsBase
    {
        public AppSettings()
        {
            SettingsLoaded += new SettingsLoadedEventHandler(Settings_SettingsLoaded);
        }

        private void Settings_SettingsLoaded(object sender, SettingsLoadedEventArgs e)
        {
            // - called when the first setting is requested and caches all values
            // - base class will handle bad input so just logic fixes needed

            if (!MigratedSettings)
            {
                MigratedSettings = GetSettingsFromRegistry();
            }

            if (DefaultImportPath == String.Empty && !Directory.Exists(DefaultImportPath))
            {
                DefaultImportPath = String.Empty;
            }

            if (PaintProgramPath == String.Empty && !File.Exists(PaintProgramPath))
            {
                PaintProgramPath = String.Empty;
            }

            if (ColorTheme == String.Empty)
            {
                ColorTheme = ColorThemeStub.DEFAULT.Name;
            }
        }

        private bool GetSettingsFromRegistry()
        {
            RegistryKey key = Registry.CurrentUser.OpenSubKey(AGSEditor.AGS_REGISTRY_KEY);

            if (key != null)
            {
                // get registry settings here
                // return false to retry later
                key.Close();
            }

            return true;
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("False")]
        public bool MigratedSettings
        {
            get
            {
                return (bool)(this["MigratedSettings"]);
            }
            set
            {
                this["MigratedSettings"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("UseGameSetup")]
        public TestGameWindowStyle TestGameWindowStyle
        {
            get
            {
                return (TestGameWindowStyle)(this["TestGameWindowStyle"]);
            }
            set
            {
                this["TestGameWindowStyle"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("StartPage")]
        public EditorStartupPane EditorStartupPane
        {
            get
            {
                return (EditorStartupPane)(this["EditorStartupPane"]);
            }
            set
            {
                this["EditorStartupPane"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("WarningsAndErrors")]
        public MessageBoxOnCompile MessageBoxOnCompile
        {
            get
            {
                return (MessageBoxOnCompile)(this["MessageBoxOnCompile"]);
            }
            set
            {
                this["MessageBoxOnCompile"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("Pixel0")]
        public SpriteImportMethod SpriteImportMethod
        {
            get
            {
                return (SpriteImportMethod)(this["SpriteImportMethod"]);
            }
            set
            {
                this["SpriteImportMethod"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("2")]
        public int TabSize
        {
            get
            {
                return (int)(this["TabSize"]);
            }
            set
            {
                this["TabSize"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("False")]
        public bool IndentUsingTabs
        {
            get
            {
                return (bool)(this["IndentUsingTabs"]);
            }
            set
            {
                this["IndentUsingTabs"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("False")]
        public bool ShowViewPreviewByDefault
        {
            get
            {
                return (bool)(this["ShowViewPreviewByDefault"]);
            }
            set
            {
                this["ShowViewPreviewByDefault"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("")]
        public string PaintProgramPath
        {
            get
            {
                return (string)(this["PaintProgramPath"]);
            }
            set
            {
                this["PaintProgramPath"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("")]
        public string NewGamePath
        {
            get
            {
                return (string)(this["NewGamePath"]);
            }
            set
            {
                this["NewGamePath"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("True")]
        public bool SendAnonymousStats
        {
            get
            {
                return (bool)(this["SendAnonymousStats"]);
            }
            set
            {
                this["SendAnonymousStats"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("1601-01-01")]
        public System.DateTime StatsLastSent
        {
            get
            {
                return (System.DateTime)(this["StatsLastSent"]);
            }
            set
            {
                this["StatsLastSent"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("1601-01-01")]
        public System.DateTime LastBackupWarning
        {
            get
            {
                return (System.DateTime)(this["LastBackupWarning"]);
            }
            set
            {
                this["LastBackupWarning"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("7")]
        public int BackupWarningInterval
        {
            get
            {
                return (int)(this["BackupWarningInterval"]);
            }
            set
            {
                this["BackupWarningInterval"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("True")]
        public bool RemapPalettizedBackgrounds
        {
            get
            {
                return (bool)(this["RemapPalettizedBackgrounds"]);
            }
            set
            {
                this["RemapPalettizedBackgrounds"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        public System.Collections.Specialized.StringCollection RecentSearches
        {
            get
            {
                return (System.Collections.Specialized.StringCollection)(this["RecentSearches"]);
            }
            set
            {
                this["RecentSearches"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("True")]
        public bool KeepHelpOnTop
        {
            get
            {
                return (bool)(this["KeepHelpOnTop"]);
            }
            set
            {
                this["KeepHelpOnTop"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("True")]
        public bool DialogOnMultibleTabsClose
        {
            get
            {
                return (bool)(this["DialogOnMultibleTabsClose"]);
            }
            set
            {
                this["DialogOnMultibleTabsClose"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("")]
        public string ColorTheme
        {
            get
            {
                return (string)(this["ColorTheme"]);
            }
            set
            {
                this["ColorTheme"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("")]
        public string DefaultImportPath
        {
            get
            {
                return (string)(this["DefaultImportPath"]);
            }
            set
            {
                this["DefaultImportPath"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("")]
        public string ExplicitNewGamePath
        {
            get
            {
                return (string)(this["ExplicitNewGamePath"]);
            }
            set
            {
                this["ExplicitNewGamePath"] = value;
            }
        }
    }
}
