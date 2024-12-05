using System;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// CompileWarningWithFunction - stores a function name in addition to regular parameters.
    /// </summary>
    public class CompileWarningWithFunction : CompileWarning
    {
        private string _funcName = string.Empty;

        public CompileWarningWithFunction(string message, string scriptName, string funcName, int lineNumber)
			: base(message, scriptName, lineNumber)
		{
            _funcName = funcName;
        }

        public string FunctionName
        {
            get { return _funcName; }
        }
    }

    /// <summary>
    /// CompileWarningWithGameObject - adds game object's type and name to the warning message.
    /// </summary>
    public class CompileWarningWithGameObject : CompileWarning
    {
        private string _typeName = string.Empty;
        private string _objectName = string.Empty;
        private bool _isObjectEvent = false;

        public CompileWarningWithGameObject(string message, string typeName, string objectName, bool isObjectEvent = false)
            : base(message)
        {
            _typeName = typeName;
            _objectName = objectName;
            _isObjectEvent = isObjectEvent;
        }

        public string TypeName
        {
            get { return _typeName; }
        }

        public string ObjectName
        {
            get { return _objectName; }
        }

        /// <summary>
        /// Tells whether this warning is related to object's events table.
        /// </summary>
        public bool IsObjectEvent
        {
            get { return _isObjectEvent; }
        }
    }
}
