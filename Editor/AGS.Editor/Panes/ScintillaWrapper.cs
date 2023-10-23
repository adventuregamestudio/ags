using AGS.Types;
using AGS.Types.AutoComplete;
using AGS.Types.Interfaces;
using ScintillaNET;
using AGS.Controls;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    // This class is a bit of a mess ... autocomplete is out of control!!
    public partial class ScintillaWrapper : UserControl, IScriptEditorControl
    {
        public enum WordListType
        {
            Keywords = 0,
            Keywords2 = 1,
            Documentation = 2,
            GlobalClasses = 3,
            Preprocessor = 4,
            Marker = 5,
            MaxCount
        }

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
        public event EventHandler OnBeforeShowingAutoComplete;
        public event EventHandler<ScintillaHelper.MarginClickExEventArgs> ToggleBreakpoint;
        public delegate void CharAddedHandler(int charAdded);
        public event CharAddedHandler CharAdded;
        public delegate void TextModifiedHandler(int startPos, int length, bool wasAdded);
        public event TextModifiedHandler TextModified;
        public delegate void AttemptModifyHandler(ref bool allowModify);
        public event AttemptModifyHandler AttemptModify;
        public event ConstructContextMenuHandler ConstructContextMenu;
        public event ActivateContextMenuHandler ActivateContextMenu;

        private bool _isDialogScript = false;
        // Keyword sets, grouped per type
        private List<string>[] _keywordSets = new List<string>[(int)WordListType.MaxCount];
        // Full keyword list, for convenient use
        private List<string> _keywords = new List<string>();
        // Full keyword list, dialog-specific
        private List<string> _dialogKeywords = new List<string>();
        // Autocomplete list
        private List<string> _autoCKeywords = new List<string>();

        private bool _skipBraceMatchOnce = false;
        private bool _doAlignIdentation = false;
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

        private string _scriptFont = Factory.AGSEditor.Settings.ScriptFont;
        private int _scriptFontSize = Factory.AGSEditor.Settings.ScriptFontSize;
        private string _calltipFont = Factory.AGSEditor.Settings.ScriptTipFont;
        private int _calltipFontSize = Factory.AGSEditor.Settings.ScriptTipFontSize;
        private ColorTheme _theme;

        private void UpdateColorThemeStyleDefault()
        {
            if (_theme == null) return;
            ColorTheme t = _theme;

            if (!t.Has("script-editor/text-editor")) return;
            t.SetColor("script-editor/text-editor/global-default/background", c => scintillaControl1.Styles[Style.Default].BackColor = c);
            t.SetColor("script-editor/text-editor/global-default/foreground", c => scintillaControl1.Styles[Style.Default].ForeColor = c);
        }

        private void UpdateColorTheme()
        {
            if (_theme == null) return;
            ColorTheme t = _theme;

            if (!t.Has("script-editor/text-editor")) return;
            UpdateColorThemeStyleDefault();
            t.SetColor("script-editor/text-editor/default/background", c => scintillaControl1.Styles[Style.Cpp.Default].BackColor = c);
            t.SetColor("script-editor/text-editor/default/foreground", c => scintillaControl1.Styles[Style.Cpp.Default].ForeColor = c);
            t.SetColor("script-editor/text-editor/word-1/background", c => scintillaControl1.Styles[Style.Cpp.Word].BackColor = c);
            t.SetColor("script-editor/text-editor/word-1/foreground", c => scintillaControl1.Styles[Style.Cpp.Word].ForeColor = c);
            t.SetColor("script-editor/text-editor/word-2/background", c => scintillaControl1.Styles[Style.Cpp.Word2].BackColor = c);
            t.SetColor("script-editor/text-editor/word-2/foreground", c => scintillaControl1.Styles[Style.Cpp.Word2].ForeColor = c);
            t.SetColor("script-editor/text-editor/word-2/background", c => scintillaControl1.Styles[Style.Cpp.GlobalClass].BackColor = c);
            t.SetColor("script-editor/text-editor/word-2/foreground", c => scintillaControl1.Styles[Style.Cpp.GlobalClass].ForeColor = c);
            t.SetColor("script-editor/text-editor/identifier/background", c => scintillaControl1.Styles[Style.Cpp.Identifier].BackColor = c);
            t.SetColor("script-editor/text-editor/identifier/foreground", c => scintillaControl1.Styles[Style.Cpp.Identifier].ForeColor = c);
            t.SetColor("script-editor/text-editor/comment/background", c => scintillaControl1.Styles[Style.Cpp.Comment].BackColor = c);
            t.SetColor("script-editor/text-editor/comment/foreground", c => scintillaControl1.Styles[Style.Cpp.Comment].ForeColor = c);
            t.SetColor("script-editor/text-editor/comment-line/background", c => scintillaControl1.Styles[Style.Cpp.CommentLine].BackColor = c);
            t.SetColor("script-editor/text-editor/comment-line/foreground", c => scintillaControl1.Styles[Style.Cpp.CommentLine].ForeColor = c);
            t.SetColor("script-editor/text-editor/comment-doc/background", c => scintillaControl1.Styles[Style.Cpp.CommentDoc].BackColor = c);
            t.SetColor("script-editor/text-editor/comment-doc/foreground", c => scintillaControl1.Styles[Style.Cpp.CommentDoc].ForeColor = c);
            t.SetColor("script-editor/text-editor/comment-line-doc/background", c => scintillaControl1.Styles[Style.Cpp.CommentLineDoc].BackColor = c);
            t.SetColor("script-editor/text-editor/comment-line-doc/foreground", c => scintillaControl1.Styles[Style.Cpp.CommentLineDoc].ForeColor = c);
            t.SetColor("script-editor/text-editor/comment-doc-keyword/background", c => scintillaControl1.Styles[Style.Cpp.CommentDocKeyword].BackColor = c);
            t.SetColor("script-editor/text-editor/comment-doc-keyword/foreground", c => scintillaControl1.Styles[Style.Cpp.CommentDocKeyword].ForeColor = c);
            t.SetColor("script-editor/text-editor/comment-doc-keyword-error/background", c => scintillaControl1.Styles[Style.Cpp.CommentDocKeywordError].BackColor = c);
            t.SetColor("script-editor/text-editor/comment-doc-keyword-error/foreground", c => scintillaControl1.Styles[Style.Cpp.CommentDocKeywordError].ForeColor = c);
            t.SetColor("script-editor/text-editor/number/background", c => scintillaControl1.Styles[Style.Cpp.Number].BackColor = c);
            t.SetColor("script-editor/text-editor/number/foreground", c => scintillaControl1.Styles[Style.Cpp.Number].ForeColor = c);
            t.SetColor("script-editor/text-editor/regex/background", c => scintillaControl1.Styles[Style.Cpp.Regex].BackColor = c);
            t.SetColor("script-editor/text-editor/regex/foreground", c => scintillaControl1.Styles[Style.Cpp.Regex].ForeColor = c);
            t.SetColor("script-editor/text-editor/string/background", c => scintillaControl1.Styles[Style.Cpp.String].BackColor = c);
            t.SetColor("script-editor/text-editor/string/foreground", c => scintillaControl1.Styles[Style.Cpp.String].ForeColor = c);
            t.SetColor("script-editor/text-editor/string-eol/background", c => scintillaControl1.Styles[Style.Cpp.StringEol].BackColor = c);
            t.SetColor("script-editor/text-editor/string-eol/foreground", c => scintillaControl1.Styles[Style.Cpp.StringEol].ForeColor = c);
            t.SetColor("script-editor/text-editor/operator/background", c => scintillaControl1.Styles[Style.Cpp.Operator].BackColor = c);
            t.SetColor("script-editor/text-editor/operator/foreground", c => scintillaControl1.Styles[Style.Cpp.Operator].ForeColor = c);
            t.SetColor("script-editor/text-editor/preprocessor/background", c => scintillaControl1.Styles[Style.Cpp.Preprocessor].BackColor = c);
            t.SetColor("script-editor/text-editor/preprocessor/foreground", c => scintillaControl1.Styles[Style.Cpp.Preprocessor].ForeColor = c);
            t.SetColor("script-editor/text-editor/line-number/background", c => scintillaControl1.Styles[Style.LineNumber].BackColor = c);
            t.SetColor("script-editor/text-editor/line-number/foreground", c => scintillaControl1.Styles[Style.LineNumber].ForeColor = c);
            t.SetColor("script-editor/text-editor/indent-guide/background", c => scintillaControl1.Styles[Style.IndentGuide].BackColor = c);
            t.SetColor("script-editor/text-editor/indent-guide/foreground", c => scintillaControl1.Styles[Style.IndentGuide].ForeColor = c);
            t.SetColor("script-editor/text-editor/fold-margin", c => scintillaControl1.SetFoldMarginColor(true, c));
            t.SetColor("script-editor/text-editor/fold-margin-hi", c => scintillaControl1.SetFoldMarginHighlightColor(true, c));
            t.SetColor("script-editor/text-editor/marknum-folder/background", c => scintillaControl1.Markers[Marker.Folder].SetBackColor(c));
            t.SetColor("script-editor/text-editor/marknum-folder/foreground", c => scintillaControl1.Markers[Marker.Folder].SetForeColor(c));
            t.SetColor("script-editor/text-editor/marknum-folder-end/background", c => scintillaControl1.Markers[Marker.FolderEnd].SetBackColor(c));
            t.SetColor("script-editor/text-editor/marknum-folder-end/foreground", c => scintillaControl1.Markers[Marker.FolderEnd].SetForeColor(c));
            t.SetColor("script-editor/text-editor/marknum-folder-open/background", c => scintillaControl1.Markers[Marker.FolderOpen].SetBackColor(c));
            t.SetColor("script-editor/text-editor/marknum-folder-open/foreground", c => scintillaControl1.Markers[Marker.FolderOpen].SetForeColor(c));
            t.SetColor("script-editor/text-editor/marknum-folder-open-mid/background", c => scintillaControl1.Markers[Marker.FolderOpenMid].SetBackColor(c));
            t.SetColor("script-editor/text-editor/marknum-folder-open-mid/foreground", c => scintillaControl1.Markers[Marker.FolderOpenMid].SetForeColor(c));
            t.SetColor("script-editor/text-editor/marknum-folder-mid-tail", c => scintillaControl1.Markers[Marker.FolderMidTail].SetBackColor(c));
            t.SetColor("script-editor/text-editor/marknum-folder-sub", c => scintillaControl1.Markers[Marker.FolderSub].SetBackColor(c));
            t.SetColor("script-editor/text-editor/marknum-folder-tail", c => scintillaControl1.Markers[Marker.FolderTail].SetBackColor(c));
            t.SetColor("script-editor/text-editor/selected", c => scintillaControl1.SetSelectionBackColor(c != Color.Transparent, c)); // compatibility with old theme
            t.SetColor("script-editor/text-editor/selected/background", c => scintillaControl1.SetSelectionBackColor(c != Color.Transparent, c));
            t.SetColor("script-editor/text-editor/selected/foreground", c => scintillaControl1.SetSelectionForeColor(c != Color.Transparent, c));

            t.SetColor("script-editor/text-editor/marker-breakpoint/background", c => scintillaControl1.Markers[MARKER_TYPE_BREAKPOINT].SetBackColor(c));
            t.SetColor("script-editor/text-editor/marker-breakpoint/foreground", c => scintillaControl1.Markers[MARKER_TYPE_BREAKPOINT].SetForeColor(c));
            t.SetColor("script-editor/text-editor/marker-breakpoint2/background", c => scintillaControl1.Markers[MARKER_TYPE_BREAKPOINT2].SetBackColor(c));
            t.SetColor("script-editor/text-editor/marker-breakpoint2/foreground", c => scintillaControl1.Markers[MARKER_TYPE_BREAKPOINT2].SetForeColor(c));
            t.SetColor("script-editor/text-editor/current-statement/background", c => scintillaControl1.Markers[MARKER_TYPE_CURRENT_STATEMENT].SetBackColor(c));
            t.SetColor("script-editor/text-editor/current-statement/foreground", c => scintillaControl1.Markers[MARKER_TYPE_CURRENT_STATEMENT].SetForeColor(c));
            t.SetColor("script-editor/text-editor/current-statement2/background", c => scintillaControl1.Markers[MARKER_TYPE_CURRENT_STATEMENT2].SetBackColor(c));
            t.SetColor("script-editor/text-editor/current-statement2/foreground", c => scintillaControl1.Markers[MARKER_TYPE_CURRENT_STATEMENT2].SetForeColor(c));

            t.SetColor("script-editor/text-editor/caret", c => scintillaControl1.CaretForeColor = c); // compatibility with old theme
            t.SetColor("script-editor/text-editor/caret/caret-fore", c => scintillaControl1.CaretForeColor = c);
            t.SetColor("script-editor/text-editor/caret/caret-line-back", c => scintillaControl1.CaretLineBackColor = c);
            t.SetInt("script-editor/text-editor/caret/caret-line-back-alpha", i => scintillaControl1.CaretLineBackColorAlpha = i);
        }

        private void UpdateColors()
        {
            this.scintillaControl1.Styles[Style.BraceBad].BackColor = Color.FromArgb(255, 0, 0);
            this.scintillaControl1.Styles[Style.BraceLight].Bold = true;
            this.scintillaControl1.Styles[Style.BraceLight].BackColor = Color.FromArgb(210, 210, 0);

            this.scintillaControl1.Styles[Style.Cpp.Word].ForeColor = Color.FromArgb(0, 0, 244);
            this.scintillaControl1.Styles[Style.Cpp.Word2].ForeColor = Color.FromArgb(43, 145, 175);
            this.scintillaControl1.Styles[Style.Cpp.GlobalClass].ForeColor = Color.FromArgb(43, 145, 175);
            this.scintillaControl1.Styles[Style.Cpp.Comment].ForeColor = Color.FromArgb(27, 127, 27);
            this.scintillaControl1.Styles[Style.Cpp.CommentLine].ForeColor = Color.FromArgb(27, 127, 27);
            this.scintillaControl1.Styles[Style.Cpp.CommentDoc].ForeColor = Color.FromArgb(27, 127, 27);
            this.scintillaControl1.Styles[Style.Cpp.CommentLineDoc].ForeColor = Color.FromArgb(27, 127, 27);
            this.scintillaControl1.Styles[Style.Cpp.Number].ForeColor = Color.FromArgb(150, 27, 27);
            this.scintillaControl1.Styles[Style.Cpp.String].ForeColor = Color.FromArgb(70, 7, 7);
            this.scintillaControl1.Styles[Style.Cpp.Operator].ForeColor = Color.FromArgb(0, 70, 0);
            this.scintillaControl1.Styles[Style.Cpp.Preprocessor].BackColor = Color.FromArgb(210, 210, 210);

            this.scintillaControl1.Styles[Style.CallTip].ForeColor = Color.Black;
            this.scintillaControl1.Styles[Style.CallTip].BackColor = Color.LightGoldenrodYellow;

            // override the selected text colour
            this.scintillaControl1.SetSelectionForeColor(true, Color.FromArgb(255, 255, 255));
            this.scintillaControl1.SetSelectionBackColor(true, Color.FromArgb(0, 34, 130));

            this.scintillaControl1.CallTipSetForeHlt(Color.FromArgb(240, 0, 0));

            this.scintillaControl1.Markers[MARKER_TYPE_BREAKPOINT].SetBackColor(Color.FromArgb(255, 100, 100));
            this.scintillaControl1.Markers[MARKER_TYPE_BREAKPOINT].SetForeColor(Color.White);

            this.scintillaControl1.Markers[MARKER_TYPE_BREAKPOINT2].SetBackColor(Color.Red);
            this.scintillaControl1.Markers[MARKER_TYPE_BREAKPOINT2].SetForeColor(Color.Black);

            this.scintillaControl1.Markers[MARKER_TYPE_CURRENT_STATEMENT].SetBackColor(Color.Yellow);
            this.scintillaControl1.Markers[MARKER_TYPE_CURRENT_STATEMENT].SetForeColor(Color.White);

            this.scintillaControl1.Markers[MARKER_TYPE_CURRENT_STATEMENT2].SetBackColor(Color.Yellow);
            this.scintillaControl1.Markers[MARKER_TYPE_CURRENT_STATEMENT2].SetForeColor(Color.White);

            Color FoldingForeColor = ColorTranslator.FromHtml("#F3F3F3");
            Color FoldingBackColor = ColorTranslator.FromHtml("#808080");

            this.scintillaControl1.Markers[Marker.Folder].SetForeColor(FoldingForeColor);
            this.scintillaControl1.Markers[Marker.Folder].SetBackColor(FoldingBackColor);
            this.scintillaControl1.Markers[Marker.FolderEnd].SetForeColor(FoldingForeColor);
            this.scintillaControl1.Markers[Marker.FolderEnd].SetBackColor(FoldingBackColor);
            this.scintillaControl1.Markers[Marker.FolderOpen].SetForeColor(FoldingForeColor);
            this.scintillaControl1.Markers[Marker.FolderOpen].SetBackColor(FoldingBackColor);
            this.scintillaControl1.Markers[Marker.FolderOpenMid].SetForeColor(FoldingForeColor);
            this.scintillaControl1.Markers[Marker.FolderOpenMid].SetBackColor(FoldingBackColor);
            this.scintillaControl1.Markers[Marker.FolderMidTail].SetForeColor(FoldingForeColor);
            this.scintillaControl1.Markers[Marker.FolderMidTail].SetBackColor(FoldingBackColor);
            this.scintillaControl1.Markers[Marker.FolderEnd].SetForeColor(FoldingForeColor);
            this.scintillaControl1.Markers[Marker.FolderEnd].SetBackColor(FoldingBackColor);
            this.scintillaControl1.Markers[Marker.FolderSub].SetForeColor(FoldingForeColor);
            this.scintillaControl1.Markers[Marker.FolderSub].SetBackColor(FoldingBackColor);
            this.scintillaControl1.Markers[Marker.FolderTail].SetForeColor(FoldingForeColor);
            this.scintillaControl1.Markers[Marker.FolderTail].SetBackColor(FoldingBackColor);

            this.scintillaControl1.Styles[Style.IndentGuide].ForeColor = ColorTranslator.FromHtml("#DDDDDD");

            UpdateColorTheme();
        }

        public void UpdateAllStyles()
        {
            scintillaControl1.StyleResetDefault();

            this.scintillaControl1.Styles[Style.Default].Font = _scriptFont;
            this.scintillaControl1.Styles[Style.Default].Size = _scriptFontSize;
            UpdateColorThemeStyleDefault();

            scintillaControl1.StyleClearAll(); // propagates default style to other styles

            this.scintillaControl1.Styles[Style.BraceBad].Font = _scriptFont;
            this.scintillaControl1.Styles[Style.BraceBad].Size = _scriptFontSize;
            this.scintillaControl1.Styles[Style.BraceLight].Font = _scriptFont;
            this.scintillaControl1.Styles[Style.BraceLight].Size = _scriptFontSize;

            this.scintillaControl1.Styles[Style.CallTip].Font = _calltipFont;
            this.scintillaControl1.Styles[Style.CallTip].Size = _calltipFontSize;

            if(this.scintillaControl1.Margins[0].Width > 0) EnableLineNumbers();
            UpdateColors();
        }

        public ScintillaWrapper()
        {
            // Scintilla is statically linked to our AGS.Native, therefore point to it
            ScintillaNET.Scintilla.SetModulePath("AGS.Native.dll");

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
                RegisterXPMImage(i + 1, AutoCompleteIcons[i]);
            }

            this.scintillaControl1.EolMode = Eol.CrLf;
            this.scintillaControl1.WrapMode = WrapMode.None;
            this.scintillaControl1.WrapIndentMode = WrapIndentMode.Indent;
            this.scintillaControl1.ClearAll();
            this.scintillaControl1.Lexer = Lexer.Cpp;
            // Disable preprocessor styling for now;
            // to make this work properly we need to supply keywords for preprocessor,
            // otherwise lexer will not know about external defines.
            scintillaControl1.SetProperty("lexer.cpp.track.preprocessor", "0");

            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            UpdateAllStyles();

            this.scintillaControl1.Markers[MARKER_TYPE_BREAKPOINT].Symbol = MarkerSymbol.Background;
            this.scintillaControl1.Markers[MARKER_TYPE_BREAKPOINT2].Symbol = MarkerSymbol.Circle;
            this.scintillaControl1.Markers[MARKER_TYPE_CURRENT_STATEMENT].Symbol = MarkerSymbol.Arrow;
            this.scintillaControl1.Markers[MARKER_TYPE_CURRENT_STATEMENT2].Symbol = MarkerSymbol.Background;

            this.scintillaControl1.CallTipTabSize(0);

            this.scintillaControl1.AutoCIgnoreCase = true;
            this.scintillaControl1.AutoCCancelAtStart = false;
            this.scintillaControl1.AutoCAutoHide = false;
            this.scintillaControl1.AutoCMaxHeight = 8;
            this.scintillaControl1.AutoCMaxWidth = 100;
            this.scintillaControl1.AutoCStops(AUTO_COMPLETE_CANCEL_CHARS);
            this.scintillaControl1.MouseDwellTime = 500;

            this.scintillaControl1.ClearCmdKey(Keys.Control | Keys.Space);
            this.scintillaControl1.ClearCmdKey(Keys.T | Keys.Space);

            this.scintillaControl1.TabWidth = Factory.AGSEditor.Settings.TabSize;
            this.scintillaControl1.UseTabs = Factory.AGSEditor.Settings.IndentUseTabs;
            this.scintillaControl1.UsePopup(false);

            // remove the default margins
            this.scintillaControl1.Margins[0].Width = 0;
            this.scintillaControl1.Margins[1].Width = 16;

            this.scintillaControl1.Margins[1].Sensitive = true;

            SetModEventMask();

            this.scintillaControl1.SavePointLeft += OnSavePointLeft;
            this.scintillaControl1.SavePointReached += OnSavePointReached;
            this.scintillaControl1.CharAdded += OnCharAdded;
            this.scintillaControl1.UpdateUI += OnUpdateUI;
            this.scintillaControl1.ModifyAttempt += OnModifyAttemptOnReadOnly;
            this.scintillaControl1.KeyPress += ScintillaControl1_KeyPress;
            this.scintillaControl1.Insert += ScintillaControl1_Insert;
            this.scintillaControl1.Delete += ScintillaControl1_Delete;
            this.scintillaControl1.MouseUp += ScintillaWrapper_MouseUp;
            this.scintillaControl1.DwellStart += scintillaControl1_DwellStart;
            this.scintillaControl1.DwellEnd += scintillaControl1_DwellEnd;
            this.scintillaControl1.MarginClick += scintillaControl1_MarginClick;

            SetFolding();

            // Prettier folding markers
            this.scintillaControl1.Markers[Marker.Folder].Symbol = MarkerSymbol.BoxPlus;
            this.scintillaControl1.Markers[Marker.FolderOpen].Symbol = MarkerSymbol.BoxMinus;
            this.scintillaControl1.Markers[Marker.FolderEnd].Symbol = MarkerSymbol.BoxPlusConnected;
            this.scintillaControl1.Markers[Marker.FolderMidTail].Symbol = MarkerSymbol.TCorner;
            this.scintillaControl1.Markers[Marker.FolderOpenMid].Symbol = MarkerSymbol.BoxMinusConnected;
            this.scintillaControl1.Markers[Marker.FolderSub].Symbol = MarkerSymbol.VLine;
            this.scintillaControl1.Markers[Marker.FolderTail].Symbol = MarkerSymbol.LCorner;

            // Indentation guides
            this.scintillaControl1.IndentationGuides = IndentView.LookBoth;

            this.scintillaControl1.ReadOnly = true;
        }

        private void RegisterXPMImage(int type, string xpm)
        {
            scintillaControl1.DirectMessage(ScintillaHelper.SCI_REGISTERIMAGE, type, xpm);
        }

        private void SetModEventMask()
        {
            // Insert/Delete text only
            scintillaControl1.DirectMessage(ScintillaHelper.SCI_SETMODEVENTMASK, (IntPtr)3, (IntPtr)0);
        }

        private void SetFolding()
        {
            scintillaControl1.SetProperty("fold", "1");
            scintillaControl1.SetProperty("fold.compact", "0");
            scintillaControl1.SetProperty("fold.comment", "1");
            scintillaControl1.SetProperty("fold.preprocessor", "1");

            scintillaControl1.Margins[2].Width = 16;
            scintillaControl1.Margins[2].Type = MarginType.Symbol;
            scintillaControl1.Margins[2].Mask = Marker.MaskFolders;

            scintillaControl1.Markers[Marker.Folder].Symbol = MarkerSymbol.Plus;
            scintillaControl1.Markers[Marker.FolderOpen].Symbol = MarkerSymbol.Minus;
            scintillaControl1.Markers[Marker.FolderEnd].Symbol = MarkerSymbol.Empty;
            scintillaControl1.Markers[Marker.FolderMidTail].Symbol = MarkerSymbol.Empty;
            scintillaControl1.Markers[Marker.FolderOpenMid].Symbol = MarkerSymbol.Empty;
            scintillaControl1.Markers[Marker.FolderSub].Symbol = MarkerSymbol.Empty;
            scintillaControl1.Markers[Marker.FolderTail].Symbol = MarkerSymbol.Empty;

            scintillaControl1.SetFoldFlags(FoldFlags.LineAfterContracted); // Draw line below if collapsed
            scintillaControl1.Margins[2].Sensitive = true;
        }

        void scintillaControl1_MarginClick(object sender, MarginClickEventArgs e)
        {
            int lineNumber = scintillaControl1.LineFromPosition(e.Position);
            if (e.Margin == 1)
            {
                if (ToggleBreakpoint != null)
                    ToggleBreakpoint(this, new ScintillaHelper.MarginClickExEventArgs(scintillaControl1, e));
            }
            else if (e.Margin == 2)
            {
                this.scintillaControl1.Lines[lineNumber].ToggleFold();
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
            int marginWidth = this.scintillaControl1.TextWidth(Style.Default, "12345");
            this.scintillaControl1.Margins[0].Type = MarginType.Number;
            this.scintillaControl1.Margins[0].Width = marginWidth + 4;
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
            get { return scintillaControl1.CurrentPosition; }
        }

        public int CurrentLine
        {
            get { return scintillaControl1.LineFromPosition(scintillaControl1.CurrentPosition); }
        }

        public int FirstVisibleLine
        {
            get { return scintillaControl1.FirstVisibleLine; }
        }

        /// <summary>
        /// Whether to style the contents as dialog script, as opposed to regular script.
        /// </summary>
        public bool DialogScriptStyle
        {
            get { return _isDialogScript; }
            set
            {
                _isDialogScript = value;
                scintillaControl1.Lexer = value ? Lexer.Container : Lexer.Cpp;
                if (value)
                    this.scintillaControl1.StyleNeeded += ScintillaControl1_StyleNeeded;
                else
                    this.scintillaControl1.StyleNeeded -= ScintillaControl1_StyleNeeded;
            }
        }

        public void GoToPosition(int newPos)
        {
            int lineNum = scintillaControl1.LineFromPosition(newPos);
            if ((lineNum <= scintillaControl1.FirstVisibleLine) ||
                (lineNum >= scintillaControl1.FirstVisibleLine + scintillaControl1.LinesOnScreen))
            {
                int bottomLine = lineNum + (scintillaControl1.LinesOnScreen / 2);
                if (bottomLine > scintillaControl1.Lines.Count)
                {
                    bottomLine = scintillaControl1.Lines.Count - 1;
                }
                int topLine = lineNum - (scintillaControl1.LinesOnScreen / 2);
                if (topLine < 0)
                {
                    topLine = 0;
                }
                scintillaControl1.Lines[bottomLine].Goto();
                scintillaControl1.Lines[topLine].Goto();
            }

            scintillaControl1.GotoPosition(newPos);
        }

        public void SelectCurrentLine()
        {
			scintillaControl1.GotoPosition(scintillaControl1.Lines[this.CurrentLine].Position);
			scintillaControl1.ExecuteCmd(Command.LineEndExtend);
			scintillaControl1.LineScroll(0, 0 - scintillaControl1.SelectedText.Length);
        }

        public void AddBreakpoint(int lineNumber)
        {
            scintillaControl1.Lines[lineNumber].MarkerAdd(MARKER_TYPE_BREAKPOINT);
            scintillaControl1.Lines[lineNumber].MarkerAdd(MARKER_TYPE_BREAKPOINT2);
        }

        public bool IsBreakpointOnLine(int lineNumber)
        {
            return (scintillaControl1.Lines[lineNumber].MarkerGet() & MARKER_MASK_BREAKPOINT) != 0;
        }

        public void RemoveBreakpoint(int lineNumber)
        {
            scintillaControl1.Lines[lineNumber].MarkerDelete(MARKER_TYPE_BREAKPOINT);
            scintillaControl1.Lines[lineNumber].MarkerDelete(MARKER_TYPE_BREAKPOINT2);
        }

        public int[] GetLineNumbersForAllBreakpoints()
        {
            List<int> breakpointLines = new List<int>();
            int line = 0;
            while (line >= 0)
            {
                int next_line = scintillaControl1.Lines[line].MarkerNext(MARKER_MASK_BREAKPOINT);
                if (next_line <= line) break;

                line = next_line + 1;
                breakpointLines.Add(line);

            }
            return breakpointLines.ToArray();
        }

        public void ShowCurrentExecutionPoint(int lineNumber)
        {
            scintillaControl1.Lines[lineNumber - 1].MarkerAdd(MARKER_TYPE_CURRENT_STATEMENT);
            scintillaControl1.Lines[lineNumber - 1].MarkerAdd(MARKER_TYPE_CURRENT_STATEMENT2);
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
            int x = scintillaControl1.PointXFromPosition(this.scintillaControl1.CurrentPosition) + 200;
            int y = scintillaControl1.PointYFromPosition(this.scintillaControl1.CurrentPosition) + 40;
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

        public void ClearAllKeyWords()
        {
            for (WordListType type = WordListType.Keywords; type <= WordListType.Marker; ++type)
            {
                scintillaControl1.SetKeywords((int)type, "");
            }
            foreach (var set in _keywordSets) set.Clear();
            _keywords.Clear();
            _autoCKeywords.Clear();
        }

        // We need this to comply to the IScriptEditorControl
        public void SetKeyWords(string keyWords)
        {
            SetKeyWords(keyWords, WordListType.Keywords, false);
        }

        public void SetKeyWords(string keyWords, WordListType type, bool dialogKeywords = false)
        {
            scintillaControl1.SetKeywords((int)type, keyWords);

            // Remove previous set keywords of the given type, then add new ones
            if (_keywordSets[(int)type] == null) _keywordSets[(int)type] = new List<string>();
            if (type == WordListType.Keywords) // other words are added differently
            {
                foreach (var k in _keywordSets[(int)type])
                    _autoCKeywords.Remove(k);
                AddAutoCompleteKeyWords(keyWords);
            }
            // Need to remove keywords from the full list, only corresponding to the given kind and set
            var thisKeyList = dialogKeywords ? _dialogKeywords : _keywords;
            foreach (var k in _keywordSets[(int)type])
                thisKeyList.Remove(k);
            _keywordSets[(int)type].Clear();
            SetNormalKeywords(keyWords, type, dialogKeywords);
        }

        private void SetNormalKeywords(string keyWords, WordListType type, bool dialogKeywords)
        {
            string[] arr = keyWords.Split(' ');
            var thisKeyList = dialogKeywords ? _dialogKeywords : _keywords;
            foreach (string s in arr)
            {
                s.Trim();
                _keywordSets[(int)type].Add(s);
                thisKeyList.Add(s);
            }
        }

        private void AddAutoCompleteKeyWords(string keyWords)
        {
            string[] keywordArray = keyWords.Split(' ');
            foreach (string keyword in keywordArray)
            {
                // "true" and "false" are actually enums, so don't list them as keywords
                if ((keyword != "true") && (keyword != "false"))
                {
                    _autoCKeywords.Add(keyword + "?" + IMAGE_INDEX_KEYWORD);
                }
            }
        }

        public void SetFillupKeys(string fillupKeys)
        {
            // pressing ( [ or . will auto-complete
            this.scintillaControl1.AutoCSetFillUps(fillupKeys);
            _fillupKeys = fillupKeys;
        }

        public void SetSavePoint()
        {
            this.scintillaControl1.SetSavePoint();
            this.scintillaControl1.ReadOnly = true;
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
            bool shouldBeReadOnly = this.scintillaControl1.ReadOnly;
            this.scintillaControl1.ReadOnly = false;

            this.scintillaControl1.Text = newText;
            this.scintillaControl1.ConvertEols(Eol.CrLf);
            if (clearModified)
            {
                this.scintillaControl1.SetSavePoint();
                this.scintillaControl1.EmptyUndoBuffer();
            }

            this.scintillaControl1.ReadOnly = shouldBeReadOnly;
        }

        public void SetAutoCompleteSource(IScript script)
        {
            _autoCompleteForThis = script;
        }

        public void ModifyText(string newText)
        {
            this.scintillaControl1.Text = newText;
        }

        public string GetText()
        {
            string text = this.scintillaControl1.Text;

            while (text.EndsWith("\0"))
            {
                text = text.Substring(0, text.Length - 1);
            }

            return text;
        }

        public bool IsModified
        {
            get { return this.scintillaControl1.Modified; }
        }

        public int FindLineNumberForText(string text)
        {
            string currentText = this.scintillaControl1.Text;
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
            this.scintillaControl1.Lines[lineNum].EnsureVisible();
            if (lineNum > 0)
            {
                this.scintillaControl1.Lines[lineNum - 1].EnsureVisible();
            }

            int bottomLine = lineNum + (scintillaControl1.LinesOnScreen / 2);
            if (bottomLine > scintillaControl1.Lines.Count)
            {
                bottomLine = scintillaControl1.Lines.Count - 1;
            }
            int topLine = lineNum - (scintillaControl1.LinesOnScreen / 2);
            if (topLine < 0)
            {
                topLine = 0;
            }
            scintillaControl1.Lines[bottomLine].Goto();
            scintillaControl1.Lines[topLine].Goto();
            this.scintillaControl1.Lines[lineNum - 1].Goto();
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
            get { return this.scintillaControl1.SelectedText; }
        }

        public bool CanPaste()
        {
            try
            {
                return Clipboard.ContainsText();
            }
            catch (ExternalException)
            {
                return false;
            }
        }

        public void Paste()
        {
            this.scintillaControl1.Paste();
        }

        public bool CanUndo()
        {
            bool shouldBeReadOnly = scintillaControl1.ReadOnly;
            scintillaControl1.ReadOnly = false;
            bool res = scintillaControl1.CanUndo;
            scintillaControl1.ReadOnly = shouldBeReadOnly;
            return res;
        }

        public void Undo()
        {
            this.scintillaControl1.Undo();
        }

        public bool CanRedo()
        {
            bool shouldBeReadOnly = scintillaControl1.ReadOnly;
            scintillaControl1.ReadOnly = false;
            bool res = scintillaControl1.CanRedo;
            scintillaControl1.ReadOnly = shouldBeReadOnly;
            return res;
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
            scintillaControl1.Lines[scintillaControl1.LineFromPosition(pos)].EnsureVisible();
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
            int currentPos = this.scintillaControl1.CurrentPosition;
            int nextPos = -1;
            if (currentPos < documentText.Length)
            {
                nextPos = documentText.IndexOf(text, currentPos, comparisonType);
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
            string line = scintillaControl1.Lines[lineIndex].Text;
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
            scintillaControl1.ReplaceSelection(withText);
        }

        public string GetFullTypeNameAtCursor()
        {
            return GetFullTypeNameAtPosition(scintillaControl1.CurrentPosition);
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
            while (charIndex < scintillaControl1.TextLength)
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

        private Tuple<int, int> GetBraceAndMatchingBracePositions()
        {
            if (InsideStringOrComment(false))
                return Tuple.Create(INVALID_POSITION, INVALID_POSITION);
            if (_isDialogScript && !scintillaControl1.Lines[scintillaControl1.CurrentLine].Text.StartsWith(" "))
                return Tuple.Create(INVALID_POSITION, INVALID_POSITION);

            int currentPos = scintillaControl1.CurrentPosition;
            int prevChar = scintillaControl1.GetCharAt(currentPos - 1);
            int curChar = scintillaControl1.GetCharAt(currentPos);
            bool isBraceBefore = (prevChar == '{' || prevChar == '}' || prevChar == '(' || prevChar == ')');
            bool isBraceAfter = (curChar == '{' || curChar == '}' || curChar == '(' || curChar == ')');
            
            if (isBraceBefore)
            {
                return Tuple.Create(currentPos - 1, scintillaControl1.BraceMatch(currentPos - 1));
            }
            if (isBraceAfter)
            {
                return Tuple.Create(currentPos, scintillaControl1.BraceMatch(currentPos));
            }
            return Tuple.Create(INVALID_POSITION, INVALID_POSITION);
        }

        public void DoIdentationAlignAfterBrace()
        {
            Tuple<int, int> pos = GetBraceAndMatchingBracePositions();
            int currentPos = pos.Item1;
            int matchPos = pos.Item2;
            if (matchPos >= 0)
            {
                AlignIndentation(currentPos, matchPos);
            }
            _doAlignIdentation = false;
        }

        public void ShowMatchingBraceIfPossible()
        {
            Tuple<int, int> pos = GetBraceAndMatchingBracePositions();
            if (pos.Item1 < 0 && pos.Item2 < 0)
                return; // no braces found under cursor

            int currentPos = pos.Item1;
            int matchPos = pos.Item2;
            if (matchPos >= 0)
            {
                scintillaControl1.BraceHighlight(matchPos, currentPos);
            }
            else
            {
                scintillaControl1.BraceBadLight(currentPos);
            }
            _braceMatchVisible = true;
        }

        private void AlignIndentation(int posToAlign, int posToAlignWith)
        {
            int lineToAlign = scintillaControl1.LineFromPosition(posToAlign);
            int lineToAlignWith = scintillaControl1.LineFromPosition(posToAlignWith);
            int indentOfPosToAlignWith = scintillaControl1.Lines[lineToAlignWith].Indentation;
            scintillaControl1.Lines[lineToAlign].Indentation = indentOfPosToAlignWith;
        }

        private void OnUpdateUI(object sender, EventArgs e)
        {
            if (_braceMatchVisible)
            {
                scintillaControl1.BraceBadLight(INVALID_POSITION);
                _braceMatchVisible = false;
            }

            if (_doAlignIdentation)
            {
                DoIdentationAlignAfterBrace();
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

            UpdateStatusText();

            if(_skipBraceMatchOnce)
            {
                _skipBraceMatchOnce = false;
            }
            else
            {
                ShowMatchingBraceIfPossible();
            }
        }

        private void UpdateStatusText()
        {
            var currentPos = this.scintillaControl1.CurrentPosition;
            var currentLine = this.scintillaControl1.LineFromPosition(currentPos);
            var currentColumn = currentPos - this.scintillaControl1.Lines[currentLine].Position;
            var selected = this.scintillaControl1.SelectionEnd - this.scintillaControl1.SelectionStart;
            var selectedLineStart = this.scintillaControl1.LineFromPosition(this.scintillaControl1.SelectionStart);
            var selectedLineEnd = this.scintillaControl1.LineFromPosition(this.scintillaControl1.SelectionEnd);
            var selectedLines = selectedLineEnd - selectedLineStart;
            if (selected > 0)
                GUIController.Instance.UpdateStatusBarText((selectedLines>0 ? (selectedLines+1) + " lines, " : "") + selected + " characters selected");
            else
                GUIController.Instance.UpdateStatusBarText("Line " + (currentLine+1) + ", Column " + (currentColumn+1));
        }

        private void OnCharAdded(object sender, ScintillaNET.CharAddedEventArgs e)
        {
            // Reset to normal fillups
            this.scintillaControl1.AutoCSetFillUps(_fillupKeys);

            if((e.Char == '(') || (e.Char == '{'))
            {
                _skipBraceMatchOnce = true;
            }

            if (e.Char == 10) // Enter/Return
            {
                int lineNumber = scintillaControl1.LineFromPosition(scintillaControl1.CurrentPosition);
                if (lineNumber > 0)
                {
                    int previousLineIndent = scintillaControl1.Lines[lineNumber - 1].Indentation;
                    string previousLine = scintillaControl1.Lines[lineNumber - 1].Text.Trim('\r', '\n', '\0');
                    if (previousLine.EndsWith("{"))
                    {
                        previousLineIndent += scintillaControl1.TabWidth;
                    }
                    scintillaControl1.Lines[lineNumber].Indentation = previousLineIndent;
                    scintillaControl1.GotoPosition(scintillaControl1.GetLineIndentationPosition(lineNumber));
                }
            }
            // The following events must be piped to the UpdateUI event,
            // otherwise they don't work properly
            else if ((e.Char == '}') || (e.Char == ')'))
            {
                if (!InsideStringOrComment(true))
                {
                    if (_autoDedentClosingBrace) _doAlignIdentation = true;
                }

                if (scintillaControl1.CallTipActive)
                {
                    scintillaControl1.CallTipCancel();
                }
            }
            else if ((e.Char == '(') || (e.Char == ','))
            {
                bool insideString = InsideStringOrComment(true);
                if ((e.Char == ',') && (!insideString) &&
                    (_autoSpaceAfterComma))
                {
                    scintillaControl1.AddText(" ");
                }

                _doCalltip = !insideString;
            }
            else if ((e.Char == '.') && (!scintillaControl1.AutoCActive))
            {
                _doShowAutocomplete = true;
            }
            else if (((Char.IsLetterOrDigit((char)e.Char) || (e.Char == '_') || (e.Char == ' ')) && (!scintillaControl1.AutoCActive)))
            {
                _doShowAutocomplete = true;
            }
            if (CharAdded != null)
            {
                CharAdded(e.Char);
            }
        }

        private void ScintillaControl1_Delete(object sender, ModificationEventArgs e)
        {
            if (TextModified != null)
            {
                TextModified(e.Position, e.Text.Length, false);
            }
        }

        private void ScintillaControl1_Insert(object sender, ModificationEventArgs e)
        {
            if (TextModified != null)
            {
                TextModified(e.Position, e.Text.Length, true);
            }
        }

        // prevents keyboard sending non-printable characters to scintilla text buffer
        private void ScintillaControl1_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar < 32)
            {
                // Prevent control characters from getting inserted
                e.Handled = true;
                return;
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
                    this.scintillaControl1.ReadOnly = false;
                }
            }
            else
            {
                this.scintillaControl1.ReadOnly = false;
            }
        }

        private void scintillaControl1_DwellEnd(object sender, DwellEventArgs e)
        {
            if (_dwellCalltipVisible)
            {
                scintillaControl1.CallTipCancel();
                _dwellCalltipVisible = false;
            }
        }

        private void scintillaControl1_DwellStart(object sender, ScintillaNET.DwellEventArgs e)
        {
            if ((_callTipsEnabled) && (e.Position > 0) &&
                (!scintillaControl1.CallTipActive) &&
                (!scintillaControl1.AutoCActive) &&
                (!InsideStringOrComment(e.Position)) &&
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
            int startOfLine = scintillaControl1.Lines[lineNumber].Position;
            string lineText = scintillaControl1.Lines[lineNumber].Text;
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
            int cursorPos = scintillaControl1.CurrentPosition;
            int lineNumber = scintillaControl1.LineFromPosition(cursorPos);
            int startOfLine = scintillaControl1.Lines[lineNumber].Position;
            string lineText = scintillaControl1.Lines[lineNumber].Text;
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

        private ScriptStruct FindGlobalType(string type)
        {
            foreach (IScript script in GetAutoCompleteScriptList())
            {
                foreach (ScriptStruct structDef in script.AutoCompleteData.Structs)
                {
                    if ((structDef.Name == type) && (structDef.FullDefinition))
                    {
                        return structDef;
                    }
                }
            }
            return null;
        }

        private ScriptStruct FindGlobalVariableOrType(string type, ref bool staticAccess)
        {
            // First try search for a type that has this name
            var foundType = FindGlobalType(type);
            if (foundType != null)
            {
                staticAccess = true;
                return foundType;
            }

            // Then, search for a variable that has this name, and try retrieving its type
            foreach (IScript script in GetAutoCompleteScriptList())
            {
                foreach (ScriptVariable varDef in script.AutoCompleteData.Variables)
                {
                    if (varDef.VariableName == type)
                    {
                        staticAccess = false;
                        return FindGlobalType(varDef.Type);
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
                string lineText = scintillaControl1.Lines[lineNumber].Text;
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

            ScriptStruct foundType = ParsePreviousExpression(scintillaControl1.CurrentPosition - 1, out charactersAfterDot, out staticAccess, out isThis);

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
            if (OnBeforeShowingAutoComplete != null)
            {
                OnBeforeShowingAutoComplete(this, null);
            }
            if (autoCompleteList.Length > 0)
            {
                scintillaControl1.AutoCShow(charsTyped, autoCompleteList);
            }
        }

        private bool CheckForAndShowEnumAutocomplete(int checkAtPos)
        {
            if (this.scintillaControl1.GetCharAt(checkAtPos) == ' ')
            {
                // potential enum autocomplete
                bool atLeastOneEquals = false;
                checkAtPos--;
                while (checkAtPos > 0 && ((this.scintillaControl1.GetCharAt(checkAtPos) == ' ') ||
                       (this.scintillaControl1.GetCharAt(checkAtPos) == '=')))
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
            return InsideStringOrComment(scintillaControl1.CurrentPosition - (charJustAdded ? 1 : 0));
        }

        public bool InsideStringOrComment(int position)
        {
            int style = scintillaControl1.GetStyleAt(position);
            if ((style == Style.Cpp.CommentLine) || (style == Style.Cpp.Comment) ||
                (style == Style.Cpp.CommentDoc) || (style == Style.Cpp.CommentLineDoc) ||
                (style == Style.Cpp.String))
            {
                return true;
            }

            int lineNumber = this.scintillaControl1.LineFromPosition(position);
            int lineStart = this.scintillaControl1.Lines[lineNumber].Position;
            string curLine = this.scintillaControl1.Lines[lineNumber].Text;
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

        public bool InsideStringOrCommentStyleOnly(int position)
        {
            int style = this.scintillaControl1.GetStyleAt(position);
            return ((style == Style.Cpp.CommentLine) || (style == Style.Cpp.Comment) ||
                (style == Style.Cpp.CommentDoc) || (style == Style.Cpp.CommentLineDoc) ||
                (style == Style.Cpp.String));
        }

        private bool IgnoringCurrentLine()
        {
            if (_ignoreLinesWithoutIndent)
            {
                int lineNumber = scintillaControl1.LineFromPosition(scintillaControl1.CurrentPosition);
                int lineIndent = scintillaControl1.Lines[lineNumber].Indentation;
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

            int checkAtPos = this.scintillaControl1.CurrentPosition - 1;
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

        // Max chars of a function call to parse when calculating current parameter (for autocomplete)
        private const int AUTOC_FUNCPARAMS_MAXCHARS = 256;

        /// <summary>
        /// Parses the assumed function call *backwards*, returns the found start of
        /// the function call and the assumed parameter index under currentPos.
        /// Returns -1 if the call start was not found within AUTOC_FUNCPARAMS_MAXCHARS.
        /// </summary>
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
            // This parses the function call *backwards* from the current cursor pos
            // trying to find the start of the call (an opening bracket at level 0)
            while (currentPos > 0)
            {
                char thisChar = (char)this.scintillaControl1.GetCharAt(currentPos);
                if ((thisChar == '(' || thisChar == ')' || thisChar == ',') &&
                    !InsideStringOrCommentStyleOnly(currentPos))
                {
                    if ((thisChar == '(') && (bracketDepth == 0))
                    {
                        break;
                    }
                    else if (thisChar == '(') bracketDepth--;
                    else if (thisChar == ')') bracketDepth++;
                    else if ((thisChar == ',') && (bracketDepth == 0))
                    {
                        parameterIndex++;
                    }
                }
                currentPos--;
                charsCounted++;

                if (charsCounted > AUTOC_FUNCPARAMS_MAXCHARS)
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
            int currentPos = FindStartOfFunctionCall(this.scintillaControl1.CurrentPosition, out parameterIndex) - 1;
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

        private ScriptAPIVersion? GetAPIVersionFromString(String s)
        {
            try
            {
                return (ScriptAPIVersion)Enum.Parse(typeof(ScriptAPIVersion), s);
            }
            catch (ArgumentException)
            {
                return null;
            }
        }

        private bool ShouldShowThis(ScriptToken token, List<ScriptDefine> defines)
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
            if ((token.IfNDefOnly == "NEW_DIALOGOPTS_API") && (!gameSettings.UseOldCustomDialogOptionsAPI))
            {
                return false;
            }
            if ((token.IfDefOnly == "NEW_DIALOGOPTS_API") && (gameSettings.UseOldCustomDialogOptionsAPI))
            {
                return false;
            }
            if ((token.IfNDefOnly == "NEW_KEYINPUT_API") && (!gameSettings.UseOldKeyboardHandling))
            {
                return false;
            }
            if ((token.IfDefOnly == "NEW_KEYINPUT_API") && (gameSettings.UseOldKeyboardHandling))
            {
                return false;
            }
            if (token.IfNDefOnly != null && token.IfNDefOnly.StartsWith("SCRIPT_API_"))
            {
                ScriptAPIVersion? v = GetAPIVersionFromString(token.IfNDefOnly.Substring("SCRIPT_API_".Length));
                if (v.HasValue && v <= gameSettings.ScriptAPIVersionReal)
                    return false;
            }
            if (token.IfDefOnly != null && token.IfDefOnly.StartsWith("SCRIPT_API_"))
            {
                ScriptAPIVersion? v = GetAPIVersionFromString(token.IfDefOnly.Substring("SCRIPT_API_".Length));
                if (v.HasValue && v > gameSettings.ScriptAPIVersionReal)
                    return false;
            }
            if (token.IfNDefOnly != null && token.IfNDefOnly.StartsWith("SCRIPT_COMPAT_"))
            {
                ScriptAPIVersion? v = GetAPIVersionFromString(token.IfNDefOnly.Substring("SCRIPT_COMPAT_".Length));
                if (v.HasValue && v >= gameSettings.ScriptCompatLevelReal)
                    return false;
            }
            if (token.IfDefOnly != null && token.IfDefOnly.StartsWith("SCRIPT_COMPAT_"))
            {
                ScriptAPIVersion? v = GetAPIVersionFromString(token.IfDefOnly.Substring("SCRIPT_COMPAT_".Length));
                if (v.HasValue && v < gameSettings.ScriptCompatLevelReal)
                    return false;
            }
            // TODO: AutoComplete feature in AGS is implemented in confusing and messy way. Thus, it does not
            // use same technique for knowing which parts of the script should be disabled (by ifdef/ifndef)
            // as precompiler. Instead it makes its own parsing, and somewhat limits perfomance and capabilities.
            // This is (one) reason why all those checks are made here explicitly, instead of relying on some
            // prefetched macro list.
            if (token.IfNDefOnly != null && token.IfNDefOnly.StartsWith("STRICT_IN_"))
            {
                ScriptAPIVersion? v = GetAPIVersionFromString(token.IfNDefOnly.Substring("STRICT_IN_".Length));
                if (v.HasValue && (gameSettings.EnforceObjectBasedScript && v <= gameSettings.ScriptCompatLevelReal))
                    return false;
            }
            if (token.IfDefOnly != null && token.IfDefOnly.StartsWith("STRICT_IN_"))
            {
                ScriptAPIVersion? v = GetAPIVersionFromString(token.IfDefOnly.Substring("STRICT_IN_".Length));
                if (v.HasValue && !(gameSettings.EnforceObjectBasedScript && v <= gameSettings.ScriptCompatLevelReal))
                    return false;
            }
            return true;
        }

        private void AddEnumValuesToAutocompleteList(List<string> list, ScriptEnum se)
        {
            foreach (ScriptEnumValue enumValue in se.EnumValues)
            {
                list.Add(enumValue.Name + "?" + IMAGE_INDEX_ENUM);
            }
        }

        private void AddGlobalsFromScript(List<string> globalsList, IScript script, Dictionary<string, object> addedNames, int onlyShowIfDefinitionBeforePos)
        {
            List<ScriptDefine> defines = script.AutoCompleteData.Defines;
            foreach (ScriptVariable sv in script.AutoCompleteData.Variables)
            {
                if (!addedNames.ContainsKey(sv.VariableName))
                {
                    if (ShouldShowThis(sv, defines))
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
                    if ((ShouldShowThis(sf, defines)) &&
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
                if (ShouldShowThis(sd, defines))
                {
                    globalsList.Add(sd.Name + "?" + IMAGE_INDEX_DEFINE);
                }
            }
            foreach (ScriptEnum se in script.AutoCompleteData.Enums)
            {
                if (ShouldShowThis(se, defines))
                {
                    globalsList.Add(se.Name + "?" + IMAGE_INDEX_STRUCT);
                    AddEnumValuesToAutocompleteList(globalsList, se);
                }
            }
            foreach (ScriptStruct ss in script.AutoCompleteData.Structs)
            {
                if ((ShouldShowThis(ss, defines)) && (ss.FullDefinition))
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
                        ShouldShowThis(sf, null) &&
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
                        ShouldShowThis(sv, null))
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
            int currentPos = this.scintillaControl1.CurrentPosition;
            ScriptFunction func = _autoCompleteForThis.AutoCompleteData.Functions.Find(
                c => c.StartsAtCharacterIndex <= currentPos && c.EndsAtCharacterIndex >= currentPos);
            return func;
        }

        public List<ScriptVariable> GetListOfLocalVariablesForCurrentPosition(bool searchWholeFunction)
        {
            return GetListOfLocalVariablesForCurrentPosition(searchWholeFunction, this.scintillaControl1.CurrentPosition);
        }

        public List<ScriptVariable> GetListOfLocalVariablesForCurrentPosition(bool searchWholeFunction, int currentPos)
        {
            List<ScriptVariable> toReturn;

            if (_autoCompleteForThis != null && _autoCompleteForThis.AutoCompleteData != null)
            {
                string scriptExtract = scintillaControl1.Text;
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
                foreach (string keyword in _autoCKeywords)
                {
                    globalsList.Add(keyword);
                }

                Dictionary<string, object> addedNames = new Dictionary<string, object>();

                foreach (IScript script in GetAutoCompleteScriptList())
                {
                    int onlyShowIfDefinitionBeforePos = Int32.MaxValue;
                    if (script == _autoCompleteForThis)
                    {
                        onlyShowIfDefinitionBeforePos = scintillaControl1.CurrentPosition;
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
                int clickAtPos = scintillaControl1.CharPositionFromPoint(e.X, e.Y);
                if ((clickAtPos < scintillaControl1.SelectionStart) ||
                    (clickAtPos > scintillaControl1.SelectionEnd))
                {
                    scintillaControl1.GotoPosition(clickAtPos);
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
            get { return scintillaControl1.Lines.Count; }
        }

        public string ScriptFont
        {
            set { _scriptFont = value; }
            get { return _scriptFont; }
        }

        public int ScriptFontSize
        {
            set { _scriptFontSize = value; }
            get { return _scriptFontSize; }
        }

        public string CallTipFont
        {
            set { _calltipFont = value; }
            get { return _calltipFont; }
        }

        public int CallTipFontSize
        {
            set { _calltipFontSize = value; }
            get { return _calltipFontSize; }
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
            get { return scintillaControl1.SelectedText; }
        }

        int IScriptEditorControl.CursorPosition
        {
            get { return scintillaControl1.CurrentPosition; }
        }

        int IScriptEditorControl.GetLineNumberForPosition(int position)
        {
            return scintillaControl1.LineFromPosition(position) + 1;
        }

        public string GetTextForLine(int lineNumber)
        {
            return scintillaControl1.Lines[lineNumber - 1].Text;
        }

        string IScriptEditorControl.GetTypeNameAtCursor()
        {
            return GetFullTypeNameAtCursor();
        }

        public void ResetSelection()
        {
            scintillaControl1.SelectionStart = 0;
            scintillaControl1.SelectionEnd = 0;
            scintillaControl1.CurrentPosition = 0;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="t"></param>
        private void LoadColorTheme(ColorTheme t)
        {
            _theme = t;
            UpdateColorTheme();
        }

        /// <summary>
        /// Custom style the script, depending on a current mode.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ScintillaControl1_StyleNeeded(object sender, StyleNeededEventArgs e)
        {
            if (!_isDialogScript) return;

            int line_number = scintillaControl1.LineFromPosition(scintillaControl1.GetEndStyled());
            int end_pos = e.Position;

            StyleDialogScript(line_number, end_pos);
        }

        private void StyleDialogScript(int linenum, int end)
        {
            int line_length = scintillaControl1.Lines[linenum].Length;
            int start_pos = scintillaControl1.Lines[linenum].Position;
            int laststyle = start_pos;

            int stylingMode = Style.Cpp.Default;
            if (start_pos > 0) stylingMode = scintillaControl1.GetStyleAt(start_pos - 1);

            bool onNewLine = true;
            bool atNewWord = true;
            bool onScriptLine = false;
            int i;
            scintillaControl1.StartStyling(start_pos);

            for (i = start_pos; i < end; i++)
            {
                char c = (char)scintillaControl1.GetCharAt(i);

                // Handle end of line
                if (c == '\n')
                {
                    onNewLine = true;
                    atNewWord = false;
                    onScriptLine = false;
                    // The multiline comments and strings should keep their style;
                    // everything else is cut of at the end of the line
                    if (stylingMode != Style.Cpp.Comment && stylingMode != Style.Cpp.String)
                    {
                        if (laststyle < i)
                        {
                            scintillaControl1.SetStyling(i - laststyle, stylingMode);
                            laststyle = i;
                        }
                        stylingMode = Style.Cpp.Default;
                    }
                    continue;
                }

                // Handle new non-empty line
                if (onNewLine)
                {
                    // first whitespace on a new line triggers normal script
                    if (Char.IsWhiteSpace(c))
                    {
                        onScriptLine = true;
                        onNewLine = false;
                        atNewWord = true;
                        continue;
                    }
                    // first non-whitespace on a new line: switch to dialog script style
                    // (unless we are inside a multiline comment or string)
                    else if (stylingMode != Style.Cpp.CommentLine &&
                        stylingMode != Style.Cpp.String)
                    {
                        scintillaControl1.SetStyling(i - laststyle, stylingMode);
                        stylingMode = Style.Cpp.Word2;
                        laststyle = i;
                    }
                    onNewLine = false;
                    atNewWord = true;
                }

                // Handle inside comments
                if (stylingMode == Style.Cpp.CommentLine)
                {
                    // if inside a one-line comment, then just skip everything (except EOL)
                    continue;
                }
                else if (stylingMode == Style.Cpp.Comment)
                {
                    // if inside a multiline comment, then test for the comment end
                    if (c == '/' && scintillaControl1.GetCharAt(i - 1) == '*')
                    {
                        scintillaControl1.SetStyling(i - laststyle + 1, stylingMode);
                        stylingMode = Style.Cpp.Default;
                        laststyle = i + 1;
                        atNewWord = true;
                    }
                    continue;
                }

                // Handle inside strings
                if (stylingMode == Style.Cpp.String)
                {
                    // test for the string end
                    if (c == '"')
                    {
                        scintillaControl1.SetStyling(i - laststyle + 1, stylingMode);
                        laststyle = i + 1;
                        stylingMode = Style.Cpp.Default;
                        atNewWord = true;
                    }
                    continue;
                }

                if (!Char.IsLetterOrDigit(c))
                {
                    string lastword = previousWordFrom(i);
                    if (lastword.Length != 0)
                    {
                        int newMode = stylingMode;
                        if (onScriptLine && _keywords.Contains(lastword.Trim())) newMode = Style.Cpp.Word;
                        if (!onScriptLine && stylingMode == Style.Cpp.Word2) // before colon
                        {
                            if (lastword.Trim() == "return" || lastword.Trim() == "stop") newMode = Style.Cpp.Word;
                        }
                        if (newMode != stylingMode)
                        {
                            scintillaControl1.SetStyling(i - laststyle - lastword.Length, stylingMode);
                            scintillaControl1.SetStyling(lastword.Length, newMode);
                            laststyle = i;
                        }
                    }
                }

                // Handle regular script
                if (onScriptLine)
                {
                    if (isOperator(c))
                    {
                        scintillaControl1.SetStyling(i - laststyle, stylingMode);
                        scintillaControl1.SetStyling(1, Style.Cpp.Operator);
                        stylingMode = Style.Cpp.Default;
                        laststyle = i + 1;
                    }
                    // Style the numbers only when the digit is the first character in a word
                    else if (atNewWord && Char.IsDigit(c))
                    {
                        scintillaControl1.SetStyling(i - laststyle, stylingMode);
                        stylingMode = Style.Cpp.Number;
                        laststyle = i;
                    }
                    else if (c == '"')
                    {
                        stylingMode = Style.Cpp.String;
                    }
                    else if (Char.IsWhiteSpace(c) || Char.IsPunctuation(c))
                    {
                        scintillaControl1.SetStyling(i - laststyle, stylingMode);
                        stylingMode = Style.Cpp.Default;
                        laststyle = i;
                    }
                }
                // Handle dialog script
                else
                {
                    if (c == ':')
                    {
                        scintillaControl1.SetStyling(i - laststyle + 1, stylingMode);
                        laststyle = i + 1;
                        stylingMode = Style.Cpp.Number;
                    }
                    if (c == '@' && stylingMode == Style.Cpp.Word2)
                    {
                        scintillaControl1.SetStyling(i - laststyle, stylingMode);
                        stylingMode = Style.Cpp.Number;
                        laststyle = i;
                    }
                }

                // Handle comment starts
                if (c == '/')
                {
                    // multiline comment
                    if (scintillaControl1.GetCharAt(i + 1) == '*' && onScriptLine)
                    {
                        scintillaControl1.SetStyling(i - laststyle, stylingMode);
                        stylingMode = Style.Cpp.Comment;
                        laststyle = i;
                    }
                    // single-line comment
                    else if (scintillaControl1.GetCharAt(i + 1) == '/')
                    {
                        scintillaControl1.SetStyling(i - laststyle, stylingMode);
                        stylingMode = Style.Cpp.CommentLine;
                        laststyle = i;
                    }
                }

                onNewLine = false;
                atNewWord = Char.IsWhiteSpace(c) || Char.IsPunctuation(c) || isOperator(c);
            }

            scintillaControl1.SetStyling(i - laststyle, stylingMode);
        }

        private string previousWordFrom(int from)
        {
            from--;
            StringBuilder word = new StringBuilder();

            while (Char.IsLetterOrDigit((char)this.scintillaControl1.GetCharAt(from)) && from > 0)
            {
                word.Insert(0, (char)this.scintillaControl1.GetCharAt(from));
                from--;
            }

            return word.ToString();
        }


        private bool isOperator(int token)
        {
            return (token == '(' ||
                    token == ')' ||
                    token == '{' ||
                    token == '}' ||
                    token == '+' ||
                    token == '=' ||
                    token == '*');
        }

        public void SetWrapMode(WrapMode wrapmode)
        {
            scintillaControl1.WrapMode = wrapmode;
        }

        public WrapMode GetWrapMode()
        {
            return scintillaControl1.WrapMode;
        }
    }
}
