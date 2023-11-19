using System;
using System.IO;
using NUnit.Framework;
using AGS.CScript.Compiler;

namespace AGS.CScript.Compiler
{
    [TestFixture]
    public class PreprocessorTests
    {
        public string NoCR(string str)
        {
            return str.Replace("\r\n", "\n").Replace('\r', '\n');
        }

        void AssertStringEqual(string str_a, string str_b)
        {
            Assert.That(NoCR(str_a), Is.EqualTo(NoCR(str_b)));
        }

        [Test]
        public void Comments()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string script = $@"
// this is a comment
// this is another comment
// 1234
//#define invalid 5
// invalid
int i;
";
            string res =  preprocessor.Preprocess(script, "ScriptName");
            Assert.That(preprocessor.Results.Count == 0);
            string script_res = $@"""__NEWSCRIPTSTART_ScriptName""






int i;
";

            AssertStringEqual(res, script_res);
        }

        [Test]
        public void MultilineComments()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string script = $@"
int hey /* comment  */= 7;
// this  
""this prints"";
//;
// /*
Display(""this does display!"");
// */
/* this
is a real
multiline comment*/
int k;
";
            string res = preprocessor.Preprocess(script, "MultiLine");
            Assert.That(preprocessor.Results.Count == 0);
            string script_res = $@"""__NEWSCRIPTSTART_MultiLine""

int hey = 7;

""this prints"";


Display(""this does display!"");




int k;
";

            AssertStringEqual(res, script_res);
        }

        [Test]
        public void Define()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string script = $@"
#define MACRO2 MACRO3
#define MACRO3 9
#define TEST1 5
Display(""a: % d"", TEST1);
Display(""b: TEST1"");
Display(""d: %d"", MACRO3);
Display(""e: %d"", MACRO2);
#define MACRO4 MACRO3
Display(""f: %d"", MACRO4);
";
            string res = preprocessor.Preprocess(script, "ScriptDefine");
            Assert.That(preprocessor.Results.Count == 0);
            string script_res = $@"""__NEWSCRIPTSTART_ScriptDefine""




Display(""a: % d"", 5);
Display(""b: TEST1"");
Display(""d: %d"", 9);
Display(""e: %d"", 9);

Display(""f: %d"", 9);
";

            AssertStringEqual(res, script_res);
        }

        [Test]
        public void MacroDoesNotExist()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string script = $@"
#define BAR
#undef FOO
";
            string res = preprocessor.Preprocess(script, "MacroDoesNotExist");
            Assert.That(preprocessor.Results.Count, Is.EqualTo(1));
            Assert.That(preprocessor.Results[0].Code, Is.EqualTo(ErrorCode.MacroDoesNotExist));
        }

        [Test]
        public void MacroNameMissing()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string script = $@"
#define BAR
#undef
";
            string res = preprocessor.Preprocess(script, "MacroNameMissing");
            Assert.That(preprocessor.Results.Count, Is.EqualTo(1));
            Assert.That(preprocessor.Results[0].Code, Is.EqualTo(ErrorCode.MacroNameMissing));
        }

        [Test]
        public void MacroStartsWithDigit()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string script = $@"
#define 1BAR
";
            string res = preprocessor.Preprocess(script, "MacroStartsWithDigit");
            Assert.That(preprocessor.Results.Count, Is.EqualTo(1));
            Assert.That(preprocessor.Results[0].Code, Is.EqualTo(ErrorCode.MacroNameInvalid));
        }

        [Test]
        public void UserError()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string script = $@"
#error my error here
Display(""this doesn't display"");
";
            string res = preprocessor.Preprocess(script, "UserDefinedError");
            Assert.That(preprocessor.Results.Count, Is.EqualTo(1));
            Assert.That(preprocessor.Results[0].Code, Is.EqualTo(ErrorCode.UserDefinedError));
        }

        [Test]
        public void RemoveEditorDirectives()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string script = $@"
#region THIS_IS_DISPLAY
Display(""Prints normally"");
#endregion THIS_IS_DISPLAY
#sectionstart
Display(""Prints normally too"");
#sectionend
";
            string res = preprocessor.Preprocess(script, "RemoveEditorDirectives");
            Assert.That(preprocessor.Results.Count == 0);
            string script_res = $@"""__NEWSCRIPTSTART_RemoveEditorDirectives""


Display(""Prints normally"");


Display(""Prints normally too"");

";

