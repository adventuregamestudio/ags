using AGS.Types;
using AGS.Types.AutoComplete;
using Scintilla.Enums;
using Scintilla.Lexers;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using AGS.Types.Interfaces;

namespace AGS.Editor
{
    // This class is a bit of a mess ... autocomplete is out of control!!
    public partial class ScintillaWrapper : UserControl, IScriptEditorControl
    {
		public delegate void ConstructContextMenuHandler(ContextMenuStrip menuStrip, int clickedPositionInDocument);
		public delegate void ActivateContextMenuHandler(string commandName);

		private const string CONTEXT_MENU_CUT = "CtxCut";
		private const string CONTEXT_MENU_COPY = "CtxCopy";
		private const string CONTEXT_MENU_PASTE = "CtxPaste";
		private const string CONTEXT_MENU_GO_TO_DEFINITION = "CtxDefinition";

        private const string THIS_STRUCT = "this";
        private const int INVALID_POSITION = -1;
        private const int AUTOCOMPLETE_MINIMUM_WORD_LENGTH = 3;
        private const int SCAN_BACK_DISTANCE = 50;
        private const string DEFAULT_FONT = "Courier New";
        private const int DEFAULT_FONT_SIZE = 10;
        private const string USER_FRIENDLY_FONT = "Tahoma";
        private const int USER_FRIENDLY_FONT_SIZE = 8;
        private const string AUTO_COMPLETE_CANCEL_CHARS = ")}; ";
        private const int IMAGE_INDEX_PROPERTY = 1;
        private const int IMAGE_INDEX_METHOD = 2;
        private const int IMAGE_INDEX_GLOBAL_VARIABLE = 3;
        private const int IMAGE_INDEX_ENUM = 4;
        private const int IMAGE_INDEX_STATIC_PROPERTY = 5;
        private const int IMAGE_INDEX_STATIC_METHOD = 6;
        private const int IMAGE_INDEX_READONLY_PROPERTY = 7;
        private const int IMAGE_INDEX_STRUCT = 8;
        private const int IMAGE_INDEX_DEFINE = 9;
        private const int IMAGE_INDEX_KEYWORD = 10;
        private const int IMAGE_INDEX_EXTENDER_METHOD = 11;
        private const int IMAGE_INDEX_LOCAL_VARIABLE = 12;
        private const int MARKER_TYPE_BREAKPOINT = 1;
        private const int MARKER_TYPE_BREAKPOINT2 = 2;
        private const int MARKER_TYPE_CURRENT_STATEMENT = 3;
        private const int MARKER_TYPE_CURRENT_STATEMENT2 = 4;
        private const int MARKER_MASK_BREAKPOINT = 0x02;
        private const int MARKER_MASK_BREAKPOINT2 = 0x04;
        private const int MARKER_MASK_CURRENT_STATEMENT = 0x08;
        private const int MARKER_MASK_CURRENT_STATEMENT2 = 0x10;

        private static List<string> AutoCompleteIcons = new List<string>();

        public event EventHandler IsModifiedChanged;
        public event EventHandler UpdateUI;
        public event EventHandler<Scintilla.MarginClickEventArgs> ToggleBreakpoint;
        public delegate void CharAddedHandler(char charAdded);
        public event CharAddedHandler CharAdded;
        public delegate void TextModifiedHandler(int startPos, int length, bool wasAdded);
        public event TextModifiedHandler TextModified;
        public delegate void AttemptModifyHandler(ref bool allowModify);
        public event AttemptModifyHandler AttemptModify;
		public event ConstructContextMenuHandler ConstructContextMenu;
		public event ActivateContextMenuHandler ActivateContextMenu;

        private List<string> _keywords = new List<string>();
        private bool _doBraceMatch = false;
        private bool _braceMatchVisible = false;
        private bool _doShowAutocomplete = false;
        private bool _doCalltip = false;
        private string _fillupKeys = string.Empty;
        private bool _autoCompleteEnabled = true;
        private bool _ignoreLinesWithoutIndent = false;
        private bool _callTipsEnabled = true;
		private bool _autoSpaceAfterComma = true;
        private bool _autoDedentClosingBrace = true;
        private IScript _autoCompleteForThis = null;
        private bool _dwellCalltipVisible = false;
        private ErrorPopup _errorPopup = null;
        private string _fixedTypeForThisKeyword = null;
        private bool _activated = false;

