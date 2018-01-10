using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace AGS.Editor
{
    public class ColorThemes
    {
        private readonly List<ColorTheme> _themes = new List<ColorTheme>();

        public ColorThemes()
        {
            if (!Directory.Exists(DiskDir))
            {
                Directory.CreateDirectory(DiskDir);
            }

            _themes.Add(ColorThemeStub.DEFAULT);
            Directory.GetFiles(DiskDir, "*.json").ToList().ForEach(f => _themes.Add(new ColorThemeJson(Path.GetFileNameWithoutExtension(f), f)));
            Current = _themes.FirstOrDefault(t => t.Name == Factory.AGSEditor.Preferences.ColorTheme);
        }

        public ColorTheme Current { get; set; }

        public IEnumerable<ColorTheme> Themes => _themes;

        public bool IsCurrentDefault => Current == ColorThemeStub.DEFAULT;

        private static string DiskDir => string.Format("{0}{1}AGS{1}Themes{1}", Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), Path.DirectorySeparatorChar);

        public void Load(Action<ColorTheme> load)
        {
            if (!IsCurrentDefault)
            {
                load.Invoke(Current);
            }
        }

        public void Import(string dir)
        {
            string newDir = DiskDir + Path.GetFileName(dir);
            _themes.Add(new ColorThemeJson(Path.GetFileNameWithoutExtension(dir), dir));
            File.Copy(dir, newDir);
        }
    }
}
