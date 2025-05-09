using AGS.Editor.Components;
using AGS.Editor.TextProcessing;
using AGS.Types;
using AGS.Types.AutoComplete;
using AGS.Types.Interfaces;
using AGS.Controls;
using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class ScriptEditor : ScriptEditorBase, IScriptEditor
    {
        public event EventHandler IsModifiedChanged;
        public delegate void AttemptToEditScriptHandler(ref bool allowEdit);
        public static event AttemptToEditScriptHandler AttemptToEditScript;

        private Action _anonymousDelegate;

        // Custom Edit menu commands
        private const string TOGGLE_BREAKPOINT_COMMAND = "ToggleBreakpoint";
        private const string SHOW_MATCHING_SCRIPT_OR_HEADER_COMMAND = "ScriptShowMatchingScript";
        private const string TOGGLE_LINE_COMMENT_COMMAND = "ToggleLineComment";
        // Custom context menu commands
        private const string TOGGLE_WORD_WRAP = "ToggleWordWrap";
        private const string CONTEXT_MENU_TOGGLE_BREAKPOINT = "CtxToggleBreakpoint";

        private readonly FileWatcher _fileWatcher;
        private Script _script;
        private Room _room;
        private int _roomNumber;

        // Menu item refs, for tracking their state
        MenuCommand _menuCmdWordWrap;

        private AutoComplete.BackgroundCacheUpdateStatusChangedHandler _autocompleteUpdateHandler;
        private Action<Script> _showMatchingScript;
        private bool _allowZoomToFunction = true;

        // we need this bool because it's not necessarily the same as scintilla.Modified
        private bool _editorTextModifiedSinceLastCopy = false;
        private int _firstVisibleLine;

        private static bool _lastWordWrap = false; // remember last used for new scripts, could be maybe saved in settings

        public ScriptEditor(Script scriptToEdit, AGSEditor agsEditor, Action<Script> showMatchingScript)
            : base(agsEditor)
        {
            InitializeComponent();
            _fileWatcher = new FileWatcher(scriptToEdit.FileName, scriptToEdit, OnFileChangedExternally);

            _agsEditor = agsEditor;
            _showMatchingScript = showMatchingScript;
            _room = null;
            _roomNumber = -1;
            Init(scriptToEdit);

            this.Load += new EventHandler(ScriptEditor_Load);
            this.Resize += new EventHandler(ScriptEditor_Resize);
        }

        public string GetScriptTabName()
        {
            if (_roomNumber >= 0)
            {
                UnloadedRoom room = Factory.AGSEditor.CurrentGame.FindRoomByID(_roomNumber);
                if(room != null && room.Number == _roomNumber && !string.IsNullOrEmpty(room.Description))
                {
                    return Script.FileNameWithoutPath + (IsModified ? " *" : "") + ": " + room.Description;
                }
            }
            return Script.FileNameWithoutPath + (IsModified ? " *" : "");
        }

        private void Init(Script scriptToEdit)
        {
            _autocompleteUpdateHandler = new AutoComplete.BackgroundCacheUpdateStatusChangedHandler(AutoComplete_BackgroundCacheUpdateStatusChanged);
            AutoComplete.BackgroundCacheUpdateStatusChanged += _autocompleteUpdateHandler;

            Script = scriptToEdit;
            // Also give script reference to the base class
            base.Script = scriptToEdit;
            InitScintilla();
        }

        private void ScriptEditor_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }

        public int FirstVisibleLine { get { return _firstVisibleLine; } }

        public string ModifiedText
        {
            get
            {
                return scintilla.IsDisposed ?
                    null : scintilla.GetText();
            }
            set
            {
                scintilla.SetTextModified(value);
            }
        }

        public void InitScintilla()
        {
            scintilla.EnableLineNumbers();

            scintilla.SetWrapMode(_lastWordWrap ? ScintillaNET.WrapMode.Word : ScintillaNET.WrapMode.None);

            scintilla.IsModifiedChanged += scintilla_IsModifiedChanged;
            scintilla.AttemptModify += scintilla_AttemptModify;
            scintilla.UpdateUI += scintilla_UpdateUI;
            scintilla.OnBeforeShowingAutoComplete += scintilla_OnBeforeShowingAutoComplete;
            scintilla.TextModified += scintilla_TextModified;
            scintilla.ToggleBreakpoint += scintilla_ToggleBreakpoint;

            if (!this.Script.IsHeader)
            {
                scintilla.SetAutoCompleteSource(Script);
            }

            scintilla.SetKeyWords(Constants.SCRIPT_KEY_WORDS);
            // pressing ( [ or . will auto-complete
            scintilla.SetFillupKeys(Constants.AUTOCOMPLETE_ACCEPT_KEYS);

            // Scripts may miss autocomplete cache when they are first opened, so update
            UpdateAutocompleteAndControls(true);

            // Assign Scintilla reference to the base class
            Scintilla = scintilla;
        }

        protected override void AddEditMenuCommands(MenuCommands commands)
        {
            commands.Commands.Add(MenuCommand.Separator);
            commands.Commands.Add(new MenuCommand(TOGGLE_BREAKPOINT_COMMAND, "Toggle Breakpoint", Keys.F9, "ToggleBreakpointMenuIcon"));
            commands.Commands.Add(new MenuCommand(TOGGLE_LINE_COMMENT_COMMAND, "Toggle Line Comment", Keys.Control | Keys.Shift | Keys.Q));
            _menuCmdWordWrap = new MenuCommand(TOGGLE_WORD_WRAP, "Word Wrap", "WordWrapIcon");
            commands.Commands.Add(_menuCmdWordWrap);
            commands.Commands.Add(new MenuCommand(SHOW_MATCHING_SCRIPT_OR_HEADER_COMMAND, "Switch to Matching Script or Header", Keys.Control | Keys.M));
        }

        protected override void AddToolbarCommands(List<MenuCommand> toolbar)
        {
            toolbar.Add(_menuCmdWordWrap);
        }

        protected override void AddCtxCommands(ContextMenuStrip menuStrip)
        {
            EventHandler onClick = new EventHandler(ContextMenuChooseOption2);
            menuStrip.Items.Add(new ToolStripSeparator());
            menuStrip.Items.Add(ToolStripExtensions.CreateMenuItem("Toggle Breakpoint", Factory.GUIController.ImageList.Images["ToggleBreakpointMenuIcon"], onClick, CONTEXT_MENU_TOGGLE_BREAKPOINT, Keys.F9));
            menuStrip.Items.Add(ToolStripExtensions.CreateMenuItem("Toggle Line Comment", null, onClick, TOGGLE_LINE_COMMENT_COMMAND, Keys.Control | Keys.Shift | Keys.Q));
        }

        public void ActivateWindow()
        {
            OnWindowActivated();
        }

        void scintilla_ToggleBreakpoint(object sender, ScintillaHelper.MarginClickExEventArgs e)
        {
            ToggleBreakpoint(e.LineNumber);
        }


        protected override void OnDispose()
        {
            _fileWatcher.Dispose();
            base.OnDispose();
        }

        private void ScriptEditor_Resize(object sender, EventArgs e)
        {
            if (this.ClientSize.Width > 50)
            {
                cmbFunctions.Width = this.ClientSize.Width - cmbFunctions.Left - 5;
            }
        }

        private void OnFileChangedExternally()
        {
            _script.LoadFromDisk();
            scintilla.SetText(_script.Text);
            _editorTextModifiedSinceLastCopy = false;
        }

        private void UpdateStructHighlighting()
        {
            StringBuilder sb = new StringBuilder(5000);
            List<Script> allScripts = _agsEditor.GetImportedScriptHeaders(_script);
            allScripts.Add(_script); // only imported scripts + current one
            foreach (Script script in allScripts)
            {
                foreach (ScriptStruct thisClass in script.AutoCompleteData.Structs)
                {
                    sb.Append(thisClass.Name + " ");
                }
                foreach (ScriptEnum thisEnum in script.AutoCompleteData.Enums)
                {
                    sb.Append(thisEnum.Name + " ");
                }
            }
            this.scintilla.SetKeyWords(sb.ToString(), ScintillaWrapper.WordListType.GlobalClasses);
        }

        private void UpdateFunctionList()
        {
            if (!this.ContainsFocus) return; // only update for the active pane to avoid expensive combo Add operations
            List<string> functions = new List<string>();
            foreach (ScriptFunction func in _script.AutoCompleteData.Functions)
            {
                if (func.EndsAtCharacterIndex > 0)
                {
                    functions.Add(func.FunctionName);
                }
            }
            foreach (ScriptStruct struc in _script.AutoCompleteData.Structs)
            {
                foreach (ScriptFunction func in struc.Functions)
                {
                    if (func.EndsAtCharacterIndex > 0)
                    {
                        functions.Add(struc.Name + "::" + func.FunctionName);
                    }
                }
            }
            cmbFunctions.BeginUpdate();
            cmbFunctions.Items.Clear();
            cmbFunctions.Items.Add("(general definitions)");
            functions.Sort();
            cmbFunctions.Items.AddRange(functions.ToArray());
            cmbFunctions.EndUpdate();
            SelectFunctionInListForCurrentPosition();
        }

        /// <summary>
        /// Updates this script's autocomplete cache and all the controls that depend on it.
        /// </summary>
        private void UpdateAutocompleteAndControls(bool force)
        {
            if (!force && !ContainsFocus)
                return; // only update for the active pane to avoid expensive combo Add operations
            AutoComplete.ConstructCache(_script, _agsEditor.GetImportedScriptHeaders(_script));
            UpdateStructHighlighting();
            UpdateFunctionList();
        }

        private void AdjustStartOfFunctionsInScript(int fromPos, int adjustment)
        {
            foreach (ScriptFunction func in _script.AutoCompleteData.Functions)
            {
                AdjustStartOfFunctionIfAppropriate(func, fromPos, adjustment);
            }

            foreach (ScriptStruct struc in _script.AutoCompleteData.Structs)
            {
                foreach (ScriptFunction func in struc.Functions)
                {
                    AdjustStartOfFunctionIfAppropriate(func, fromPos, adjustment);
                }
            }
        }

        private void AdjustStartOfFunctionIfAppropriate(ScriptFunction func, int currentPos, int adjustment)
        {
            if (func.StartsAtCharacterIndex > currentPos)
            {
                func.StartsAtCharacterIndex += adjustment;
            }

            if (func.EndsAtCharacterIndex > currentPos)
            {
                func.EndsAtCharacterIndex += adjustment;
            }

            if (func.StartsAtCharacterIndex < 0)
            {
                // Function has probably just been deleted
                func.StartsAtCharacterIndex = -1;
                func.EndsAtCharacterIndex = -1;
            }
        }

        private void SelectFunctionInListForCurrentPosition()
        {
            lock (cmbFunctions)
            {
                _allowZoomToFunction = false;

                int currentPos = scintilla.CurrentPos;
                foreach (ScriptFunction func in _script.AutoCompleteData.Functions)
                {
                    if ((currentPos >= func.StartsAtCharacterIndex) &&
                        (currentPos < func.EndsAtCharacterIndex))
                    {
                        int index = cmbFunctions.FindStringExact(func.FunctionName);
                        if (index >= 0)
                        {
                            cmbFunctions.SelectedIndex = index;
                            _allowZoomToFunction = true;
                            return;
                        }
                    }
                }

                foreach (ScriptStruct struc in _script.AutoCompleteData.Structs)
                {
                    foreach (ScriptFunction func in struc.Functions)
                    {
                        if ((currentPos >= func.StartsAtCharacterIndex) &&
                            (currentPos < func.EndsAtCharacterIndex))
                        {
                            int index = cmbFunctions.FindStringExact(struc.Name + "::" + func.FunctionName);
                            if (index >= 0)
                            {
                                cmbFunctions.SelectedIndex = index;
                                _allowZoomToFunction = true;
                                return;
                            }
                        }
                    }
                }

                if (cmbFunctions.Items.Count > 0)
                {
                    cmbFunctions.SelectedIndex = 0;
                }

                _allowZoomToFunction = true;
            }
        }

        private void AutoComplete_BackgroundCacheUpdateStatusChanged(BackgroundAutoCompleteStatus status, Exception errorDetails)
        {
            if (status == BackgroundAutoCompleteStatus.Finished)
            {
                if (this.IsHandleCreated)
                {
                    this.Invoke(new Action(_anonymousDelegate));
                }
            }
        }

        public void ActivateTextEditor()
        {
            scintilla.ActivateTextEditor();
            if (scintilla.CurrentLine == 0) //If no item was seleced via Find/Replace etc
            {
                scintilla.GoToLine(_firstVisibleLine);
            }
        }

        public void DeactivateTextEditor()
        {
            scintilla.DeactivateTextEditor();
        }

        public new Script Script
        {
            get { return _script; }
            set
            {
                _script = value;
                scintilla.SetText(_script.Text);
                _editorTextModifiedSinceLastCopy = false;
            }
        }

        public void ScriptModifiedExternally()
        {
            scintilla.ModifyText(_script.Text);
        }

        public bool IsModified
        {
            get { return scintilla.IsModified; }
        }

        public Room Room
        {
            get { return _room; }
            set { _room = value; }
        }

        public int RoomNumber
        {
            get { return _roomNumber; }
            set { _roomNumber = value; }
        }

        public static bool HoveringCombo { get; private set; }

        public IScriptEditorControl ScriptEditorControl
        {
            get
            {
                return scintilla;
            }
        }

        public void UpdateScriptObjectWithLatestTextInWindow()
        {
            _script.Text = scintilla.GetText();
            _editorTextModifiedSinceLastCopy = false;
        }

        public void SaveChanges()
        {
            if (_editorTextModifiedSinceLastCopy)
            {
                UpdateScriptObjectWithLatestTextInWindow();
            }

            if (!scintilla.IsDisposed && scintilla.IsModified)
            {
                _fileWatcher.Enabled = false;
                _script.SaveToDisk();
                _fileWatcher.Enabled = true;

                scintilla.SetSavePoint();
                UpdateAutocompleteAndControls(true);
            }
        }

        public void GoToLineOfCharacterPosition(int position)
        {
            GoToLineOfCharacterPosition(position, true);
        }

        public bool GoToLineOfCharacterPosition(int position, bool selectLine)
        {
            if (position < 0 || position >= scintilla.TextLength)
                return false;

            scintilla.GoToPosition(position);
            if (selectLine)
            {
                scintilla.SelectCurrentLine();
            }
            return true;
        }

        public void SetExecutionPointMarker(int lineNumber)
        {
            scintilla.ShowCurrentExecutionPoint(lineNumber);
        }

        public void SetErrorMessagePopup(string errorMessage)
        {
            scintilla.ShowErrorMessagePopup(errorMessage);
        }

        /// <summary>
        /// Tries to find exact text in script, and returns a corresponding line number,
        /// or -1 if no such text was found.
        /// plainCodeOnly tells if comments and string literals should be ignored.
        /// </summary>
        public int GetLineNumberForText(string text, bool plainCodeOnly)
        {
            return scintilla.FindLineNumberForText(text, plainCodeOnly);
        }

        /// <summary>
        /// Searches the script for the regex pattern, and returns a corresponding line number,
        /// or -1 if pattern search failed.
        /// plainCodeOnly tells if comments and string literals should be ignored.
        /// </summary>
        public int GetLineNumberForPattern(string pattern, bool plainCodeOnly)
        {
            return scintilla.FindLineNumberForPattern(pattern, plainCodeOnly);
        }

        public void RemoveExecutionPointMarker()
        {
            scintilla.HideCurrentExecutionPoint();
            scintilla.HideErrorMessagePopup();
        }

        private void ToggleBreakpoint(int line)
        {
            if (scintilla.IsBreakpointOnLine(line))
            {
                scintilla.RemoveBreakpoint(line);
                _agsEditor.Debugger.RemovedBreakpoint(this.Script, line + 1);
            }
            else
            {
                scintilla.AddBreakpoint(line);
                _agsEditor.Debugger.AddedBreakpoint(this.Script, line + 1);
            }

            this.Script.BreakpointedLines = scintilla.GetLineNumbersForAllBreakpoints();

        }

        private void ToggleBreakpointOnCurrentLine()
        {
            ToggleBreakpoint(scintilla.CurrentLine);
        }

        protected override void OnKeyPressed(System.Windows.Forms.Keys keyData)
        {
            if (keyData.Equals(
                Keys.Escape))
            {
                FindReplace.CloseDialogIfNeeded();
            }
        }

        protected override void OnCommandClick(string command)
        {
            if (command == TOGGLE_BREAKPOINT_COMMAND)
            {
                ToggleBreakpointOnCurrentLine();
            }
            else if (command == SHOW_MATCHING_SCRIPT_OR_HEADER_COMMAND)
            {
                if (_showMatchingScript != null)
                {
                    _showMatchingScript(this.Script);
                }
            }
            else if (command == TOGGLE_LINE_COMMENT_COMMAND)
            {
                scintilla.ToggleLineComment();
            }
            else if (command == TOGGLE_WORD_WRAP)
            {
                bool wrap_enabled = !(scintilla.GetWrapMode() != ScintillaNET.WrapMode.None); // toggle
                _lastWordWrap = wrap_enabled;
                scintilla.SetWrapMode(wrap_enabled ? ScintillaNET.WrapMode.Word : ScintillaNET.WrapMode.None);
            }
            else
            {
                base.OnCommandClick(command);
            }
            UpdateUICommands();
        }

        protected override void UpdateUICommands()
        {
            base.UpdateUICommands();

            bool isWrapEnabled = scintilla.GetWrapMode() != ScintillaNET.WrapMode.None;
            if ((_menuCmdWordWrap.Checked != isWrapEnabled))
            {
                _menuCmdWordWrap.Checked = isWrapEnabled;
                Factory.ToolBarManager.RefreshCurrentPane();
                Factory.MenuManager.RefreshCurrentPane();
            }
        }

        protected override void OnWindowActivated()
        {
            _agsEditor.RegenerateScriptHeader(_room);
            if (_editorTextModifiedSinceLastCopy)
            {
                UpdateScriptObjectWithLatestTextInWindow();
            }
            AutoComplete.RequestBackgroundCacheUpdate(_script, _agsEditor.GetImportedScriptHeaders(_script));
            ActivateTextEditor();
        }

        protected override void OnWindowDeactivated()
        {
            DeactivateTextEditor();
        }

        protected override string OnGetHelpKeyword()
        {
            var keyword = scintilla.GetFullTypeNameAtCursor();
            if (string.IsNullOrEmpty(keyword))
                return "Scripting Language";
            return keyword;
        }

        protected override void OnPanelClosing(bool canCancel, ref bool cancelClose)
        {
            if ((canCancel) && (scintilla.IsModified))
            {
                DialogResult answer = MessageBox.Show("Do you want to save your changes before closing?", "Save changes?", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
                if (answer == DialogResult.Cancel)
                {
                    cancelClose = true;
                }
                else if (answer == DialogResult.Yes)
                {
                    DisconnectEventHandlers();
                    SaveChanges();
                    return;
                }
                else if (System.IO.File.Exists(_script.FileName))
                {
                    // Revert back to saved version
                    _script.LoadFromDisk();
                }
            }

            if (!cancelClose)
            {
                DisconnectEventHandlers();
            }
        }

        private void DisconnectEventHandlers()
        {
            AutoComplete.BackgroundCacheUpdateStatusChanged -= _autocompleteUpdateHandler;
        }

        private void scintilla_IsModifiedChanged(object sender, EventArgs e)
        {
            if (IsModifiedChanged != null)
            {
                IsModifiedChanged(this, e);
            }
        }

        private void scintilla_AttemptModify(ref bool allowModify)
        {
            if (AttemptToEditScript != null)
            {
                AttemptToEditScript(ref allowModify);
                if (!allowModify)
                {
                    return;
                }
            }
            if (!_agsEditor.AttemptToGetWriteAccess(_script.FileName))
            {
                allowModify = false;
            }
        }

        private void scintilla_UpdateUI(object sender, EventArgs e)
        {
            if (cmbFunctions.Items.Count > 0)
            {
                SelectFunctionInListForCurrentPosition();
            }
            if (scintilla.FirstVisibleLine != 0)
            {
                //This 'hack' is used in order to save the position of the scrollbar
                //when the docking has changed, in order to recreate the document
                //with the previous scrollbar position.
                //When the docking state changes, the first visible line in scintilla
                //changes to 0, before we have a chance of saving it, and use it
                //to recreate the scrollbar position.
                //The only scenario in which this will not work is if the scrollbar position
                //really was 0, but then the user could simply press Ctrl+Home and fix this easily.
                _firstVisibleLine = scintilla.FirstVisibleLine;
            }
        }

        private void scintilla_OnBeforeShowingAutoComplete(object sender, EventArgs e)
        {
            if (_editorTextModifiedSinceLastCopy)
            {
                UpdateScriptObjectWithLatestTextInWindow();
            }
            AutoComplete.ConstructCache(_script, _agsEditor.GetImportedScriptHeaders(_script));
        }

        private void scintilla_TextModified(int startPos, int length, bool wasAdded)
        {
            _editorTextModifiedSinceLastCopy = IsModified; // IsModified is what keeps track of what is the actual last saved checkpoint
            if (!IsModified)
            {
                // we reached the currently saved state, but with _editorTextModifiedSinceLastCopy being false we need to update Script.Text manually
                // and make sure Script.Modified is false to ensure that ScriptEditor, Scintilla, and Script are in sync.
                UpdateScriptObjectWithLatestTextInWindow();
                Script.Modified = false;
            }
            int adjustment = length;
            if (!wasAdded)
            {
                adjustment = -length;
            }
            AdjustStartOfFunctionsInScript(startPos, adjustment);
        }

        private ScriptFunction FindFunctionInAutocompleteData(string funcName)
        {
            ScriptFunction func = _script.AutoCompleteData.FindFunction(funcName);
            if ((func == null) && (funcName.Contains("::")))
            {
                string[] structAndFuncNames = funcName.Split(new string[] { "::" }, StringSplitOptions.None);
                ScriptStruct struc = _script.AutoCompleteData.FindStruct(structAndFuncNames[0]);
                if (struc != null)
                {
                    func = struc.FindMemberFunction(structAndFuncNames[1]);
                }
            }
            return func;
        }

        void cmbFunctions_MouseLeave(object sender, System.EventArgs e)
        {
            HoveringCombo = false;
        }

        void cmbFunctions_MouseEnter(object sender, System.EventArgs e)
        {
            HoveringCombo = true;
        }

        private void cmbFunctions_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (_allowZoomToFunction)
            {
                ScriptFunction func = FindFunctionInAutocompleteData(cmbFunctions.SelectedItem.ToString());
                if (func != null)
                {
                    scintilla.GoToPosition(func.StartsAtCharacterIndex);
                    scintilla.SelectCurrentLine();
                }
                else
                {
                    scintilla.GoToPosition(0);
                }
                scintilla.Focus();
            }
        }

        private void ContextMenuChooseOption2(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            if (item.Name == CONTEXT_MENU_TOGGLE_BREAKPOINT)
            {
                ToggleBreakpointOnCurrentLine();
            }
            else if (item.Name == TOGGLE_LINE_COMMENT_COMMAND)
            {
                scintilla.ToggleLineComment();
            }
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.ControlHelper(this, "script-editor");
            if (t.ComboBoxHelper(panel1.Controls, ref cmbFunctions, "script-editor/combo-functions"))
            {
                cmbFunctions.SelectedIndexChanged += cmbFunctions_SelectedIndexChanged;
                cmbFunctions.MouseEnter += cmbFunctions_MouseEnter;
                cmbFunctions.MouseLeave += cmbFunctions_MouseLeave;
            }
        }

        void ScriptEditor_HandleCreated(object sender, System.EventArgs e)
        {
            if ((_script != null) && (_script.AutoCompleteData != null))
            {
                UpdateFunctionList();
            }
        }
    }
}
