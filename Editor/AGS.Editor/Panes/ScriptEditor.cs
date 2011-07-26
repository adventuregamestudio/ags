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
using AGS.Types.Enums;
using AGS.Editor.TextProcessing;
using AGS.Types.Interfaces;



namespace AGS.Editor
{
    public partial class ScriptEditor : EditorContentPanel, IScriptEditor
    {
        public event EventHandler IsModifiedChanged;
        public delegate void AttemptToEditScriptHandler(ref bool allowEdit);
        public static event AttemptToEditScriptHandler AttemptToEditScript;

        private delegate void AnonymousDelegate();

        private const string CUT_COMMAND = "ScriptCut";
        private const string COPY_COMMAND = "ScriptCopy";
        private const string PASTE_COMMAND = "ScriptPaste";
        private const string UNDO_COMMAND = "ScriptUndo";
        private const string REDO_COMMAND = "ScriptRedo";
        private const string SHOW_AUTOCOMPLETE_COMMAND = "ScriptShowAutoComplete";
		private const string MATCH_BRACE_COMMAND = "MatchBrace";
        private const string TOGGLE_BREAKPOINT_COMMAND = "ToggleBreakpoint";
        private const string FIND_COMMAND = "ScriptFind";
        private const string FIND_NEXT_COMMAND = "ScriptFindNext";
        private const string REPLACE_COMMAND = "ScriptReplace";
        private const string FIND_ALL_COMMAND = "ScriptFindAll";
        private const string REPLACE_ALL_COMMAND = "ScriptReplaceAll";
		private const string CONTEXT_MENU_GO_TO_DEFINITION = "CtxGoToDefiniton";
        private const string CONTEXT_MENU_FIND_ALL_USAGES = "CtxFindAllUsages";
        private const string CONTEXT_MENU_GO_TO_SPRITE = "CtxGoToSprite";
		private const string CONTEXT_MENU_TOGGLE_BREAKPOINT = "CtxToggleBreakpoint";

        private Script _script;
        private Room _room;
        private int _roomNumber;
        private AGSEditor _agsEditor;
        private List<MenuCommand> _toolbarIcons = new List<MenuCommand>();
        private MenuCommands _extraMenu = new MenuCommands("&Edit", GUIController.FILE_MENU_ID);
        private string _lastSearchText = string.Empty;
		private bool _lastCaseSensitive = false;
        private AutoComplete.BackgroundCacheUpdateStatusChangedHandler _autocompleteUpdateHandler;
        private EditorEvents.FileChangedInGameFolderHandler _fileChangedHandler;
        private EventHandler _mainWindowActivatedHandler;
        private bool _allowZoomToFunction = true;
		private string _goToDefinition = null;
        private int? _goToSprite = null;
        private bool _fileChangedExternally = false;
        // we need this bool because it's not necessarily the same as scintilla.Modified
        private bool _editorTextModifiedSinceLastCopy = false;

        public ScriptEditor(Script scriptToEdit, AGSEditor agsEditor)
        {
            _agsEditor = agsEditor;
            Init(scriptToEdit);
            _room = null;
            _roomNumber = 0;
        }

        public void Clear()
        {
            this.Controls.Clear();
            _toolbarIcons.Clear();
            _extraMenu.Commands.Clear();
            this.Resize -= new EventHandler(ScriptEditor_Resize);
        }

