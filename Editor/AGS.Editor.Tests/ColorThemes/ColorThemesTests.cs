using System;
using AGS.Editor.Preferences;
using NSubstitute;
using NUnit.Framework;

namespace AGS.Editor
{
    [TestFixture]
    public class ColorThemesTests
    {
        private IAppSettings _settings;
        private IColorThemes _themes;

        [SetUp]
        public void SetUp()
        {
            _settings = Substitute.For<IAppSettings>();
            _themes = new ColorThemes(_settings);
        }

        [Test]
        public void SettingCurrentToNullWillThrowNullReferenceException()
        {
            Assert.Throws<NullReferenceException>(() => _themes.Current = null);
        }

        [Test]
        public void SettingCurrentWillUpdateSettings()
        {
            _themes.Current = ColorThemeStub.DEFAULT;

            Assert.That(_settings.ColorTheme, Is.EqualTo(ColorThemeStub.DEFAULT.Name));
            _settings.Received().Save();
        }
    }
}
