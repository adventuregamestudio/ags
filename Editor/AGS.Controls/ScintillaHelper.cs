using System;
using System.Text;
using System.Windows.Forms;
using ScintillaNET;

namespace AGS.Controls
{
    public static class ScintillaHelper
    {
        public const int SCI_GETLINEINDENTPOSITION = 2128;
        public const int SCI_SETMODEVENTMASK = 2359;
        public const int SCI_REGISTERIMAGE = 2405;

        public class MarginClickExEventArgs : MarginClickEventArgs
        {
            private readonly int _position;
            private readonly int _lineNumber;

            public int LineNumber { get { return _lineNumber; } }
            public new int Position { get { return _position; } }

            public MarginClickExEventArgs(Scintilla scintilla, MarginClickEventArgs baseArgs)
                : base(scintilla, baseArgs.Modifiers, 0, baseArgs.Margin)
            {
                _position = baseArgs.Position;
                _lineNumber = scintilla.LineFromPosition(_position);
            }
        }

        /// <summary>
        /// Gets the caret position at the end of indentation on the specified line
        /// </summary>
        public static int GetLineIndentationPosition(this Scintilla control, int line)
        {
            return control.Lines.ByteToCharPosition(
                (int)control.DirectMessage(SCI_GETLINEINDENTPOSITION, (IntPtr)line, (IntPtr)0));
        }

        public static unsafe int DirectMessage(this Scintilla control, int msg, int wParam, string lParam)
        {
            fixed (byte* bp = Encoding.UTF8.GetBytes(ZeroTerminated(lParam)))
                return (int)control.DirectMessage(msg, (IntPtr)wParam, (IntPtr)bp);
        }

        private static string ZeroTerminated(string param)
        {
            if (string.IsNullOrEmpty(param))
                return "\0";
            else if (!param.EndsWith("\0"))
                return param + "\0";
            return param;
        }
    }
}
