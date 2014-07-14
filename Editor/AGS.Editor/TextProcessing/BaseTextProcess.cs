using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public abstract class BaseTextProcess
    {
        protected Dictionary<string, FunctionCallType> GetFunctionCallsToProcessForSpeech(bool includeNarrator)
        {
            Dictionary<string, FunctionCallType> speechableFunctionCalls = new Dictionary<string, FunctionCallType>();
            speechableFunctionCalls.Add("DisplaySpeech", FunctionCallType.GlobalSpeech);
            speechableFunctionCalls.Add("DisplayThought", FunctionCallType.GlobalSpeech);
            speechableFunctionCalls.Add(".Say", FunctionCallType.ObjectBasedSpeech);
            speechableFunctionCalls.Add(".Think", FunctionCallType.ObjectBasedSpeech);
            if (includeNarrator)
            {
                speechableFunctionCalls.Add("Display ", FunctionCallType.GlobalNarrator);
                speechableFunctionCalls.Add("Display(", FunctionCallType.GlobalNarrator);
                speechableFunctionCalls.Add("DisplayAt", FunctionCallType.GlobalNarrator);
            }
            return speechableFunctionCalls;
        }
    }
}
