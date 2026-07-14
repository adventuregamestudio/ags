using AGS.Editor.Components;
using AGS.Types;
using NUnit.Framework;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using static System.Net.Mime.MediaTypeNames;
using static System.Windows.Forms.VisualStyles.VisualStyleElement.TaskbarClock;

namespace AGS.Editor
{
    public class TranslationTests
    {
        private const string GeneratedTranslationHeader = @"# AGS TRANSLATION SOURCE FILE
# This is a PO file generated according to the gettext specifications.
# Special characters such as %%s symbolise things within the game,
# so should be left in an appropriate place in the message.
# 
# Game identification: this lets game to detect a translation meant
# for another game. These may be left blank too.
# $GameID=
# $GameName=
# ** Translation settings are below
# ** Leave them as ""DEFAULT"" to use the game settings
# The normal font to use - DEFAULT or font number
# $NormalFont=4
# The speech font to use - DEFAULT or font number
# $SpeechFont=8
# Text direction - DEFAULT, LEFT or RIGHT
# $TextDirection=RIGHT
# Text encoding hint - ASCII or UTF-8
# $Encoding=UTF-8
# Text language, use standard locale strings, like 'en', 'en_US', etc
# $Language=fr_FR
# Whether engine should translate Parser.Said strings automatically - ON or OFF
# $AutoTranslateParserSaid=ON
#  
# ** IT IS SUGGESTED TO USE A THIRD-PARTY TOOL TO EDIT THIS FILE
msgid """"
msgstr """"
""Last-Translator: \n""
""Language-Team: \n""
""Language: NewLanguage\n""
""X-Source-Language: en_US\n""
""MIME-Version: 1.0\n""
""Content-Type: text/plain; charset=UTF-8\n""
""Content-Transfer-Encoding: 8bit\n""
""X-Generator: AGS " + AGS.Types.Version.AGS_EDITOR_VERSION + @"\n""
";

        /// <summary>
        /// Unify line endings in the input string, in case they depend on a enviroment.
        /// </summary>
        string UniformTestString(string s)
        {
            return s.Replace("\r\n", "\n");
        }

        private Translation CreateTranslationForTest()
        {
            Translation translation = new Translation("NewLanguage", "en_US");
            translation.AutoTranslateParserSaid = true;
            translation.EncodingHint = "UTF-8";
            translation.Modified = true;
            translation.NormalFont = 4;
            translation.RightToLeftText = true;
            translation.SpeechFont = 8;
            translation.TextLanguage = "fr_FR";
            return translation;
        }

        [Test]
        public void TranslationOptions_Test()
        {
            Translation translation = CreateTranslationForTest();

            Assert.AreEqual(true, translation.AutoTranslateParserSaid);
            Assert.AreEqual("NewLanguage.tra", translation.CompiledFileName);
            Assert.IsTrue(translation.Encoding != null);
            if (translation.Encoding != null)
                Assert.AreEqual("utf-8", translation.Encoding.WebName);
            Assert.AreEqual("UTF-8", translation.EncodingHint);
            Assert.AreEqual("NewLanguage.po", translation.FileName);
            Assert.AreEqual(true, translation.Modified);
            Assert.AreEqual(4, translation.NormalFont);
            Assert.AreEqual(true, translation.RightToLeftText);
            Assert.AreEqual(8, translation.SpeechFont);
            Assert.AreEqual("fr_FR", translation.TextLanguage);
        }

        private TranslationEntry CreateEntry(string text, string translation = null, string context = null, string sourceRef = null)
        {
            TranslationEntry entry = new TranslationEntry(text, translation);
            entry.Context = context;
            entry.SourceReference = sourceRef;
            return entry;
        }