        public void Init(Script scriptToEdit)
        {
            InitializeComponent();

            _autocompleteUpdateHandler = new AutoComplete.BackgroundCacheUpdateStatusChangedHandler(AutoComplete_BackgroundCacheUpdateStatusChanged);
            AutoComplete.BackgroundCacheUpdateStatusChanged += _autocompleteUpdateHandler;
            _fileChangedHandler = new EditorEvents.FileChangedInGameFolderHandler(Events_FileChangedInGameFolder);
            Factory.Events.FileChangedInGameFolder += _fileChangedHandler;
            _mainWindowActivatedHandler = new EventHandler(GUIController_OnMainWindowActivated);
            Factory.GUIController.OnMainWindowActivated += _mainWindowActivatedHandler;

            _toolbarIcons.Add(new MenuCommand(CUT_COMMAND, "Cut", "CutIcon"));
            _toolbarIcons.Add(new MenuCommand(COPY_COMMAND, "Copy", "CopyIcon"));
            _toolbarIcons.Add(new MenuCommand(PASTE_COMMAND, "Paste", "PasteIcon"));
            _toolbarIcons.Add(new MenuCommand(UNDO_COMMAND, "Undo", "UndoIcon"));
            _toolbarIcons.Add(new MenuCommand(REDO_COMMAND, "Redo", "RedoIcon"));
            _extraMenu.Commands.Add(new MenuCommand(UNDO_COMMAND, "Undo", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Z, "UndoMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(REDO_COMMAND, "Redo", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Y, "RedoMenuIcon"));
            _extraMenu.Commands.Add(MenuCommand.Separator);
            _extraMenu.Commands.Add(new MenuCommand(CUT_COMMAND, "Cut", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.X, "CutMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(COPY_COMMAND, "Copy", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.C, "CopyMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(PASTE_COMMAND, "Paste", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.V, "PasteMenuIcon"));
            _extraMenu.Commands.Add(MenuCommand.Separator);
            _extraMenu.Commands.Add(new MenuCommand(FIND_COMMAND, "Find...", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.F, "FindMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(FIND_NEXT_COMMAND, "Find next", System.Windows.Forms.Keys.F3, "FindNextMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(REPLACE_COMMAND, "Replace...", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.E));
            _extraMenu.Commands.Add(MenuCommand.Separator);
            _extraMenu.Commands.Add(new MenuCommand(FIND_ALL_COMMAND, "Find All...", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift | System.Windows.Forms.Keys.F, "FindMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(REPLACE_ALL_COMMAND, "Replace All...", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift | System.Windows.Forms.Keys.E));
            _extraMenu.Commands.Add(MenuCommand.Separator);
            _extraMenu.Commands.Add(new MenuCommand(SHOW_AUTOCOMPLETE_COMMAND, "Show Autocomplete", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Space, "ShowAutocompleteMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(TOGGLE_BREAKPOINT_COMMAND, "Toggle Breakpoint", System.Windows.Forms.Keys.F9, "ToggleBreakpointMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(MATCH_BRACE_COMMAND, "Match Brace", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.B));

            this.Resize += new EventHandler(ScriptEditor_Resize);
            this.Script = scriptToEdit;

            InitScintilla();            
        }
        
        public bool MovedFromDocument { get; set; }

        public string ModifiedText
        {
            get { return scintilla.GetText(); }
            set { scintilla.SetTextModified(value); }
        }

        public void InitScintilla()
        {            
            scintilla.SetKeyWords(Constants.SCRIPT_KEY_WORDS);
            UpdateStructHighlighting();

            // pressing ( [ or . will auto-complete
            scintilla.SetFillupKeys(Constants.AUTOCOMPLETE_ACCEPT_KEYS);

            scintilla.EnableLineNumbers();

            scintilla.IsModifiedChanged += new EventHandler(scintilla_IsModifiedChanged);
            scintilla.AttemptModify += new ScintillaWrapper.AttemptModifyHandler(scintilla_AttemptModify);
            scintilla.UpdateUI += new EventHandler(scintilla_UpdateUI);
            scintilla.TextModified += new ScintillaWrapper.TextModifiedHandler(scintilla_TextModified);
            scintilla.ConstructContextMenu += new ScintillaWrapper.ConstructContextMenuHandler(scintilla_ConstructContextMenu);
            scintilla.ActivateContextMenu += new ScintillaWrapper.ActivateContextMenuHandler(scintilla_ActivateContextMenu);
            scintilla.ToggleBreakpoint += new EventHandler<Scintilla.MarginClickEventArgs>(scintilla_ToggleBreakpoint);

            if (!this.Script.IsHeader)
            {
                scintilla.SetAutoCompleteSource(this.Script);
            }

            scintilla.SetKeyWords(Constants.SCRIPT_KEY_WORDS);
            UpdateStructHighlighting();
        }

        public void ActivateWindow()
        {
            OnWindowActivated();
        }

        void scintilla_ToggleBreakpoint(object sender, Scintilla.MarginClickEventArgs e)
        {
            ToggleBreakpoint(e.LineNumber);   
        }

        private void ScriptEditor_Resize(object sender, EventArgs e)
        {
            if (this.ClientSize.Width > 50)
            {
                cmbFunctions.Width = this.ClientSize.Width - cmbFunctions.Left - 5;
            }
        }

        private void PromptUserThatFileHasChangedExternally()
        {
            _fileChangedExternally = false;
            if (Factory.GUIController.ShowQuestion("The file '" + _script.FileName + "' has been modified externally. Do you want to reload it?", MessageBoxIcon.Question) == DialogResult.Yes)
            {
                _script.LoadFromDisk();
                scintilla.SetText(_script.Text);
                _editorTextModifiedSinceLastCopy = false;
            }
        }

        private void GUIController_OnMainWindowActivated(object sender, EventArgs e)
        {
            if (_fileChangedExternally)
            {
                PromptUserThatFileHasChangedExternally();
            }
        }

        private void Events_FileChangedInGameFolder(string fileName)
        {
            if (fileName.ToLower() == _script.FileName.ToLower())
            {
                if (DateTime.Now.Subtract(_script.LastSavedAt).TotalSeconds > 2)
                {
                    if (Utilities.IsThisApplicationCurrentlyActive())
                    {
                        PromptUserThatFileHasChangedExternally();
                    }
                    else
                    {
                        _fileChangedExternally = true;
                    }
                }
            }
        }

		private void UpdateStructHighlighting()
		{
			StringBuilder sb = new StringBuilder(5000);
			foreach (Script script in _agsEditor.GetAllScriptHeaders())
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
			this.scintilla.SetClassNamesList(sb.ToString());
		}

        private void UpdateFunctionList()
        {
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
            cmbFunctions.Items.Clear();
            cmbFunctions.Items.Add("(general definitions)");
            functions.Sort();
            foreach (string func in functions)
            {
                cmbFunctions.Items.Add(func);
            }
            SelectFunctionInListForCurrentPosition();
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

                cmbFunctions.SelectedIndex = 0;

                _allowZoomToFunction = true;
            }
        }

        private void AutoComplete_BackgroundCacheUpdateStatusChanged(BackgroundAutoCompleteStatus status, Exception errorDetails)
        {
            if (status == BackgroundAutoCompleteStatus.Finished)
            {
                if (this.IsHandleCreated)
                {
                    this.Invoke(new AnonymousDelegate(UpdateFunctionList));
                }
            }
        }

        public void ActivateTextEditor()
        {
            scintilla.ActivateTextEditor();
        }

        public void DeactivateTextEditor()
        {
            scintilla.DeactivateTextEditor();
        }

        public List<MenuCommand> ToolbarIcons
        {
            get { return _toolbarIcons; }
        }

        public MenuCommands ExtraMenu
        {
            get { return _extraMenu; }
        }

        public Script Script
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

            if (scintilla.IsModified) 
            {
                _script.SaveToDisk();
                scintilla.SetSavePoint();
                if (_script.IsHeader)
                {
                    AutoComplete.ConstructCache(_script);
                }
            }
        }

		public void GoToLine(int lineNumber)
		{
			GoToLine(lineNumber, true, false);
		}

        public void GoToLine(int lineNumber, bool selectLine, bool goToLineAfterOpeningBrace)
        {
            if (goToLineAfterOpeningBrace)
            {
                lineNumber = FindLineNumberAfterOpeningBrace(lineNumber);
            }

            scintilla.GoToLine(lineNumber);

			if (selectLine)
			{
				scintilla.SelectCurrentLine();
			}
        }

        private int FindLineNumberAfterOpeningBrace(int startFromLine)
        {
            while (startFromLine < scintilla.LineCount)
            {
                if (scintilla.GetTextForLine(startFromLine).Contains("{"))
                {
                    return startFromLine + 1;
                }
                startFromLine++;
            }
            return startFromLine;
        }

		public void GoToLineOfCharacterPosition(int position)
		{
			GoToLineOfCharacterPosition(position, true);
		}

		public void GoToLineOfCharacterPosition(int position, bool selectLine)
		{
			scintilla.GoToPosition(position);
			if (selectLine)
			{
				scintilla.SelectCurrentLine();
			}
		}

        public void SetExecutionPointMarker(int lineNumber) 
        {
            scintilla.ShowCurrentExecutionPoint(lineNumber);
        }

        public void SetErrorMessagePopup(string errorMessage)
        {
            scintilla.ShowErrorMessagePopup(errorMessage);
        }

        public int GetLineNumberForText(string text)
        {
            return scintilla.FindLineNumberForText(text);
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
                System.Windows.Forms.Keys.Escape))
            {
                FindReplace.CloseDialogIfNeeded();
            }
        }

        protected override void OnCommandClick(string command)
        {
            if (command == CUT_COMMAND)
            {
                scintilla.Cut();
            }
            else if (command == COPY_COMMAND)
            {
                scintilla.Copy();
            }
            else if (command == PASTE_COMMAND)
            {
                scintilla.Paste();
            }
            else if (command == UNDO_COMMAND)
            {
                if (scintilla.CanUndo())
                {
                    scintilla.Undo();
                }
            }
            else if (command == REDO_COMMAND)
            {
                if (scintilla.CanRedo())
                {
                    scintilla.Redo();
                }
            }
            else if (command == SHOW_AUTOCOMPLETE_COMMAND)
            {
                scintilla.ShowAutocompleteNow();
            }
            else if (command == TOGGLE_BREAKPOINT_COMMAND)
            {
				ToggleBreakpointOnCurrentLine();
            }
			else if (command == MATCH_BRACE_COMMAND)
			{
				scintilla.ShowMatchingBrace(true);
			}
			else if ((command == FIND_COMMAND) || (command == REPLACE_COMMAND)
                || (command == FIND_ALL_COMMAND) || (command == REPLACE_ALL_COMMAND))
			{
                if (scintilla.IsSomeSelectedText())
                {
                    _lastSearchText = scintilla.SelectedText;
                }
                else _lastSearchText = string.Empty;
                ShowFindReplaceDialog(command == REPLACE_COMMAND || command == REPLACE_ALL_COMMAND,
                    command == FIND_ALL_COMMAND || command == REPLACE_ALL_COMMAND);
			}
			else if (command == FIND_NEXT_COMMAND)
			{
				if (_lastSearchText.Length > 0)
				{
					scintilla.FindNextOccurrence(_lastSearchText, _lastCaseSensitive, true);
				}
			}
            UpdateToolbarButtonsIfNecessary();
        }

        private void ShowFindReplaceDialog(bool showReplace, bool showAll)
        {
            FindReplace findReplace = new FindReplace(_script, _agsEditor,
                _lastSearchText, _lastCaseSensitive);
            findReplace.LastSearchTextChanged += new FindReplace.LastSearchTextChangedHandler(findReplace_LastSearchTextChanged);
            findReplace.ShowFindReplaceDialog(showReplace, showAll);
        }

        private void findReplace_LastSearchTextChanged(string searchText)
        {
            _lastSearchText = searchText;
        }

        protected override void OnWindowActivated()
        {
            _agsEditor.RegenerateScriptHeader(_room);
            if (_editorTextModifiedSinceLastCopy)
            {
                UpdateScriptObjectWithLatestTextInWindow();
            }
            AutoComplete.RequestBackgroundCacheUpdate(_script);
            ActivateTextEditor();

			Factory.GUIController.ShowCuppit("You've opened a script editor. This is where you set up how the game will react to various events like the player clicking on things. Read the Scripting Tutorial in the manual to get started.", "Script editor introduction");
        }

        protected override void OnWindowDeactivated()
        {
            DeactivateTextEditor();
        }

        protected override string OnGetHelpKeyword()
        {
            return scintilla.GetFullTypeNameAtCursor();
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
            Factory.Events.FileChangedInGameFolder -= _fileChangedHandler;
            Factory.GUIController.OnMainWindowActivated -= _mainWindowActivatedHandler;
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
            UpdateToolbarButtonsIfNecessary();
            if (cmbFunctions.Items.Count > 0)
            {
                SelectFunctionInListForCurrentPosition();
            }
        }

        private void scintilla_TextModified(int startPos, int length, bool wasAdded)
        {
            _editorTextModifiedSinceLastCopy = true;
            int adjustment = length;
            if (!wasAdded)
            {
                adjustment = -length;
            }
            AdjustStartOfFunctionsInScript(startPos, adjustment);
        }

        private void UpdateToolbarButtonsIfNecessary()
        {
            bool canCutAndCopy = scintilla.CanCutAndCopy();
            bool canPaste = scintilla.CanPaste();
            bool canUndo = scintilla.CanUndo();
            bool canRedo = scintilla.CanRedo();
            if ((_toolbarIcons[0].Enabled != canCutAndCopy) ||
                (_toolbarIcons[2].Enabled != canPaste) ||
                (_toolbarIcons[3].Enabled != canUndo) ||
                (_toolbarIcons[4].Enabled != canRedo))
            {
                _toolbarIcons[0].Enabled = canCutAndCopy;
                _toolbarIcons[1].Enabled = canCutAndCopy;
                _toolbarIcons[2].Enabled = canPaste;
                _toolbarIcons[3].Enabled = canUndo;
                _toolbarIcons[4].Enabled = canRedo;
                _extraMenu.Commands[0].Enabled = canUndo;
                _extraMenu.Commands[1].Enabled = canRedo;
                _extraMenu.Commands[3].Enabled = canCutAndCopy;
                _extraMenu.Commands[4].Enabled = canCutAndCopy;
                _extraMenu.Commands[5].Enabled = canPaste;
                Factory.ToolBarManager.RefreshCurrentPane();
                Factory.MenuManager.RefreshCurrentPane();
            }
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

		private void scintilla_ActivateContextMenu(string commandName)
		{
			UpdateToolbarButtonsIfNecessary();
		}

		private ScriptToken FindTokenInScript(Script script, string structName, string memberName)
		{
			ScriptToken found = null;

			if (structName != null)
			{
				ScriptStruct struc = script.AutoCompleteData.FindStruct(structName);
				if (struc != null)
				{
					found = struc.FindMemberFunction(memberName);
					if (found == null)
					{
						found = struc.FindMemberVariable(memberName);
					}
				}
				else
				{
					found = script.AutoCompleteData.FindFunction(_goToDefinition.Replace(".", "::"));
				}
			}
			else
			{
				found = script.AutoCompleteData.FindFunction(memberName);
				if (found == null)
				{
					found = script.AutoCompleteData.FindVariable(memberName);
				}
				if (found == null)
				{
					found = script.AutoCompleteData.FindStruct(memberName);
				}
			}

			return found;
		}

        public ScriptStruct FindGlobalVariableOrType(string type)
        {
            return scintilla.FindGlobalVariableOrType(type);
        }

		public ScriptToken FindTokenAsLocalVariable(string memberName, bool searchWholeFunction)
		{
			ScriptToken found = null;
            List<ScriptVariable> localVars = scintilla.GetListOfLocalVariablesForCurrentPosition(searchWholeFunction);
			foreach (ScriptVariable localVar in localVars)
			{
				if (localVar.VariableName == memberName)
				{
					found = localVar;
				}
			}
			return found;
		}

        private void FindAllUsages(string structName, string memberName)
        {
            TextProcessing.FindAllUsages findAllUsages = new TextProcessing.FindAllUsages(scintilla,
                this, _script, _agsEditor);
            findAllUsages.Find(structName, memberName);
        }

		private void GoToDefinition(string structName, string memberName)
		{
			ScriptToken found = null;
			Script foundInScript = null;
			List<Script> scriptsToSearch = new List<Script>();
            scriptsToSearch.AddRange(_agsEditor.GetAllScriptHeaders());
			scriptsToSearch.Add(_script);

			foreach (Script script in scriptsToSearch)
			{
				found = FindTokenInScript(script, structName, memberName);
				foundInScript = script;

				if ((found != null) && (script.IsHeader))
				{
					// Always prefer the definition in the main script to
					// the import in the header
                    Script mainScript = _agsEditor.CurrentGame.Scripts.FindMatchingScriptOrHeader(script);
					if (!mainScript.AutoCompleteData.Populated)
					{
						AutoComplete.ConstructCache(mainScript);
					}
					ScriptToken foundInScriptBody = FindTokenInScript(mainScript, structName, memberName);
					if (foundInScriptBody != null)
					{
						found = foundInScriptBody;
						foundInScript = mainScript;
					}
				}

				if (found != null)
				{
					break;
				}
			}

			if ((found == null) && (structName == null))
			{
				found = FindTokenAsLocalVariable(memberName, false);
			}

			if (found != null)
			{
				if (foundInScript.FileName == AGSEditor.BUILT_IN_HEADER_FILE_NAME)
				{
					Factory.GUIController.LaunchHelpForKeyword(_goToDefinition);
				}
				else if (foundInScript.FileName == Tasks.AUTO_GENERATED_HEADER_NAME)
				{
					Factory.GUIController.ShowMessage("This variable is internally defined by AGS and probably corresponds to an in-game entity such as a Character or Inventory Item.", MessageBoxIcon.Information);
				}
				else
				{
					Factory.GUIController.ZoomToFile(foundInScript.FileName, ZoomToFileZoomType.ZoomToCharacterPosition, found.StartsAtCharacterIndex);
				}
			}
		}

		private void ContextMenuChooseOption(object sender, EventArgs e)
		{
			ToolStripMenuItem item = (ToolStripMenuItem)sender;
			if (item.Name == CONTEXT_MENU_TOGGLE_BREAKPOINT)
			{
				ToggleBreakpointOnCurrentLine();
			}
			else if (item.Name == CONTEXT_MENU_GO_TO_DEFINITION ||
                item.Name == CONTEXT_MENU_FIND_ALL_USAGES)
			{
				string[] structAndMember = _goToDefinition.Split('.');
				string structName = null;
				string memberName = structAndMember[0];
				if (structAndMember.Length > 1)
				{
					structName = structAndMember[0];
					memberName = structAndMember[1];
				}

                if (item.Name == CONTEXT_MENU_GO_TO_DEFINITION)
                {
				GoToDefinition(structName, memberName);
			}
                else
                {
                    FindAllUsages(structName, memberName);
                }
			}
            else if (item.Name == CONTEXT_MENU_GO_TO_SPRITE)
            {
                if (!Factory.Events.OnShowSpriteManager(_goToSprite.Value))
                {
                    Factory.GUIController.ShowMessage("Unable to display sprite " + _goToSprite + ". Could not find a sprite with that number.", MessageBoxIcon.Warning);
                }
            }
		}

		private void scintilla_ConstructContextMenu(ContextMenuStrip menuStrip, int clickedPositionInDocument)
		{
			EventHandler onClick = new EventHandler(ContextMenuChooseOption);

            _goToSprite = null;
			string clickedOnType = string.Empty;
			if (!scintilla.InsideStringOrComment(false, clickedPositionInDocument))
			{
				float dummy;
				clickedOnType = scintilla.GetFullTypeNameAtPosition(clickedPositionInDocument);
				// if on nothing, or a number, ignore
				if (clickedOnType.Length > 0)
				{
                    int temp;
                    if (int.TryParse(clickedOnType, out temp))
                    {
                        _goToSprite = temp;
                        clickedOnType = string.Empty;
                    }
                    else if (!float.TryParse(clickedOnType, out dummy))
                    {
                        _goToDefinition = clickedOnType;
                        clickedOnType = " of " + clickedOnType;
                    }
				}
				else
				{
					clickedOnType = string.Empty;
				}
			}

			menuStrip.Items.Add(new ToolStripMenuItem("Go to Definition" + clickedOnType, null, onClick, CONTEXT_MENU_GO_TO_DEFINITION));
			if (clickedOnType == string.Empty)
			{
				menuStrip.Items[menuStrip.Items.Count - 1].Enabled = false;
			}

            menuStrip.Items.Add(new ToolStripMenuItem("Find All Usages" + clickedOnType, null, onClick, CONTEXT_MENU_FIND_ALL_USAGES));
            if (clickedOnType == string.Empty)
            {
                menuStrip.Items[menuStrip.Items.Count - 1].Enabled = false;
            }

            menuStrip.Items.Add(new ToolStripMenuItem("Go to sprite " + (_goToSprite.HasValue ? _goToSprite.ToString() : ""), null, onClick, CONTEXT_MENU_GO_TO_SPRITE));
            if (_goToSprite == null)
            {
                menuStrip.Items[menuStrip.Items.Count - 1].Enabled = false;
            }

			menuStrip.Items.Add(new ToolStripSeparator());
			menuStrip.Items.Add(new ToolStripMenuItem("Toggle Breakpoint", Factory.GUIController.ImageList.Images["ToggleBreakpointMenuIcon"], onClick, CONTEXT_MENU_TOGGLE_BREAKPOINT));
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
