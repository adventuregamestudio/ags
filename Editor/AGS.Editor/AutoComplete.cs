using AGS.CScript.Compiler;
using AGS.Types;
using AGS.Types.AutoComplete;
using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;

namespace AGS.Editor
{
    public class AutoComplete
    {
        private const string AUTO_COMPLETE_IGNORE = "$AUTOCOMPLETEIGNORE$";
        private const string AUTO_COMPLETE_STATIC_ONLY = "$AUTOCOMPLETESTATICONLY$";
        private const string AUTO_COMPLETE_NO_INHERIT = "$AUTOCOMPLETENOINHERIT$";
        private static Script _scriptToUpdateInBackground = null;
        private static object _scriptLockObject = new object();

        public delegate void BackgroundCacheUpdateStatusChangedHandler(BackgroundAutoCompleteStatus status, Exception errorDetails);
        public static event BackgroundCacheUpdateStatusChangedHandler BackgroundCacheUpdateStatusChanged;

        static AutoComplete()
        {
            Thread thread = new Thread(new ThreadStart(UpdateAutocompleteCacheThread));
            thread.IsBackground = true;
            thread.Name = "AutoCompleteUpdateThread";
            thread.Priority = ThreadPriority.Lowest;
            thread.Start();
        }

        public static void RequestBackgroundCacheUpdate(Script scriptToUpdate)
        {
            lock (_scriptLockObject)
            {
                _scriptToUpdateInBackground = scriptToUpdate;
            }
        }

        private static void OnBackgroundCacheUpdateStatusChanged(BackgroundAutoCompleteStatus status, Exception errorDetails)
        {
            if (BackgroundCacheUpdateStatusChanged != null)
            {
                BackgroundCacheUpdateStatusChanged(status, errorDetails);
            }
        }

        private static void UpdateAutocompleteCacheThread()
        {
            while (true)
            {
                Thread.Sleep(50);
                if (_scriptToUpdateInBackground != null)
                {
                    Script scriptToUpdate;
                    lock (_scriptLockObject)
                    {
                        scriptToUpdate = _scriptToUpdateInBackground;
                        _scriptToUpdateInBackground = null;
                    }
                    try
                    {
                        OnBackgroundCacheUpdateStatusChanged(BackgroundAutoCompleteStatus.Processing, null);

                        ConstructCache(scriptToUpdate, true);

                        OnBackgroundCacheUpdateStatusChanged(BackgroundAutoCompleteStatus.Finished, null);
                    }
                    catch (Exception ex)
                    {
                        OnBackgroundCacheUpdateStatusChanged(BackgroundAutoCompleteStatus.Error, ex);
                    }
                }
            }
        }

        public static void ConstructCache(Script scriptToCache)
        {
            ConstructCache(scriptToCache, false);
        }

        private static bool IncrementIndexToSkipAnyComments(FastString script, ref int index)
        {
            if (index < script.Length - 1)
            {
                if ((script[index] == '/') && (script[index + 1] == '/'))
                {
                    while ((script[index] != '\r') && (index < script.Length - 1))
                    {
                        index++;
                    }
                }
                if ((script[index] == '/') && (script[index + 1] == '*'))
                {
                    index = script.IndexOf("*/", index + 2);
                    if (index < 0)
                    {
                        index = script.Length - 1;
                        return true;
                    }
                }
            }
            return false;
        }

        private static void SkipUntilMatchingClosingBrace(ref FastString script)
        {
            int braceIndent = 1, index = 1;
            while ((braceIndent > 0) && (index < script.Length))
            {
                if (IncrementIndexToSkipAnyComments(script, ref index))
                {
                    break;
                }

                if ((script[index] == '"') || (script[index] == '\''))
                {
                    char firstChar = script[index];
                    index++;
                    while (index < script.Length - 1)
                    {
                        if ((script[index] == firstChar) &&
                            ((script[index - 1] != '\\') || (script[index - 2] == '\\')))
                        {
                            break;
                        }
                        index++;
                    }
                }
                if (script[index] == '{')
                {
                    braceIndent++;
                }
                else if (script[index] == '}')
                {
                    braceIndent--;
                }
                index++;
            }
            script = script.Substring(index);
        }

