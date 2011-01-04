using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace Scintilla
{
    #region CharAddedEventArgs

    //public delegate void CharAddedEventHandler(object sender, CharAddedEventArgs e);
    public class CharAddedEventArgs : EventArgs
    {
        private char _ch;
        public char Ch
        {
            get
            {
                return _ch;
            }
        }

        public CharAddedEventArgs(char ch)
        {
            _ch = ch;
        }
    }
    #endregion

    #region FoldChangedEventArgs
    //public delegate void FoldChangedEventHandler(object sender, FoldChangedEventArgs e);
    public class FoldChangedEventArgs : ModifiedEventArgs
    {
        private int _line;
        private int _newFoldLevel;
        private int _previousFoldLevel;

        public int Line
        {
            get
            {
                return _line;
            }
            set
            {
                _line = value;
            }
        }

        public int NewFoldLevel
        {
            get
            {
                return _newFoldLevel;
            }
            set
            {
                _newFoldLevel = value;
            }
        }

        public int PreviousFoldLevel
        {
            get
            {
                return _previousFoldLevel;
            }
            set
            {
                _previousFoldLevel = value;
            }
        }

        public FoldChangedEventArgs(int line, int newFoldLevel, int previousFoldLevel, int modificationType)
            : base(modificationType)
        {
            _line = line;
            _newFoldLevel = newFoldLevel;
            _previousFoldLevel = previousFoldLevel;
        }
    }
    #endregion

    #region LinesNeedShownEventArgs
    //public delegate void LinesNeedShownEventHandler(object sender, LinesNeedShownEventArgs e);
    public class LinesNeedShownEventArgs : EventArgs
    {
        private int _firstLine;
        private int _lastLine;

        public int FirstLine
        {
            get { return _firstLine; }
            set { _firstLine = value; }
        }

        public int LastLine
        {
            get { return _lastLine; }
            set { _lastLine = value; }
        }

        public LinesNeedShownEventArgs(int startLine, int endLine)
        {
            _firstLine = startLine;
            _lastLine = endLine;
        }
    }

    #endregion

    #region MarkerChangedEventArgs
    //public delegate void MarkerChangedEventHandler(object sender, MarkerChangedEventArgs e);
    public class MarkerChangedEventArgs : ModifiedEventArgs
    {
        private int _line;
        public int Line
        {
            get
            {
                return _line;
            }
            set
            {
                _line = value;
            }
        }

        public MarkerChangedEventArgs(int line, int modificationType)
            : base(modificationType)
        {
            _line = line;
        }

    }
    #endregion

    #region ModifiedEventArgs

    //	ModifiedEventArgs is the base class for all events that are fired 
    //	in response to an SCN_MODIFY notification message. They all have 
    //	the Undo/Redo flags in common and I'm also including the raw 
    //	modificationType integer value for convenience purposes.
    public abstract class ModifiedEventArgs : EventArgs
    {
        private UndoRedoFlags _undoRedoFlags;
        private int _modificationType;

        public int ModificationType
        {
            get
            {
                return _modificationType;
            }
            set
            {
                _modificationType = value;
            }
        }

        public UndoRedoFlags UndoRedoFlags
        {
            get
            {
                return _undoRedoFlags;
            }
            set
            {
                _undoRedoFlags = value;
            }
        }

        public ModifiedEventArgs(int modificationType)
        {
            _modificationType = modificationType;
            _undoRedoFlags = new UndoRedoFlags(modificationType);
        }
    }
    #endregion

    #region NativeScintillaEventArgs

    //	All events fired from the INativeScintilla Interface use this
    //	delegate whice passes NativeScintillaEventArgs. Msg is a copy
    //	of the Notification Message sent to Scintilla's Parent WndProc
    //	and SCNotification is the SCNotification Struct pointed to by 
    //	Msg's lParam. 
    //public delegate void NativeScintillaEventHandler(object sender, NativeScintillaEventArgs e);

    public class NativeScintillaEventArgs : EventArgs
    {
        private Message _msg;
        private SCNotification _notification;

        public Message Msg
        {
            get
            {
                return _msg;
            }
        }

        public SCNotification SCNotification
        {
            get
            {
                return _notification;
            }
        }

        public NativeScintillaEventArgs(Message Msg, SCNotification notification)
        {
            _msg = Msg;
            _notification = notification;
        }
    }
    #endregion

    #region ScintillaMouseEventArgs
    //public delegate void ScintillaMouseEventHandler(object sender, ScintillaMouseEventArgs e);

    public class ScintillaMouseEventArgs : EventArgs
    {
        private int _x;
        private int _y;
        private int _position;

        public int X
        {
            get { return _x; }
            set { _x = value; }
        }

        public int Y
        {
            get { return _y; }
            set { _y = value; }
        }

        public int Position
        {
            get { return _position; }
            set { _position = value; }
        }

        public ScintillaMouseEventArgs(int x, int y, int position)
        {
            _x = x;
            _y = y;
            _position = position;
        }
    }

    #endregion

    #region StyleChangedEventArgs
    //	StyleChangedEventHandler is used for the StyleChanged Event which is also used as 
    //	a more specific abstraction around the SCN_MODIFIED notification message.
    //public delegate void StyleChangedEventHandler(object sender, StyleChangedEventArgs e);
    public class StyleChangedEventArgs : ModifiedEventArgs
    {
        private int _position;
        private int _length;

        public int Position
        {
            get
            {
                return _position;
            }
            set
            {
                _position = value;
            }
        }

        public int Length
        {
            get
            {
                return _length;
            }
            set
            {
                _length = value;
            }
        }

        public StyleChangedEventArgs(int position, int length, int modificationType)
            : base(modificationType)
        {
            _position = position;
            _length = length;
        }
    }
    #endregion

    #region StyleNeededEventArgs
    //public delegate void StyleNeededEventHandler(object sender, StyleNeededEventArgs e);

    public class StyleNeededEventArgs : EventArgs
    {
        private Range _range;
        public Range Range
        {
            get { return _range; }
            set { _range = value; }
        }

        public StyleNeededEventArgs(Range range)
        {
            _range = range;
        }
    }



    #endregion

    #region TextModifiedEventArgs

    //	TextModifiedEventHandler is used as an abstracted subset of the
    //	SCN_MODIFIED notification message. It's used whenever the SCNotification's
    //	modificationType flags are SC_MOD_INSERTTEXT ,SC_MOD_DELETETEXT, 
    //	SC_MOD_BEFOREINSERT and SC_MOD_BEFORE_DELETE. They all use a 
    //	TextModifiedEventArgs which corresponds to a subset of the 
    //	SCNotification struct having to do with these modification types.

    //public delegate void TextModifiedEventHandler(object sender, TextModifiedEventArgs e);

    public class TextModifiedEventArgs : ModifiedEventArgs
    {
        private int _position;
        private int _length;
        private int _linesAddedCount;
        private string _text;
        private bool _isUserChange;
        private int _markerChangedLine;

        private const string STRING_FORMAT = "ModificationTypeFlags\t:{0}\r\nPosition\t\t\t:{1}\r\nLength\t\t\t\t:{2}\r\nLinesAddedCount\t\t:{3}\r\nText\t\t\t\t:{4}\r\nIsUserChange\t\t\t:{5}\r\nMarkerChangeLine\t\t:{6}";

        public override string ToString()
        {
            return string.Format(STRING_FORMAT, ModificationType, _position, _length, _linesAddedCount, _text, _isUserChange, _markerChangedLine) + Environment.NewLine + UndoRedoFlags.ToString();
        }

        public bool IsUserChange
        {
            get
            {
                return _isUserChange;
            }
            set
            {
                _isUserChange = value;
            }
        }

        public int MarkerChangedLine
        {
            get
            {
                return _markerChangedLine;
            }
            set
            {
                _markerChangedLine = value;
            }
        }

        public int Position
        {
            get
            {
                return _position;
            }
            set
            {
                _position = value;
            }
        }

        public int Length
        {
            get
            {
                return _length;
            }
            set
            {
                _length = value;
            }
        }

        public int LinesAddedCount
        {
            get
            {
                return _linesAddedCount;
            }
            set
            {
                _linesAddedCount = value;
            }
        }


        public string Text
        {
            get
            {
                return _text;
            }
            set
            {
                _text = value;
            }
        }

        public TextModifiedEventArgs(SCNotification notification)
            : base(notification.modificationType)
        {
            _position = notification.position;
            _length = notification.length;
            _isUserChange = (notification.position & 0x10) != 0;
        }

        public TextModifiedEventArgs(int modificationType, bool isUserChange, int markerChangedLine, int position, int length, int linesAddedCount, string text)
            : base(modificationType)
        {
            _isUserChange = isUserChange;
            _markerChangedLine = markerChangedLine;
            _position = position;
            _length = length;
            _linesAddedCount = linesAddedCount;
            _text = text;
        }
    }
    #endregion

    #region UndoRedoFlags
    //	Used by TextModifiedEventArgs, StyeChangedEventArgs and FoldChangedEventArgs
    //	this provides a friendly wrapper around the SCNotification's modificationType
    //	flags having to do with Undo and Redo
    public struct UndoRedoFlags
    {
        public bool IsUndo;
        public bool IsRedo;
        public bool IsMultiStep;
        public bool IsLastStep;
        public bool IsMultiLine;

        private const string STRING_FORMAT = "IsUndo\t\t\t\t:{0}\r\nIsRedo\t\t\t\t:{1}\r\nIsMultiStep\t\t\t:{2}\r\nIsLastStep\t\t\t:{3}\r\nIsMultiLine\t\t\t:{4}";

        public override string ToString()
        {
            return string.Format(STRING_FORMAT, IsUndo, IsRedo, IsMultiStep, IsLastStep, IsMultiLine);
        }

        public UndoRedoFlags(int modificationType)
        {
            IsLastStep = (modificationType & (int)Scintilla.Enums.ModificationFlags.StepInUndoRedo ) > 0;
            /** FIXME : 
             * GENERATION ISSUE -- The Scintilla.iface breaks the pattern with the following three flags:
             *  val SC_MULTISTEPUNDOREDO=0x80
             *  val SC_MULTILINEUNDOREDO=0x1000
             *  val SC_MODEVENTMASKALL=0x1FFF
             **/

            IsMultiLine = (modificationType & /* SC_MULTILINEUNDOREDO */ 0x1000) > 0;
            IsMultiStep = (modificationType & /* SC_MULTISTEPUNDOREDO*/ 0x80) > 0;
            IsRedo = (modificationType & (int)Scintilla.Enums.ModificationFlags.Redo) > 0;
            IsUndo = (modificationType & (int)Scintilla.Enums.ModificationFlags.Undo) > 0;
        }
    }
    #endregion

    #region UriDroppedEventArgs
    //public delegate void UriDroppedEventHandler(object sender, UriDroppedEventArgs e);

    public class UriDroppedEventArgs
    {
        //	I decided to leave it a string because I can't really
        //	be sure it is a Uri.
        private string _uriText;
        public string UriText
        {
            get { return _uriText; }
            set { _uriText = value; }
        }

        public UriDroppedEventArgs(string uriText)
        {
            _uriText = uriText;
        }
    }
    #endregion

    //public delegate void AutoCSelectionEventHandler( object sender, AutoCSelectionEventArgs eventArgs);

    public class AutoCSelectionEventArgs : EventArgs
    {
        private string _text;
        public string Text
        {
            get { return _text; }
            set { _text = value; }
        }

        public AutoCSelectionEventArgs (SCNotification eventSource)
        {
            _text = Utilities.PtrToStringUtf8( eventSource.text , eventSource.length );
        }

    }

    public class DwellStartEventArgs : EventArgs
    {
        private int _x;
        private int _y;
        private int _position;

        public int X
        {
            get { return _x; }
            set { _x = value; }
        }

        public int Y
        {
            get { return _y; }
            set { _y = value; }
        }

        public int Position
        {
            get { return _position; }
            set { _position = value; }
        }

        public DwellStartEventArgs(SCNotification eventSource)
        {
            _x = eventSource.x;
            _y = eventSource.y;
            _position = eventSource.position;
        }

    }

}