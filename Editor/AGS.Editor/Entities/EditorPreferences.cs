using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Configuration;
using System.IO;
using System.Windows.Forms.Design;
using System.Drawing.Design;
using Microsoft.Win32;
using AGS.Types;
using System.Reflection;
using System.Linq;

namespace AGS.Editor.Preferences
{
    [Flags]
    public enum StartupPane
    {
        [Description("Show Start Page")]
        StartPage = 0,
        [Description("Show Game Settings")]
        GeneralSettings = 1,
        [Description("No panes should open")]
        None = 2
    }

    [Flags]
    public enum MessageBoxOnCompile
    {
        [Description("Always")]
        Always = 0,
        [Description("When there are warnings or errors")]
        WarningsAndErrors = 1,
        [Description("When there are errors")]
        OnlyErrors = 2,
        [Description("Never")]
        Never = 3
    }

    [Flags]
    public enum ReloadScriptOnExternalChange
    {
        [Description("Ask what to do")]
        Prompt = 0,
        [Description("Always reload the file")]
        Always = 1,
        [Description("Never reload the file")]
        Never = 2
    }

    [Flags]
    public enum SpriteImportMethod
    {
        [Description("Palette index 0")]
        Pixel0 = 0,
        [Description("Top -left Pixel")]
        TopLeft = 1,
        [Description("Bottom-left Pixel")]
        BottomLeft = 2,
        [Description("Top-right Pixel")]
        TopRight = 3,
        [Description("Bottom-right Pixel")]
        BottomRight = 4,
        [Description("Leave as-is")]
        LeaveAsIs = 5,
        [Description("No transparency")]
        NoTransparency = 6
    }

    [Flags]
    public enum TestGameWindowStyle
    {
        [Description("Use game setup configuration")]
        UseGameSetup = 0,
        [Description("Always run full-screen")]
        FullScreen = 1,
        [Description("Always run in a window")]
        Windowed = 2
    }

    public class ColorThemeTypeConverter : BaseListSelectTypeConverter<string, string>
    {
        protected override Dictionary<string, string> GetValueList(ITypeDescriptorContext context)
        {
            return Factory.GUIController.ColorThemes.Themes.ToDictionary(t => t.Name, t => t.Name);
        }
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

        SettingsLoadedEventHandler eventHandlerLoaded = null;
        ListChangedEventHandler eventHandlerRecentSearches = null;
        ListChangedEventHandler eventHandlerRecentGames = null;

        public AppSettings()
        {
            eventHandlerLoaded = new SettingsLoadedEventHandler(Settings_SettingsLoaded);
            eventHandlerRecentSearches = new ListChangedEventHandler(Settings_LimitRecentSearches);
            eventHandlerRecentGames = new ListChangedEventHandler(Settings_LimitRecentGames);

            SettingsLoaded += eventHandlerLoaded;
            RecentSearches.ListChanged += eventHandlerRecentSearches;
            RecentGames.ListChanged += eventHandlerRecentGames;
        }

        ~AppSettings()
        {
            SettingsLoaded -= eventHandlerLoaded;
            RecentSearches.ListChanged -= eventHandlerRecentSearches;
            RecentGames.ListChanged -= eventHandlerRecentGames;
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

        private IEnumerable<PropertyInfo> GetBrowsableProperties()
        {
            return GetType().GetProperties()
               .Where(f => f.GetCustomAttributes<BrowsableAttribute>().Contains(BrowsableAttribute.Yes));
        }

        public AppSettings CloneAppSettings()
        {
            AppSettings clone = new AppSettings();

            foreach (PropertyInfo prop in GetBrowsableProperties())
            {
                clone[prop.Name] = this[prop.Name];
            }

            return clone;
        }

        public void Apply(AppSettings settings)
        {
            foreach (PropertyInfo prop in GetBrowsableProperties())
            {
                this[prop.Name] = settings[prop.Name];
            }
        }

        [Browsable(false)]
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

        [Browsable(false)]
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

        [Browsable(true)]
        [DisplayName("Test Game Style")]
        [Description("Game should run in window or full-screen when you test it. When using F5 game will always run in a window.")]
        [Category("Test Game")]
        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("UseGameSetup")]
        [TypeConverter(typeof(EnumTypeConverter))]
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