        private static void ConstructCache(Script scriptToCache, bool isBackgroundThread)
        {
            string originalText = scriptToCache.Text;
            ScriptAutoCompleteData newCache = new ScriptAutoCompleteData();
            List<ScriptVariable> variables = newCache.Variables;
            List<ScriptFunction> functions = newCache.Functions;
            List<ScriptDefine> defines = newCache.Defines;
            List<ScriptEnum> enums = newCache.Enums;
            List<ScriptStruct> structs = newCache.Structs;
            variables.Clear();
            functions.Clear();
            defines.Clear();
            enums.Clear();
            structs.Clear();
            FastString script = originalText;
            AutoCompleteParserState state = new AutoCompleteParserState();
            ScriptFunction lastFunction = null;
            int counter = 0;

            while (script.Length > 0)
            {
                if (isBackgroundThread)
                {
                    counter++;
                    if (counter % 20 == 0)
                    {
                        Thread.Sleep(2);
                    }
                }
                SkipWhitespace(ref script);
                state.CurrentScriptCharacterIndex = originalText.Length - script.Length;

                if (script.Length == 0)
                {
                    break;
                }
                if (script.StartsWith("//"))
                {
                    FastString scriptAtComment = script;
                    GoToNextLine(ref script);

                    if (scriptAtComment.StartsWith("///"))
                    {
                        FastString commentText = scriptAtComment.Substring(3, (scriptAtComment.Length - script.Length) - 4);
                        state.PreviousComment = commentText.ToString().Trim();
                    }
                    continue;
                }
                if (script.StartsWith("/*"))
                {
                    int endOfComment = script.IndexOf("*/");
                    if (endOfComment < 0)
                    {
                        break;
                    }
                    script = script.Substring(endOfComment + 2);
                    continue;
                }
                if (script.StartsWith("#"))
                {
                    ProcessPreProcessorDirective(defines, ref script, state);
                    continue;
                }
                if (script.StartsWith("{"))
                {
                    if (state.WordBeforeLast == "enum")
                    {
                        state.InsideEnumDefinition = new ScriptEnum(state.LastWord, state.InsideIfDefBlock, state.InsideIfNDefBlock);
                    }
                    else if (state.WordBeforeLast == "extends")
                    {
                        // inherited struct
                        foreach (ScriptStruct baseStruct in structs)
                        {
                            if (baseStruct.Name == state.LastWord)
                            {
                                state.InsideStructDefinition = CreateInheritedStruct(baseStruct, state);
                                functions = state.InsideStructDefinition.Functions;
                                variables = state.InsideStructDefinition.Variables;
                                break;
                            }
                        }
                    }
                    else if (state.WordBeforeLast == "struct")
                    {
                        state.InsideStructDefinition = new ScriptStruct(state.LastWord, state.InsideIfDefBlock, state.InsideIfNDefBlock, state.CurrentScriptCharacterIndex);
                        functions = state.InsideStructDefinition.Functions;
                        variables = state.InsideStructDefinition.Variables;
                    }
                    else
                    {
                        state.ClearPreviousWords();

                        SkipUntilMatchingClosingBrace(ref script);

                        if ((lastFunction != null) && (lastFunction.EndsAtCharacterIndex == 0))
                        {
                            lastFunction.EndsAtCharacterIndex = originalText.Length - script.Length;
                        }
                        continue;
                    }
                }

                string thisWord = GetNextWord(ref script);
                if (thisWord == "(")
                {
                    List<ScriptFunction> functionList = functions;
                    bool isStaticExtender = script.StartsWith("static ");
                    bool isExtenderMethod = isStaticExtender || script.StartsWith("this ");
                    if (isExtenderMethod)
                        AdjustFunctionListForExtenderFunction(structs, ref functionList, ref script);
                    if (AddFunctionDeclaration(functionList, ref script, thisWord, state, isExtenderMethod, isStaticExtender, isStaticExtender))
                    {
                        lastFunction = functionList[functionList.Count - 1];
                    }
                    state.ClearPreviousWords();
                }
                else if ((thisWord == "[") && (PeekNextWord(script) == "]"))
                {
                    GetNextWord(ref script);
                    state.DynamicArrayDefinition = true;
                    state.AddNextWord("[]");
                }
                else if ((thisWord == "=") || (thisWord == ";") ||
                         (thisWord == ",") || (thisWord == "["))
                {
                    if (state.InsideEnumDefinition != null)
                    {
                        AddEnumValue(state.InsideEnumDefinition, script, state.LastWord);

                        if (thisWord == "=")
                        {
                            // skip whatever the value of the enum is
                            GetNextWord(ref script);
                        }
                    }
                    else
                    {
                        AddVariableDeclaration(variables, ref script, thisWord, state);
                        if (thisWord == "=")
                        {
                            while ((thisWord != ";") && (thisWord != ",") && (script.Length > 0))
                            {
                                thisWord = GetNextWord(ref script);
                            }
                        }
                        if (thisWord == ",")
                        {
                            // eg. "int x,y"; ensure "y" gets recorded next time round
                            state.UndoLastWord();
                            continue;
                        }
                        if (thisWord == "[")
                        {
                            // eg. "int a[10], b[10], c[10];"
                            SkipWhitespace(ref script);
                            if (script.StartsWith(","))
                            {
                                GetNextWord(ref script);
                                state.UndoLastWord();
                                continue;
                            }
                        }
                    }
                    state.ClearPreviousWords();
                    state.DynamicArrayDefinition = false;
                }
                else if ((thisWord == "}") && (state.InsideEnumDefinition != null))
                {
                    // add the last value (unless it's an empty enum)
                    if (state.LastWord != "{")
                    {
                        AddEnumValue(state.InsideEnumDefinition, script, state.LastWord);
                    }
                    enums.Add(state.InsideEnumDefinition);
                    state.InsideEnumDefinition = null;
                    state.ClearPreviousWords();
                }
                else if ((thisWord == "}") && (state.InsideStructDefinition != null))
                {
                    structs.Add(state.InsideStructDefinition);
                    functions = newCache.Functions;
                    variables = newCache.Variables;
                    state.InsideStructDefinition = null;
                    state.ClearPreviousWords();
                }
                else
                {
                    state.AddNextWord(thisWord);
                }
            }
            scriptToCache.AutoCompleteData.CopyFrom(newCache);
            scriptToCache.AutoCompleteData.Populated = true;
        }

