using AGS.Editor.Components;
using AGS.Types;
using NUnit.Framework;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class TranslationTests
    {
        private const string GeneratedTranslationHeader = @"// AGS TRANSLATION SOURCE FILE
// Format is alternating lines with original game text and replacement
// text. If you don't want to translate a line, just leave the following
// line blank. Lines starting with '//' are comments - DO NOT translate
// them. Special characters such as [ and %%s symbolise things within the
// game, so should be left in an appropriate place in the message.
// 
// ** Translation settings are below
// ** Leave them as ""DEFAULT"" to use the game settings
// The normal font to use - DEFAULT or font number
//#NormalFont=4
// The speech font to use - DEFAULT or font number
//#SpeechFont=8
// Text direction - DEFAULT, LEFT or RIGHT
//#TextDirection=RIGHT
// Text encoding hint - ASCII or UTF-8
//#Encoding=UTF-8
// Text language, use standard locale strings, like 'en', 'en_US', etc
//#Language=fr_FR
// Whether engine should translate Parser.Said strings automatically - ON or OFF
//#AutoTranslateParserSaid=ON
//  
// ** REMEMBER, WRITE YOUR TRANSLATION IN THE EMPTY LINES, DO
// ** NOT CHANGE THE EXISTING TEXT.
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
            Translation translation = new Translation("NewLanguage");
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
            Assert.AreEqual("NewLanguage.trs", translation.FileName);
            Assert.AreEqual(true, translation.Modified);
            Assert.AreEqual(4, translation.NormalFont);
            Assert.AreEqual(true, translation.RightToLeftText);
            Assert.AreEqual(8, translation.SpeechFont);
            Assert.AreEqual("fr_FR", translation.TextLanguage);
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

            // WARNING: this test is tricky, because TranslatedLines is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedLines.Add("first line", "première réplique");
            translation.TranslatedLines.Add("second line", "deuxième réplique");
            translation.TranslatedLines.Add("Don't translate this", "");
            translation.TranslatedLines.Add("Good morning", "bonjour");
            translation.TranslatedLines.Add("Good evening", "bonsoir");

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader +
@"//-----------------------------------------------------------------------------
//$SECTION:
//-----------------------------------------------------------------------------
first line
première réplique
second line
deuxième réplique
Don't translate this

Good morning
bonjour
Good evening
bonsoir
";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        [Test]
        public void WriteTranslationWithSections_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedLines is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedLines.Add("first line", "première réplique");
            translation.TranslatedLines.Add("second line", "deuxième réplique");
            translation.TranslatedLines.Add("Don't translate this", "");
            translation.TranslatedLines.Add("Good morning", "bonjour");
            translation.TranslatedLines.Add("Good evening", "bonsoir");

            TranslationSection simpleSection = new TranslationSection("Simple Section");
            TranslationSection noTranslateSection = new TranslationSection("No Translate");
            TranslationSection goodDaySection = new TranslationSection("Good day", "A comment");
            translation.TranslationSections.Add("first line", simpleSection);
            translation.TranslationSections.Add("second line", simpleSection);
            translation.TranslationSections.Add("Don't translate this", noTranslateSection);
            translation.TranslationSections.Add("Good morning", goodDaySection);
            translation.TranslationSections.Add("Good evening", goodDaySection);

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader +
@"//-----------------------------------------------------------------------------
//$SECTION: Simple Section
//-----------------------------------------------------------------------------
first line
première réplique
second line
deuxième réplique
//-----------------------------------------------------------------------------
//$SECTION: No Translate
//-----------------------------------------------------------------------------
Don't translate this

//-----------------------------------------------------------------------------
//$SECTION: Good day; A comment
//-----------------------------------------------------------------------------
Good morning
bonjour
Good evening
bonsoir
";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        private TranslationEntryOptions CreateEntryOptions(string[] arr)
        {
            var options = new TranslationEntryOptions();
            options.Metadata = new List<string>(arr);
            return options;
        }

        [Test]
        public void WriteTranslationWithAnnotations_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedLines is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedLines.Add("first line", "première réplique");
            translation.TranslatedLines.Add("second line", "deuxième réplique");
            translation.TranslatedLines.Add("Don't translate this", "");
            translation.TranslatedLines.Add("Good morning", "bonjour");
            translation.TranslatedLines.Add("Good evening", "bonsoir");
            translation.TranslatedLines.Add(",apple,fruit", "");
            translation.TranslatedLines.Add(",take,pick,pick up", "");

            translation.TranslatedEntryOptions.Add("first line", CreateEntryOptions(new string[] { "OBSOLETE", "COMMENT: type anything" }));
            translation.TranslatedEntryOptions.Add("Don't translate this", CreateEntryOptions(new string[] { "DON'T TRANSLATE ME" }));
            translation.TranslatedEntryOptions.Add(",apple,fruit", CreateEntryOptions(new string[] { "PARSERWORD:1" }));
            translation.TranslatedEntryOptions.Add(",take,pick,pick up", CreateEntryOptions(new string[] { "PARSERWORD:2" }));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader +
@"//-----------------------------------------------------------------------------
//$SECTION:
//-----------------------------------------------------------------------------
//$OBSOLETE
//$COMMENT: type anything
first line
première réplique
second line
deuxième réplique
//$DON'T TRANSLATE ME
Don't translate this

Good morning
bonjour
Good evening
bonsoir
//$PARSERWORD:1
,apple,fruit

//$PARSERWORD:2
,take,pick,pick up

";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        [Test]
        public void LoadTranslationBackWithEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedLines is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedLines.Add("first line", "première réplique");
            translation.TranslatedLines.Add("second line", "deuxième réplique");
            translation.TranslatedLines.Add("Don't translate this", "");
            translation.TranslatedLines.Add("Good morning", "bonjour");
            translation.TranslatedLines.Add("Good evening", "bonsoir");

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage");
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
            Assert.AreEqual("NewLanguage.trs", translation.FileName);
            Assert.AreEqual(false, translation.Modified);
            Assert.AreEqual(4, translation.NormalFont);
            Assert.AreEqual(true, translation.RightToLeftText);
            Assert.AreEqual(8, translation.SpeechFont);
            Assert.AreEqual("fr_FR", translation.TextLanguage);

            Assert.AreEqual(5, translation.TranslatedLines.Count);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("first line"));
            Assert.AreEqual("première réplique", translation.TranslatedLines["first line"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("second line"));
            Assert.AreEqual("deuxième réplique", translation.TranslatedLines["second line"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("Don't translate this"));
            Assert.AreEqual("", translation.TranslatedLines["Don't translate this"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("Good morning"));
            Assert.AreEqual("bonjour", translation.TranslatedLines["Good morning"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("Good evening"));
            Assert.AreEqual("bonsoir", translation.TranslatedLines["Good evening"]);
        }

        [Test]
        public void LoadTranslationBackWithSections_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedLines is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedLines.Add("first line", "première réplique");
            translation.TranslatedLines.Add("second line", "deuxième réplique");
            translation.TranslatedLines.Add("Don't translate this", "");
            translation.TranslatedLines.Add("Good morning", "bonjour");
            translation.TranslatedLines.Add("Good evening", "bonsoir");

            TranslationSection simpleSection = new TranslationSection("Simple Section");
            TranslationSection noTranslateSection = new TranslationSection("No Translate");
            TranslationSection goodDaySection = new TranslationSection("Good day", "A comment");
            translation.TranslationSections.Add("first line", simpleSection);
            translation.TranslationSections.Add("second line", simpleSection);
            translation.TranslationSections.Add("Don't translate this", noTranslateSection);
            translation.TranslationSections.Add("Good morning", goodDaySection);
            translation.TranslationSections.Add("Good evening", goodDaySection);

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage");
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
            Assert.AreEqual("NewLanguage.trs", translation.FileName);
            Assert.AreEqual(false, translation.Modified);
            Assert.AreEqual(4, translation.NormalFont);
            Assert.AreEqual(true, translation.RightToLeftText);
            Assert.AreEqual(8, translation.SpeechFont);
            Assert.AreEqual("fr_FR", translation.TextLanguage);

            Assert.AreEqual(5, translation.TranslatedLines.Count);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("first line"));
            Assert.AreEqual("première réplique", translation.TranslatedLines["first line"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("second line"));
            Assert.AreEqual("deuxième réplique", translation.TranslatedLines["second line"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("Don't translate this"));
            Assert.AreEqual("", translation.TranslatedLines["Don't translate this"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("Good morning"));
            Assert.AreEqual("bonjour", translation.TranslatedLines["Good morning"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("Good evening"));
            Assert.AreEqual("bonsoir", translation.TranslatedLines["Good evening"]);

            Assert.AreEqual(5, translation.TranslationSections.Count);
            Assert.IsTrue(translation.TranslationSections.ContainsKey("first line"));
            Assert.AreEqual("Simple Section", translation.TranslationSections["first line"].Name);
            Assert.AreEqual("", translation.TranslationSections["first line"].Comment);
            Assert.IsTrue(translation.TranslationSections.ContainsKey("second line"));
            Assert.AreEqual("Simple Section", translation.TranslationSections["second line"].Name);
            Assert.AreEqual("", translation.TranslationSections["second line"].Comment);
            Assert.IsTrue(translation.TranslationSections.ContainsKey("Don't translate this"));
            Assert.AreEqual("No Translate", translation.TranslationSections["Don't translate this"].Name);
            Assert.AreEqual("", translation.TranslationSections["Don't translate this"].Comment);
            Assert.IsTrue(translation.TranslationSections.ContainsKey("Good morning"));
            Assert.AreEqual("Good day", translation.TranslationSections["Good morning"].Name);
            Assert.AreEqual("A comment", translation.TranslationSections["Good morning"].Comment);
            Assert.IsTrue(translation.TranslationSections.ContainsKey("Good evening"));
            Assert.AreEqual("Good day", translation.TranslationSections["Good evening"].Name);
            Assert.AreEqual("A comment", translation.TranslationSections["Good evening"].Comment);
        }

        [Test]
        public void LoadTranslationBackWithAnnotations_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedLines is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedLines.Add("first line", "première réplique");
            translation.TranslatedLines.Add("second line", "deuxième réplique");
            translation.TranslatedLines.Add("Don't translate this", "");
            translation.TranslatedLines.Add("Good morning", "bonjour");
            translation.TranslatedLines.Add("Good evening", "bonsoir");
            translation.TranslatedLines.Add(",apple,fruit", "");
            translation.TranslatedLines.Add(",take,pick,pick up", "");

            translation.TranslatedEntryOptions.Add("first line", CreateEntryOptions(new string[] { "OBSOLETE", "COMMENT: type anything" }));
            translation.TranslatedEntryOptions.Add("Don't translate this", CreateEntryOptions(new string[] { "DON'T TRANSLATE ME" }));
            translation.TranslatedEntryOptions.Add(",apple,fruit", CreateEntryOptions(new string[] { "PARSERWORD:1" }));
            translation.TranslatedEntryOptions.Add(",take,pick,pick up", CreateEntryOptions(new string[] { "PARSERWORD:2" }));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage");
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
            Assert.AreEqual("NewLanguage.trs", translation.FileName);
            Assert.AreEqual(false, translation.Modified);
            Assert.AreEqual(4, translation.NormalFont);
            Assert.AreEqual(true, translation.RightToLeftText);
            Assert.AreEqual(8, translation.SpeechFont);
            Assert.AreEqual("fr_FR", translation.TextLanguage);

            Assert.AreEqual(7, translation.TranslatedLines.Count);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("first line"));
            Assert.AreEqual("première réplique", translation.TranslatedLines["first line"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("second line"));
            Assert.AreEqual("deuxième réplique", translation.TranslatedLines["second line"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("Don't translate this"));
            Assert.AreEqual("", translation.TranslatedLines["Don't translate this"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("Good morning"));
            Assert.AreEqual("bonjour", translation.TranslatedLines["Good morning"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey("Good evening"));
            Assert.AreEqual("bonsoir", translation.TranslatedLines["Good evening"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey(",apple,fruit"));
            Assert.AreEqual("", translation.TranslatedLines[",apple,fruit"]);
            Assert.IsTrue(translation.TranslatedLines.ContainsKey(",take,pick,pick up"));
            Assert.AreEqual("", translation.TranslatedLines[",take,pick,pick up"]);

            Assert.AreEqual(4, translation.TranslatedEntryOptions.Count);
            Assert.IsTrue(translation.TranslatedEntryOptions.ContainsKey("first line"));
            var options = translation.TranslatedEntryOptions["first line"];
            Assert.AreEqual(2, options.Metadata.Count);
            Assert.AreEqual("OBSOLETE", options.Metadata[0]);
            Assert.AreEqual("COMMENT: type anything", options.Metadata[1]);
            Assert.AreEqual(true, options.IsObsolete);
            Assert.IsTrue(translation.TranslatedEntryOptions.ContainsKey("Don't translate this"));
            options = translation.TranslatedEntryOptions["Don't translate this"];
            Assert.AreEqual(1, options.Metadata.Count);
            Assert.AreEqual("DON'T TRANSLATE ME", options.Metadata[0]);
            Assert.IsTrue(translation.TranslatedEntryOptions.ContainsKey(",apple,fruit"));
            options = translation.TranslatedEntryOptions[",apple,fruit"];
            Assert.AreEqual(1, options.Metadata.Count);
            Assert.AreEqual("PARSERWORD:1", options.Metadata[0]);
            Assert.IsTrue(options.IsParserDictionary);
            Assert.AreEqual(1, options.ParserWordID);
            Assert.IsTrue(translation.TranslatedEntryOptions.ContainsKey(",take,pick,pick up"));
            options = translation.TranslatedEntryOptions[",take,pick,pick up"];
            Assert.AreEqual(1, options.Metadata.Count);
            Assert.AreEqual("PARSERWORD:2", options.Metadata[0]);
            Assert.IsTrue(options.IsParserDictionary);
            Assert.AreEqual(2, options.ParserWordID);
        }

        [Test]
        public void UpdateTranslationWithEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedLines is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedLines.Add("first line", "première réplique");
            translation.TranslatedLines.Add("second line", "deuxième réplique");
            translation.TranslatedLines.Add("Don't translate this", "");
            translation.TranslatedLines.Add("Good morning", "bonjour");
            translation.TranslatedLines.Add("Good evening", "bonsoir");

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage");
            var errors = new CompileMessages();
            using (StreamReader sr = new StreamReader(new MemoryStream(buffer), translationEncoding))
            {
                errors = translation.TryLoadData(sr);
            }

            Assert.That(!errors.HasErrors);
            Assert.That(!translation.Modified);

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
            Assert.That(translation.Modified);

            ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader +
@"//-----------------------------------------------------------------------------
//$SECTION:
//-----------------------------------------------------------------------------
first line
première réplique
//$OBSOLETE
second line
deuxième réplique
Good morning
bonjour
Good evening
bonsoir
This is a new line

This is another new line

";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        [Test]
        public void UpdateTranslationWithSections_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedLines is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedLines.Add("first line", "première réplique");
            translation.TranslatedLines.Add("second line", "deuxième réplique");
            translation.TranslatedLines.Add("Don't translate this", "");
            translation.TranslatedLines.Add("Good morning", "bonjour");
            translation.TranslatedLines.Add("Good evening", "bonsoir");

            TranslationSection simpleSection = new TranslationSection("Simple Section");
            TranslationSection noTranslateSection = new TranslationSection("No Translate");
            TranslationSection goodDaySection = new TranslationSection("Good day", "A comment");
            translation.TranslationSections.Add("first line", simpleSection);
            translation.TranslationSections.Add("second line", simpleSection);
            translation.TranslationSections.Add("Don't translate this", noTranslateSection);
            translation.TranslationSections.Add("Good morning", goodDaySection);
            translation.TranslationSections.Add("Good evening", goodDaySection);

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage");
            var errors = new CompileMessages();
            using (StreamReader sr = new StreamReader(new MemoryStream(buffer), translationEncoding))
            {
                errors = translation.TryLoadData(sr);
            }

            Assert.That(!errors.HasErrors);
            Assert.That(!translation.Modified);

            var gameTexts = new GameTextLine[] {
                new GameTextLine("first line", "Simple Section", "New comment"),
                // "second line" is deprecated
                // "Don't translate this" is deprecated
                new GameTextLine("Good morning", "Good day", "A comment"),
                new GameTextLine("Good evening", "Good day"),
                new GameTextLine("This is a new line", "Simple Section"),
                new GameTextLine("This is another new line", "Simple Section"),
                new GameTextLine("Good night", "Good day"),
                new GameTextLine("This line is in a completely new section", "New section"),
            };
            errors = TranslationsComponent.UpdateTranslation(translation, gameTexts);
            Assert.That(translation.Modified);

            ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader +
@"//-----------------------------------------------------------------------------
//$SECTION: Simple Section; New comment
//-----------------------------------------------------------------------------
first line
première réplique
//$OBSOLETE
second line
deuxième réplique
This is a new line

This is another new line

//-----------------------------------------------------------------------------
//$SECTION: Good day; A comment
//-----------------------------------------------------------------------------
Good morning
bonjour
Good evening
bonsoir
Good night

//-----------------------------------------------------------------------------
//$SECTION: New section
//-----------------------------------------------------------------------------
This line is in a completely new section

";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        [Test]
        public void UpdateTranslationWithAnnotations_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedLines is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedLines.Add("first line", "première réplique");
            translation.TranslatedLines.Add("second line", "deuxième réplique");
            translation.TranslatedLines.Add("Don't translate this", "");
            translation.TranslatedLines.Add("Good morning", "bonjour");
            translation.TranslatedLines.Add("Good evening", "bonsoir");
            translation.TranslatedLines.Add(",apple,fruit", "");
            translation.TranslatedLines.Add(",take,pick,pick up", "");

            translation.TranslatedEntryOptions.Add("first line", CreateEntryOptions(new string[] { "OBSOLETE", "COMMENT: type anything" }));
            translation.TranslatedEntryOptions.Add("Don't translate this", CreateEntryOptions(new string[] { "DON'T TRANSLATE ME" }));
            translation.TranslatedEntryOptions.Add(",apple,fruit", CreateEntryOptions(new string[] { "PARSERWORD:1" }));
            translation.TranslatedEntryOptions.Add(",take,pick,pick up", CreateEntryOptions(new string[] { "PARSERWORD:2" }));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage");
            var errors = new CompileMessages();
            using (StreamReader sr = new StreamReader(new MemoryStream(buffer), translationEncoding))
            {
                errors = translation.TryLoadData(sr);
            }

            Assert.That(!errors.HasErrors);
            Assert.That(!translation.Modified);

            // NOTE: unfortunately, there's no way to add custom annotations into update atm,
            // but the existing custom annotations found in the source file should be kept!
            var gameTexts = new GameTextLine[] {
                new GameTextLine("first line"),
                // "second line" is deprecated
                // "Don't translate this" is deprecated
                new GameTextLine("Good morning"),
                new GameTextLine("Good evening"),
                new GameTextLine("This is a new line"),
                new GameTextLine("This is another new line"),
                GameTextLine.MakeParserWord(1, ",apple,fruit", ""),
                GameTextLine.MakeParserWord(2, ",take,pick,pick up", ""),
            };
            errors = TranslationsComponent.UpdateTranslation(translation, gameTexts);
            Assert.That(translation.Modified);

            ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader +
@"//-----------------------------------------------------------------------------
//$SECTION:
//-----------------------------------------------------------------------------
//$COMMENT: type anything
first line
première réplique
//$OBSOLETE
second line
deuxième réplique
Good morning
bonjour
Good evening
bonsoir
//$PARSERWORD:1
,apple,fruit

//$PARSERWORD:2
,take,pick,pick up

This is a new line

This is another new line

";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }
    }
}
