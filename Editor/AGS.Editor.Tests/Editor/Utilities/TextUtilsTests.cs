using System.Text.RegularExpressions;
using NUnit.Framework;

namespace AGS.Editor
{
    public class TextUtilsTests
    {
        [TestCase("TextText", ExpectedResult = "TextText")]
        [TestCase("Text[Text", ExpectedResult = "Text[Text")]
        [TestCase("Text\\[Text", ExpectedResult = "Text\\\\[Text")]
        [TestCase("Text\\\\[Text", ExpectedResult = "Text\\\\[Text")]
        [TestCase("Text\\\\\\[Text", ExpectedResult = "Text\\\\\\\\[Text")]
        [TestCase("Text\\\\\\[[Text", ExpectedResult = "Text\\\\\\\\[[Text")]
        [TestCase("Text\\nText", ExpectedResult = "Text\\nText")]
        public string PreprocessLineForOldStyleLinebreaks_Test(string text)
        {
            return TextUtils.PreprocessLineForOldStyleLinebreaks(text);
        }

        [TestCase("TextText", ExpectedResult = "TextText")]
        [TestCase("Text[Text", ExpectedResult = "Text[Text")]
        [TestCase("Text\\[Text", ExpectedResult = "Text\\[Text")]
        // NOTE: there's no difference in the end result compared to the previous test
        // where the '[' is escaped, therefore in practice the engine will still think
        // that the '[' is escaped at runtime.
        // This cannot be resolved without using '\n' instead.
        [TestCase("Text\\\\[Text", ExpectedResult = "Text\\[Text")]
        [TestCase("Text\\\\\\[Text", ExpectedResult = "Text\\\\[Text")]
        [TestCase("Text\\\\\\[[Text", ExpectedResult = "Text\\\\[[Text")]
        [TestCase("Text\\nText", ExpectedResult = "Text\nText")]
        public string UnescapeLineWithOldStyleLinebreaks_Test(string text)
        {
            return Regex.Unescape(TextUtils.PreprocessLineForOldStyleLinebreaks(text));
        }
    }
}
