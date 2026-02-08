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

        [Test]
        public void FillCommentSections_Test()
        {
            ScriptParsing.ParserState state = new ScriptParsing.ParserState(TestScript);
            ScriptParsing.FillCommentSections(state);

            var commentsAsDict = state.Comments.ToDictionary(c => c.Item1, c => new Tuple<int, int>(c.Item1, c.Item2));

            Assert.AreEqual(9, commentsAsDict.Count);
            Assert.That(commentsAsDict.ContainsKey(23));
            Assert.AreEqual(23, commentsAsDict[23].Item1);
            Assert.AreEqual(33, commentsAsDict[23].Item2);
            Assert.That(commentsAsDict.ContainsKey(35));
            Assert.AreEqual(35, commentsAsDict[35].Item1);
            Assert.AreEqual(48, commentsAsDict[35].Item2);
            Assert.That(commentsAsDict.ContainsKey(50));
            Assert.AreEqual(50, commentsAsDict[50].Item1);
            Assert.AreEqual(80, commentsAsDict[50].Item2);
            Assert.That(commentsAsDict.ContainsKey(151));
            Assert.AreEqual(151, commentsAsDict[151].Item1);
            Assert.AreEqual(161, commentsAsDict[151].Item2);
            Assert.That(commentsAsDict.ContainsKey(167));
            Assert.AreEqual(167, commentsAsDict[167].Item1);
            Assert.AreEqual(180, commentsAsDict[167].Item2);
            Assert.That(commentsAsDict.ContainsKey(186));
            Assert.AreEqual(186, commentsAsDict[186].Item1);
            Assert.AreEqual(208, commentsAsDict[186].Item2);
            Assert.That(commentsAsDict.ContainsKey(295));
            Assert.AreEqual(295, commentsAsDict[295].Item1);
            Assert.AreEqual(308, commentsAsDict[295].Item2);
            Assert.That(commentsAsDict.ContainsKey(321));
            Assert.AreEqual(321, commentsAsDict[321].Item1);
            Assert.AreEqual(331, commentsAsDict[321].Item2);
            Assert.That(commentsAsDict.ContainsKey(341));
            Assert.AreEqual(341, commentsAsDict[341].Item1);
            Assert.AreEqual(371, commentsAsDict[341].Item2);
        }

        [Test]
        public void GetCurrentFunctionCall_Test()
        {
            ScriptParsing.ParserState state = new ScriptParsing.ParserState(TestScript);
            ScriptParsing.FillCommentSections(state);

            Assert.AreEqual(string.Empty, ScriptParsing.GetCurrentFunctionCall(state, 115));
            Assert.AreEqual(string.Empty, ScriptParsing.GetCurrentFunctionCall(state, 209));
            Assert.AreEqual("FuncWithStringArg(", ScriptParsing.GetCurrentFunctionCall(state, 245));
            Assert.AreEqual("FuncWithStringArg(0, ", ScriptParsing.GetCurrentFunctionCall(state, 248));
            Assert.AreEqual(@"FuncWithStringArg( /* comment */
        1, // comment
        /* multiline comment
*
*
*/
        ",
                ScriptParsing.GetCurrentFunctionCall(state, 381));
            Assert.AreEqual("InnerCall(  3, 4, ", ScriptParsing.GetCurrentFunctionCall(state, 451));
            Assert.AreEqual("Outer_Call     (1, 2, InnerCall(  3, 4, ", ScriptParsing.GetCurrentFunctionCall(state, 451, true));
            Assert.AreEqual("Object_ . Call (Constant, ", ScriptParsing.GetCurrentFunctionCall(state, 496));
            Assert.AreEqual(@"Object_
        .
        Call
        (Constant,
        ", ScriptParsing.GetCurrentFunctionCall(state, 577));
        }
    }
}