        private static void AdjustFunctionListForExtenderFunction(List<ScriptStruct> structs, ref List<ScriptFunction> functionList, ref FastString script)
        {
            GetNextWord(ref script);
            string structName = GetNextWord(ref script);
            while ((script.Length > 0) && (script[0] != ',') && (script[0] != ')'))
            {
                script = script.Substring(1);
            }
            if ((script.Length > 0) && script[0] == ',')
            {
                script = script.Substring(1);
            }
            script = script.Trim();

            foreach (ScriptStruct struc in structs)
            {
                if (struc.Name == structName)
                {
                    functionList = struc.Functions;
                    return;
                }
            }
            ScriptStruct newStruct = new ScriptStruct(structName);
            functionList = newStruct.Functions;
            structs.Add(newStruct);
            return;
        }

        private static ScriptStruct CreateInheritedStruct(ScriptStruct baseStruct, AutoCompleteParserState state)
        {
            ScriptStruct newStruct = new ScriptStruct(state.WordBeforeWordBeforeLast, state.InsideIfDefBlock, state.InsideIfNDefBlock, state.CurrentScriptCharacterIndex);
            foreach (ScriptFunction func in baseStruct.Functions)
            {
                if (!func.NoInherit)
                {
                    newStruct.Functions.Add(func);
                }
            }
            foreach (ScriptVariable var in baseStruct.Variables)
            {
                if (!var.NoInherit)
                {
                    newStruct.Variables.Add(var);
                }
            }
            return newStruct;
        }

        private static void ProcessPreProcessorDirective(List<ScriptDefine> defines, ref FastString script, AutoCompleteParserState state)
        {
            script = script.Substring(1);
            string preProcessorDirective = GetNextWord(ref script);
            if (preProcessorDirective == "define")
            {
                string macroName = GetNextWord(ref script);
                if (!string.IsNullOrEmpty(macroName) && (Char.IsLetter(macroName[0])) &&
                    (!DoesCurrentLineHaveToken(script, AUTO_COMPLETE_IGNORE)))
                {
                    defines.Add(new ScriptDefine(macroName, state.InsideIfDefBlock, state.InsideIfNDefBlock));
                }
            }
            else if (preProcessorDirective == "undef")
            {
                string macroName = GetNextWord(ref script);
                if (Char.IsLetter(macroName[0]))
                {
                    foreach (ScriptDefine define in defines)
                    {
                        if (define.Name == macroName)
                        {
                            defines.Remove(define);
                            break;
                        }
                    }
                }
            }
            else if (preProcessorDirective == "ifndef")
            {
                string macroName = GetNextWord(ref script);
                if (Char.IsLetter(macroName[0]))
                {
                    state.InsideIfNDefBlock = macroName;
                }
            }
            else if (preProcessorDirective == "ifdef")
            {
                string macroName = GetNextWord(ref script);
                if (Char.IsLetter(macroName[0]))
                {
                    state.InsideIfDefBlock = macroName;
                }
            }
            else if (preProcessorDirective == "endif")
            {
                state.InsideIfNDefBlock = null;
                state.InsideIfDefBlock = null;
            }
            GoToNextLine(ref script);
            state.ClearPreviousWords();
        }

