using System;
using System.Linq;
using NUnit.Framework;

namespace AGS.Editor
{
    public class ScriptParsingTests
    {
        private const string TestScript = @"
int globalvariable;
// comment
/* comment */
/* multiline comment
*
*
*/
function f()
{
    String s1 = ""some text"";
    String s2 = 
    // comment
    /* comment */
    /* comment
        */ ""some text"";
    FuncWithStringArg(0, ""literal string arg"");
    FuncWithStringArg( /* comment */
        1, // comment
        /* multiline comment
*
*
*/
        ""literal string arg"");

    Outer_Call     (1, 2, InnerCall(  3, 4, ""text arg""));
    Object_ . Call (Constant, ""text arg""));
    Object_
        .
        Call
        (Constant,
        ""text arg""));

    Outer_Call(1, 2, UnrelatedCall(3, 4, ""text arg""), InnerCall(5, 6, ""text arg""));
    Object.Call(1, 2, Object.UnrelatedCall(3, 4, ""text arg""), Object.InnerCall(5, 6, ""text arg""));
}
";

        /// <summary>
        /// Unify line endings in the input string, in case they depend on a enviroment.
        /// </summary>
        string UniformTestString(string s)
        {
            return s.Replace("\r\n", "\n");
        }

        [Test]
        public void FillCommentSections_Test()
        {
            // Unify line endings in the input string, in case they depend on a enviroment.
            string script = UniformTestString(TestScript);

            ScriptParsing.ParserState state = new ScriptParsing.ParserState(script);
            ScriptParsing.FillCommentSections(state);

            var commentsAsDict = state.Comments.ToDictionary(c => c.Item1, c => new Tuple<int, int>(c.Item1, c.Item2));

            Assert.AreEqual(9, commentsAsDict.Count);
            Assert.That(commentsAsDict.ContainsKey(21));
            Assert.AreEqual(21, commentsAsDict[21].Item1);
            Assert.AreEqual(31, commentsAsDict[21].Item2);
            Assert.That(commentsAsDict.ContainsKey(32));
            Assert.AreEqual(32, commentsAsDict[32].Item1);
            Assert.AreEqual(45, commentsAsDict[32].Item2);
            Assert.That(commentsAsDict.ContainsKey(46));
            Assert.AreEqual(46, commentsAsDict[46].Item1);
            Assert.AreEqual(73, commentsAsDict[46].Item2);
            Assert.That(commentsAsDict.ContainsKey(139));
            Assert.AreEqual(139, commentsAsDict[139].Item1);
            Assert.AreEqual(149, commentsAsDict[139].Item2);
            Assert.That(commentsAsDict.ContainsKey(154));
            Assert.AreEqual(154, commentsAsDict[154].Item1);
            Assert.AreEqual(167, commentsAsDict[154].Item2);
            Assert.That(commentsAsDict.ContainsKey(172));
            Assert.AreEqual(172, commentsAsDict[172].Item1);
            Assert.AreEqual(193, commentsAsDict[172].Item2);
            Assert.That(commentsAsDict.ContainsKey(278));
            Assert.AreEqual(278, commentsAsDict[278].Item1);
            Assert.AreEqual(291, commentsAsDict[278].Item2);
            Assert.That(commentsAsDict.ContainsKey(303));
            Assert.AreEqual(303, commentsAsDict[303].Item1);
            Assert.AreEqual(313, commentsAsDict[303].Item2);
            Assert.That(commentsAsDict.ContainsKey(322));
            Assert.AreEqual(322, commentsAsDict[322].Item1);
            Assert.AreEqual(349, commentsAsDict[322].Item2);
        }

        [Test]
        public void GetCurrentFunctionCall_Test()
        {
            string script = UniformTestString(TestScript);

            ScriptParsing.ParserState state = new ScriptParsing.ParserState(script);
            ScriptParsing.FillCommentSections(state);

            Assert.AreEqual(string.Empty, ScriptParsing.GetCurrentFunctionCall(state, 105));
            Assert.AreEqual(string.Empty, ScriptParsing.GetCurrentFunctionCall(state, 194));
            Assert.AreEqual("FuncWithStringArg(", ScriptParsing.GetCurrentFunctionCall(state, 229));
            Assert.AreEqual("FuncWithStringArg(0, ", ScriptParsing.GetCurrentFunctionCall(state, 232));
            Assert.AreEqual(UniformTestString(@"FuncWithStringArg( /* comment */
        1, // comment
        /* multiline comment
*
*
*/
        "),
                ScriptParsing.GetCurrentFunctionCall(state, 358));
            Assert.AreEqual("InnerCall(  3, 4, ", ScriptParsing.GetCurrentFunctionCall(state, 426));
            Assert.AreEqual("Outer_Call     (1, 2, InnerCall(  3, 4, ", ScriptParsing.GetCurrentFunctionCall(state, 426, true));
            Assert.AreEqual("Object_ . Call (Constant, ", ScriptParsing.GetCurrentFunctionCall(state, 470));
            Assert.AreEqual(UniformTestString(@"Object_
        .
        Call
        (Constant,
        "), ScriptParsing.GetCurrentFunctionCall(state, 546));
            Assert.AreEqual("Outer_Call(1, 2, UnrelatedCall(3, 4, \"text arg\"), InnerCall(5, 6, ", ScriptParsing.GetCurrentFunctionCall(state, 631, true));
            Assert.AreEqual("Object.Call(1, 2, Object.UnrelatedCall(3, 4, \"text arg\"), Object.InnerCall(5, 6, ", ScriptParsing.GetCurrentFunctionCall(state, 730, true));
        }

        [Test]
        public void GetCurrentFunctionName_Test()
        {
            string script = UniformTestString(TestScript);

            ScriptParsing.ParserState state = new ScriptParsing.ParserState(script);
            ScriptParsing.FillCommentSections(state);

            Assert.AreEqual(string.Empty, ScriptParsing.GetCurrentFunctionName(state, 105));
            Assert.AreEqual(string.Empty, ScriptParsing.GetCurrentFunctionName(state, 194));
            Assert.AreEqual("FuncWithStringArg", ScriptParsing.GetCurrentFunctionName(state, 229));
            Assert.AreEqual("FuncWithStringArg", ScriptParsing.GetCurrentFunctionName(state, 232));
            Assert.AreEqual("FuncWithStringArg", ScriptParsing.GetCurrentFunctionName(state, 358));
            Assert.AreEqual("InnerCall", ScriptParsing.GetCurrentFunctionName(state, 426));
            Assert.AreEqual("Outer_Call", ScriptParsing.GetCurrentFunctionName(state, 426, true));
            Assert.AreEqual("Object_.Call", ScriptParsing.GetCurrentFunctionName(state, 470));
            Assert.AreEqual("Object_.Call", ScriptParsing.GetCurrentFunctionName(state, 546));
            Assert.AreEqual("InnerCall", ScriptParsing.GetCurrentFunctionName(state, 631));
            Assert.AreEqual("Object.InnerCall", ScriptParsing.GetCurrentFunctionName(state, 730));
        }

        [Test]
        public void GetAllCurrentFunctionNames_Test()
        {
            string script = UniformTestString(TestScript);

            ScriptParsing.ParserState state = new ScriptParsing.ParserState(script);
            ScriptParsing.FillCommentSections(state);

            System.Collections.Generic.List<string> functionNames = new System.Collections.Generic.List<string>();
            ScriptParsing.GetAllCurrentFunctionNames(state, 105, functionNames);
            Assert.AreEqual(0, functionNames.Count);
            ScriptParsing.GetAllCurrentFunctionNames(state, 194, functionNames);
            Assert.AreEqual(0, functionNames.Count);
            ScriptParsing.GetAllCurrentFunctionNames(state, 229, functionNames);
            Assert.AreEqual(1, functionNames.Count);
            Assert.AreEqual("FuncWithStringArg", functionNames[0]);
            ScriptParsing.GetAllCurrentFunctionNames(state, 232, functionNames);
            Assert.AreEqual(1, functionNames.Count);
            Assert.AreEqual("FuncWithStringArg", functionNames[0]);
            Assert.AreEqual(1, functionNames.Count);
            ScriptParsing.GetAllCurrentFunctionNames(state, 358, functionNames);
            Assert.AreEqual("FuncWithStringArg", functionNames[0]);
            ScriptParsing.GetAllCurrentFunctionNames(state, 426, functionNames);
            Assert.AreEqual(2, functionNames.Count);
            Assert.AreEqual("InnerCall", functionNames[0]);
            Assert.AreEqual("Outer_Call", functionNames[1]);
            ScriptParsing.GetAllCurrentFunctionNames(state, 470, functionNames);
            Assert.AreEqual(1, functionNames.Count);
            Assert.AreEqual("Object_.Call", functionNames[0]);
            ScriptParsing.GetAllCurrentFunctionNames(state, 546, functionNames);
            Assert.AreEqual(1, functionNames.Count);
            Assert.AreEqual("Object_.Call", functionNames[0]);
            ScriptParsing.GetAllCurrentFunctionNames(state, 631, functionNames);
            Assert.AreEqual(2, functionNames.Count);
            Assert.AreEqual("InnerCall", functionNames[0]);
            Assert.AreEqual("Outer_Call", functionNames[1]);
            ScriptParsing.GetAllCurrentFunctionNames(state, 730, functionNames);
            Assert.AreEqual(2, functionNames.Count);
            Assert.AreEqual("Object.InnerCall", functionNames[0]);
            Assert.AreEqual("Object.Call", functionNames[1]);
        }
    }
}