        public ScintillaWrapper()
        {
            InitializeComponent();

            if (AutoCompleteIcons.Count == 0)
            {
                AutoCompleteIcons.Add(Resources.ResourceManager.GetResourceAsString("prop.xpm"));
                AutoCompleteIcons.Add(Resources.ResourceManager.GetResourceAsString("method.xpm"));
                AutoCompleteIcons.Add(Resources.ResourceManager.GetResourceAsString("gvar.xpm"));
                AutoCompleteIcons.Add(Resources.ResourceManager.GetResourceAsString("enum.xpm"));
                AutoCompleteIcons.Add(Resources.ResourceManager.GetResourceAsString("statprop.xpm"));
                AutoCompleteIcons.Add(Resources.ResourceManager.GetResourceAsString("statmeth.xpm"));
                AutoCompleteIcons.Add(Resources.ResourceManager.GetResourceAsString("roprop.xpm"));
                AutoCompleteIcons.Add(Resources.ResourceManager.GetResourceAsString("struct.xpm"));
                AutoCompleteIcons.Add(Resources.ResourceManager.GetResourceAsString("define.xpm"));
                AutoCompleteIcons.Add(Resources.ResourceManager.GetResourceAsString("keyword.xpm"));
                AutoCompleteIcons.Add(Resources.ResourceManager.GetResourceAsString("methodext.xpm"));
                AutoCompleteIcons.Add(Resources.ResourceManager.GetResourceAsString("localvar.xpm"));
            }

            for (int i = 0; i < AutoCompleteIcons.Count; i++)
            {
                this.scintillaControl1.RegisterImage(i + 1, AutoCompleteIcons[i]);
            }

            this.scintillaControl1.EOLMode = (int)EndOfLine.Crlf;
            this.scintillaControl1.WrapMode = (int)Wrap.None;
            this.scintillaControl1.ClearAll();
            this.scintillaControl1.SetLexer(Scintilla.Enums.Lexer.Cpp);

            this.scintillaControl1.StyleSetFont((int)Scintilla.Enums.StylesCommon.Default, DEFAULT_FONT);
            this.scintillaControl1.StyleSetFontSize((int)Scintilla.Enums.StylesCommon.Default, DEFAULT_FONT_SIZE);

            this.scintillaControl1.StyleSetFont((int)Cpp.BraceBad, DEFAULT_FONT);
            this.scintillaControl1.StyleSetFontSize((int)Cpp.BraceBad, DEFAULT_FONT_SIZE);
            this.scintillaControl1.StyleSetBack(Cpp.BraceBad, Color.FromArgb(255, 0, 0));
            this.scintillaControl1.StyleSetFont((int)Cpp.BraceLight, DEFAULT_FONT);
            this.scintillaControl1.StyleSetFontSize((int)Cpp.BraceLight, DEFAULT_FONT_SIZE);
            this.scintillaControl1.StyleSetBold(Cpp.BraceLight, true);
            this.scintillaControl1.StyleSetBack(Cpp.BraceLight, Color.FromArgb(210, 210, 0));
        
            this.scintillaControl1.StyleSetFore(Cpp.Word, Color.FromArgb(0, 0, 244));
			this.scintillaControl1.StyleSetFore(Cpp.Word2, Color.FromArgb(43, 145, 175));
            this.scintillaControl1.StyleSetFore(Cpp.Comment, Color.FromArgb(27, 127, 27));
            this.scintillaControl1.StyleSetFore(Cpp.CommentLine, Color.FromArgb(27, 127, 27));
            this.scintillaControl1.StyleSetFore(Cpp.CommentDoc, Color.FromArgb(27, 127, 27));
            this.scintillaControl1.StyleSetFore(Cpp.CommentLineDoc, Color.FromArgb(27, 127, 27));
            this.scintillaControl1.StyleSetFore(Cpp.Number, Color.FromArgb(150, 27, 27));
            this.scintillaControl1.StyleSetFore(Cpp.String, Color.FromArgb(70, 7, 7));
            this.scintillaControl1.StyleSetFore(Cpp.Operator, Color.FromArgb(0, 70, 0));
            this.scintillaControl1.StyleSetBack(Cpp.Preprocessor, Color.FromArgb(210, 210, 210));

            this.scintillaControl1.StyleSetFont((int)Cpp.CallTip, USER_FRIENDLY_FONT);
            this.scintillaControl1.StyleSetFontSize((int)Cpp.CallTip, USER_FRIENDLY_FONT_SIZE);
            this.scintillaControl1.StyleSetFore(Cpp.CallTip, Color.Black);
            this.scintillaControl1.StyleSetBack(Cpp.CallTip, Color.LightGoldenrodYellow);

            this.scintillaControl1.CallTipSetForeHlt(Color.FromArgb(240, 0, 0));
            this.scintillaControl1.CallTipUseStyle(0);
            this.scintillaControl1.IsAutoCIgnoreCase = true;
            this.scintillaControl1.IsAutoCCancelAtStart = false;
            this.scintillaControl1.IsAutoCAutoHide = false;
            this.scintillaControl1.AutoCMaxHeight = 8;
            this.scintillaControl1.AutoCMaxWidth = 100;
            this.scintillaControl1.AutoCStops(AUTO_COMPLETE_CANCEL_CHARS);
            this.scintillaControl1.MouseDwellTime = 500;

            // ensure scintilla does not handle Ctrl+Space
            this.scintillaControl1.ClearCmdKey(' ' | ((int)KeyMod.Ctrl << 16));
			// disable Ctrl+T swapping lines since it used to be Test Game
			this.scintillaControl1.ClearCmdKey('T' | ((int)KeyMod.Ctrl << 16));

            this.scintillaControl1.TabWidth = Factory.AGSEditor.Preferences.TabSize;
			this.scintillaControl1.IsUseTabs = Factory.AGSEditor.Preferences.IndentUsingTabs;
			this.scintillaControl1.UsePopUp(false);

            // override the selected text colour
            this.scintillaControl1.SetSelFore(true, Color.FromArgb(255, 255, 255));
            this.scintillaControl1.SetSelBack(true, Color.FromArgb(0, 34, 130));

            // remove the default margins
            this.scintillaControl1.SetMarginWidth(0, 0);
            this.scintillaControl1.SetMarginWidth(1, 16);
            
            this.scintillaControl1.MarkerDefine(MARKER_TYPE_BREAKPOINT, (int)MarkerSymbol.Background);
            this.scintillaControl1.MarkerSetBack(MARKER_TYPE_BREAKPOINT, Color.FromArgb(255, 100, 100));
            this.scintillaControl1.MarkerSetFore(MARKER_TYPE_BREAKPOINT, Color.White);

            this.scintillaControl1.MarkerDefine(MARKER_TYPE_BREAKPOINT2, (int)MarkerSymbol.Circle);
            this.scintillaControl1.MarkerSetBack(MARKER_TYPE_BREAKPOINT2, Color.Red);
            this.scintillaControl1.MarkerSetFore(MARKER_TYPE_BREAKPOINT2, Color.Black);

            this.scintillaControl1.SetMarginSensitivity(1, 1);

            this.scintillaControl1.MarkerDefine(MARKER_TYPE_CURRENT_STATEMENT, (int)MarkerSymbol.Arrow);
            this.scintillaControl1.MarkerSetBack(MARKER_TYPE_CURRENT_STATEMENT, Color.Yellow);
            this.scintillaControl1.MarkerSetFore(MARKER_TYPE_CURRENT_STATEMENT, Color.White);

            this.scintillaControl1.MarkerDefine(MARKER_TYPE_CURRENT_STATEMENT2, (int)MarkerSymbol.Background);
            this.scintillaControl1.MarkerSetBack(MARKER_TYPE_CURRENT_STATEMENT2, Color.Yellow);
            this.scintillaControl1.MarkerSetFore(MARKER_TYPE_CURRENT_STATEMENT2, Color.White);

            this.scintillaControl1.ModEventMask = 3;  // Insert/Delete text only

            this.scintillaControl1.SavePointLeft += new EventHandler(OnSavePointLeft);
            this.scintillaControl1.SavePointReached += new EventHandler(OnSavePointReached);
            this.scintillaControl1.CharAdded += new EventHandler<Scintilla.CharAddedEventArgs>(OnCharAdded);
            this.scintillaControl1.UpdateUI += new EventHandler(OnUpdateUI);
            this.scintillaControl1.ModifyAttemptOnReadOnly += new EventHandler(OnModifyAttemptOnReadOnly);
            this.scintillaControl1.TextModified += new EventHandler<Scintilla.TextModifiedEventArgs>(scintillaControl1_TextModified);
			this.scintillaControl1.MouseUp += new MouseEventHandler(ScintillaWrapper_MouseUp);
            this.scintillaControl1.DwellStart += new EventHandler<Scintilla.DwellStartEventArgs>(scintillaControl1_DwellStart);
            this.scintillaControl1.DwellEnd += new EventHandler(scintillaControl1_DwellEnd);
            this.scintillaControl1.MarginClick += new EventHandler<Scintilla.MarginClickEventArgs>(scintillaControl1_MarginClick);

            this.scintillaControl1.SetFolding();
            

            this.scintillaControl1.IsReadOnly = true;
        }

        void scintillaControl1_MarginClick(object sender, Scintilla.MarginClickEventArgs e)
        {
            if (e.Margin == 1)
            {
                if (ToggleBreakpoint != null)
                    ToggleBreakpoint(this, e);
            }
            else if (e.Margin == 2)
            {
                this.scintillaControl1.ToggleFold(e.LineNumber);
            }

            this.scintillaControl1.Invalidate();
        }

        public bool AutoCompleteEnabled
        {
            get { return _autoCompleteEnabled; }
            set { _autoCompleteEnabled = value; }
        }

        public bool IgnoreLinesWithoutIndent
        {
            get { return _ignoreLinesWithoutIndent; }
            set { _ignoreLinesWithoutIndent = value; }
        }

		public bool AutoSpaceAfterComma
		{
			get { return _autoSpaceAfterComma; }
			set { _autoSpaceAfterComma = value; }
		}

        public string FixedTypeForThisKeyword
        {
            get { return _fixedTypeForThisKeyword; }
            set { _fixedTypeForThisKeyword = value; }
        }

        public bool CallTipsEnabled
        {
            get { return _callTipsEnabled; }
            set { _callTipsEnabled = value; }
        }

        public void EnableLineNumbers()
        {
            // set up line numbers in left margin
            int marginWidth = this.scintillaControl1.TextWidth((int)StylesCommon.Default, "12345");
            this.scintillaControl1.SetMarginType(0, MarginType.Number);
            this.scintillaControl1.SetMarginWidth(0, marginWidth + 4);
        }

        public void UndoLastModification()
        {
            this.scintillaControl1.Undo();
        }

        public void ActivateTextEditor()
        {
            _activated = true;
            this.Focus();
            this.scintillaControl1.Focus();
        }

        public void DeactivateTextEditor()
        {
            _activated = false; 
        }

        public int CurrentPos
        {
            get { return scintillaControl1.CurrentPos; }
        }

        public int CurrentLine
        {
            get { return scintillaControl1.LineFromPosition(scintillaControl1.CurrentPos); }
        }

		public void SetAsDialog()
        {
            scintillaControl1.SetLexer(0);
            scintillaControl1.IsDialog = true;
        }
        public void GoToPosition(int newPos)
        {
			int lineNum = scintillaControl1.LineFromPosition(newPos);

			if ((lineNum <= scintillaControl1.FirstVisibleLine) ||
				(lineNum >= scintillaControl1.FirstVisibleLine + scintillaControl1.LinesOnScreen))
			{
				int bottomLine = lineNum + (scintillaControl1.LinesOnScreen / 2);
				if (bottomLine > scintillaControl1.LineCount)
				{
					bottomLine = scintillaControl1.LineCount - 1;
				}
				int topLine = lineNum - (scintillaControl1.LinesOnScreen / 2);
				if (topLine < 0)
				{
					topLine = 0;
				}
				scintillaControl1.GotoLine(bottomLine);
				scintillaControl1.GotoLine(topLine);
			}

            scintillaControl1.GotoPos(newPos);
        }

		public void SelectCurrentLine()
		{
			int curLine = this.CurrentLine;
			scintillaControl1.SetSel(scintillaControl1.PositionFromLine(curLine), scintillaControl1.PositionFromLine(curLine + 1) - 1);
		}

        public void AddBreakpoint(int lineNumber)
        {
            scintillaControl1.MarkerAdd(lineNumber, MARKER_TYPE_BREAKPOINT);
            scintillaControl1.MarkerAdd(lineNumber, MARKER_TYPE_BREAKPOINT2);
        }

        public bool IsBreakpointOnLine(int lineNumber)
        {
            return (scintillaControl1.MarkerGet(lineNumber) & MARKER_MASK_BREAKPOINT) != 0;
        }

        public void RemoveBreakpoint(int lineNumber)
        {
            scintillaControl1.MarkerDelete(lineNumber, MARKER_TYPE_BREAKPOINT);
            scintillaControl1.MarkerDelete(lineNumber, MARKER_TYPE_BREAKPOINT2);
        }