        private static void AddEnumValue(ScriptEnum insideEnumDefinition, FastString script, string lastWord)
        {
            if ((lastWord.Length > 0) && (Char.IsLetter(lastWord[0])))
            {
                if (!DoesCurrentLineHaveToken(script, AUTO_COMPLETE_IGNORE))
                {
                    insideEnumDefinition.EnumValues.Add(lastWord);
                }
            }
        }

        private static bool DoesCurrentLineHaveToken(FastString script, string tokenToCheckFor)
        {
            int indexOfNextLine = script.IndexOf("\r\n");
            if (indexOfNextLine > 0)
            {
                if (script.Substring(0, indexOfNextLine).IndexOf(tokenToCheckFor) > 0)
                {
                    return true;
                }
            }
            return false;
        }

        private static bool AddFunctionDeclaration(List<ScriptFunction> functions, ref FastString script, string thisWord, AutoCompleteParserState state, bool isExtenderMethod, bool isStatic, bool isStaticOnly)
        {
            bool succeeded = false;

            if ((state.LastWord.Length > 0) && (state.WordBeforeLast.Length > 0))
            {
                if (!DoesCurrentLineHaveToken(script, AUTO_COMPLETE_IGNORE))
                {
                    string functionName = state.LastWord;
                    string type = state.WordBeforeLast;
                    bool isPointer = false, isNoInherit = false;
                    bool isProtected = false;
                    if (type == "::")
                    {
                        functionName = state.WordBeforeWordBeforeLast + "::" + functionName;
                        type = (state.PreviousWords.Length > 3) ? state.PreviousWords[3] : "unknown";
                    }
                    if (type == "*")
                    {
                        isPointer = true;
                        type = state.WordBeforeWordBeforeLast;
                    }
                    if (state.DynamicArrayDefinition)
                    {
                        // get the type name and the []
                        type = state.WordBeforeWordBeforeLast + state.WordBeforeLast;
                    }
                    if (state.IsWordInPreviousList("static"))
                    {
                        isStatic = true;
                    }
                    if (state.IsWordInPreviousList("protected"))
                    {
                        isProtected = true;
                    }
                    if (DoesCurrentLineHaveToken(script, AUTO_COMPLETE_STATIC_ONLY))
                    {
                        isStaticOnly = true;
                    }
                    if (DoesCurrentLineHaveToken(script, AUTO_COMPLETE_NO_INHERIT))
                    {
                        isNoInherit = true;
                    }

                    int parameterListEndIndex = script.IndexOf(')');
                    if (parameterListEndIndex >= 0)
                    {
                        string parameterList = script.Substring(0, parameterListEndIndex);
                        script = script.Substring(parameterListEndIndex + 1);
                        ScriptFunction newFunc = new ScriptFunction(functionName, type, parameterList, state.InsideIfDefBlock, state.InsideIfNDefBlock, isPointer, isStatic, isStaticOnly, isNoInherit, isProtected, isExtenderMethod, state.CurrentScriptCharacterIndex - 1);
                        if (!string.IsNullOrEmpty(state.PreviousComment))
                        {
                            newFunc.Description = state.PreviousComment;
                            state.PreviousComment = null;
                        }
                        functions.Add(newFunc);
                        succeeded = true;
                    }
                    state.DynamicArrayDefinition = false;
                }
            }

            return succeeded;
        }

        private static void AddVariableDeclaration(List<ScriptVariable> variables, ref FastString script, string thisWord, AutoCompleteParserState state)
        {
            if ((state.LastWord.Length > 0) && (state.WordBeforeLast.Length > 0))
            {
                if (!DoesCurrentLineHaveToken(script, AUTO_COMPLETE_IGNORE))
                {
                    bool isArray = false, isPointer = false;
                    bool isStatic = false, isStaticOnly = false;
                    bool isNoInherit = false, isProtected = false;
                    string type = state.WordBeforeLast;
                    string varName = state.LastWord;
                    if (thisWord == "[")
                    {
                        while ((script.Length > 0) && (GetNextWord(ref script) != "]")) ;
                        isArray = true;
                    }
                    else if (state.DynamicArrayDefinition)
                    {
                        varName = state.WordBeforeLast;
                        type = state.WordBeforeWordBeforeLast;
                        isArray = true;
                    }
                    if (type == "*")
                    {
                        isPointer = true;
                        if (state.DynamicArrayDefinition)
                        {
                            type = state.PreviousWords[3];
                        }
                        else
                        {
                            type = state.WordBeforeWordBeforeLast;
                        }
                    }
                    if (state.IsWordInPreviousList("static"))
                    {
                        isStatic = true;
                    }
                    if (state.IsWordInPreviousList("protected"))
                    {
                        isProtected = true;
                    }
                    if (DoesCurrentLineHaveToken(script, AUTO_COMPLETE_STATIC_ONLY))
                    {
                        isStaticOnly = true;
                    }
                    if (DoesCurrentLineHaveToken(script, AUTO_COMPLETE_NO_INHERIT))
                    {
                        isNoInherit = true;
                    }
                    // ignore "struct GUI;" prototypes
                    if (type != "struct")
                    {
                        //if (varName == "{") System.Diagnostics.Debugger.Break();
                        ScriptVariable newVar = new ScriptVariable(varName, type, isArray, isPointer, state.InsideIfDefBlock, state.InsideIfNDefBlock, isStatic, isStaticOnly, isNoInherit, isProtected, state.CurrentScriptCharacterIndex);

                        if (!string.IsNullOrEmpty(state.PreviousComment))
                        {
                            newVar.Description = state.PreviousComment;
                            state.PreviousComment = null;
                        }

                        variables.Add(newVar);
                    }
                }
                state.DynamicArrayDefinition = false;
            }
        }

