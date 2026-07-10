using System;
using System.Linq;
using System.Text.RegularExpressions;
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
        public void FillLinesSection_Test()
        {
            string script = UniformTestString(TestScript);

            ScriptParsing.ParserState state = new ScriptParsing.ParserState(script);
            ScriptParsing.FillLines(state);

            Assert.AreEqual(33, state.Lines.Count);
            Assert.AreEqual(0, state.Lines[0]);
            Assert.AreEqual(1, state.Lines[1]);
            Assert.AreEqual(21, state.Lines[2]);
            Assert.AreEqual(32, state.Lines[3]);
            Assert.AreEqual(46, state.Lines[4]);
            Assert.AreEqual(67, state.Lines[5]);
            Assert.AreEqual(69, state.Lines[6]);
            Assert.AreEqual(71, state.Lines[7]);
            Assert.AreEqual(74, state.Lines[8]);
            Assert.AreEqual(87, state.Lines[9]);
            Assert.AreEqual(89, state.Lines[10]);
            Assert.AreEqual(118, state.Lines[11]);
            Assert.AreEqual(135, state.Lines[12]);
            Assert.AreEqual(150, state.Lines[13]);
            Assert.AreEqual(168, state.Lines[14]);
            Assert.AreEqual(183, state.Lines[15]);
            Assert.AreEqual(207, state.Lines[16]);
            Assert.AreEqual(255, state.Lines[17]);
            Assert.AreEqual(292, state.Lines[18]);
            Assert.AreEqual(314, state.Lines[19]);
            Assert.AreEqual(343, state.Lines[20]);
            Assert.AreEqual(345, state.Lines[21]);
            Assert.AreEqual(347, state.Lines[22]);
            Assert.AreEqual(350, state.Lines[23]);
            Assert.AreEqual(381, state.Lines[24]);
            Assert.AreEqual(382, state.Lines[25]);
            Assert.AreEqual(440, state.Lines[26]);
            Assert.AreEqual(484, state.Lines[27]);
            Assert.AreEqual(496, state.Lines[28]);
            Assert.AreEqual(506, state.Lines[29]);
            Assert.AreEqual(519, state.Lines[30]);
            Assert.AreEqual(538, state.Lines[31]);
            Assert.AreEqual(560, state.Lines[32]);
        }

        [Test]
        public void FillCommentSections_Test()
        {
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
        }
    }
}
