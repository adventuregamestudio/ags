using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using AGS.Editor.Preferences;

namespace AGS.Editor
{
    public class ColorThemes : IColorThemes
    {
        private readonly IAGSEditorDirectories _agsEditorDirs;
        private readonly IAppSettings _settings;
        private readonly List<ColorTheme> _themes = new List<ColorTheme>();
        private ColorTheme _current;

        public ColorThemes(IAGSEditorDirectories agsEditorDirs, IAppSettings settings)
        {
            _agsEditorDirs = agsEditorDirs;
            _settings = settings;
            Load();
            Init();
        }

        public ColorTheme Current
        {
            get
            {
                return _current;
            }

            set
            {
                if (value == null)
                {
                    throw new NullReferenceException($"{Current} cannot be null.");
                }

                _current = value;
                _settings.ColorTheme = Current.Name;
                _settings.Save();
            }
        }

        public IEnumerable<ColorTheme> Themes => _themes;

        public bool IsCurrentDefault => Current == ColorThemeStub.DEFAULT;

        private string DiskDir => Path.Combine(_agsEditorDirs.LocalAppData, "Themes");

        public void Load()
        {
            if (!Directory.Exists(DiskDir))
            {
                Directory.CreateDirectory(DiskDir);
            }

            _themes.Clear();
            _themes.Add(ColorThemeStub.DEFAULT);
            Directory.GetFiles(DiskDir, "*.json").ToList().ForEach(f => _themes.Add(new ColorThemeJson(Path.GetFileNameWithoutExtension(f), f)));
            Current = _themes.FirstOrDefault(t => t.Name == _settings.ColorTheme) ?? ColorThemeStub.DEFAULT;
        }

        public void Init()
        {
            try
            {
                Current.Init();
            }
            catch (Exception e)
            {
                Factory.GUIController.ShowMessage(
                    $"Something went wrong when loading color theme {Current.Name}. The editor " +
                    $"will set the color them back to the default them and continue as normal. " +
                    $"See stack trace for more details.\n\n{e}",
                    MessageBoxIcon.Warning);
                Current = ColorThemeStub.DEFAULT;
            }
        }

        public void Apply(Action<ColorTheme> apply)
        {
            if (!IsCurrentDefault)
            {
                try
                {
                    apply.Invoke(Current);
                }
                catch (Exception e)
                {
                    Factory.GUIController.ShowMessage(
                        $"Something went wrong when trying to apply color theme {Current}. " +
                        $"The editor will set the color theme back to the default theme and continue " +
                        $"as normal, however it may not look right until the next time you restart " +
                        $"the editor. See stack trace for more details.\n\n{e}",
                        MessageBoxIcon.Warning);
                    Current = ColorThemeStub.DEFAULT;
                }
            }
        }

        public void Import(string dir)
        {
            try
            {
                string newDir = Path.Combine(DiskDir, Path.GetFileName(dir));
                _themes.Add(new ColorThemeJson(Path.GetFileNameWithoutExtension(dir), dir));
                File.Copy(dir, newDir);
            }
            catch (Exception e)
            {
                Factory.GUIController.ShowMessage(
                    $"Something went wrong importing the color theme, see stack trace for more details.\n\n{e}",
                    MessageBoxIcon.Error);
            }
        }
    }
}