        [Browsable(true)]
        [DisplayName("Editor Startup Action")]
        [Description("What editor should do at startup.")]
        [Category("Editor Appearance")]
        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("StartPage")]
        [TypeConverter(typeof(EnumTypeConverter))]
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

        [Browsable(true)]
        [DisplayName("Pop-up messages on Compile")]
        [Description("In which cases the editor should show pop-up windows when compiling.")]
        [Category("Editor Appearance")]
        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("WarningsAndErrors")]
        [TypeConverter(typeof(EnumTypeConverter))]
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

        [Browsable(true)]
        [DisplayName("Script file modified externally, should it reload?")]
        [Description("If a script is open for editing and is modified by another program, should it reload?")]
        [Category("Script Editor")]
        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("Prompt")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public ReloadScriptOnExternalChange ReloadScriptOnExternalChange
        {
            get
            {
                return (ReloadScriptOnExternalChange)(this["ReloadScriptOnExternalChange"]);
            }
            set
            {
                this["ReloadScriptOnExternalChange"] = value;
            }
        }

        [Browsable(true)]
        [DisplayName("Default sprite import transparency")]
        [Description("Sprite transparency import method to use by default.")]
        [Category("Sprite Editor")]
        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("LeaveAsIs")]
        [TypeConverter(typeof(EnumTypeConverter))]
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

        [Browsable(true)]
        [DisplayName("Tab width")]
        [Description("How many space characters a tab width should be. This setting requires editor restart to be applied.")]
        [Category("Script Editor")]
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

        [Browsable(true)]
        [DisplayName("Indent Uses Tabs")]
        [Description("Should editor use tabs instead of spaces when indenting?")]
        [Category("Script Editor")]
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

        [Browsable(true)]
        [DisplayName("Show view preview by default in view editors")]
        [Description("Wheter view preview is always showing when a view editor is loaded.")]
        [Category("Editor Appearance")]
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

        [Browsable(true)]
        [DisplayName("Default image editor")]
        [Description("When you double-click a sprite, what program do you want to use to edit it? This program must support PNG and BMP files.")]
        [Category("Sprite Editor")]
        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("")]
        [EditorAttribute(typeof(FileNameEditor), typeof(UITypeEditor))]
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

        [Browsable(true)]
        [DisplayName("New Game Directory")]
        [Description("When you create a new game, where do you want it to go?")]
        [Category("New Game Directory")]
        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("")]
        [EditorAttribute(typeof(FolderNameEditor), typeof(UITypeEditor))]
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

        [Browsable(false)] // this is disabled until we can fix the server
        [DisplayName("Send Anonymous Stats")]
        [Description("When you create a new game, where do you want it to go?")]
        [Category("New Game Directory")]
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

        [Browsable(false)]
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

        [Browsable(false)]
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

        [Browsable(true)]
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

        [Browsable(true)]
        [DisplayName("Remap palette of room backgrounds")]
        [Description("Remap paletter of room background into allocated background palette slots (8-bit games only).")]
        [Category("Import of 8-bit background")]
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

        [Browsable(false)]
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

        [Browsable(false)]
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

        [Browsable(true)]
        [DisplayName("Keep Help window on top")]
        [Description("Should Help window always be on top of the Editor window when shown?")]
        [Category("Editor Appearance")]
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

        [Browsable(true)]
        [DisplayName("Ask before closing multiple tabs")]
        [Description("Prompt dialog on closing multiple tabs.")]
        [Category("Editor Appearance")]
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

        [Browsable(true)]
        [DisplayName("Color Theme")]
        [Description("Select which theme the editor should be using.")]
        [Category("Editor Appearance")]
        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("")]
        [TypeConverter(typeof(ColorThemeTypeConverter))]
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

        [Browsable(true)]
        [DisplayName("Import Directory")]
        [Description("When you import files, where do you want to look first?")]
        [Category("Import Directory")]
        [UserScopedSettingAttribute()]
        [DefaultSettingValueAttribute("")]
        [EditorAttribute(typeof(FolderNameEditor), typeof(UITypeEditor))]
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

        [Browsable(false)]
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

        [Browsable(false)]
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

        [Browsable(false)]
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

        [Browsable(false)]
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

        [Browsable(false)]
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
