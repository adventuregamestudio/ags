using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;
using Microsoft.Win32;

namespace AGS.Editor
{
    public class EditorPreferences
    {
        private TestGameWindowStyle _testGameScreen = TestGameWindowStyle.Windowed;
        private int _tabSize = 2;
		private EditorStartupPane _editorStartupPane = EditorStartupPane.StartPage;
		private string _defaultImportPath = string.Empty;
		private bool _projectTreeOnRight = true;
		private MessageBoxOnCompile _messageBoxOnCompileErrors = MessageBoxOnCompile.WarningsAndErrors;
		private bool _indentUseTabs = false;
		private bool _showViewPreviewByDefault = false;
		private string _paintProgramPath = string.Empty;
		private SpriteImportMethod _defaultSpriteImportTransparency = SpriteImportMethod.TopLeft;
		private string _defaultNewGamePath = string.Empty;
        private bool _sendAnonymousStats = true;
        private DateTime _statsLastSent = DateTime.MinValue;
        private int _backupWarningInterval = 7;
        private DateTime _lastBackupWarning = DateTime.MinValue;
        private bool _remapPalettizedBackgrounds = true;
        private List<string> _previousSearches = new List<string>();
        private bool _keepHelpOnTop = true;
        private bool _dialogOnMultibleTabsClose = true;

        private string _registryKey;

        public EditorPreferences(string registryKey)
        {
            _registryKey = registryKey;

            RegistryKey key = Registry.CurrentUser.OpenSubKey(registryKey);
            if (key != null)
            {
                _tabSize = Convert.ToInt32(key.GetValue("ScEdTabWidth", _tabSize));
                _testGameScreen = (TestGameWindowStyle)Convert.ToInt32(key.GetValue("TestGameStyle", _testGameScreen));
				_editorStartupPane = (EditorStartupPane)Convert.ToInt32(key.GetValue("StartupPane", _editorStartupPane));
				_defaultImportPath = Convert.ToString(key.GetValue("DefaultImportPath", string.Empty));
				_projectTreeOnRight = Convert.ToInt32(key.GetValue("ProjectTreeOnRight", _projectTreeOnRight)) != 0;
				_messageBoxOnCompileErrors = (MessageBoxOnCompile)Convert.ToInt32(key.GetValue("MessageBoxOnCompileErrors", _messageBoxOnCompileErrors));
				_indentUseTabs = Convert.ToInt32(key.GetValue("IndentUsingTabs", _indentUseTabs)) != 0;
				_showViewPreviewByDefault = Convert.ToInt32(key.GetValue("ShowViewPreviewByDefault", _showViewPreviewByDefault)) != 0;
				_paintProgramPath = Convert.ToString(key.GetValue("PaintProgramPath", string.Empty));
				_defaultSpriteImportTransparency = (SpriteImportMethod)Convert.ToInt32(key.GetValue("SpriteImportTransparency", _defaultSpriteImportTransparency));
				_defaultNewGamePath = Convert.ToString(key.GetValue("NewGamePath", _defaultNewGamePath));
                _sendAnonymousStats = Convert.ToInt32(key.GetValue("SendAnonymousStats", _sendAnonymousStats)) != 0;
                _statsLastSent = ReadDateFromRegistry(key, "StatsLastSent", _statsLastSent);
                _backupWarningInterval = Convert.ToInt32(key.GetValue("BackupWarningInterval", _backupWarningInterval));
                _lastBackupWarning = ReadDateFromRegistry(key, "LastBackupWarning", _lastBackupWarning);
                _remapPalettizedBackgrounds = Convert.ToInt32(key.GetValue("RemapPaletteBackgrounds", _remapPalettizedBackgrounds)) != 0;
                _keepHelpOnTop = Convert.ToInt32(key.GetValue("KeepHelpOnTop", _keepHelpOnTop)) != 0;
                _dialogOnMultibleTabsClose = Convert.ToInt32(key.GetValue("DialogOnMultipleTabsClose", _dialogOnMultibleTabsClose)) != 0;
                ReadRecentSearchesList(key);
                key.Close();

				if ((_defaultImportPath.Length > 0) &&
					(!System.IO.Directory.Exists(_defaultImportPath)))
				{
					_defaultImportPath = string.Empty;
				}

				if ((_paintProgramPath.Length > 0) &&
					(!System.IO.File.Exists(_paintProgramPath)))
				{
					_paintProgramPath = string.Empty;
				}
            }
        }

        private void ReadRecentSearchesList(RegistryKey key)
        {
            for (int i = 0; i < 20; i++)
            {
                string recentSearch = key.GetValue("RecentSearch" + i, string.Empty).ToString();
                if (recentSearch == string.Empty)
                {
                    break;
                }
                _previousSearches.Add(recentSearch);
            }
        }

        private void WriteRecentSearchesList(RegistryKey key)
        {
            for (int i = 0; i < _previousSearches.Count; i++)
            {
                key.SetValue("RecentSearch" + i, _previousSearches[i]);
            }
        }

