using System.Collections.Generic;
using System.Linq;
using NUnit.Framework;
using AGS.Editor;
using AGS.Types;
using AGS.Types.AutoComplete;
using System.Text.RegularExpressions;

namespace AGS.Editor
{
    [TestFixture]
    public class AutoCompleteTests
    {
        private static Script CachedAutoCompletedScriptFromCode(string scriptCode)
        {
            scriptCode = Regex.Replace(scriptCode, @"\r\n|\n\r|\n|\r", "\r\n");
            Script script = new Script("_test", scriptCode, false);
            AutoComplete.ConstructCache(script, new List<Script>());
            return script;
        }

        [SetUp]
        public void SetUp()
        {
        }


        [Test]
        public void CheckHandlingSpaces()
        {
            string scriptCode = "int a = 5;";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Variables.Count, Is.EqualTo(1));
        }

        [Test]
        public void CheckHandlingTabs()
        {
            string scriptCode = "\tint\tb\t=\t10\t;";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Variables.Count, Is.EqualTo(1));
        }

        [Test]
        public void CheckHandlingWhitespace()
        {
            string scriptCode = "\tint\t\tvarName\t=\t30\t;\n";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Variables.Count, Is.EqualTo(1));
        }

        [Test]
        public void CheckHandlingComments()
        {
            string scriptCode = "// int is_a_comment = 1\nint a = 5;";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Variables.Count, Is.EqualTo(1));
            Assert.That(scriptToTest.AutoCompleteData.FindVariable("a"), Is.Not.Null);
        }

        [Test]
        public void CheckHandlingMixedWhitespaceAndComments()
        {
            string scriptCode = "// int is_a_comment = 1\nint\t/* int comment = 1 */ b =\t 15\t;";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Variables.Count, Is.EqualTo(1));
            Assert.That(scriptToTest.AutoCompleteData.FindVariable("b"), Is.Not.Null);
        }

        [Test]
        public void CheckHandlingEmptyLines()
        {
            string scriptCode = "int a = 5;\r\n\r\n\r\nstring b = \"text\";\r\n";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Variables.Count, Is.EqualTo(2));
            Assert.That(scriptToTest.AutoCompleteData.FindVariable("a"), Is.Not.Null);
            Assert.That(scriptToTest.AutoCompleteData.FindVariable("b"), Is.Not.Null);
        }

        [Test]
        public void ContainsVariable()
        {
            string scriptCode = $@"
int a;
";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Variables.Count, Is.EqualTo(1));
        }

        [Test]
        public void ContainsArray()
        {
            string scriptCode = $@"
int arrOfInts[] = new int[100];
";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Variables.Count, Is.EqualTo(1));

            ScriptVariable variable = scriptToTest.AutoCompleteData.Variables.FirstOrDefault(v => v.VariableName == "arrOfInts");
            Assert.That(variable, Is.Not.Null);
            Assert.That(variable.Type, Is.EqualTo("int"));
            Assert.That(variable.IsArray, Is.True);
            Assert.That(variable.IsPointer, Is.False);
        }

        [Test]
        public void ContainsImportedFunction()
        {
            string scriptCode = $@"
import int fooFactory();
";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);
            ScriptFunction method = scriptToTest.AutoCompleteData.FindFunction("fooFactory");

            Assert.That(method, Is.Not.Null);
            Assert.That(method.Type, Is.EqualTo("int"));
        }

        [Test]
        public void ContainsFunction()
        {
            string scriptCode = $@"
void MyMethod() {{
}}
";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Functions.Count, Is.EqualTo(1));
            ScriptFunction method = scriptToTest.AutoCompleteData.Functions.FirstOrDefault(m => m.FunctionName == "MyMethod");
        }

        [Test]
        public void ContainsStaticFunction()
        {
            string scriptCode = $@"
static void BarFactory() {{
}}
";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);
            ScriptFunction method = scriptToTest.AutoCompleteData.Functions.FirstOrDefault(m => m.FunctionName == "BarFactory");

            Assert.That(method, Is.Not.Null);
            Assert.That(scriptToTest.AutoCompleteData.Functions.Any(m => m.IsStatic), Is.True);
        }

        [Test]
        public void ContainsStaticFunctionWithParameters()
        {
            string scriptCode = $@"
static void MyStaticMethod(int param1, string param2) {{
}}
";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);
            ScriptFunction staticMethod = scriptToTest.AutoCompleteData.Functions.FirstOrDefault(m => m.FunctionName == "MyStaticMethod");

            Assert.That(staticMethod, Is.Not.Null);
            Assert.That(staticMethod.ParamList, Is.EqualTo("int param1, string param2"));
        }

        [Test]
        public void ContainsEnums()
        {
            string scriptCode = $@"
enum MyEnum {{
    Value1,
    Value2
}};
";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Enums.Count, Is.EqualTo(1));
            ScriptEnum myEnum = scriptToTest.AutoCompleteData.Enums.FirstOrDefault(e => e.Name == "MyEnum");
            Assert.That(myEnum.EnumValues[0].Name, Is.EqualTo("Value1"));
            Assert.That(myEnum.EnumValues[1].Name, Is.EqualTo("Value2"));
        }

        [Test]
        public void ContainsStructs()
        {
            string scriptCode = $@"
struct MyStruct {{
    int x;
    int y;
}}
";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Structs.Count, Is.EqualTo(1));
            ScriptStruct myStruct = scriptToTest.AutoCompleteData.Structs.FirstOrDefault(s => s.Name == "MyStruct");
            Assert.That(myStruct.Variables.Count, Is.EqualTo(2));
        }

        [Test]
        public void ContainsInheritedStruct()
        {
            string scriptCode = $@"
struct Base {{
    int x;
}};
struct Child extends Base {{
    int y;
}};
";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Structs.Count, Is.EqualTo(2));
            ScriptStruct baseStruct = scriptToTest.AutoCompleteData.FindStruct("Base");
            ScriptStruct childStruct = scriptToTest.AutoCompleteData.FindStruct("Child");

            Assert.That(baseStruct, Is.Not.Null);
            Assert.That(childStruct, Is.Not.Null);
            Assert.That(childStruct.Functions.Count, Is.EqualTo(0));
            Assert.That(childStruct.Variables.Count, Is.EqualTo(2));
            Assert.That(childStruct.Variables[0].VariableName, Is.EqualTo("x"));
            Assert.That(childStruct.Variables[1].VariableName, Is.EqualTo("y"));
        }

        [Test]
        public void CheckAutoCompleteMinimalScript()
        {
            string scriptCode = $@"
import void AbortGame();

internalstring autoptr builtin managed struct String {{
  /// Returns a new string with the specified string appended to this string.
  import String  Append(const string appendText);
}};

String mytext = ""Hello"";
mytext = mytext.Append(""World"");
if (mytext != ""HelloWorld"")
  AbortGame();
";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Functions.Count, Is.EqualTo(1));
            ScriptFunction method = scriptToTest.AutoCompleteData.FindFunction("AbortGame");
            Assert.That(method, Is.Not.Null);

            ScriptVariable variable = scriptToTest.AutoCompleteData.FindVariable("mytext");
            Assert.That(variable, Is.Not.Null);
            Assert.That(variable.Type, Is.EqualTo("String"));
        }

        [Test]
        public void CheckAutoCompleteComplexScript()
        {
            string scriptCode = $@"
#define MAX_THINGS 5
int a;
int arrOfInts[] = new int[100];

import void MyImportedFunction();
void MyLocalFunction() {{
}}
static void MyStaticFunction(int param1, string param2) {{
}}
enum MyEnum {{
    Value1,
    Value2
}};
struct MyStruct {{
    int x;
    int y;
}};

function room_AfterFadeIn()
{{
    int localVar = 42;
    int result = MyImportedFunction();
    string text = ""AGS Script"";
    MyLocalFunction();
    MyStaticFunction(10, ""Test"");
    MyEnum enumVal = MyEnum.Value1;
    MyStruct myObj;
    myObj.x = 5;
}}

import void AbortGame();

internalstring autoptr builtin managed struct String {{
  /// Returns a new string with the specified string appended to this string.
  import String  Append(const string appendText);
}};

String mytext = ""Hello"";
mytext = mytext.Append(""World"");
";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Variables.Count, Is.EqualTo(3));
            Assert.That(scriptToTest.AutoCompleteData.Functions.Count, Is.EqualTo(5));
            Assert.That(scriptToTest.AutoCompleteData.Enums.Count, Is.EqualTo(1));
            Assert.That(scriptToTest.AutoCompleteData.Structs.Count, Is.EqualTo(2));
            Assert.That(scriptToTest.AutoCompleteData.Defines.Count, Is.EqualTo(1));

            Assert.That(scriptToTest.AutoCompleteData.FindVariable("a"), Is.Not.Null);
            Assert.That(scriptToTest.AutoCompleteData.FindVariable("arrOfInts"), Is.Not.Null);
            Assert.That(scriptToTest.AutoCompleteData.FindVariable("mytext"), Is.Not.Null);

            Assert.That(scriptToTest.AutoCompleteData.FindFunction("MyImportedFunction"), Is.Not.Null);
            Assert.That(scriptToTest.AutoCompleteData.FindFunction("MyLocalFunction"), Is.Not.Null);
            Assert.That(scriptToTest.AutoCompleteData.FindFunction("MyStaticFunction"), Is.Not.Null);
            Assert.That(scriptToTest.AutoCompleteData.FindFunction("room_AfterFadeIn"), Is.Not.Null);
            Assert.That(scriptToTest.AutoCompleteData.FindFunction("AbortGame"), Is.Not.Null);

            Assert.That(scriptToTest.AutoCompleteData.FindEnum("MyEnum"), Is.Not.Null);

            Assert.That(scriptToTest.AutoCompleteData.FindStruct("MyStruct"), Is.Not.Null);
            Assert.That(scriptToTest.AutoCompleteData.FindStruct("String"), Is.Not.Null);

            Assert.That(scriptToTest.AutoCompleteData.FindDefine("MAX_THINGS"), Is.Not.Null);
        }


        [Test]
        public void CheckAutoCompleteMemberFunctions()
        {
            string scriptCode = $@"
import void AbortGame();

internalstring autoptr builtin managed struct String {{
  /// Creates a formatted string using the supplied parameters.
  import static String Format(const string format, ...);    // $AUTOCOMPLETESTATICONLY$
  /// Checks whether the supplied string is null or empty.
  import static bool IsNullOrEmpty(String stringToCheck);  // $AUTOCOMPLETESTATICONLY$
  /// Returns a new string with the specified string appended to this string.
  import String  Append(const string appendText);
  /// Returns a new string that has the extra character appended.
  import String  AppendChar(int extraChar);
  import int     Contains(const string needle);   // $AUTOCOMPLETEIGNORE$
  /// Creates a copy of the string.
  import String  Copy();
  /// Returns the index of the first occurrence of the needle in this string.
  import int     IndexOf(const string needle);
  /// Returns a lower-cased version of this string.
  import String  LowerCase();
  /// Returns a portion of the string.
  import String  Substring(int index, int length);
  /// Returns an upper-cased version of this string.
  import String  UpperCase();
  /// Converts the string to a float.
  readonly import attribute float AsFloat;
  /// Converts the string to an integer.
  readonly import attribute int AsInt;
  /// Accesses individual characters of the string.
  readonly import attribute int Chars[];
  /// Returns the length of the string.
  readonly import attribute int Length;
}};

function room_AfterFadeIn()
{{
  // String.Append
  {{ 
    String mytext = ""Hello"";
    mytext = mytext.Append(""World"");
            if (mytext != ""HelloWorld"") AbortGame();
  }}
  
  // String.AppendChar
  {{ 
    String mytext = ""Hell"";
    mytext = mytext.AppendChar('o');
    if(mytext != ""Hello"") AbortGame();
  }}
}}
";
            Script scriptToTest = CachedAutoCompletedScriptFromCode(scriptCode);

            Assert.That(scriptToTest.AutoCompleteData.Variables.Count, Is.EqualTo(0));
            Assert.That(scriptToTest.AutoCompleteData.Functions.Count, Is.EqualTo(2));
            Assert.That(scriptToTest.AutoCompleteData.Enums.Count, Is.EqualTo(0));
            Assert.That(scriptToTest.AutoCompleteData.Structs.Count, Is.EqualTo(1));
            Assert.That(scriptToTest.AutoCompleteData.Defines.Count, Is.EqualTo(0));

            ScriptFunction scriptFunction = null;
            ScriptStruct scriptStruct = scriptToTest.AutoCompleteData.FindStruct("String");
            Assert.That(scriptStruct, Is.Not.Null);

            scriptFunction = scriptStruct.FindMemberFunction("Format");
            Assert.That(scriptFunction, Is.Not.Null);
            Assert.That(scriptFunction.IsStatic, Is.True);
            Assert.That(scriptFunction.IsStaticOnly, Is.True);
            Assert.That(scriptFunction.IsProtected, Is.False);
            Assert.That(scriptFunction.ParamList, Is.EqualTo("const string format, ..."));
            Assert.That(scriptFunction.Type, Is.EqualTo("String"));
            Assert.That(scriptFunction.Description, Is.EqualTo("Creates a formatted string using the supplied parameters."));

            scriptFunction = scriptStruct.FindMemberFunction("IsNullOrEmpty");
            Assert.That(scriptFunction, Is.Not.Null);
            Assert.That(scriptFunction.IsStatic, Is.True);
            Assert.That(scriptFunction.IsStaticOnly, Is.True);
            Assert.That(scriptFunction.IsProtected, Is.False);
            Assert.That(scriptFunction.ParamList, Is.EqualTo("String stringToCheck"));
            Assert.That(scriptFunction.Type, Is.EqualTo("bool"));
            Assert.That(scriptFunction.Description, Is.EqualTo("Checks whether the supplied string is null or empty."));
            
            scriptFunction = scriptStruct.FindMemberFunction("Append");
            Assert.That(scriptFunction, Is.Not.Null);
            Assert.That(scriptFunction.IsStatic, Is.False);
            Assert.That(scriptFunction.IsStaticOnly, Is.False);
            Assert.That(scriptFunction.IsProtected, Is.False);
            Assert.That(scriptFunction.ParamList, Is.EqualTo("const string appendText"));
            Assert.That(scriptFunction.Type, Is.EqualTo("String"));
            Assert.That(scriptFunction.Description, Is.EqualTo("Returns a new string with the specified string appended to this string."));
        }
    }
}