using System;
using System.Collections.Generic;
using System.Collections.Specialized;
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
        WarningsAndErrors = 0,
        OnlyErrors = 1,
        Never = 2
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

    [SettingsSerializeAs(SettingsSerializeAs.String)]
    public class RecentGame
    {
        // default constructor is needed to serialize
        public RecentGame() {}

        public RecentGame(string name, string path)
        {
            Name = name;
            Path = path;
        }

        public string Name { get; set; }
        public string Path { get; set; }
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
                        switch (regname.Substring(6, 4)) {
                            case "Path":
                                gamePaths.Add(value);
                                break;
                            case "Name":
                                gameNames.Add(value);
                                break;
                            default:
                                RecentSearches.Add(value);
                                break;
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

                // now check recent game values
                int gameCount = Math.Min(gameNames.Count, gamePaths.Count);

                for (int i = 0; i < gameCount; i ++)
                {
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
        [DefaultSettingValueAttribute("")]
        public List<RecentGame> RecentGames
        {
            get
            {
                return (List<RecentGame>)(this["RecentGames"]);
            }
            set
            {
                this["RecentGames"] = value;
            }
        }

        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("")]
        public StringCollection RecentSearches
        {
            get
            {
                return (StringCollection)(this["RecentSearches"]);
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
