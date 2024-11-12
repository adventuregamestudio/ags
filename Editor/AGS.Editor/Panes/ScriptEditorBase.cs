﻿using System;
using System.Collections.Generic;
using System.Windows.Forms;
using AGS.Types;
using AGS.Types.AutoComplete;
using AGS.Types.Interfaces;
using AGS.Editor.Components;
using AGS.Editor.TextProcessing;


namespace AGS.Editor
{
    /// <summary>
    /// A parent class for a script editor panel.
    /// Contains shared Edit menu and context menu commands, common operations;
    /// lets to override some of these operations in the derived classes.
    /// </summary>
    public class ScriptEditorBase : EditorContentPanel
    {
        // Common Edit menu commands
        protected const string CUT_COMMAND = "ScriptCut";
        protected const string COPY_COMMAND = "ScriptCopy";
        protected const string PASTE_COMMAND = "ScriptPaste";
        protected const string UNDO_COMMAND = "ScriptUndo";
        protected const string REDO_COMMAND = "ScriptRedo";
        private const string SHOW_AUTOCOMPLETE_COMMAND = "ScriptShowAutoComplete";
        private const string MATCH_BRACE_COMMAND = "MatchBrace";
        private const string FIND_COMMAND = "ScriptFind";
        private const string FIND_NEXT_COMMAND = "ScriptFindNext";
        private const string REPLACE_COMMAND = "ScriptReplace";
        private const string FIND_ALL_COMMAND = "ScriptFindAll";
        private const string REPLACE_ALL_COMMAND = "ScriptReplaceAll";
        private const string GOTO_LINE_COMMAND = "ScriptGotoLine";
        private const string GO_TO_DEFINITION_COMMAND = "ScriptGoToDefiniton";
        private const string FIND_ALL_USAGES_COMMAND = "ScriptFindAllUsages";
        private const string GO_TO_SPRITE_COMMAND = "ScriptGoToSprite";
        private const string ADD_TO_WATCH_PANE_COMMAND = "ScriptAddToWatch";

        protected AGSEditor _agsEditor;
        // Loaded script reference, is assigned by the child class.
        private IScript _iScript;
        // Scintilla's reference is assigned by the child class from their
        // own scintilla member.
        // TODO: refactor to have it exclusively member of the base class,
        // possibly created and configured in the child class
        private ScintillaWrapper _scintilla;

        // Menus
        private MenuCommands _extraMenu = new MenuCommands("&Edit", GUIController.FILE_MENU_ID);
        private List<MenuCommand> _toolbarIcons = new List<MenuCommand>();
        // Menu item refs, for tracking their state
        private MenuCommand _menuCmdUndo,
             _menuCmdRedo,
            _menuCmdCut,
            _menuCmdCopy,
            _menuCmdPaste,
            _menuCmdGoToDefinition,
            _menuCmdAddToWatchPane,
            _menuCmdFindAllUsages,
            _menuCmdGoToSprite;

        // Find/replace data
        // TODO: pick this out into a separate Find/Replace class?
        private string _lastSearchText = string.Empty;
        private bool _lastCaseSensitive = false;
        // Other operations data
        private int? _goToSprite = null;
        private string _goToDefinition = null;


        public ScriptEditorBase(AGSEditor agsEditor)
        {
            _agsEditor = agsEditor;

            InitEditorBase();
        }

        public MenuCommands ExtraMenu
        {
            get { return _extraMenu; }
        }

        public List<MenuCommand> ToolbarIcons
        {
            get { return _toolbarIcons; }
        }

        /// <summary>
        /// Gets instance of IScript loaded into this script editor pane.
        /// Currently this is for the internal use only.
        /// Must be implemented in the child class.
        /// </summary>
        protected IScript Script
        {
            get { return _iScript; }
            set { _iScript = value; }
        }

        /// <summary>
        /// Lets for a child class to assign an actual Scintilla control.
        /// </summary>
        protected ScintillaWrapper Scintilla
        {
            get { return _scintilla; }
            set
            {
                if (_scintilla != null)
                    DisconnectScintilla();
                InitScintilla(value);
            }
        }

        private void InitEditorBase()
        {
            InitEditorMenus();
        }