        public int[] GetLineNumbersForAllBreakpoints()
        {
            List<int> breakpointLines = new List<int>();
            int currentLineOffset = 0;
            while (currentLineOffset >= 0)
            {
                currentLineOffset = scintillaControl1.MarkerNext(currentLineOffset, MARKER_MASK_BREAKPOINT);
                if (currentLineOffset >= 0)
                {
                    breakpointLines.Add(currentLineOffset + 1);
                    currentLineOffset++;
                }
            }
            return breakpointLines.ToArray();
        }

        public void ShowCurrentExecutionPoint(int lineNumber) 
        {
            scintillaControl1.MarkerAdd(lineNumber - 1, MARKER_TYPE_CURRENT_STATEMENT);
            scintillaControl1.MarkerAdd(lineNumber - 1, MARKER_TYPE_CURRENT_STATEMENT2);
        }

        public void HideCurrentExecutionPoint()
        {
            scintillaControl1.MarkerDeleteAll(MARKER_TYPE_CURRENT_STATEMENT);
            scintillaControl1.MarkerDeleteAll(MARKER_TYPE_CURRENT_STATEMENT2);
        }

        public void ShowErrorMessagePopup(string errorMessage)
        {
            Form activeForm = Form.ActiveForm;

            _errorPopup = new ErrorPopup(errorMessage);
            int x = scintillaControl1.PointXFromPosition(this.scintillaControl1.CurrentPos) + 200;
            int y = scintillaControl1.PointYFromPosition(this.scintillaControl1.CurrentPos) + 40;
            Point mainWindowOffset = this.PointToScreen(new Point(0, 0));
            _errorPopup.Location = new Point(mainWindowOffset.X + x, mainWindowOffset.Y + y);
            _errorPopup.Show(this);

            // don't allow the popup to steal the focus
            if (activeForm != null)
            {
                activeForm.Activate();
            }
        }

        public void HideErrorMessagePopup()
        {
            if (_errorPopup != null)
            {
                _errorPopup.Hide();
                _errorPopup.Dispose();
                _errorPopup = null;
            }
        }

        public void SetKeyWords(string keyWords)
        {
            this.scintillaControl1.SetKeyWords(keyWords);
            SetAutoCompleteKeyWords(keyWords);
        }

        public void SetAutoCompleteKeyWords(string keyWords)
        {
            _keywords.Clear();
            string[] keywordArray = keyWords.Split(' ');
            foreach (string keyword in keywordArray)
            {
                // "true" and "false" are actually enums, so don't list them as keywords
                if ((keyword != "true") && (keyword != "false"))
                {
                    _keywords.Add(keyword + "?" + IMAGE_INDEX_KEYWORD);
                }
            }
        }

		public void SetClassNamesList(string classNames)
		{
			this.scintillaControl1.SetClassListHighlightedWords(classNames);
		}

        public void SetFillupKeys(string fillupKeys)
        {
            // pressing ( [ or . will auto-complete
            this.scintillaControl1.AutoCSetFillups(fillupKeys);
            _fillupKeys = fillupKeys;
        }

        public void SetSavePoint()
        {
            this.scintillaControl1.SetSavePoint();
            this.scintillaControl1.IsReadOnly = true;
        }

        public void SetText(string newText)
        {
            SetText(newText, true);
        }

        public void SetTextModified(string newText)
        {
            SetText(newText, false);
        }

        public void SetText(string newText, bool clearModified)
        {
            bool shouldBeReadOnly = this.scintillaControl1.IsReadOnly;
            this.scintillaControl1.IsReadOnly = false;

            this.scintillaControl1.SetText(newText);
            this.scintillaControl1.ConvertEOLs(EndOfLine.Crlf);
            if (clearModified)
            {
                this.scintillaControl1.SetSavePoint();
                this.scintillaControl1.EmptyUndoBuffer();
            }

            this.scintillaControl1.IsReadOnly = shouldBeReadOnly;
        }

        public void SetAutoCompleteSource(IScript script)
        {
            _autoCompleteForThis = script;
        }

        public void ModifyText(string newText)
        {
            this.scintillaControl1.SetText(newText);
        }

        public string GetText()
        {
            string text = this.scintillaControl1.GetText();

            while (text.EndsWith("\0"))
            {
                text = text.Substring(0, text.Length - 1);
            }

            return text;
        }

        public bool IsModified
        {
            get { return this.scintillaControl1.IsModify; }
        }

        public int FindLineNumberForText(string text)
        {
            string currentText = this.scintillaControl1.GetText();
            if (currentText.IndexOf(text) >= 0)
            {
                return FindLineNumberForCharacterIndex(currentText.IndexOf(text));
            }
            return 0;
        }

        public int FindLineNumberForCharacterIndex(int pos)
        {
            return this.scintillaControl1.LineFromPosition(pos) + 1;
        }

        public void GoToLine(int lineNum)
        {
            this.scintillaControl1.EnsureVisibleEnforcePolicy(lineNum);
            if (lineNum > 0)
            {
            this.scintillaControl1.EnsureVisibleEnforcePolicy(lineNum - 1);
            }
            
			int bottomLine = lineNum + (scintillaControl1.LinesOnScreen / 2);
			if (bottomLine > scintillaControl1.LineCount)
			{
				bottomLine = scintillaControl1.LineCount - 1;
			}
			int topLine = lineNum - (scintillaControl1.LinesOnScreen / 2);
			if (topLine < 0)
			{
				topLine = 0;
			}
			scintillaControl1.GotoLine(bottomLine);
			scintillaControl1.GotoLine(topLine);
            this.scintillaControl1.GotoLine(lineNum - 1);
        }

        public void Cut()
        {
            this.scintillaControl1.Cut();
        }

        public void Copy()
        {
            this.scintillaControl1.Copy();
        }

        public bool CanCutAndCopy()
        {
            return IsSomeSelectedText();
        }

        public bool IsSomeSelectedText()
        {
            return (this.scintillaControl1.SelectionStart != this.scintillaControl1.SelectionEnd);
        }

        public string SelectedText
        {
            get { return this.scintillaControl1.GetSelText(); }
        }

        public bool CanPaste()
        {
            //return this.scintillaControl1.CanPaste;
            return Clipboard.ContainsText();
        }

        public void Paste()
        {
            this.scintillaControl1.Paste();
        }

        public bool CanUndo()
        {
            return this.scintillaControl1.CanUndo;
        }

        public void Undo()
        {
            this.scintillaControl1.Undo();
        }

        public bool CanRedo()
        {
            return this.scintillaControl1.CanRedo;
        }

        public void Redo()
        {
            this.scintillaControl1.Redo();
        }

        public void ShowAutocompleteNow()
        {
            ShowAutoCompleteIfAppropriate(0);
        }

        public void SetSelection(int pos, int length)
        {            
            scintillaControl1.EnsureVisible(scintillaControl1.LineFromPosition(pos));
            scintillaControl1.SetSel(pos, pos + length);            
        }

        public ScriptTokenReference FindNextOccurrence(string text, bool caseSensitive, bool jumpToStart)
        {
            StringComparison comparisonType = StringComparison.CurrentCulture;
            if (!caseSensitive)
            {
                comparisonType = StringComparison.CurrentCultureIgnoreCase;
            }
            string documentText = GetText();
            int currentPos = this.scintillaControl1.CurrentPos;
            int nextPos = -1;
            if (currentPos < documentText.Length)
            {
                nextPos = documentText.IndexOf(text, currentPos + 1, comparisonType);
            }
            if (nextPos < 0)
            {
                if (!jumpToStart)
                {
                    return null;
                }
                nextPos = documentText.IndexOf(text, 0, comparisonType);
                if (nextPos < 0)
                {
                    return null;
                }
            }
            
            SetSelection(nextPos, text.Length);

            return GetTokenReferenceForCurrentState();
        }

        public ScriptTokenReference GetTokenReferenceForCurrentState()
        {
            int nextPos = scintillaControl1.SelectionStart;
            int lineIndex = scintillaControl1.LineFromPosition(nextPos);
            string line = scintillaControl1.GetLine(lineIndex);
            return new ScriptTokenReference
            {
                CharacterIndex = nextPos,
                CurrentLine = line,
                LineIndex = lineIndex,
                Token = this.SelectedText,
                Script = _autoCompleteForThis
            };
        }

		public void ReplaceSelectedText(string withText)
		{
			scintillaControl1.ReplaceSel(withText);
		}

		public string GetFullTypeNameAtCursor()
		{
			return GetFullTypeNameAtPosition(scintillaControl1.CurrentPos);
		}

        public string GetFullTypeNameAtPosition(int charIndex)
        {
            string fullTypeName;
            bool staticAccess;
            bool isThis;

            ScriptStruct foundType = ParsePreviousExpression(charIndex - 1, out fullTypeName, out staticAccess, out isThis);
            if (foundType != null)
            {
                fullTypeName = foundType.Name + "." + fullTypeName;
            }
            while (charIndex < scintillaControl1.Length)
            {
                int thisChar = scintillaControl1.GetCharAt(charIndex);
                if (Char.IsLetterOrDigit((char)thisChar) || (thisChar == '_'))
                {
                    fullTypeName += (char)thisChar;
                }
                else
                {
                    break;
                }
                charIndex++;
            }

            return fullTypeName;
        }

