using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using Scintilla.Enums;

namespace Scintilla
{
    internal enum VOID
    {
        NULL
    }

    public partial class ScintillaControl : Control
    {
        public const string DEFAULT_DLL_NAME = "SciLexer.dll";
        private static readonly object _nativeEventKey = new object();

        public ScintillaControl()
        {
        }

        public void SetLineIndentation(int line, int indentation) 
        {
            this.SendMessageDirect(2126, line, indentation);
        }

        public int GetLineIndentation(int line) 
        {
            return this.SendMessageDirect(2127, line);
        }

        /// <summary>
        /// Gets the caret position at the end of indentation on the specified line
        /// </summary>
        public int GetLineIndentationPosition(int line)
        {
            return this.SendMessageDirect(2128, line);
        }

        public void CallTipUseStyle(int tabSize)
        {
            this.SendMessageDirect(2212, tabSize);
        }

        public void StyleSetFont(int style, string font)
        {
            this.SendMessageDirect(2056, style, font);
        }

        public void StyleSetFontSize(int style, int fontSize)
        {
            this.SendMessageDirect(2055, style, fontSize);
        }

        public int GetCharAt(int position)
        {
            return this.SendMessageDirect(2007, position);
        }

        public int GetStyleAt(int position)
        {
            return this.SendMessageDirect(2010, position);
        }

        public void StyleSetFore(Scintilla.Lexers.Cpp syntaxType, Color value)
        {
            this.SendMessageDirect(2051, (int)syntaxType, Utilities.ColorToRgb(value));
        }

        public void StyleSetBack(Scintilla.Lexers.Cpp syntaxType, Color value)
        {
            this.SendMessageDirect(2052, (int)syntaxType, Utilities.ColorToRgb(value));
        }

        public void StyleSetBold(Scintilla.Lexers.Cpp syntaxType, bool bold)
        {
            this.SendMessageDirect(2053, (int)syntaxType, bold);
        }

        public void SetSelFore(bool useSetting, Color fore)
        {
            this.SendMessageDirect(2067, useSetting, Utilities.ColorToRgb(fore));
        }

        public void SetSelBack(bool useSetting, Color back)
        {
            this.SendMessageDirect(2068, useSetting, Utilities.ColorToRgb(back));
        }

        public void AutoCSetFillups(string fillupKeys)
        {
            this.SendMessageDirect(2112, 0, fillupKeys);
        }

        public void CallTipSetForeHlt(Color color)
        {
            this.SendMessageDirect(2207, Utilities.ColorToRgb(color), 0);
        }

        public void SetMarginType(int margin, MarginType marginType)
        {
            this.SendMessageDirect(2240, margin, (int)marginType);
        }

        public void SetMarginWidth(int margin, int width)
        {
            this.SendMessageDirect(2242, margin, width);
        }

        public void SetLexer(Lexer lexer)
        {
            this.SendMessageDirect(4001, (int)lexer);
        }

        public void SetKeyWords(string keywords)
        {
            this.SendMessageDirect(4005, 0, keywords);
        }

		public void SetClassListHighlightedWords(string keywords)
		{
			this.SendMessageDirect(4005, 1, keywords);
		}

        /// <summary>
        /// Retrieve the contents of a line. Returns the length of the line. 
        /// </summary>
        public virtual string GetLine(int line)
        {
            string result;
            int length = LineLength(line);
            if (length < 1)
            {
                return string.Empty;
            }
            SendMessageDirect(2153, (IntPtr)line, out result, length);
            if (result.EndsWith("\r\n"))
            {
                result = result.Substring(0, result.Length - 2);
            }
            return result;
        }

        /// <summary>
        /// Convert all line endings in the document to one mode. 
        /// </summary>
        public virtual void ConvertEOLs(EndOfLine eolMode)
        {
            this.SendMessageDirect(2029, (int)eolMode);
        }
        