        private static void GoToNextLine(ref FastString script)
        {
            int indexOfNextLine = script.IndexOf("\r\n");
            if (indexOfNextLine < 0)
            {
                script = string.Empty;
            }
            else
            {
                script = script.Substring(indexOfNextLine + 2);
            }
        }

        private static void SkipWhitespace(ref FastString script)
        {
            if (script.Length == 0)
            {
                return;
            }

            int index = 0;
            while ((script[index] == ' ') || (script[index] == '\t')
                || (script[index] == '\r') || (script[index] == '\n'))
            {
                index++;
                if (index >= script.Length)
                {
                    script = string.Empty;
                    return;
                }
            }
            if (index > 0)
            {
                script = script.Substring(index);
            }
        }

        private static string PeekNextWord(FastString script)
        {
            FastString tester = script;
            return GetNextWord(ref tester);
        }

        private static string GetNextWord(ref FastString script)
        {
            SkipWhitespace(ref script);
            if (script.Length == 0)
            {
                return string.Empty;
            }
            int index = 0;
            while (Char.IsLetterOrDigit(script[index]) || (script[index] == '_'))
            {
                index++;
                if (index >= script.Length)
                {
                    break;
                }
            }

            if (index == 0)
            {
                index++;
            }

            if ((script[0] == ':') && (script.Length > 1) && (script[1] == ':'))
            {
                // Make :: into one word
                index++;
            }

            string nextWord = script.Substring(0, index);
            script = script.Substring(index);
            return nextWord;
        }

        public static List<ScriptVariable> GetLocalVariableDeclarationsFromScriptExtract(string scriptToParse, int relativeCharacterIndex)
        {
            FastString script = scriptToParse;
            List<ScriptVariable> variables = new List<ScriptVariable>();
            string lastWord = string.Empty;
            while (script.Length > 0)
            {
                string nextWord = GetNextWord(ref script);
                if (nextWord.Length == 0)
                {
                    continue;
                }
                if ((Char.IsLetter(nextWord[0])) || (nextWord == "*") || (nextWord[0] == '_'))
                {
                    if ((lastWord.Length > 0) && (script.Length > 0))
                    {
                        bool isPointer = false;
                        if (nextWord == "*")
                        {
                            isPointer = true;
                            nextWord = GetNextWord(ref script);
                            if ((script.Length == 0) || (!Char.IsLetter(nextWord[0])))
                            {
                                lastWord = string.Empty;
                                continue;
                            }
                        }
                        string variableName = nextWord;
                        nextWord = GetNextWord(ref script);
                        bool isArray = false;
                        if (nextWord == "[")
                        {
                            isArray = true;
                            while ((script.Length > 0) && (GetNextWord(ref script) != "]")) ;
                            nextWord = GetNextWord(ref script);
                        }

                        if (((nextWord == "=") || (nextWord == ";") || (nextWord == ",")) &&
                            (lastWord != "return") && (lastWord != "else"))
                        {
                            variables.Add(new ScriptVariable(variableName, lastWord, isArray, isPointer, null, null, false, false, false, false, (scriptToParse.Length - script.Length) + relativeCharacterIndex));
                        }
                        if (nextWord != ",")
                        {
                            lastWord = string.Empty;
                        }
                    }
                    else
                    {
                        lastWord = nextWord;
                    }
                }
                else
                {
                    lastWord = string.Empty;
                }
            }
            return variables;
        }
    }
}