        public void OnSavePointLeft(object sender, EventArgs e)
        {
            if (IsModifiedChanged != null)
            {
                IsModifiedChanged(sender, e);
            }
        }

        public void OnSavePointReached(object sender, EventArgs e)
        {
            if (IsModifiedChanged != null)
            {
                IsModifiedChanged(sender, e);
            }
        }

        public void ShowMatchingBrace(bool beforeAndAfterCursor)
        {
            ShowMatchingBrace(beforeAndAfterCursor, false);
        }

		public void ShowMatchingBrace(bool beforeAndAfterCursor, bool alignIndentation)
		{
			int currentPos = scintillaControl1.CurrentPos - 1;
			int matchPos = scintillaControl1.BraceMatch(currentPos);
			if ((matchPos < 0) && (beforeAndAfterCursor))
			{
				// try the following character instead
				currentPos++;
				matchPos = scintillaControl1.BraceMatch(currentPos);
			}
			if (matchPos >= 0)
			{
                if (alignIndentation)
                {
                    AlignIndentation(currentPos, matchPos);
                    currentPos = scintillaControl1.CurrentPos - 1;
                }
                scintillaControl1.BraceHighlight(matchPos, currentPos);
			}
			else
			{
				scintillaControl1.BraceBadLight(currentPos);
			}
			_braceMatchVisible = true;
			_doBraceMatch = false;
		}

        private void AlignIndentation(int posToAlign, int posToAlignWith)
        {
            int lineToAlign = scintillaControl1.LineFromPosition(posToAlign);
            int lineToAlignWith = scintillaControl1.LineFromPosition(posToAlignWith);
            int indentOfPosToAlignWith = scintillaControl1.GetLineIndentation(lineToAlignWith);
            scintillaControl1.SetLineIndentation(lineToAlign, indentOfPosToAlignWith);
        }

        private void OnUpdateUI(object sender, EventArgs e)
        {
            if (_braceMatchVisible)
            {
                scintillaControl1.BraceBadLight(INVALID_POSITION);
                _braceMatchVisible = false;
            }

            if (_doBraceMatch)
            {
                ShowMatchingBrace(false, _autoDedentClosingBrace);
            }

            if (_doShowAutocomplete)
            {
                if (_autoCompleteEnabled)
                {
                    ShowAutoCompleteIfAppropriate(AUTOCOMPLETE_MINIMUM_WORD_LENGTH);
                }
                _doShowAutocomplete = false;
            }

            if (_doCalltip)
            {
                if ((_callTipsEnabled) && (!IgnoringCurrentLine()))
                {
                    ShowFunctionCalltip();
                }
                _doCalltip = false;
            }

            if (UpdateUI != null)
            {
                UpdateUI(this, null);
            }
        }

        private void OnCharAdded(object sender, Scintilla.CharAddedEventArgs e)
        {
            // Reset to normal fillups
            this.scintillaControl1.AutoCSetFillups(_fillupKeys);

            if (e.Ch == 10)
            {
                int lineNumber = scintillaControl1.LineFromPosition(scintillaControl1.CurrentPos);
                if (lineNumber > 0)
                {
                    int previousLineIndent = scintillaControl1.GetLineIndentation(lineNumber - 1);
                    string previousLine = scintillaControl1.GetLine(lineNumber - 1).Trim('\r', '\n', '\0');
                    if (previousLine.EndsWith("{"))
                    {
                        previousLineIndent += scintillaControl1.TabWidth;
                    }
                    /*else if (previousLine.EndsWith("}"))
                    {
                        previousLineIndent -= scintillaControl1.TabWidth;
                        if (previousLineIndent < 0) previousLineIndent = 0;
                        if (_autoDedentClosingBrace)
                        {
                            scintillaControl1.SetLineIndentation(lineNumber - 1, previousLineIndent);
                        }
                    }*/
                    scintillaControl1.SetLineIndentation(lineNumber, previousLineIndent);
                    scintillaControl1.GotoPos(scintillaControl1.GetLineIndentationPosition(lineNumber));
                }
            }
			// The following events must be piped to the UpdateUI event,
			// otherwise they don't work properly
			else if ((e.Ch == '}') || (e.Ch == ')'))
			{
				if (!InsideStringOrComment(true))
				{
					_doBraceMatch = true;
				}

				if (scintillaControl1.IsCallTipActive)
				{
					scintillaControl1.CallTipCancel();
				}
			}
			else if ((e.Ch == '(') || (e.Ch == ','))
			{
				if ((e.Ch == ',') && (!InsideStringOrComment(true)) &&
					(_autoSpaceAfterComma))
				{
					scintillaControl1.AddText(" ");
				}

				_doCalltip = true;
			}
			else if ((e.Ch == '.') && (!scintillaControl1.IsAutoCActive))
			{
				_doShowAutocomplete = true;
			}
			else if (((Char.IsLetterOrDigit(e.Ch)) || (e.Ch == '_') || (e.Ch == ' ')) && (!scintillaControl1.IsAutoCActive))
			{
				_doShowAutocomplete = true;
			}

            if (CharAdded != null)
            {
                CharAdded(e.Ch);
            }
        }

        private void scintillaControl1_TextModified(object sender, Scintilla.TextModifiedEventArgs e)
        {
            if (TextModified != null)
            {
                TextModified(e.Position, e.Length, (e.ModificationType & 1) != 0);
            }
        }

        private void OnModifyAttemptOnReadOnly(object sender, EventArgs e)
        {
            if (AttemptModify != null)
            {
                bool allowModify = true;
                AttemptModify(ref allowModify);
                if (allowModify)
                {
                    this.scintillaControl1.IsReadOnly = false;
                }
            }
            else
            {
                this.scintillaControl1.IsReadOnly = false;
            }
        }

        private void scintillaControl1_DwellEnd(object sender, EventArgs e)
        {
            if (_dwellCalltipVisible)
            {
                scintillaControl1.CallTipCancel();
                _dwellCalltipVisible = false;
            }
        }

        private void scintillaControl1_DwellStart(object sender, Scintilla.DwellStartEventArgs e)
        {
            if ((_callTipsEnabled) && (e.Position > 0) &&
                (!scintillaControl1.IsCallTipActive) &&
                (!scintillaControl1.IsAutoCActive) &&
                (!InsideStringOrComment(false, e.Position)) &&
                _activated && !TabbedDocumentManager.HoveringTabs
                && !ScriptEditor.HoveringCombo)
            {
                ShowCalltip(FindEndOfCurrentWord(e.Position), -1, false);
                _dwellCalltipVisible = true;
            }
        }

        private int FindEndOfCurrentWord(int currentPos)
        {
            int pos = currentPos;
            while (true)
            {
                char theChar = (char)scintillaControl1.GetCharAt(pos);
                if ((!Char.IsLetterOrDigit(theChar)) && (theChar != '_'))
                {
                    pos--;
                    break;
                }
                pos++;
            }
            return pos;
        }

        private string GetPreviousPathedExpression(int startAtPos, bool skipSpacesBeforeCursor)
        {
			bool startedParsingExpression = !skipSpacesBeforeCursor;
            if (startAtPos < 0) startAtPos = 0;
            int cursorPos = startAtPos;

            while (cursorPos > 0)
            {
                int thisChar = scintillaControl1.GetCharAt(cursorPos);
                if (thisChar == ']')
                {
                    int bracketDepth = 1;
                    cursorPos--;
                    while ((bracketDepth > 0) && (cursorPos > 0))
                    {
                        int checkChar = scintillaControl1.GetCharAt(cursorPos);
                        if (checkChar == ']') bracketDepth++;
                        if (checkChar == '[') bracketDepth--;
                        if ((checkChar == '\n') || (checkChar == '\r'))
                        {
                            cursorPos++;
                            break;
                        }
                        cursorPos--;
                    }
					startedParsingExpression = true;
                }
                else if ((thisChar == '.') || (thisChar == '_') ||
                         (Char.IsLetterOrDigit((char)thisChar)))
                {
                    cursorPos--;
					startedParsingExpression = true;
                }
				else if ((thisChar == ' ') && (!startedParsingExpression))
				{
					// if they put a space between function name and parameter
					// list, skip back over it
					cursorPos--;
				}
				else
				{
					cursorPos++;
					break;
				}

				if (cursorPos < startAtPos - 200)
				{
					// there is probably some invalid syntax like too many
					// closing brackets, so abort
					return string.Empty;
				}
            }
			if (cursorPos == 0)
			{
				// parse error
				return string.Empty;
			}
            if (startAtPos < cursorPos)
            {
                return string.Empty;
            }
            int lineNumber = scintillaControl1.LineFromPosition(cursorPos);
            int startOfLine = scintillaControl1.PositionFromLine(lineNumber);
            string lineText = scintillaControl1.GetLine(lineNumber);
            int startAtIndex = cursorPos - startOfLine;

            if (startAtIndex >= lineText.Length)
            {
                // can happen if the cursor is at the end of the line
                return string.Empty;
            }
            return lineText.Substring(startAtIndex, (startAtPos - cursorPos) + 1).Trim();
        }