        protected override CreateParams CreateParams
        {
            get
            {
                //	Otherwise Scintilla won't paint. When UserPaint is set to
                //	true the base Class (Control) eats the WM_PAINT message.
                //	Of course when this set to false we can't use the Paint
                //	events. This is why I'm relying on the Paint notification
                //	sent from scintilla to paint the Marker Arrows.
                SetStyle(ControlStyles.UserPaint, false);

                //	Registers the Scintilla Window Class
                //	When in the VS Designed, the DLL is used
                //  At run-time the AGS.Native DLL has this builtin
                //System.Diagnostics.Trace.WriteLine(System.IO.Directory.GetCurrentDirectory());
                if (System.IO.File.Exists(DEFAULT_DLL_NAME))
                {
                    WinAPI.LoadLibrary(DEFAULT_DLL_NAME);
                }

                //	Tell Windows Forms to create a Scintilla
                //	derived Window Class for this control
                CreateParams cp = base.CreateParams;
                cp.ClassName = "Scintilla";
                return cp;
            }
        }

        #region FIXME: SHOULD BE GENERATED CODE
        protected void DispatchScintillaEvent(SCNotification notification)
        {
            switch (notification.nmhdr.code)
            {
                case Scintilla.Enums.Events.AutoCSelection:
                    if (Events[Scintilla.Enums.Events.AutoCSelection] != null)
                        ((EventHandler<AutoCSelectionEventArgs>)Events[Scintilla.Enums.Events.AutoCSelection])(this, new AutoCSelectionEventArgs(notification));
                    break;

                case Scintilla.Enums.Events.SavePointLeft:
                    if (SavePointLeft != null)
                    {
                        SavePointLeft(this, null);
                    }
                    break;

                case Scintilla.Enums.Events.SavePointReached:
                    if (SavePointReached != null)
                    {
                        SavePointReached(this, null);
                    }
                    break;

                case Scintilla.Enums.Events.CharAdded:
                    if (CharAdded != null)
                    {
                        CharAdded(this, new CharAddedEventArgs(notification.ch));
                    }
                    break;

                case Scintilla.Enums.Events.UpdateUI:
                    if (UpdateUI != null)
                    {
                        UpdateUI(this, null);
                    }
                    break;

                case Scintilla.Enums.Events.ModifyAttemptRO:
                    if (ModifyAttemptOnReadOnly != null)
                    {
                        ModifyAttemptOnReadOnly(this, null);
                    }
                    break;

                case Scintilla.Enums.Events.Modified:
                    if (TextModified != null)
                    {
                        TextModified(this, new TextModifiedEventArgs(notification));
                    }
                    break;

                case Scintilla.Enums.Events.DwellStart:
                    if (DwellStart != null)
                    {
                        DwellStart(this, new DwellStartEventArgs(notification));
                    }
                    break;
                case Scintilla.Enums.Events.DwellEnd:
                    if (DwellEnd != null)
                    {
                        DwellEnd(this, null);
                    }
                    break;
            }
        }

        public event EventHandler<AutoCSelectionEventArgs> AutoCSelection
        {
            add { Events.AddHandler(Scintilla.Enums.Events.AutoCSelection, value); }
            remove { Events.RemoveHandler(Scintilla.Enums.Events.AutoCSelection, value); }
        }

        public event EventHandler SavePointLeft;
        public event EventHandler SavePointReached;
        public event EventHandler<CharAddedEventArgs> CharAdded;
        public event EventHandler UpdateUI;
        public event EventHandler ModifyAttemptOnReadOnly;
        public event EventHandler<TextModifiedEventArgs> TextModified;
        public event EventHandler<DwellStartEventArgs> DwellStart;
        public event EventHandler DwellEnd;

        #endregion

