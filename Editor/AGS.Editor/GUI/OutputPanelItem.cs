using System;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// OutputPanelItem is a ListViewItem descendant that wraps CompileMessage
    /// and handles user interaction with this message.
    /// </summary>
    public class OutputPanelItem : ListViewItem
    {
        private const string IMAGE_KEY_ERROR = "CompileErrorIcon";
        private const string IMAGE_KEY_WARNING = "CompileWarningIcon";
        private CompileMessage _message;

        /// <summary>
        /// Construct OutputPanelItem using plain text.
        /// </summary>
        public OutputPanelItem(string text, string imageKey)
            : base(text)
        {
            ImageKey = imageKey;
        }

        /// <summary>
        /// Construct OutputPanelItem using CompileMessage.
        /// </summary>
        public OutputPanelItem(CompileMessage message)
        {
            _message = message;
            Text = message.Message;

            if (message is CompileError)
            {
                ImageKey = IMAGE_KEY_ERROR;
            }
            else
            {
                ImageKey = IMAGE_KEY_WARNING;
            }

            if (_message.ScriptName.Length > 0)
            {
                SubItems.Add(_message.ScriptName);
            }

            if (_message.ScriptName.Length > 0 && _message.LineNumber > 0)
            {
                SubItems.Add(_message.LineNumber.ToString());
            }
        }

        public CompileMessage Message
        {
            get { return _message; }
        }

        public virtual void DefaultAction()
        {
            if (_message.LineNumber > 0)
            {
                Factory.GUIController.ZoomToFile(_message.ScriptName, _message.LineNumber);
            }
            // TODO: following is possibly a temporary hack, until we find a way
            // to initialize post-step warnings that check event functions with line numbers.
            else if (_message is CompileWarningWithFunction)
            {
                Factory.GUIController.ZoomToFile(_message.ScriptName, (_message as CompileWarningWithFunction).FunctionName);
            }
            else if (_message is CompileWarningWithGameObject)
            {
                var errorWithObject = _message as CompileWarningWithGameObject;
                Factory.GUIController.ZoomToComponentObject(errorWithObject.TypeName, errorWithObject.ObjectName, errorWithObject.IsObjectEvent);
            }
        }

        public override string ToString()
        {
            string thisLine = string.Empty;
            if (SubItems.Count > 1)
            {
                thisLine += SubItems[1].Text;  // filename

                if ((SubItems.Count > 2) &&
                    (SubItems[2].Text.Length > 0))
                {
                    thisLine += "(" + SubItems[2].Text + ")";  // line number
                }
            }
            if (thisLine.Length > 0)
            {
                thisLine += ": ";
            }
            thisLine += SubItems[0].Text;
            return thisLine;
        }
    }
}
