using System;
using System.Runtime.InteropServices;
namespace Scintilla
{
    [StructLayout(LayoutKind.Sequential)]
    public struct CharacterRange
    {
        public int cpMin;
        public int cpMax;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct TextRange
    {
        public CharacterRange chrg;
        public IntPtr lpstrText;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct TextToFind
    {
        public CharacterRange chrg;			// range to search
        public IntPtr lpstrText;			// the search pattern (zero terminated)
        public CharacterRange chrgText;		// returned as position of matching text
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct RangeToFormat
    {
        public IntPtr hdc;			// The HDC (device context) we print to
        public IntPtr hdcTarget;	// The HDC we use for measuring (may be same as hdc)
        public IntPtr rc;			// Rectangle in which to print
        public IntPtr rcPage;		// Physically printable page size
        CharacterRange chrg;		// Range of characters to print
    }

    /// <summary>
    /// This matches the Win32 NMHDR structure
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct NotifyHeader
    {
        public IntPtr hwndFrom;	// environment specific window handle/pointer
        public IntPtr idFrom;	// CtrlID of the window issuing the notification
        // public uint code;		// The SCN_* notification code
        public Enums.Events code;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct SCNotification
    {
        public NotifyHeader nmhdr;
        public int position;			// SCN_STYLENEEDED, SCN_MODIFIED, SCN_DWELLSTART, SCN_DWELLEND, 
        // SCN_CALLTIPCLICK, SCN_HOTSPOTCLICK, SCN_HOTSPOTDOUBLECLICK
        public char ch;					// SCN_CHARADDED, SCN_KEY
        public int modifiers;			// SCN_KEY
        public int modificationType;	// SCN_MODIFIED
        public IntPtr text;				// SCN_MODIFIED
        public int length;				// SCN_MODIFIED
        public int linesAdded;			// SCN_MODIFIED
        public int message;				// SCN_MACRORECORD
        public IntPtr wParam;			// SCN_MACRORECORD
        public IntPtr lParam;			// SCN_MACRORECORD
        public int line;				// SCN_MODIFIED
        public int foldLevelNow;		// SCN_MODIFIED
        public int foldLevelPrev;		// SCN_MODIFIED
        public int margin;				// SCN_MARGINCLICK
        public int listType;			// SCN_USERLISTSELECTION
        public int x;					// SCN_DWELLSTART, SCN_DWELLEND
        public int y;					// SCN_DWELLSTART, SCN_DWELLEND
    }

    /// <summary>
    /// Expresses a position within the Editor by Line # of Column Offset.
    /// Use ScintillaControl.LineColumnFromPosition and 
    /// ScintillaControl.PositionFromLineColumn to converto to/from a Position
    /// (absolute position within the editor)
    /// </summary>
    public struct LineColumn
    {
        public int Line;
        public int Column;

        public LineColumn(int line, int column)
        {
            Line = line;
            Column = column;
        }
    }

    /// <summary>
    /// A range within the editor. Start and End are both Positions.
    /// </summary>
    public struct Range
    {
        public int Start;
        public int End;

        public Range(int start, int end)
        {
            Start = start;
            End = end;
        }
    }
}