        #region Event Dispatch Mechanism
        protected override void WndProc(ref Message m)
        {
            //	Uh-oh. Code based on undocumented unsupported .NET behavior coming up!
            //	Windows Forms Sends Notify messages back to the originating
            //	control ORed with 0x2000. This is way cool becuase we can listen for
            //	WM_NOTIFY messages originating form our own hWnd (from Scintilla)
            if ((m.Msg ^ 0x2000) != WinAPI.WM_NOTIFY)
            {
                base.WndProc(ref m);
                return;
            }

            SCNotification scnotification = (SCNotification)Marshal.PtrToStructure(m.LParam, typeof(SCNotification));
            // dispatch to listeners of the native event first
            // this allows listeners to get the raw event if they really wish
            // but ideally, they'd just use the .NET event 
            if (Events[_nativeEventKey] != null)
                ((EventHandler<NativeScintillaEventArgs>)Events[_nativeEventKey])(this, new NativeScintillaEventArgs(m, scnotification));

            DispatchScintillaEvent(scnotification);
            base.WndProc(ref m);

        }
        
        protected event EventHandler<NativeScintillaEventArgs> NativeScintillaEvent
        {
            add { Events.AddHandler(_nativeEventKey, value); }
            remove { Events.RemoveHandler(_nativeEventKey, value); }
        }
        #endregion

        #region SendMessageDirect 

        /// <summary>
        /// This is the primary Native communication method with Scintilla
        /// used by this control. All the other overloads call into this one.
        /// </summary>
        private IntPtr SendMessageDirect(uint msg, IntPtr wParam, IntPtr lParam)
        {
            Message m = new Message();
            m.Msg = (int)msg;
            m.WParam = wParam;
            m.LParam = lParam;
            m.HWnd = Handle;

            //  DefWndProc is the Window Proc associated with the window
            //  class for this control created by Windows Forms. It will
            //  in turn call Scintilla's DefWndProc Directly. This has 
            //  the same net effect as using Scintilla's DirectFunction
            //  in that SendMessage isn't used to get the message to 
            //  Scintilla but requires 1 less PInvoke and I don't have
            //  to maintain the FunctionPointer and "this" reference
            DefWndProc(ref m);
            return m.Result;
        }