        private void InitScintilla(ScintillaWrapper scintilla)
        {
            _scintilla = scintilla;
            _scintilla.ConstructContextMenu += scintilla_ConstructContextMenu;
            _scintilla.ActivateContextMenu += scintilla_ActivateContextMenu;
            _scintilla.UpdateUI += scintilla_UpdateUI;
        }

        private void DisconnectScintilla()
        {
            _scintilla.ConstructContextMenu -= scintilla_ConstructContextMenu;
            _scintilla.ActivateContextMenu -= scintilla_ActivateContextMenu;
            _scintilla.UpdateUI -= scintilla_UpdateUI;
            _scintilla = null;
        }

        #region Menus and Toolbar

        private void InitEditorMenus()
        {
            _menuCmdUndo = new MenuCommand(UNDO_COMMAND, "Undo", "UndoMenuIcon");
            _menuCmdUndo.ShortcutKeyDisplayString = "Ctrl+Z";
            _menuCmdRedo = new MenuCommand(REDO_COMMAND, "Redo", "RedoMenuIcon");
            _menuCmdRedo.ShortcutKeyDisplayString = "Ctrl+Y";
            _menuCmdCut = new MenuCommand(CUT_COMMAND, "Cut", "CutMenuIcon");
            _menuCmdCut.ShortcutKeyDisplayString = "Ctrl+X";
            _menuCmdCopy = new MenuCommand(COPY_COMMAND, "Copy", "CopyMenuIcon");
            _menuCmdCopy.ShortcutKeyDisplayString = "Ctrl+C";
            _menuCmdPaste = new MenuCommand(PASTE_COMMAND, "Paste", "PasteMenuIcon");
            _menuCmdPaste.ShortcutKeyDisplayString = "Ctrl+V";
            _extraMenu.Commands.Add(_menuCmdUndo);
            _extraMenu.Commands.Add(_menuCmdRedo);
            _extraMenu.Commands.Add(MenuCommand.Separator);
            _extraMenu.Commands.Add(_menuCmdCut);
            _extraMenu.Commands.Add(_menuCmdCopy);
            _extraMenu.Commands.Add(_menuCmdPaste);
            _extraMenu.Commands.Add(MenuCommand.Separator);
            _extraMenu.Commands.Add(new MenuCommand(FIND_COMMAND, "Find...", Keys.Control | Keys.F, "FindMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(FIND_NEXT_COMMAND, "Find next", Keys.F3, "FindNextMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(REPLACE_COMMAND, "Replace...", Keys.Control | Keys.E));
            _extraMenu.Commands.Add(MenuCommand.Separator);
            _extraMenu.Commands.Add(new MenuCommand(FIND_ALL_COMMAND, "Find All...", Keys.Control | Keys.Shift | Keys.F, "FindMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(REPLACE_ALL_COMMAND, "Replace All...", Keys.Control | Keys.Shift | Keys.E));
            _extraMenu.Commands.Add(MenuCommand.Separator);
            _extraMenu.Commands.Add(new MenuCommand(SHOW_AUTOCOMPLETE_COMMAND, "Show Autocomplete", Keys.Control | Keys.Space, "ShowAutocompleteMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(MATCH_BRACE_COMMAND, "Match Brace", Keys.Control | Keys.B));
            _extraMenu.Commands.Add(new MenuCommand(GOTO_LINE_COMMAND, "Go to Line...", Keys.Control | Keys.G));

            _menuCmdGoToDefinition = new MenuCommand(GO_TO_DEFINITION_COMMAND, "Go to Definition", Keys.F12);
            _menuCmdGoToDefinition.Enabled = false;
            _menuCmdAddToWatchPane = new MenuCommand(ADD_TO_WATCH_PANE_COMMAND, "Add to Watch", null);
            _menuCmdAddToWatchPane.Enabled = false;
            _menuCmdFindAllUsages = new MenuCommand(FIND_ALL_USAGES_COMMAND, "Find All Usages", Keys.Shift | Keys.F12);
            _menuCmdFindAllUsages.Enabled = false;
            _menuCmdGoToSprite = new MenuCommand(GO_TO_SPRITE_COMMAND, "Go to Sprite", Keys.Shift | Keys.F7);
            _menuCmdGoToSprite.Enabled = false;

            _extraMenu.Commands.Add(MenuCommand.Separator);
            _extraMenu.Commands.Add(_menuCmdGoToDefinition);
            _extraMenu.Commands.Add(_menuCmdAddToWatchPane);
            _extraMenu.Commands.Add(_menuCmdFindAllUsages);
            _extraMenu.Commands.Add(_menuCmdGoToSprite);

            AddEditMenuCommands(_extraMenu);

            _toolbarIcons.Add(_menuCmdCut);
            _toolbarIcons.Add(_menuCmdCopy);
            _toolbarIcons.Add(_menuCmdPaste);
            _toolbarIcons.Add(_menuCmdUndo);
            _toolbarIcons.Add(_menuCmdRedo);
            AddToolbarCommands(_toolbarIcons);
        }

        /// <summary>
        /// Lets derived editors to add their specific Edit menu commands.
        /// </summary>
        /// <param name="commands"></param>
        protected virtual void AddEditMenuCommands(MenuCommands commands)
        {
        }

        /// <summary>
        /// Lets derived editors to add their specific toolbar commands.
        /// </summary>
        /// <param name=""></param>
        protected virtual void AddToolbarCommands(List<MenuCommand> toolbar)
        {
        }

        /// <summary>
        /// Lets derived editors to add their specific context menu commands.
        /// </summary>
        /// <param name="menuStrip"></param>
        protected virtual void AddCtxCommands(ContextMenuStrip menuStrip)
        {
        }

        /// <summary>
        /// Performs a command.
        /// May be overriden in derived classes for their specific commands.
        /// </summary>
        /// <param name="command"></param>
        protected override void OnCommandClick(string command)
        {
            if (IsStandardEditCommand(command))
            {
                if (!_scintilla.ContainsFocus) return;
                if (command == CUT_COMMAND)
                {
                    _scintilla.Cut();
                }
                else if (command == COPY_COMMAND)
                {
                    _scintilla.Copy();
                }
                else if (command == PASTE_COMMAND)
                {
                    _scintilla.Paste();
                }
                else if (command == UNDO_COMMAND)
                {
                    if (_scintilla.CanUndo())
                    {
                        _scintilla.Undo();
                    }
                }
                else if (command == REDO_COMMAND)
                {
                    if (_scintilla.CanRedo())
                    {
                        _scintilla.Redo();
                    }
                }
            }
            else if (IsContextCommand(command))
            {
                if (command == ADD_TO_WATCH_PANE_COMMAND)
                {
                    Factory.GUIController.AddVariableToWatchPanel(_goToDefinition);
                }
                else if (command == GO_TO_DEFINITION_COMMAND ||
                    command == FIND_ALL_USAGES_COMMAND)
                {
                    string[] structAndMember = _goToDefinition.Split('.');
                    string structName = null;
                    string memberName = structAndMember[0];
                    if (structAndMember.Length > 1)
                    {
                        structName = structAndMember[0];
                        memberName = structAndMember[1];
                    }

                    if (command == GO_TO_DEFINITION_COMMAND)
                    {
                        GoToDefinition(structName, memberName);
                    }
                    else
                    {
                        FindAllUsages(structName, memberName);
                    }
                }
                else if (command == GO_TO_SPRITE_COMMAND)
                {
                    if (!Factory.Events.OnShowSpriteManager(_goToSprite.Value))
                    {
                        Factory.GUIController.ShowMessage("Unable to display sprite " + _goToSprite + ". Could not find a sprite with that number.", MessageBoxIcon.Warning);
                    }
                }
            }
            else 
            {
                _scintilla.Focus();
                if (command == SHOW_AUTOCOMPLETE_COMMAND)
                {
                    _scintilla.ShowAutocompleteNow();
                }
                else if (command == MATCH_BRACE_COMMAND)
                {
                    _scintilla.ShowMatchingBraceIfPossible();
                }
                else if (command == GOTO_LINE_COMMAND)
                {
                    GotoLineDialog gotoLineDialog = new GotoLineDialog
                    {
                        Minimum = 1,
                        Maximum = _scintilla.LineCount,
                        LineNumber = _scintilla.CurrentLine + 1
                    };
                    if (gotoLineDialog.ShowDialog() != DialogResult.OK) return;
                    GoToLine(gotoLineDialog.LineNumber);
                }
                else if ((command == FIND_COMMAND) || (command == REPLACE_COMMAND)
                    || (command == FIND_ALL_COMMAND) || (command == REPLACE_ALL_COMMAND))
                {
                    if (_scintilla.IsSomeSelectedText())
                    {
                        _lastSearchText = _scintilla.SelectedText;
                    }
                    else _lastSearchText = string.Empty;
                    ShowFindReplaceDialog(command == REPLACE_COMMAND || command == REPLACE_ALL_COMMAND,
                        command == FIND_ALL_COMMAND || command == REPLACE_ALL_COMMAND);
                }
                else if (command == FIND_NEXT_COMMAND)
                {
                    if (_lastSearchText.Length > 0)
                    {
                        _scintilla.FindNextOccurrence(_lastSearchText, _lastCaseSensitive, true);
                    }
                }
            }
            UpdateUICommands();
        }

        /// <summary>
        /// Updates the state of menu commands (this affects both menu and toolbar icons).
        /// May be overriden in derived classes for their specific commands.
        /// </summary>
        protected virtual void UpdateUICommands()
        {
            UpdateScriptDocumentContext(_scintilla.CurrentPos);

            bool canCutAndCopy = _scintilla.CanCutAndCopy();
            bool canPaste = _scintilla.CanPaste();
            bool canUndo = _scintilla.CanUndo();
            bool canRedo = _scintilla.CanRedo();
            bool canGoToDefinition = _goToDefinition != null;
            bool canGoToSprite = _goToSprite != null;

            bool changed =
                (_menuCmdCopy.Enabled != canCutAndCopy) ||
                (_menuCmdPaste.Enabled != canPaste) ||
                (_menuCmdUndo.Enabled != canUndo) ||
                (_menuCmdRedo.Enabled != canRedo) ||
                (_menuCmdGoToDefinition.Enabled != canGoToDefinition) ||
                (_menuCmdAddToWatchPane.Enabled != canGoToDefinition) ||
                (_menuCmdFindAllUsages.Enabled != canGoToDefinition) ||
                (_menuCmdGoToSprite.Enabled != canGoToSprite);

            if (changed)
            {
                _menuCmdCopy.Enabled = canCutAndCopy;
                _menuCmdCut.Enabled = canCutAndCopy;
                _menuCmdPaste.Enabled = canPaste;
                _menuCmdUndo.Enabled = canUndo;
                _menuCmdRedo.Enabled = canRedo;
                _menuCmdGoToDefinition.Enabled = canGoToDefinition;
                _menuCmdAddToWatchPane.Enabled = canGoToDefinition;
                _menuCmdFindAllUsages.Enabled = canGoToDefinition;
                _menuCmdGoToSprite.Enabled = canGoToSprite;

                Factory.ToolBarManager.RefreshCurrentPane();
                Factory.MenuManager.RefreshCurrentPane();
            }
        }

        protected static bool IsStandardEditCommand(string c)
        {
            return (c == CUT_COMMAND) || (c == COPY_COMMAND) || (c == PASTE_COMMAND) || (c == UNDO_COMMAND) || (c == REDO_COMMAND);
        }

        protected static bool IsContextCommand(string c)
        {
            return (c == GO_TO_DEFINITION_COMMAND) ||
                (c == FIND_ALL_USAGES_COMMAND) ||
                (c == GO_TO_SPRITE_COMMAND) ||
                (c == ADD_TO_WATCH_PANE_COMMAND);
        }

        protected void UpdateScriptDocumentContext(int clickedPositionInDocument)
        {
            _goToSprite = null;
            _goToDefinition = null;

            if (_scintilla.InsideStringOrComment(clickedPositionInDocument))
            {
                return;
            }

            string clickedOnType = _scintilla.GetFullTypeNameAtPosition(clickedPositionInDocument);
            
            // if on nothing, or a number, ignore
            if (clickedOnType.Length > 0)
            {
                int temp;
                float dummy;

                if (int.TryParse(clickedOnType, out temp))
                {
                    _goToSprite = temp;
                }
                else if (!float.TryParse(clickedOnType, out dummy))
                {
                    _goToDefinition = clickedOnType;
                }
            }
        }

        protected void EnableStandardEditCommands(bool copy = true, bool cut = true, bool paste = true, bool undo = true, bool redo = true)
        {
            _menuCmdCopy.Enabled = copy;
            _menuCmdCut.Enabled = cut;
            _menuCmdPaste.Enabled = paste;
            _menuCmdUndo.Enabled = undo;
            _menuCmdRedo.Enabled = redo;
            Factory.ToolBarManager.RefreshCurrentPane();
            Factory.MenuManager.RefreshCurrentPane();
        }

        private void scintilla_UpdateUI(object sender, EventArgs e)
        {
            UpdateUICommands();
        }

        private void scintilla_ConstructContextMenu(ContextMenuStrip menuStrip, int clickedPositionInDocument)
        {
            EventHandler onClick = new EventHandler(ContextMenuChooseOption);
            ToolStripMenuItem menuItem;

            UpdateScriptDocumentContext(clickedPositionInDocument);

            string typeName = _goToDefinition != null ? (" of " +  _goToDefinition) : string.Empty;
            string varName = _goToDefinition != null ? _goToDefinition : string.Empty;

            menuItem = new ToolStripMenuItem(_menuCmdGoToDefinition.Name + typeName, null, onClick, GO_TO_DEFINITION_COMMAND);
            menuItem.ShortcutKeys = _menuCmdGoToDefinition.ShortcutKey;
            menuItem.Enabled = (_goToDefinition != null);
            menuStrip.Items.Add(menuItem);

            menuItem = new ToolStripMenuItem(_menuCmdFindAllUsages.Name + typeName, null, onClick, FIND_ALL_USAGES_COMMAND);
            menuItem.ShortcutKeys = _menuCmdFindAllUsages.ShortcutKey;
            menuItem.Enabled = (_goToDefinition != null);
            menuStrip.Items.Add(menuItem);

            menuItem = new ToolStripMenuItem("Add " + varName + " to Watch Panel", null, onClick, ADD_TO_WATCH_PANE_COMMAND);
            menuItem.ShortcutKeys = _menuCmdAddToWatchPane.ShortcutKey;
            menuItem.Enabled = (_goToDefinition != null);
            menuStrip.Items.Add(menuItem);

            menuItem = new ToolStripMenuItem(_menuCmdGoToSprite.Name + (_goToSprite.HasValue ? " " + _goToSprite.ToString() : ""), null, onClick, GO_TO_SPRITE_COMMAND);
            menuItem.ShortcutKeys = _menuCmdGoToSprite.ShortcutKey;
            menuItem.Enabled = (_goToSprite != null);
            menuStrip.Items.Add(menuItem);

            AddCtxCommands(menuStrip);
        }

        private void scintilla_ActivateContextMenu(string commandName)
        {
            UpdateUICommands();
        }

        private void ContextMenuChooseOption(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            OnCommandClick(item.Name);
        }

        #endregion

        #region Operations

        public void GoToLine(int lineNumber)
        {
            GoToLine(lineNumber, false, false);
        }

        public void GoToLine(int lineNumber, bool selectLine, bool goToLineAfterOpeningBrace)
        {
            if (goToLineAfterOpeningBrace)
            {
                lineNumber = FindLineNumberAfterOpeningBrace(lineNumber);
            }

            _scintilla.GoToLine(lineNumber);

            if (selectLine)
            {
                _scintilla.SelectCurrentLine();
            }
        }

        private int FindLineNumberAfterOpeningBrace(int startFromLine)
        {
            while (startFromLine < _scintilla.LineCount)
            {
                if (_scintilla.GetTextForLine(startFromLine).Contains("{"))
                {
                    return startFromLine + 1;
                }
                startFromLine++;
            }
            return startFromLine;
        }

        private void ShowFindReplaceDialog(bool showReplace, bool showAll)
        {
            FindReplace findReplace = new FindReplace(_iScript, _agsEditor,
                _lastSearchText, _lastCaseSensitive);
            findReplace.LastSearchTextChanged += new FindReplace.LastSearchTextChangedHandler(findReplace_LastSearchTextChanged);
            findReplace.ShowFindReplaceDialog(showReplace, showAll);
        }

        private void findReplace_LastSearchTextChanged(string searchText)
        {
            _lastSearchText = searchText;
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
                if (found == null)
                {
                    found = script.AutoCompleteData.FindEnum(memberName);
                }
                if (found == null)
                {
                    found = script.AutoCompleteData.FindEnumValue(memberName);
                }
                if (found == null)
                {
                    found = script.AutoCompleteData.FindDefine(memberName);
                }
            }

            return found;
        }

        public ScriptStruct FindGlobalVariableOrType(string type)
        {
            return _scintilla.FindGlobalVariableOrType(type);
        }

        public ScriptToken FindTokenAsLocalVariable(string memberName, bool searchWholeFunction)
        {
            ScriptToken found = null;
            List<ScriptVariable> localVars = _scintilla.GetListOfLocalVariablesForCurrentPosition(searchWholeFunction);
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
            TextProcessing.FindAllUsages findAllUsages = new TextProcessing.FindAllUsages(_scintilla,
                this, _iScript, _agsEditor);
            findAllUsages.Find(structName, memberName);
        }

        private void GoToDefinition(string structName, string memberName)
        {
            ScriptToken found = null;
            Script foundInScript = null;
            List<IScript> scriptsToSearch = new List<IScript>();
            scriptsToSearch.AddRange(_agsEditor.GetAllScriptHeaders()); // all scripts!
            scriptsToSearch.Add(_iScript);

            foreach (Script script in scriptsToSearch)
            {
                found = FindTokenInScript(script, structName, memberName);
                foundInScript = script;

                if ((found != null) && (script.IsHeader))
                {
                    // Always prefer the definition in the main script to
                    // the import in the header
                    Script mainScript = _agsEditor.CurrentGame.RootScriptFolder.FindMatchingScriptOrHeader(script);
                    if (mainScript != null)
                    {
                        if (!mainScript.AutoCompleteData.Populated)
                        {
                            AutoComplete.ConstructCache(mainScript, _agsEditor.GetImportedScriptHeaders(mainScript));
                        }
                        ScriptToken foundInScriptBody = FindTokenInScript(mainScript, structName, memberName);
                        if (foundInScriptBody != null)
                        {
                            found = foundInScriptBody;
                            foundInScript = mainScript;
                        }
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
                    if (_goToDefinition == "player")
                    {
                        CharactersComponent charactersComponent = Factory.ComponentController.FindComponent<CharactersComponent>();
                        charactersComponent.ShowPlayerCharacter();
                    }
                    else
                    {
                        Factory.GUIController.LaunchHelpForKeyword(_goToDefinition);
                    }
                }
                else if (foundInScript.FileName == Tasks.AUTO_GENERATED_HEADER_NAME)
                {
                    string scriptElementType = null, scriptElementName = null;
                    if (found is ScriptVariable)
                    {
                        ScriptVariable sVar = found as ScriptVariable;
                        scriptElementType = sVar.Type;
                        scriptElementName = sVar.VariableName;
                    }
                    else if (found is ScriptEnumValue)
                    {
                        ScriptEnumValue sEnum = found as ScriptEnumValue;
                        scriptElementType = sEnum.Type;
                        scriptElementName = sEnum.Name;
                    }

                    if (!string.IsNullOrEmpty(scriptElementType))
                    { 
                        BaseComponent component = Factory.ComponentController.FindComponentThatManageScriptElement(scriptElementType) as BaseComponent;
                        if(component != null)
                        {
                            component.ShowItemPaneByName(scriptElementName);
                        }
                        else
                        {
                            Factory.GUIController.ShowMessage("This variable is internally defined by AGS and probably corresponds to an in-game entity which does not support Go to Definition at the moment.", MessageBoxIcon.Information);
                        }
                    }
                    else
                    {
                        Factory.GUIController.ShowMessage("This is internally defined by AGS.", MessageBoxIcon.Information);
                    }
                }
                else if (foundInScript.FileName == GlobalVariablesComponent.GLOBAL_VARS_HEADER_FILE_NAME)
                {
                    IGlobalVariablesController globalVariables = (IGlobalVariablesController)Factory.ComponentController.FindComponentThatImplementsInterface(typeof(IGlobalVariablesController));
                    globalVariables.SelectGlobalVariable(_goToDefinition);
                }
                else
                {
                    Factory.GUIController.ZoomToFile(foundInScript.FileName, ZoomToFileZoomType.ZoomToCharacterPosition, found.StartsAtCharacterIndex);
                }
            }
        }

        #endregion
    }
}