        [Test]
        public void WriteEmptyTranslation_Test()
        {
            Translation translation = CreateTranslationForTest();

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader;

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        [Test]
        public void WriteTranslationWithEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedEntries is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedEntries.Add("first line", CreateEntry("first line", "première réplique"));
            translation.TranslatedEntries.Add("second line", CreateEntry("second line", "deuxième réplique"));
            translation.TranslatedEntries.Add("Don't translate this", CreateEntry("Don't translate this"));
            translation.TranslatedEntries.Add("Good morning", CreateEntry("Good morning", "bonjour"));
            translation.TranslatedEntries.Add("Good evening", CreateEntry("Good evening", "bonsoir"));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader + @"
msgid ""first line""
msgstr ""première réplique""

msgid ""second line""
msgstr ""deuxième réplique""

msgid ""Don't translate this""
msgstr """"

msgid ""Good morning""
msgstr ""bonjour""

msgid ""Good evening""
msgstr ""bonsoir""

";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        [Test]
        public void WriteTranslationWithExtendedEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedEntries is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedEntries.Add("first line", CreateEntry("first line", "première réplique", "context", "source.code: 10"));
            translation.TranslatedEntries.Add("second line", CreateEntry("second line", "deuxième réplique", "context", "source.code: 20"));
            translation.TranslatedEntries.Add("Don't translate this", CreateEntry("Don't translate this", "", null, "non-translated source"));
            translation.TranslatedEntries.Add("Good morning", CreateEntry("Good morning", "bonjour", "Good day", null));
            translation.TranslatedEntries.Add("Good evening", CreateEntry("Good evening", "bonsoir", "Good day", null));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader + @"
#: source.code: 10
msgctxt ""context""
msgid ""first line""
msgstr ""première réplique""

#: source.code: 20
msgctxt ""context""
msgid ""second line""
msgstr ""deuxième réplique""

#: non-translated source
msgid ""Don't translate this""
msgstr """"

msgctxt ""Good day""
msgid ""Good morning""
msgstr ""bonjour""

msgctxt ""Good day""
msgid ""Good evening""
msgstr ""bonsoir""

";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        [Test]
        public void LoadTranslationBackWithEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedEntries is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedEntries.Add("first line", CreateEntry("first line", "première réplique"));
            translation.TranslatedEntries.Add("second line", CreateEntry("second line", "deuxième réplique"));
            translation.TranslatedEntries.Add("Don't translate this", CreateEntry("Don't translate this"));
            translation.TranslatedEntries.Add("Good morning", CreateEntry("Good morning", "bonjour"));
            translation.TranslatedEntries.Add("Good evening", CreateEntry("Good evening", "bonsoir"));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage", "en_US");
            var errors = new CompileMessages();
            using (StreamReader sr = new StreamReader(new MemoryStream(buffer), translationEncoding))
            {
                errors = translation.TryLoadData(sr);
            }

            Assert.That(!errors.HasErrors);
            Assert.AreEqual(true, translation.AutoTranslateParserSaid);
            Assert.AreEqual("NewLanguage.tra", translation.CompiledFileName);
            Assert.IsTrue(translation.Encoding != null);
            if (translation.Encoding != null)
                Assert.AreEqual("utf-8", translation.Encoding.WebName);
            Assert.AreEqual("UTF-8", translation.EncodingHint);
            Assert.AreEqual("NewLanguage.po", translation.FileName);
            Assert.AreEqual(false, translation.Modified);
            Assert.AreEqual(4, translation.NormalFont);
            Assert.AreEqual(true, translation.RightToLeftText);
            Assert.AreEqual(8, translation.SpeechFont);
            Assert.AreEqual("fr_FR", translation.TextLanguage);

            Assert.AreEqual(5, translation.TranslatedEntries.Count);
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("first line"));
            {
                var entry = translation.TranslatedEntries["first line"];
                Assert.AreEqual("first line", entry.Key);
                Assert.AreEqual("première réplique", entry.Value);
                Assert.AreEqual(null, entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("second line"));
            {
                var entry = translation.TranslatedEntries["second line"];
                Assert.AreEqual("second line", entry.Key);
                Assert.AreEqual("deuxième réplique", entry.Value);
                Assert.AreEqual(null, entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("Don't translate this"));
            {
                var entry = translation.TranslatedEntries["Don't translate this"];
                Assert.AreEqual("Don't translate this", entry.Key);
                Assert.AreEqual("", entry.Value);
                Assert.AreEqual(null, entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("Good morning"));
            {
                var entry = translation.TranslatedEntries["Good morning"];
                Assert.AreEqual("Good morning", entry.Key);
                Assert.AreEqual("bonjour", entry.Value);
                Assert.AreEqual(null, entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("Good evening"));
            {
                var entry = translation.TranslatedEntries["Good evening"];
                Assert.AreEqual("Good evening", entry.Key);
                Assert.AreEqual("bonsoir", entry.Value);
                Assert.AreEqual(null, entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
        }

        [Test]
        public void LoadTranslationBackWithExtendedEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedEntries is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedEntries.Add("first line", CreateEntry("first line", "première réplique", "context", "source.code: 10"));
            translation.TranslatedEntries.Add("second line", CreateEntry("second line", "deuxième réplique", "context", "source.code: 20"));
            translation.TranslatedEntries.Add("Don't translate this", CreateEntry("Don't translate this", "", null, "non-translated source"));
            translation.TranslatedEntries.Add("Good morning", CreateEntry("Good morning", "bonjour", "Good day", null));
            translation.TranslatedEntries.Add("Good evening", CreateEntry("Good evening", "bonsoir", "Good day", null));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage", "en_US");
            var errors = new CompileMessages();
            using (StreamReader sr = new StreamReader(new MemoryStream(buffer), translationEncoding))
            {
                errors = translation.TryLoadData(sr);
            }

            Assert.That(!errors.HasErrors);
            Assert.AreEqual(true, translation.AutoTranslateParserSaid);
            Assert.AreEqual("NewLanguage.tra", translation.CompiledFileName);
            Assert.IsTrue(translation.Encoding != null);
            if (translation.Encoding != null)
                Assert.AreEqual("utf-8", translation.Encoding.WebName);
            Assert.AreEqual("UTF-8", translation.EncodingHint);
            Assert.AreEqual("NewLanguage.po", translation.FileName);
            Assert.AreEqual(false, translation.Modified);
            Assert.AreEqual(4, translation.NormalFont);
            Assert.AreEqual(true, translation.RightToLeftText);
            Assert.AreEqual(8, translation.SpeechFont);
            Assert.AreEqual("fr_FR", translation.TextLanguage);

            Assert.AreEqual(5, translation.TranslatedEntries.Count);
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("first line"));
            {
                var entry = translation.TranslatedEntries["first line"];
                Assert.AreEqual("first line", entry.Key);
                Assert.AreEqual("première réplique", entry.Value);
                Assert.AreEqual("context", entry.Context);
                Assert.AreEqual("source.code: 10", entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("second line"));
            {
                var entry = translation.TranslatedEntries["second line"];
                Assert.AreEqual("second line", entry.Key);
                Assert.AreEqual("deuxième réplique", entry.Value);
                Assert.AreEqual("context", entry.Context);
                Assert.AreEqual("source.code: 20", entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("Don't translate this"));
            {
                var entry = translation.TranslatedEntries["Don't translate this"];
                Assert.AreEqual("Don't translate this", entry.Key);
                Assert.AreEqual("", entry.Value);
                Assert.AreEqual(null, entry.Context);
                Assert.AreEqual("non-translated source", entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("Good morning"));
            {
                var entry = translation.TranslatedEntries["Good morning"];
                Assert.AreEqual("Good morning", entry.Key);
                Assert.AreEqual("bonjour", entry.Value);
                Assert.AreEqual("Good day", entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("Good evening"));
            {
                var entry = translation.TranslatedEntries["Good evening"];
                Assert.AreEqual("Good evening", entry.Key);
                Assert.AreEqual("bonsoir", entry.Value);
                Assert.AreEqual("Good day", entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
        }

        [Test]
        public void UpdateTranslationWithEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedEntries is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedEntries.Add("first line", CreateEntry("first line", "première réplique"));
            translation.TranslatedEntries.Add("second line", CreateEntry("second line", "deuxième réplique"));
            translation.TranslatedEntries.Add("Don't translate this", CreateEntry("Don't translate this"));
            translation.TranslatedEntries.Add("Good morning", CreateEntry("Good morning", "bonjour"));
            translation.TranslatedEntries.Add("Good evening", CreateEntry("Good evening", "bonsoir"));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage", "en_US");
            var errors = new CompileMessages();
            using (StreamReader sr = new StreamReader(new MemoryStream(buffer), translationEncoding))
            {
                errors = translation.TryLoadData(sr);
            }

            Assert.That(!errors.HasErrors);

            var gameTexts = new GameTextLine[] { 
                new GameTextLine("first line"),
                // "second line" is deprecated
                // "Don't translate this" is deprecated
                new GameTextLine("Good morning"),
                new GameTextLine("Good evening"),
                new GameTextLine("This is a new line"),
                new GameTextLine("This is another new line"),
            };
            errors = TranslationsComponent.UpdateTranslation(translation, gameTexts);

            ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader + @"
msgid ""first line""
msgstr ""première réplique""

msgctxt ""OBSOLETE""
msgid ""second line""
msgstr ""deuxième réplique""

msgid ""This is a new line""
msgstr """"

msgid ""Good morning""
msgstr ""bonjour""

msgid ""Good evening""
msgstr ""bonsoir""

msgid ""This is another new line""
msgstr """"

";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        [Test]
        public void UpdateTranslationWithExtendedEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedEntries is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedEntries.Add("first line", CreateEntry("first line", "première réplique", "context", "source.code: 10"));
            translation.TranslatedEntries.Add("second line", CreateEntry("second line", "deuxième réplique", "context", "source.code: 20"));
            translation.TranslatedEntries.Add("Don't translate this", CreateEntry("Don't translate this", "", null, "non-translated source"));
            translation.TranslatedEntries.Add("Good morning", CreateEntry("Good morning", "bonjour", "Good day", null));
            translation.TranslatedEntries.Add("Good evening", CreateEntry("Good evening", "bonsoir", "Good day", null));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage", "en_US");
            var errors = new CompileMessages();
            using (StreamReader sr = new StreamReader(new MemoryStream(buffer), translationEncoding))
            {
                errors = translation.TryLoadData(sr);
            }

            Assert.That(!errors.HasErrors);

            // NOTE: unfortunately, there's no way to add custom annotations into update atm,
            // but the existing custom annotations found in the source file should be kept!
            var gameTexts = new GameTextLine[] {
                new GameTextLine("first line", "source.code: 15"), // source ref changed
                // "second line" is deprecated
                // "Don't translate this" is deprecated
                new GameTextLine("Good morning", "good.source.code: 10"), // source ref added
                new GameTextLine("Good evening", "good.source.code: 20"), // source ref added
                new GameTextLine("This is a new line", "source.code: 30"),
                new GameTextLine("This is another new line", "source.code: 40"),
            };
            errors = TranslationsComponent.UpdateTranslation(translation, gameTexts);

            ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader + @"
#: source.code: 15
msgctxt ""context""
msgid ""first line""
msgstr ""première réplique""

#: source.code: 20
msgctxt ""OBSOLETE""
msgid ""second line""
msgstr ""deuxième réplique""

#: source.code: 30
msgid ""This is a new line""
msgstr """"

#: good.source.code: 10
msgctxt ""Good day""
msgid ""Good morning""
msgstr ""bonjour""

#: good.source.code: 20
msgctxt ""Good day""
msgid ""Good evening""
msgstr ""bonsoir""

#: source.code: 40
msgid ""This is another new line""
msgstr """"

";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }
    }
}
