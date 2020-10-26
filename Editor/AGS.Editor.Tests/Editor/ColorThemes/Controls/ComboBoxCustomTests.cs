using System.Windows.Forms;
using NSubstitute;
using NUnit.Framework;

namespace AGS.Editor
{
    [TestFixture]
    public class ComboBoxCustomTests
    {
        private IColorThemes _themes;
        private ComboBox _original;
        private ComboBoxCustom _customBox;

        [SetUp]
        public void SetUp()
        {
            _themes = Substitute.For<IColorThemes>();
            _original = new ComboBox();
            _original.Items.AddRange(new[] { "Test Item" });
            _customBox = new ComboBoxCustom(_themes, "test-root/", _original);
        }

        [Test]
        public void CopiesItemsFromOriginal()
        {
            Assert.That(_customBox.Items, Is.EqualTo(_original.Items));
        }
    }
}
