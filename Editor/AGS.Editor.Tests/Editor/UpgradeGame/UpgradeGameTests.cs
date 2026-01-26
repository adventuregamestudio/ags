using System;
using NUnit.Framework;

namespace AGS.Editor
{
    public class UpgradeGameTests
    {
        [TestCase("TextText", ExpectedResult = "TextText")]
        [TestCase("Text[Text", ExpectedResult = "Text\\nText")]
        [TestCase("Text[[Text", ExpectedResult = "Text\\n\\nText")]
        [TestCase("Text\\[Text", ExpectedResult = "Text[Text")]
        [TestCase("Text\\\\[Text", ExpectedResult = "Text\\\\\\nText")]
        [TestCase("Text\\\\\\[Text", ExpectedResult = "Text\\\\[Text")]
        [TestCase("Text\\\\\\[[Text", ExpectedResult = "Text\\\\[\\nText")]
        public string ConvertOldStyleLinebreak_Test(string text)
        {
            return UpgradeGameUtils.ConvertOldStyleLinebreak(text);
        }
    }
}