            AssertStringEqual(res, script_res);
        }


        [Test]
        public void UnknownDirective()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string script = $@"
#valhalla
";
            string res = preprocessor.Preprocess(script, "UnknownDirective");
            Assert.That(preprocessor.Results.Count, Is.EqualTo(1));
            Assert.That(preprocessor.Results[0].Code, Is.EqualTo(ErrorCode.UnknownPreprocessorDirective));
        }

        [Test]
        public void IfDef()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string script = $@"
#define FOO
#ifdef FOO
Display(""This displays!"");
#endif
#undef FOO
#ifdef FOO
Display(""This doesn't"");
#endif
#ifdef BAR
Display(""This doesn't too"");
#endif
#define BAR
#ifdef BAR
Display(""This displays dude"");
Display(""and this too"");
#endif
#ifndef BAR
Display(""This doesn't"");
#endif
#ifndef BORK
Display(""This does"");
#endif
";
            string res = preprocessor.Preprocess(script, "ScriptIfDef");
            Assert.That(preprocessor.Results.Count == 0);
            string script_res = $@"""__NEWSCRIPTSTART_ScriptIfDef""



Display(""This displays!"");










Display(""This displays dude"");
Display(""and this too"");





Display(""This does"");

";

            AssertStringEqual(res, script_res);
        }

        [Test]
        public void IfDefElse()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string script = $@"
#define FOO
#ifdef FOO
Display(""This displays!"");
#else
Display(""This doesn't"");
#endif
#ifndef FOO
            Display(""This doesn't"");
#else
            Display(""This displays!"");
#endif
#undef FOO
#ifdef FOO
            Display(""This doesn't"");
#else
            Display(""This displays!"");
#endif
#ifndef FOO
            Display(""This displays!"");
#else
            Display(""This doesn't"");
#endif
";
            string res = preprocessor.Preprocess(script, "ScriptIfDefElse");
            Assert.That(preprocessor.Results.Count == 0);
            string script_res = $@"""__NEWSCRIPTSTART_ScriptIfDefElse""



Display(""This displays!"");






Display(""This displays!"");





Display(""This displays!"");


Display(""This displays!"");



";

            AssertStringEqual(res, script_res);
        }

        [Test]
        public void IfVer()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor("3.6.0.5");
            string script = $@"
#ifver 3.6.0
Display(""This displays!"");
#endif
#ifver 3.7.0
Display(""This doesn't"");
#endif
#ifver 3.4.0
Display(""This displays yey!"");
#endif
#ifver 2.7.2
Display(""This displays dude"");
#endif
#ifver 3.6.0.6
Display(""doesn't display"");
#endif
#ifnver 3.6.0.6
Display(""But this displays"");
#endif
";
            string res = preprocessor.Preprocess(script, "ScriptIfVer");
            Assert.That(preprocessor.Results.Count == 0);
            string script_res = $@"""__NEWSCRIPTSTART_ScriptIfVer""


Display(""This displays!"");





Display(""This displays yey!"");


Display(""This displays dude"");





Display(""But this displays"");

";

            AssertStringEqual(res, script_res);
        }

        [Test]
        public void IfWithoutEndIf()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string script = $@"
#ifdef BAR
#endif
#ifdef FOO
Display(""test"");
";
            string res = preprocessor.Preprocess(script, "IfWithoutEndIf");
            Assert.That(preprocessor.Results.Count, Is.EqualTo(1));
            Assert.That(preprocessor.Results[0].Code, Is.EqualTo(ErrorCode.IfWithoutEndIf));
        }

        [Test]
        public void EndIfWithoutIf()
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string script = $@"
#ifdef BAR
#endif
Display(""test"");
#endif
";
            string res = preprocessor.Preprocess(script, "EndIfWithoutIf");
            Assert.That(preprocessor.Results.Count, Is.EqualTo(1));
            Assert.That(preprocessor.Results[0].Code, Is.EqualTo(ErrorCode.EndIfWithoutIf));
        }

        [TestCase("room2.asc", "room2.asc")]
        [TestCase(@"Rooms\2\room2.asc", @"Rooms\\2\\room2.asc")]
        public void FormatsScriptName(string scriptName, string expected)
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            string res = preprocessor.Preprocess(string.Empty, scriptName);
            AssertStringEqual(res, $@"""__NEWSCRIPTSTART_{expected}""
");
        }
    }
}