        private DateTime ReadDateFromRegistry(RegistryKey key, string regValue, DateTime defaultValue)
        {
            string dateString = key.GetValue(regValue, string.Empty).ToString();
            if (dateString.Length > 0)
            {
                return DateTime.Parse(dateString, CultureInfo.InvariantCulture, DateTimeStyles.AssumeUniversal | DateTimeStyles.AdjustToUniversal);
            }
            return defaultValue;
        }

        public void SaveToRegistry()
        {
            RegistryKey key = Registry.CurrentUser.CreateSubKey(_registryKey);
            if (key == null)
            {
                Factory.GUIController.ShowMessage("Unable to access registry key: " + _registryKey, System.Windows.Forms.MessageBoxIcon.Warning);
            }
            else
            {
                key.SetValue("ScEdTabWidth", _tabSize.ToString());
                key.SetValue("TestGameStyle", ((int)_testGameScreen).ToString());
				key.SetValue("StartupPane", ((int)_editorStartupPane).ToString());
				key.SetValue("DefaultImportPath", _defaultImportPath);
				key.SetValue("ProjectTreeOnRight", _projectTreeOnRight ? "1" : "0");
				key.SetValue("MessageBoxOnCompileErrors", ((int)_messageBoxOnCompileErrors).ToString());
				key.SetValue("IndentUsingTabs", _indentUseTabs ? "1" : "0");
				key.SetValue("ShowViewPreviewByDefault", _showViewPreviewByDefault ? "1" : "0");
				key.SetValue("PaintProgramPath", _paintProgramPath);
				key.SetValue("SpriteImportTransparency", ((int)_defaultSpriteImportTransparency).ToString());
				key.SetValue("NewGamePath", _defaultNewGamePath);
                key.SetValue("SendAnonymousStats", _sendAnonymousStats ? "1" : "0");
                key.SetValue("StatsLastSent", _statsLastSent.ToString("u"));
                key.SetValue("BackupWarningInterval", _backupWarningInterval.ToString());
                key.SetValue("LastBackupWarning", _lastBackupWarning.ToString("u"));
                key.SetValue("RemapPaletteBackgrounds", _remapPalettizedBackgrounds ? "1" : "0");
                key.SetValue("KeepHelpOnTop", _keepHelpOnTop ? "1" : "0");
                key.SetValue("DialogOnMultipleTabsClose", _dialogOnMultibleTabsClose ? "1" : "0");
                WriteRecentSearchesList(key);
                key.Close();
            }
        }

        public int TabSize
        {
            get { return _tabSize; }
            set { _tabSize = value; }
        }

        public TestGameWindowStyle TestGameStyle
        {
            get { return _testGameScreen; }
            set { _testGameScreen = value; }
        }

		public EditorStartupPane StartupPane
		{
			get { return _editorStartupPane; }
			set { _editorStartupPane = value; }
		}

		public string DefaultImportPath
		{
			get { return _defaultImportPath; }
			set { _defaultImportPath = value; }
		}

        [Obsolete]
		public bool ProjectTreeOnRight
		{
			get { return _projectTreeOnRight; }
			set { _projectTreeOnRight = value; }
		}

		public MessageBoxOnCompile MessageBoxOnCompileErrors
		{
			get { return _messageBoxOnCompileErrors; }
			set { _messageBoxOnCompileErrors = value; }
		}

		public bool IndentUsingTabs
		{
			get { return _indentUseTabs; }
			set { _indentUseTabs = value; }
		}

		public bool ShowViewPreviewByDefault
		{
			get { return _showViewPreviewByDefault; }
			set { _showViewPreviewByDefault = value; }
		}

		public string PaintProgramPath
		{
			get { return _paintProgramPath; }
			set { _paintProgramPath = value; }
		}

		public SpriteImportMethod DefaultSpriteImportTransparency
		{
			get { return _defaultSpriteImportTransparency; }
			set { _defaultSpriteImportTransparency = value; }
		}

		public string ExplicitNewGamePath
		{
			get { return _defaultNewGamePath; }
			set { _defaultNewGamePath = value; }
		}

		public string NewGamePath
		{
			get
			{
				if (!string.IsNullOrEmpty(_defaultNewGamePath))
				{
					return _defaultNewGamePath;
				}
				else
				{
					return Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
				}
			}
		}

        public bool SendAnonymousStats
        {
            get { return _sendAnonymousStats; }
            set { _sendAnonymousStats = value; }
        }

        public DateTime StatsLastSent
        {
            get { return _statsLastSent; }
            set { _statsLastSent = value; }
        }

        public int BackupWarningInterval
        {
            get { return _backupWarningInterval; }
            set { _backupWarningInterval = value; }
        }

        public DateTime LastBackupWarning
        {
            get { return _lastBackupWarning; }
            set { _lastBackupWarning = value; }
        }

        public bool RemapPalettizedBackgrounds
        {
            get { return _remapPalettizedBackgrounds; }
            set { _remapPalettizedBackgrounds = value; }
        }

        public List<string> RecentSearches
        {
            get { return _previousSearches; }
        }

        public bool KeepHelpOnTop
        {
            get { return _keepHelpOnTop; }
            set { _keepHelpOnTop = value; }            
        }

        public bool DialogOnMultibleTabsClose
        {
            get { return _dialogOnMultibleTabsClose; }
            set { _dialogOnMultibleTabsClose = value; }
        }
    }
}