        //  Various overloads provided for syntactical convinience.
        //  note that the return value is int (32 bit signed Integer). 
        //  If you are invoking a message that returns a pointer or
        //  handle like SCI_GETDIRECTFUNCTION or SCI_GETDOCPOINTER
        //  you MUST use the IntPtr overload to ensure 64bit compatibility

        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (,)
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <returns></returns>
        private int SendMessageDirect(uint msg)
        {
            return (int)SendMessageDirect(msg, IntPtr.Zero, IntPtr.Zero);
        }

        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (int,int)    
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="wParam">wParam</param>
        /// <param name="lParam">lParam</param>
        /// <returns></returns>
        private int SendMessageDirect(uint msg, int wParam, int lParam)
        {
            return (int)SendMessageDirect(msg, (IntPtr)wParam, (IntPtr)lParam);
        }

        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (int,)    
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="wParam">wParam</param>
        /// <returns></returns>
        private int SendMessageDirect(uint msg, int wParam)
        {
            return (int)SendMessageDirect(msg, (IntPtr)wParam, IntPtr.Zero);
        }

        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (,int)    
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="NULL">always pass null--Unused parameter</param>
        /// <param name="lParam">lParam</param>
        /// <returns></returns>
        private int SendMessageDirect(uint msg, VOID NULL, int lParam)
        {
            return (int)SendMessageDirect(msg, IntPtr.Zero, (IntPtr)lParam);
        }

        
        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (bool,int)    
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="wParam">boolean wParam</param>
        /// <param name="lParam">int lParam</param>
        /// <returns></returns>
        private int SendMessageDirect(uint msg, bool wParam, int lParam)
        {
            return (int)SendMessageDirect(msg, (IntPtr)(wParam ? 1 : 0), (IntPtr)lParam);
        }

        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (bool,)    
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="wParam">boolean wParam</param>
        /// <returns></returns>
        private int SendMessageDirect(uint msg, bool wParam)
        {
            return (int)SendMessageDirect(msg, (IntPtr)(wParam ? 1 : 0), IntPtr.Zero);
        }

        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (int,bool)    
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="wParam">int wParam</param>
        /// <param name="lParam">boolean lParam</param>
        /// <returns></returns>
        private int SendMessageDirect(uint msg, int wParam, bool lParam)
        {
            return (int)SendMessageDirect(msg, (IntPtr)wParam, (IntPtr)(lParam ? 1 : 0));
        }

        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (,stringresult)    
        /// Notes:
        ///  Helper method to wrap all calls to messages that take a char*
        ///  in the lParam and returns a regular .NET String. This overload
        ///  assumes there will be no wParam and obtains the string length
        ///  by calling the message with a 0 lParam. 
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="text">String output</param>
        /// <returns></returns>
        private int SendMessageDirect(uint msg, out string text)
        {
            int length = SendMessageDirect(msg, 0, 0);
            return SendMessageDirect(msg, IntPtr.Zero, out text, length);
        }

        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (int,stringresult)    
        /// Notes:
        ///  Helper method to wrap all calls to messages that take a char*
        ///  in the lParam and returns a regular .NET String. This overload
        ///  assumes there will be no wParam and obtains the string length
        ///  by calling the message with a 0 lParam. 
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="text">String output</param>
        /// <returns></returns>
        private int SendMessageDirect(uint msg, int wParam, out string text)
        {
            int length = SendMessageDirect(msg, 0, 0);
            return SendMessageDirect(msg, (IntPtr)wParam, out text, length);
        }

        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (?)    
        /// Notes:
        ///  Helper method to wrap all calls to messages that take a char*
        ///  in the wParam and set a regular .NET String in the lParam. 
        ///  Both the length of the string and an additional wParam are used 
        ///  so that various string Message styles can be acommodated.
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="wParam">int wParam</param>
        /// <param name="text">String output</param>
        /// <param name="length">length of the input buffer</param>
        /// <returns></returns>
        private unsafe int SendMessageDirect(uint msg, IntPtr wParam, out string text, int length)
        {
            IntPtr ret;

            //  Allocate a buffer the size of the string + 1 for 
            //  the NULL terminator. Scintilla always sets this
            //  regardless of the encoding
            byte[] buffer = new byte[length + 1];

            //  Get a direct pointer to the the head of the buffer
            //  to pass to the message along with the wParam. 
            //  Scintilla will fill the buffer with string data.
            fixed (byte* bp = buffer)
            {
                ret = SendMessageDirect(msg, wParam, (IntPtr)bp);

                //	If this string is NULL terminated we want to trim the
                //	NULL before converting it to a .NET String
                if (bp[length - 1] == 0)
                    length--;
            }


            //  We always assume UTF8 encoding to ensure maximum
            //  compatibility. Manually changing the encoding to 
            //  something else will cuase 2 Byte characters to
            //  be interpreted as junk.
            // ** MY CHANGE: CHANGE TO DEFAULT ENCODING SO THAT ASCII CHARS 128-255 WORK
            text = Encoding.Default.GetString(buffer, 0, length);

            return (int)ret;
        }


        
        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (int,string)    
        /// Notes:
        ///  This helper method handles all messages that take
        ///  const char* as an input string in the lParam. In
        ///  some messages Scintilla expects a NULL terminated string
        ///  and in others it depends on the string length passed in
        ///  as wParam. This method handles both situations and will
        ///  NULL terminate the string either way. 
        /// 
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="wParam">int wParam</param>
        /// <param name="lParam">string lParam</param>
        /// <returns></returns>
        private unsafe int SendMessageDirect(uint msg, int wParam, string lParam)
        {
            //  Just as when retrieving we make to convert .NET's
            //  UTF-16 strings into a UTF-8 encoded byte array.
            fixed (byte* bp = UTF8Encoding.UTF8.GetBytes(ZeroTerminated(lParam)))
                return (int)SendMessageDirect(msg, (IntPtr)wParam, (IntPtr)bp);
        }

        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (,string)    
        /// 
        /// Notes:
        ///  This helper method handles all messages that take
        ///  const char* as an input string in the lParam. In
        ///  some messages Scintilla expects a NULL terminated string
        ///  and in others it depends on the string length passed in
        ///  as wParam. This method handles both situations and will
        ///  NULL terminate the string either way. 
        /// 
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="NULL">always pass null--Unused parameter</param>
        /// <param name="lParam">string lParam</param>
        /// <returns></returns>
        private unsafe int SendMessageDirect(uint msg, VOID NULL, string lParam)
        {
            //  Just as when retrieving we make to convert .NET's
            //  UTF-16 strings into a UTF-8 encoded byte array.
            //fixed (byte* bp = UTF8Encoding.UTF8.GetBytes(ZeroTerminated(lParam)))
            // ** MY CHANGE: Use 8-bit ANSI
            fixed (byte* bp = Encoding.Default.GetBytes(ZeroTerminated(lParam)))
                return (int)SendMessageDirect(msg, IntPtr.Zero, (IntPtr)bp);
        }


     

        
        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (string,string)    
        /// 
        /// Notes:
        ///    Used by SCI_SETPROPERTY
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="wParam">string wParam</param>
        /// <param name="lParam">string lParam</param>
        /// <returns></returns>
        private unsafe int SendMessageDirect(uint msg, string wParam, string lParam)
        {
            fixed (byte* bpw = UTF8Encoding.UTF8.GetBytes(ZeroTerminated(wParam)))
            fixed (byte* bpl = UTF8Encoding.UTF8.GetBytes(ZeroTerminated(lParam)))
                return (int)SendMessageDirect(msg, (IntPtr)bpw, (IntPtr)bpl);
        }

        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (string,stringresult)    
        /// 
        /// Notes:
        ///  This one is used specifically by SCI_GETPROPERTY and SCI_GETPROPERTYEXPANDED
        ///  so it assumes it's usage
        /// 
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="wParam">string wParam</param>
        /// <param name="stringResult">Stringresult output</param>
        /// <returns></returns>
        private unsafe int SendMessageDirect(uint msg, string wParam, out string stringResult)
        {
            IntPtr ret;

            fixed (byte* bpw = UTF8Encoding.UTF8.GetBytes(ZeroTerminated(wParam)))
            {
                int length = (int)SendMessageDirect(msg, (IntPtr)bpw, IntPtr.Zero);


                byte[] buffer = new byte[length + 1];

                fixed (byte* bpl = buffer)
                    ret = SendMessageDirect(msg, (IntPtr)bpw, (IntPtr)bpl);

                stringResult = UTF8Encoding.UTF8.GetString(buffer, 0, length);
            }

            return (int)ret;
        }
   
        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (string,int)    
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="wParam">string wParam</param>
        /// <param name="lParam">int lParam</param>
        /// <returns></returns>
        private unsafe int SendMessageDirect(uint msg, string wParam, int lParam)
        {
            fixed (byte* bp = UTF8Encoding.UTF8.GetBytes(ZeroTerminated(wParam)))
                return (int)SendMessageDirect(msg, (IntPtr)bp, (IntPtr)lParam);
        }

        /// <summary>
        /// Handles Scintilla Call Style:
        ///    (string,)    
        /// </summary>
        /// <param name="msg">Scintilla Message Number</param>
        /// <param name="wParam">string wParam</param>
        /// <returns></returns>
        private unsafe int SendMessageDirect(uint msg, string wParam)
        {
            fixed (byte* bp = UTF8Encoding.UTF8.GetBytes(ZeroTerminated(wParam)))
                return (int)SendMessageDirect(msg, (IntPtr)bp, IntPtr.Zero);
        }

        private String ZeroTerminated(string param)
        {
            if (string.IsNullOrEmpty(param))
                return "\0";
            else if (!param.EndsWith("\0"))
                return param + "\0";
            return param;
        }
        #endregion

    }
}