        private string GetPreviousWord()
        {
            int cursorPos = scintillaControl1.CurrentPos;
            int lineNumber = scintillaControl1.LineFromPosition(cursorPos);
            int startOfLine = scintillaControl1.PositionFromLine(lineNumber);
            string lineText = scintillaControl1.GetLine(lineNumber);
            int offset = (cursorPos - startOfLine) - 1;

            if ((offset > 0) && (lineText[offset] == '.'))
            {
                offset--;
            }

            int numChars = 0;
            while (offset >= 0)
            {
                if ((lineText[offset] == '_') || Char.IsLetterOrDigit(lineText, offset))
                {
                    offset--;
                    numChars++;
                }
                else
                {
                    break;
                }
            }
            offset++;

            return lineText.Substring(offset, numChars);
        }

        public ScriptStruct FindGlobalVariableOrType(string type)
        {
            bool dummy = false;
            return FindGlobalVariableOrType(type, ref dummy);
        }

        private IList<IScript> GetAutoCompleteScriptList()
        {
            var scriptListConcrete = Factory.AGSEditor.GetAllScriptHeaders();
            IList<IScript> scriptList = new List<IScript>();
            foreach (Script script in scriptListConcrete)
            {
                scriptList.Add(script);
            }

            if (_autoCompleteForThis != null && 
                _autoCompleteForThis.AutoCompleteData != null)
            {
                scriptList.Add(_autoCompleteForThis);
            }
            return scriptList;
        }

        private ScriptStruct FindGlobalVariableOrType(string type, ref bool staticAccess)
        {
            foreach (IScript script in GetAutoCompleteScriptList())
            {
                foreach (ScriptStruct structDef in script.AutoCompleteData.Structs)
                {
                    if ((structDef.Name == type) && (structDef.FullDefinition))
                    {
                        staticAccess = true;
                        return structDef;
                    }
                }
                foreach (ScriptVariable varDef in script.AutoCompleteData.Variables)
                {
                    if (varDef.VariableName == type)
                    {
                        staticAccess = false;
                        return FindGlobalVariableOrType(varDef.Type);
                    }
                }
            }

            staticAccess = false;
            return null;
        }

        private string ReadNextWord(ref string pathedExpression)
        {
            string thisWord = string.Empty;
            while ((pathedExpression.Length > 0) && 
                ((Char.IsLetterOrDigit(pathedExpression[0])) || (pathedExpression[0] == '_')))
            {
                thisWord += pathedExpression[0];
                pathedExpression = pathedExpression.Substring(1);
            }
            if ((pathedExpression.Length > 0) && (pathedExpression[0] == '['))
            {
                int bracketDepth = 1;
                int cursorPos = 1;
                while ((bracketDepth > 0) && (cursorPos < pathedExpression.Length))
                {
                    int checkChar = pathedExpression[cursorPos];
                    if (checkChar == ']') bracketDepth--;
                    if (checkChar == '[') bracketDepth++;
                    cursorPos++;
                }
                pathedExpression = pathedExpression.Substring(cursorPos);
            }
            if ((pathedExpression.Length > 0) && (pathedExpression[0] == '.'))
            {
                pathedExpression = pathedExpression.Substring(1);
            }
            return thisWord;
        }

        private string FindTypeForThis(int startAtPos)
        {
            if (_fixedTypeForThisKeyword != null)
            {
                return _fixedTypeForThisKeyword;
            }
            int lineNumber = scintillaControl1.LineFromPosition(startAtPos) - 1;
            int searchBackUntil = (lineNumber > SCAN_BACK_DISTANCE) ? lineNumber - SCAN_BACK_DISTANCE : 0;
            while (lineNumber > searchBackUntil)
            {
                string lineText = scintillaControl1.GetLine(lineNumber);
                int structNameEnd = lineText.IndexOf("::");
                if (structNameEnd >= 0)
                {
                    int structNameStart = structNameEnd - 1;
                    while ((structNameStart >= 0) && (Char.IsLetterOrDigit(lineText[structNameStart]) || (lineText[structNameStart] == '_')))
                    {
                        structNameStart--;
                    }
                    structNameStart++;
                    return lineText.Substring(structNameStart, structNameEnd - structNameStart);
                }
                int extenderFunction = lineText.IndexOf("(this ");
                if (extenderFunction >= 0)
                {
                    int structNameStart = extenderFunction + 5;
                    while (lineText[structNameStart] == ' ')
                    {
                        structNameStart++;
                    }
                    structNameEnd = structNameStart + 1;
                    while (Char.IsLetterOrDigit(lineText[structNameEnd]))
                    {
                        structNameEnd++;
                    }
                    return lineText.Substring(structNameStart, structNameEnd - structNameStart);
                }
                if (lineText.IndexOf("function ") >= 0)
                {
                    // we're in a function that wasn't a struct member
                    break;
                }
                lineNumber--;
            }
            return string.Empty;
        }

        private ScriptStruct ParsePreviousExpression(int startAtPos, out string charactersAfterDot, out bool staticAccess, out bool isThis)
        {
            isThis = false;

            string pathedExpression = GetPreviousPathedExpression(startAtPos, true);
            // strip off anything after the last dot (since that's what they're typing now)
            int lastIndex = pathedExpression.LastIndexOf('.');
            charactersAfterDot = string.Empty;
            if (lastIndex >= 0)
            {
                charactersAfterDot = pathedExpression.Substring(lastIndex + 1);
                pathedExpression = pathedExpression.Substring(0, lastIndex);
            }
            else
            {
                charactersAfterDot = pathedExpression;
                pathedExpression = string.Empty;
            }

            string thisWord = ReadNextWord(ref pathedExpression);
            staticAccess = false;
            ScriptStruct foundType;

            if (thisWord == THIS_STRUCT)
            {
                thisWord = FindTypeForThis(startAtPos);
                foundType = FindGlobalVariableOrType(thisWord, ref staticAccess);
                isThis = true;
                // force this to false for the "this" variable
                staticAccess = false;
            }
            else
            {
                foundType = FindGlobalVariableOrType(thisWord, ref staticAccess);
                if ((foundType == null) && (thisWord.Length > 0))
                {
                    ScriptVariable var = FindLocalVariableWithName(startAtPos, thisWord);
                    if (var != null)
                    {
                        foundType = FindGlobalVariableOrType(var.Type);
                    }
                }
            }

            while ((pathedExpression.Length > 0) && (foundType != null))
            {
                thisWord = ReadNextWord(ref pathedExpression);
                ScriptVariable memberVar = foundType.FindMemberVariable(thisWord);
                if (memberVar != null)
                {
                    foundType = FindGlobalVariableOrType(memberVar.Type);
                    staticAccess = false;
                }
                else
                {
                    foundType = null;
                }
            }
            return foundType;
        }

        private void ShowStructMemberAutoComplete()
        {
            string charactersAfterDot;
            bool staticAccess;
            bool isThis;

            ScriptStruct foundType = ParsePreviousExpression(scintillaControl1.CurrentPos - 1, out charactersAfterDot, out staticAccess, out isThis);

            if (foundType != null)
            {
                ShowAutoComplete(charactersAfterDot.Length, ConstructScintillaAutocompleteList(GetAllStructsWithMatchingName(foundType.Name), staticAccess, isThis, null));
            }
        }

        private List<ScriptStruct> GetAllStructsWithMatchingName(string structName)
        {
            List<ScriptStruct> matchingTypes = new List<ScriptStruct>();
            foreach (IScript script in GetAutoCompleteScriptList())
            {
                foreach (ScriptStruct structDef in script.AutoCompleteData.Structs)
                {
                    if (structDef.Name == structName)
                    {
                        matchingTypes.Add(structDef);
                    }
                }
            }
            return matchingTypes;
        }

        private void ShowAutoComplete(int charsTyped, string autoCompleteList)
        {
            if (autoCompleteList.Length > 0)
            {
                scintillaControl1.StyleSetFont((int)Cpp.GlobalDefault, USER_FRIENDLY_FONT);
                scintillaControl1.StyleSetFontSize((int)Cpp.GlobalDefault, USER_FRIENDLY_FONT_SIZE);

                scintillaControl1.AutoCShow(charsTyped, autoCompleteList);

                scintillaControl1.StyleSetFont((int)Cpp.GlobalDefault, DEFAULT_FONT);
                scintillaControl1.StyleSetFontSize((int)Cpp.GlobalDefault, DEFAULT_FONT_SIZE);
            }
        }

