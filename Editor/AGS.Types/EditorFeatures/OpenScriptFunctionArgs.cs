using System;

namespace AGS.Types
{
    public class CreateScriptFunctionArgs
    {
        public string ScriptName { get; private set; }
        public string FunctionName { get; private set; }
        public string FunctionParameters { get; private set; }

        public CreateScriptFunctionArgs(string scriptName, string functionName, string parameters)
        {
            ScriptName = scriptName;
            FunctionName = functionName;
            FunctionParameters = parameters;
        }
    }

    public class OpenScriptFunctionArgs : CreateScriptFunctionArgs
    {
        public bool CreateIfNotExists { get; private set; }

        public OpenScriptFunctionArgs(string scriptName, string functionName, string parameters, bool createIfNotExists)
            : base(scriptName, functionName, parameters)
        {
            CreateIfNotExists = createIfNotExists;
        }
    }
}
