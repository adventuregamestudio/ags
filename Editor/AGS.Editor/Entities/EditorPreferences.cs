using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Configuration;
using System.IO;
using Microsoft.Win32;

namespace AGS.Editor.Preferences
{
    [Flags]
    public enum StartupPane
    {
        StartPage = 0,
        GeneralSettings = 1,
        None = 2
    }

    [Flags]
    public enum MessageBoxOnCompile
    {
        Always = 0,
        WarningsAndErrors = 1,
        OnlyErrors = 2,
        Never = 3
    }

    [Flags]
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

    [Flags]
    public enum TestGameWindowStyle
    {
        UseGameSetup = 0,
        FullScreen = 1,
        Windowed = 2
    }

    public class RecentGame : IEquatable<RecentGame>
    {
        // default constructor is needed to serialize
        public RecentGame() {}

        public RecentGame(string name, string path)
        {
            Name = name;
            Path = path;
        }

        public bool Equals(RecentGame otherGame)
        {
            // for lack of an identifier, if the path on disk matches
            // this is considered to be the same game
            return Path == otherGame.Path;
        }

        public string Name { get; set; }
        public string Path { get; set; }
    }

    public sealed class AppSettings : ApplicationSettingsBase
    {
        const int MAX_RECENT_GAMES = 10;
        const int MAX_RECENT_SEARCHES = 10;

        public AppSettings()
        {
            SettingsLoaded += new SettingsLoadedEventHandler(Settings_SettingsLoaded);
            RecentSearches.ListChanged += new ListChangedEventHandler(Settings_LimitRecentSearches);
            RecentGames.ListChanged += new ListChangedEventHandler(Settings_LimitRecentGames);
        }

        private void Settings_LimitRecentSearches(object sender, ListChangedEventArgs e)
        {
            ApplyLimit(RecentSearches, MAX_RECENT_SEARCHES);
        }

        private void Settings_LimitRecentGames(object sender, ListChangedEventArgs e)
        {
            ApplyLimit(RecentGames, MAX_RECENT_GAMES);
        }

        private void ApplyLimit<T>(BindingList<T> list, int max)
        {
            if (list.Count > max)
            {
                for (int i = max; i < list.Count; i++)
                {
                    list.RemoveAt(i);
                }
            }
        }

        private void Settings_SettingsLoaded(object sender, SettingsLoadedEventArgs e)
        {
            // - called when the first setting is requested and caches all values
            // - base class will handle bad input so just logic fixes needed

            if (!UpgradedSettings)
            {
                Upgrade();
                UpgradedSettings = true;
            }

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
            Dictionary<string, string> regmap = new Dictionary<string, string>()
            {
            //  [<registry name>] = <setting name>
                ["ScEdTabWidth"] = "TabSize",
                ["TestGameStyle"] = "TestGameWindowStyle",
                ["MessageBoxOnCompileErrors"] = "MessageBoxOnCompile",
                ["IndentUsingTabs"] = "IndentUseTabs",
                ["SpriteImportTransparency"] = "SpriteImportMethod",
                ["RemapPaletteBackgrounds"] = "RemapPalettizedBackgrounds"
            };

            RegistryKey key = Registry.CurrentUser.OpenSubKey(AGSEditor.AGS_REGISTRY_KEY);
            List<string> gameNames = new List<string>();
            List<string> gamePaths = new List<string>();
            bool success = true;

            if (key != null)
            {
                foreach (string regname in key.GetValueNames())
                {
                    string value;

                    try
                    {
                        value = key.GetValue(regname).ToString();
                    }
                    catch
                    {
                        // failed to read as a string
                        success = false;
                        continue;
                    }

                    if (regname.StartsWith("Recent"))
                    {
                        if (regname.StartsWith("RecentPath"))
                        {
                            gamePaths.Add(value);
                        }
                        else if (regname.StartsWith("RecentName"))
                        {
                            gameNames.Add(value);
                        }
                        else if (regname.StartsWith("RecentSearch") && RecentSearches.Count < MAX_RECENT_SEARCHES)
                        {
                            RecentSearches.Insert(0, value);
                        }
                    }
                    else
                    {
                        string name = regmap.ContainsKey(regname) ? regmap[regname] : regname;
                        int numeric;

                        try
                        {
                            // will throw SettingsPropertyNotFoundException
                            // for legacy settings which no longer exist
                            Type type = this[name].GetType();

                            // will throw System.InvalidCastException if can't be converted
                            if (type.BaseType == typeof(Enum))
                            {
                                this[name] = Enum.Parse(type, value);
                            }
                            else if (int.TryParse(value, out numeric))
                            {
                                this[name] = Convert.ChangeType(numeric, type);
                            }
                            else
                            {
                                this[name] = Convert.ChangeType(value, type);
                            }
                        }
                        catch (Exception ex)
                        {
                            if (!(ex is SettingsPropertyNotFoundException))
                            {
                                success = false;
                            }
                            // continue
                        }
                    }
                }

                key.Close();
                int gameCount = Math.Min(gameNames.Count, gamePaths.Count);

                for (int i = 0; i < gameCount; i ++)
                {
                    if (RecentGames.Count >= MAX_RECENT_GAMES)
                    {
                        break;
                    }

                    RecentGames.Add(new RecentGame(gameNames[i], gamePaths[i]));
                }
            }

            return success;
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
        [DefaultSettingValueAttribute("False")]
        public bool UpgradedSettings
        {
            get
            {
                return (bool)(this["UpgradedSettings"]);
            }
            set
            {
                this["UpgradedSettings"] = value;
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
        public StartupPane StartupPane
        {
            get
            {
                return (StartupPane)(this["StartupPane"]);
            }
            set
            {
                this["StartupPane"] = value;
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
        public bool IndentUseTabs
        {
            get
            {
                return (bool)(this["IndentUseTabs"]);
            }
            set
            {
                this["IndentUseTabs"] = value;
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
        [DefaultSettingValueAttribute("")]
        public BindingList<RecentGame> RecentGames
        {
            get
            {
                return (BindingList<RecentGame>)(this["RecentGames"]);
            }
            private set
            {
                this["RecentGames"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("")]
        public BindingList<string> RecentSearches
        {
            get
            {
                return (BindingList<string>)(this["RecentSearches"]);
            }
            private set
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
        public bool DialogOnMultipleTabsClose
        {
            get
            {
                return (bool)(this["DialogOnMultipleTabsClose"]);
            }
            set
            {
                this["DialogOnMultipleTabsClose"] = value;
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
        [DefaultSettingValueAttribute("640")]
        public int MainWinWidth
        {
            get
            {
                return (int)(this["MainWinWidth"]);
            }
            set
            {
                this["MainWinWidth"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("480")]
        public int MainWinHeight
        {
            get
            {
                return (int)(this["MainWinHeight"]);
            }
            set
            {
                this["MainWinHeight"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("0")]
        public int MainWinX
        {
            get
            {
                return (int)(this["MainWinX"]);
            }
            set
            {
                this["MainWinX"] = value;
            }
        }


        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("0")]
        public int MainWinY
        {
            get
            {
                return (int)(this["MainWinY"]);
            }
            set
            {
                this["MainWinY"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("True")]
        public bool MainWinMaximize
        {
            get
            {
                return (bool)(this["MainWinMaximize"]);
            }
            set
            {
                this["MainWinMaximize"] = value;
            }
        }
    }
}
