using System;
using System.Collections.Generic;

namespace AGS.Editor
{
    public interface IColorThemes
    {
        ColorTheme Current { get; set; }

        bool IsCurrentDefault { get; }

        IEnumerable<ColorTheme> Themes { get; }

        void Apply(Action<ColorTheme> apply);

        void Import(string dir);

        void OpenFolder();

        void Init();

        void Load();
    }
}