        private bool CheckForAndShowEnumAutocomplete(int checkAtPos)
        {
            if (this.scintillaControl1.GetCharAt(checkAtPos) == ' ')
            {
                // potential enum autocomplete
                bool atLeastOneEquals = false;
                checkAtPos--;
                while ((this.scintillaControl1.GetCharAt(checkAtPos) == ' ') ||
                       (this.scintillaControl1.GetCharAt(checkAtPos) == '='))
                {
                    if (this.scintillaControl1.GetCharAt(checkAtPos) == '=')
                    {
                        atLeastOneEquals = true;
                    }
                    checkAtPos--;
                }

                if (atLeastOneEquals)
                {
                    ScriptStruct structType;
                    ScriptToken token = GetFinalPartOfExpression(checkAtPos, out structType, false);
                    if (token != null)
                    {
                        string checkForType = null;
                        if (token is ScriptVariable)
                        {
                            checkForType = ((ScriptVariable)token).Type;
                        }
                        else if (token is ScriptFunction)
                        {
                            checkForType = ((ScriptFunction)token).Type;
                        }

                        if (ShowAutoCompleteForEnum(checkForType))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

		/// <summary>
		/// Checks whether the cursor is inside a string literal or a comment.
		/// </summary>
		/// <param name="charJustAdded">Set to true if calling from OnCharAdded,
		/// because the new character won't have any formatting yet.</param>
		private bool InsideStringOrComment(bool charJustAdded)
		{
			return InsideStringOrComment(charJustAdded, this.scintillaControl1.CurrentPos);
		}

		public bool InsideStringOrComment(bool charJustAdded, int position)
		{
			Cpp style = (Cpp)this.scintillaControl1.GetStyleAt(position - (charJustAdded ? 2 : 1));
			if ((style == Cpp.CommentLine) || (style == Cpp.Comment) ||
				(style == Cpp.CommentDoc) || (style == Cpp.CommentLineDoc) || 
				(style == Cpp.String))
			{
				return true;
			}

			int lineNumber = this.scintillaControl1.LineFromPosition(position);
			int lineStart = this.scintillaControl1.PositionFromLine(lineNumber);
            string curLine = this.scintillaControl1.GetLine(lineNumber);
            if (curLine.Length > 0)
            {
                int length = position - lineStart;
                if (length >= curLine.Length)
                {
                    length = curLine.Length - 1;
                }
                curLine = curLine.Substring(0, length);
            }
			if (curLine.IndexOf('"') >= 0)
			{
				int curIndex = 0;
				int numSpeechMarks = 0;
				while ((curIndex = curLine.IndexOf('"', curIndex)) >= 0)
				{
					numSpeechMarks++;
					curIndex++;
				}
				if (numSpeechMarks % 2 == 1)
				{
					// in a string literal
					return true;
				}
			}

			return false;
		}

        private bool IgnoringCurrentLine()
        {
            if (_ignoreLinesWithoutIndent)
            {
                int lineNumber = scintillaControl1.LineFromPosition(scintillaControl1.CurrentPos);
                int lineIndent = scintillaControl1.GetLineIndentation(lineNumber);
                if (lineIndent == 0)
                {
                    return true;
                }
            }
            return false;
        }

        private void ShowAutoCompleteIfAppropriate(int minimumLength)
        {
			if ((InsideStringOrComment(false)) || (IgnoringCurrentLine()))
			{
				return;
			}

            int checkAtPos = this.scintillaControl1.CurrentPos - 1;
            if (CheckForAndShowEnumAutocomplete(checkAtPos))
            {
                return;
            }

            string pathedExpression = GetPreviousPathedExpression(checkAtPos, false);
            if (pathedExpression.IndexOf('.') >= 0)
            {
                ShowStructMemberAutoComplete();
            }
            else
            {
                string previousWord = GetPreviousWord();
                if ((previousWord.Length >= minimumLength) && 
                    ((previousWord.Length == 0) || (!Char.IsDigit(previousWord[0]))))
                {
					string needMatch = previousWord;
					if (minimumLength == 0)
					{
						needMatch = null;
					}
					ShowAutoComplete(previousWord.Length, ConstructScintillaAutocompleteList(null, false, false, needMatch));
                }
            }
        }

        private ScriptFunction FindGlobalFunction(string name)
        {
            foreach (IScript script in GetAutoCompleteScriptList())
            {
                foreach (ScriptFunction func in script.AutoCompleteData.Functions)
                {
                    if (func.FunctionName == name)
                    {
                        return func;
                    }
                }
            }
            return null;
        }

        private int FindStartOfFunctionCall(int currentPos, out int parameterIndex)
        {
            int bracketDepth = 0;
            parameterIndex = 0;
			if ((char)this.scintillaControl1.GetCharAt(currentPos) == ')')
			{
				// if they already have   Func(1);  and insert a comma after 
				// the 1, don't count it as opening a new sub-function
				currentPos--;
			}

			int charsCounted = 0;
            while (currentPos > 0)
            {
                char thisChar = (char)this.scintillaControl1.GetCharAt(currentPos);
                if ((thisChar == '(') && (bracketDepth == 0))
                {
                    break;
                }
                if (thisChar == '(') bracketDepth--;
                if (thisChar == ')') bracketDepth++;
                if ((thisChar == ',') && (bracketDepth == 0)) parameterIndex++;
                currentPos--;
				charsCounted++;

				if (charsCounted > 100)
				{
					// not inside function parmaeters
					return -1;
				}
            }
            return currentPos;
        }

        private ScriptToken GetFinalPartOfExpression(int currentPos, out ScriptStruct memberOfStruct, bool functionsOnly)
        {
            string charactersAfterDot;
            bool staticAccess;
            bool isThis;

            ScriptStruct foundType = ParsePreviousExpression(currentPos, out charactersAfterDot, out staticAccess, out isThis);
            memberOfStruct = foundType;

            ScriptToken functionTyped = null;
            if (charactersAfterDot.IndexOf('(') > 0)
            {
                charactersAfterDot = charactersAfterDot.Substring(0, charactersAfterDot.IndexOf('('));
            }

            if (foundType != null)
            {
                foreach (ScriptStruct thisStruct in GetAllStructsWithMatchingName(foundType.Name))
                {
                    functionTyped = thisStruct.FindMemberFunction(charactersAfterDot);

                    if ((!functionsOnly) && (functionTyped == null))
                    {
                        functionTyped = thisStruct.FindMemberVariable(charactersAfterDot);
                    }

                    if (functionTyped != null)
                    {
                        break;
                    }
                }
            }
            else
            {
                functionTyped = FindGlobalFunction(charactersAfterDot);

                if (functionTyped == null)
                {
                    functionTyped = FindLocalVariableWithName(currentPos, charactersAfterDot);
                }
            }
            return functionTyped;
        }

        private void ShowFunctionCalltip()
        {
            int parameterIndex;
            int currentPos = FindStartOfFunctionCall(this.scintillaControl1.CurrentPos, out parameterIndex) - 1;
            if (currentPos > 0)
            {
                ShowCalltip(currentPos, parameterIndex, true);
            }
        }

        private void ShowCalltip(int openingBracketPos, int parameterIndex, bool functionsOnly)
        {
            ScriptStruct foundType;

            ScriptToken tokenFound = GetFinalPartOfExpression(openingBracketPos, out foundType, functionsOnly);
            if (tokenFound != null)
            {
                int nameLength = 0;
                if (tokenFound is ScriptFunction)
                {
                    nameLength = ((ScriptFunction)tokenFound).FunctionName.Length;
                }
                else if (tokenFound is ScriptVariable)
                {
                    nameLength = ((ScriptVariable)tokenFound).VariableName.Length;
                }
                
                ConstructAndShowCalltip((openingBracketPos - nameLength) + 1, parameterIndex, foundType, tokenFound);
            }
        }

        private string ConstructFunctionCalltipText(ScriptFunction func, ScriptStruct owningStruct, int selectedParameter, out int selectionStart, out int selectionEnd)
        {
            string callTip = func.Type + " ";
            if (owningStruct != null)
            {
                callTip += owningStruct.Name + ".";
            }
            callTip += func.FunctionName + "(";

            selectionStart = 0;
            selectionEnd = 0;

            string[] paramList = func.ParamList.Split(',');
            for (int i = 0; i < paramList.Length; i++)
            {
                string thisParam = paramList[i].Trim();
                if (thisParam.IndexOf('=') > 0)
                {
                    thisParam = "optional " + thisParam.Substring(0, thisParam.IndexOf('='));
                }
                if (i == selectedParameter)
                {
                    selectionStart = callTip.Length;
                    selectionEnd = callTip.Length + thisParam.Length;

                    if (ShowEnumForParameterIfAppropriate(thisParam))
                    {
                        // showing enum list rather than calltip
                        return null;
                    }
                }
                callTip += thisParam;
                if (i < paramList.Length - 1)
                {
                    callTip += ", ";
                }
            }

            callTip += ")";
            return callTip;
        }

        private string ConstructVariableCalltipText(ScriptVariable variable, ScriptStruct owningStruct)
        {
            string callTip = variable.Type;
            if (variable.IsArray)
            {
                callTip += "[ ]";
            }
            callTip += " ";
            if (owningStruct != null)
            {
                callTip += owningStruct.Name + ".";
            }
            callTip += variable.VariableName;
            return callTip;
        }

        private void ConstructAndShowCalltip(int currentPos, int selectedParameter, ScriptStruct owningStruct, ScriptToken func)
        {
            int selectionStart = 0, selectionEnd = 0;
            string callTip;

            if (func is ScriptFunction)
            {
                callTip = ConstructFunctionCalltipText((ScriptFunction)func, owningStruct, selectedParameter, out selectionStart, out selectionEnd);
            }
            else
            {
                callTip = ConstructVariableCalltipText((ScriptVariable)func, owningStruct);
            }

            if (callTip != null)
            {
                if (!string.IsNullOrEmpty(func.Description))
                {
                    callTip += "\n" + func.Description;
                }

                this.scintillaControl1.CallTipShow(currentPos, callTip);
                this.scintillaControl1.CallTipSetHlt(selectionStart, selectionEnd);
            }
        }

        private bool ShowEnumForParameterIfAppropriate(string parameter)
        {
            if (parameter.StartsWith("optional "))
            {
                parameter = parameter.Substring(9);
            }

            int spaceLocation = parameter.IndexOf(' ');
            if (spaceLocation > 0)
            {
                parameter = parameter.Substring(0, spaceLocation);
            }

            return ShowAutoCompleteForEnum(parameter);
        }

        private bool ShowAutoCompleteForEnum(string typeName)
        {
            foreach (IScript script in GetAutoCompleteScriptList())
            {
                foreach (ScriptEnum scEnum in script.AutoCompleteData.Enums)
                {
                    if (scEnum.Name == typeName)
                    {
                        List<string> enumMembers = new List<string>();
                        AddEnumValuesToAutocompleteList(enumMembers, scEnum);
                        ShowAutoComplete(0, ConvertAutocompleteListToScintillaFormat(enumMembers));
                        return true;
                    }
                }
            }

            return false;
        }

        private bool ShouldShowThis(ScriptToken token)
        {
            Settings gameSettings = Factory.AGSEditor.CurrentGame.Settings;
            if ((token.IfNDefOnly == "STRICT") && (gameSettings.EnforceObjectBasedScript))
            {
                return false;
            }
            if ((token.IfDefOnly == "STRICT") && (!gameSettings.EnforceObjectBasedScript))
            {
                return false;
            }
            if ((token.IfNDefOnly == "STRICT_STRINGS") && (gameSettings.EnforceNewStrings))
            {
                return false;
            }
            if ((token.IfDefOnly == "STRICT_STRINGS") && (!gameSettings.EnforceNewStrings))
            {
                return false;
            }
            if ((token.IfNDefOnly == "STRICT_AUDIO") && (gameSettings.EnforceNewAudio))
            {
                return false;
            }
            if ((token.IfDefOnly == "STRICT_AUDIO") && (!gameSettings.EnforceNewAudio))
            {
                return false;
            }
            return true;
        }

        private void AddEnumValuesToAutocompleteList(List<string> list, ScriptEnum se)
        {
            foreach (string enumValue in se.EnumValues)
            {
                list.Add(enumValue + "?" + IMAGE_INDEX_ENUM);
            }
        }

        private void AddGlobalsFromScript(List<string> globalsList, IScript script, Dictionary<string, object> addedNames, int onlyShowIfDefinitionBeforePos)
        {
            foreach (ScriptVariable sv in script.AutoCompleteData.Variables)
            {
                if (!addedNames.ContainsKey(sv.VariableName))
                {
                    if (ShouldShowThis(sv))
                    {
                        globalsList.Add(sv.VariableName + "?" + IMAGE_INDEX_GLOBAL_VARIABLE);
                        addedNames.Add(sv.VariableName, null);
                    }
                }
            }
            foreach (ScriptFunction sf in script.AutoCompleteData.Functions)
            {
                if (!addedNames.ContainsKey(sf.FunctionName))
                {
                    if ((ShouldShowThis(sf)) && 
                        (sf.StartsAtCharacterIndex < onlyShowIfDefinitionBeforePos) &&
                        (!sf.HideOnMainFunctionList))
                    {
                        globalsList.Add(sf.FunctionName + "?" + IMAGE_INDEX_METHOD);
                        addedNames.Add(sf.FunctionName, null);
                    }
                }
            }
            foreach (ScriptDefine sd in script.AutoCompleteData.Defines)
            {
                if (ShouldShowThis(sd))
                {
                    globalsList.Add(sd.Name + "?" + IMAGE_INDEX_DEFINE);
                }
            }
            foreach (ScriptEnum se in script.AutoCompleteData.Enums)
            {
                if (ShouldShowThis(se))
                {
                    AddEnumValuesToAutocompleteList(globalsList, se);
                }
            }
            foreach (ScriptStruct ss in script.AutoCompleteData.Structs)
            {
                if ((ShouldShowThis(ss)) && (ss.FullDefinition))
                {
                    globalsList.Add(ss.Name + "?" + IMAGE_INDEX_STRUCT);
                }
            }
        }

        private void AddMembersOfStruct(List<string> autoCompleteList, List<ScriptStruct> scriptStructs, bool staticOnly, bool isThis)
        {
            Dictionary<string, object> alreadyAdded = new Dictionary<string, object>();

            foreach (ScriptStruct scriptStruct in scriptStructs)
            {
                foreach (ScriptFunction sf in scriptStruct.Functions)
                {
                    if (((sf.IsStatic) || (!staticOnly)) &&
                        ((!sf.IsStaticOnly) || (staticOnly)) &&
                        ((!sf.IsProtected) || (isThis)) &&
                        ShouldShowThis(sf) &&
                        !alreadyAdded.ContainsKey(sf.FunctionName))
                    {
                        int imageIndex = IMAGE_INDEX_METHOD;
                        if (sf.IsStatic)
                        {
                            imageIndex = IMAGE_INDEX_STATIC_METHOD;
                        }
                        else if (sf.IsExtenderMethod)
                        {
                            imageIndex = IMAGE_INDEX_EXTENDER_METHOD;
                        }
                        autoCompleteList.Add(sf.FunctionName + "?" + imageIndex);
                        alreadyAdded.Add(sf.FunctionName, null);
                    }
                }
                foreach (ScriptVariable sv in scriptStruct.Variables)
                {
                    if (((sv.IsStatic) || (!staticOnly)) &&
                        ((!sv.IsStaticOnly) || (staticOnly)) &&
                        ((!sv.IsProtected) || (isThis)) &&
                        ShouldShowThis(sv))
                    {
                        autoCompleteList.Add(sv.VariableName + "?" + (sv.IsStatic ? IMAGE_INDEX_STATIC_PROPERTY : IMAGE_INDEX_PROPERTY));
                    }
                }
            }
        }

        private void AddFunctionParametersToVariableList(ScriptFunction func, List<ScriptVariable> variables)
        {
            if (func.ParamList.Length == 0)
            {
                return;
            }
            string[] parameters = func.ParamList.Split(',');
            foreach (string thisParam in parameters)
            {
                string param = thisParam.Trim();
                if (param.StartsWith("optional "))
                {
                    param = param.Substring(9).Trim();
                }
                int index = param.Length - 1;
                while ((index >= 0) && 
                       (Char.IsLetterOrDigit(param[index]) || param[index] == '_'))
                {
                    index--;
                }
                string paramName = param.Substring(index + 1);
                string paramType = param.Substring(0, index + 1).Trim();
                bool isPointer = false;
                if (paramType.EndsWith("*"))
                {
                    isPointer = true;
                    paramType = paramType.Substring(0, paramType.Length - 1).Trim();
                }
                if ((paramName.Length > 0) && (paramType.Length > 0))
                {
                    variables.Add(new ScriptVariable(paramName, paramType, false, isPointer, null, null, false, false, false, false, func.StartsAtCharacterIndex));
                }
            }
        }

        private ScriptVariable FindLocalVariableWithName(int startAtPos, string nameToFind)
        {
            List<ScriptVariable> localVars = GetListOfLocalVariablesForCurrentPosition(false, startAtPos);
            foreach (ScriptVariable var in localVars)
            {
                if (var.VariableName == nameToFind)
                {
                    return var;
                }
            }
            return null;
        }

        public ScriptFunction FindFunctionAtCurrentPosition()
        {
            int currentPos = this.scintillaControl1.CurrentPos;
            string scriptExtract = scintillaControl1.GetText();
            ScriptFunction func = _autoCompleteForThis.AutoCompleteData.Functions.Find(
                c => c.StartsAtCharacterIndex <= currentPos && c.EndsAtCharacterIndex >= currentPos);
            return func;
        }

        public List<ScriptVariable> GetListOfLocalVariablesForCurrentPosition(bool searchWholeFunction)
        {
            return GetListOfLocalVariablesForCurrentPosition(searchWholeFunction, this.scintillaControl1.CurrentPos);
        }

        public List<ScriptVariable> GetListOfLocalVariablesForCurrentPosition(bool searchWholeFunction, int currentPos)
        {
            List<ScriptVariable> toReturn;

            if (_autoCompleteForThis != null && _autoCompleteForThis.AutoCompleteData != null)
            {
                string scriptExtract = scintillaControl1.GetText();
                foreach (ScriptFunction func in _autoCompleteForThis.AutoCompleteData.Functions)
                {
                    toReturn = CheckFunctionForLocalVariables(currentPos, func, scriptExtract, searchWholeFunction);
                    if (toReturn != null)
                    {
                        return toReturn;
                    }
                }

                foreach (ScriptStruct struc in _autoCompleteForThis.AutoCompleteData.Structs)
                {
                    foreach (ScriptFunction func in struc.Functions)
                    {
                        toReturn = CheckFunctionForLocalVariables(currentPos, func, scriptExtract, searchWholeFunction);
                        if (toReturn != null)
                        {
                            return toReturn;
                        }
                    }
                }
            }
            return new List<ScriptVariable>();
        }

        private List<ScriptVariable> CheckFunctionForLocalVariables(int currentPos, ScriptFunction func, string scriptExtract, bool searchWholeFunction)
        {
            if ((func.EndsAtCharacterIndex > currentPos) &&
                (func.StartsAtCharacterIndex >= 0))
            {
                if ((scriptExtract.Length > currentPos) &&
                    (currentPos > func.StartsAtCharacterIndex))
                {
                    int startPos = func.StartsAtCharacterIndex;
                    int endPos = searchWholeFunction ? func.EndsAtCharacterIndex :
                        currentPos;
                    scriptExtract = scriptExtract.Substring(func.StartsAtCharacterIndex, (endPos - func.StartsAtCharacterIndex));
                    int openBracketOffset = scriptExtract.IndexOf("{");
                    if (openBracketOffset > 0)
                    {
                        startPos += openBracketOffset;
                        scriptExtract = scriptExtract.Substring(openBracketOffset);
                    }
                    List<ScriptVariable> localVars = AutoComplete.GetLocalVariableDeclarationsFromScriptExtract(scriptExtract, startPos);
                    AddFunctionParametersToVariableList(func, localVars);
                    return localVars;
                }
            }
            return null;
        }

        private string ConstructScintillaAutocompleteList(List<ScriptStruct> onlyMembersOf, bool staticOnly, bool isThis, string onlyIfMatchForThis)
        {
            List<string> globalsList = new List<string>();

            if (onlyMembersOf != null)
            {
                AddMembersOfStruct(globalsList, onlyMembersOf, staticOnly, isThis);
            }
            else
            {
                foreach (string keyword in _keywords)
                {
                    globalsList.Add(keyword);
                }

                Dictionary<string, object> addedNames = new Dictionary<string, object>();

                foreach (IScript script in GetAutoCompleteScriptList())
                {
                    int onlyShowIfDefinitionBeforePos = Int32.MaxValue;
                    if (script == _autoCompleteForThis)
                    {
                        onlyShowIfDefinitionBeforePos = scintillaControl1.CurrentPos;
                    }
                    AddGlobalsFromScript(globalsList, script, addedNames, onlyShowIfDefinitionBeforePos);
                }

                List<ScriptVariable> variables = GetListOfLocalVariablesForCurrentPosition(false);
                foreach (ScriptVariable var in variables)
                {
                    globalsList.Add(var.VariableName + "?" + IMAGE_INDEX_LOCAL_VARIABLE);
                }
            }

			if (onlyIfMatchForThis != null)
			{
				onlyIfMatchForThis = onlyIfMatchForThis.ToLower();
				int matchLength = onlyIfMatchForThis.Length;
				bool foundMatch = false;
				foreach (string entry in globalsList)
				{
					if (entry.Length >= matchLength)
					{
						if (entry.Substring(0, matchLength).ToLower() == onlyIfMatchForThis)
						{
							foundMatch = true;
							break;
						}
					}
				}
				if (!foundMatch)
				{
					return string.Empty;
				}
			}

            return ConvertAutocompleteListToScintillaFormat(globalsList);
        }

        private string ConvertAutocompleteListToScintillaFormat(List<string> list)
        {
            StringBuilder sb = new StringBuilder(1000);
            list.Sort();

            foreach (string global in list)
            {
                if (sb.Length > 0)
                {
                    sb.Append(' ');
                }
                sb.Append(global);
            }
            return sb.ToString();
        }

		private void ContextMenuChooseOption(object sender, EventArgs e)
		{
			ToolStripMenuItem item = (ToolStripMenuItem)sender;
			if (item.Name == CONTEXT_MENU_CUT)
			{
				this.Cut();
			}
			else if (item.Name == CONTEXT_MENU_COPY)
			{
				this.Copy();
			}
			else if (item.Name == CONTEXT_MENU_PASTE)
			{
				this.Paste();
			}

			if (ActivateContextMenu != null)
			{
				ActivateContextMenu(item.Name);
			}
		}

		private void ScintillaWrapper_MouseUp(object sender, MouseEventArgs e)
		{
			if (e.Button == MouseButtons.Right)
			{
				int clickAtPos = scintillaControl1.PositionFromPoint(e.X, e.Y);
				if ((clickAtPos < scintillaControl1.SelectionStart) ||
					(clickAtPos > scintillaControl1.SelectionEnd))
				{
					scintillaControl1.GotoPos(clickAtPos);
				}

				EventHandler onClick = new EventHandler(ContextMenuChooseOption);
				ContextMenuStrip menu = new ContextMenuStrip();

				if (ConstructContextMenu != null)
				{
					ConstructContextMenu(menu, clickAtPos);
				}
				if (menu.Items.Count > 0)
				{
					menu.Items.Add(new ToolStripSeparator());
				}
				menu.Items.Add(new ToolStripMenuItem("Cut", Factory.GUIController.ImageList.Images["CutIcon"], onClick, CONTEXT_MENU_CUT));
				menu.Items[menu.Items.Count - 1].Enabled = this.CanCutAndCopy();
				menu.Items.Add(new ToolStripMenuItem("Copy", Factory.GUIController.ImageList.Images["CopyIcon"], onClick, CONTEXT_MENU_COPY));
				menu.Items[menu.Items.Count - 1].Enabled = this.CanCutAndCopy();
				menu.Items.Add(new ToolStripMenuItem("Paste", Factory.GUIController.ImageList.Images["PasteIcon"], onClick, CONTEXT_MENU_PASTE));
				menu.Items[menu.Items.Count - 1].Enabled = this.CanPaste();

				menu.Show(this.scintillaControl1, e.X, e.Y);
			}
		}

        public int LineCount
        {
            get { return scintillaControl1.LineCount; }
        }



        void IScriptEditorControl.ShowLineNumbers()
        {
            EnableLineNumbers();
        }

        Control IScriptEditorControl.Control
        {
            get { return this; }
        }

        string IScriptEditorControl.Text
        {
            get { return GetText(); }
            set { SetText(value); }
        }

        int IScriptEditorControl.SelectionStart
        {
            get { return scintillaControl1.SelectionStart; }
        }

        int IScriptEditorControl.SelectionEnd
        {
            get { return scintillaControl1.SelectionEnd; }
        }

        string IScriptEditorControl.SelectedText
        {
            get { return scintillaControl1.GetSelText(); }
        }

        int IScriptEditorControl.CursorPosition
        {
            get { return scintillaControl1.CurrentPos; }
        }

        int IScriptEditorControl.GetLineNumberForPosition(int position)
        {
            return scintillaControl1.LineFromPosition(position) + 1;
        }

        public string GetTextForLine(int lineNumber)
        {
            return scintillaControl1.GetLine(lineNumber - 1);
        }

        string IScriptEditorControl.GetTypeNameAtCursor()
        {
            return GetFullTypeNameAtCursor();
        }

        public void ResetSelection()
        {
            scintillaControl1.SelectionStart = 0;
            scintillaControl1.SelectionEnd = 0;
            scintillaControl1.CurrentPos = 0;
        }
    }
}
