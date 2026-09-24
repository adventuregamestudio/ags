using AGS.Editor.TextProcessing;
using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class DialogEditor : ScriptEditorBase
    {
        private const string dialogKeyWords = "return stop";

        private Dialog _dialog;
        private DialogScript _script;
        private List<DialogOptionEditor> _optionPanes = new List<DialogOptionEditor>();

        public DialogEditor(Dialog dialogToEdit, AGSEditor agsEditor)
            : base(agsEditor)
        {
            InitializeComponent();
            Init(dialogToEdit);
            this.Load += DialogEditor_Load;
            this.ScriptChangedExternally += OnFileChangedExternally;
        }

        private void Init(Dialog dialog)
        {
            _dialog = dialog;
            _script = dialog.Script;
            // Also give script reference to the base class
            base.Script = dialog.Script;

            InitScintilla();

            flowLayoutPanel1.Controls.Remove(btnNewOption);
            foreach (DialogOption option in _dialog.Options)
            {
                DialogOptionEditor optionEditor = new DialogOptionEditor(option, DialogOptionChanged);
                RegisterDialogOptionsEditorEvents(optionEditor);
                _optionPanes.Add(optionEditor);
                flowLayoutPanel1.Controls.Add(optionEditor);
            }
            flowLayoutPanel1.Controls.Add(btnNewOption);
            flowLayoutPanel1.Controls.Add(btnDeleteOption);

            if (_dialog.Options.Count < 1)
            {
                btnDeleteOption.Visible = false;
            }

            RegisterEvents();
            scintillaEditor.ActivateTextEditor();
        }

        private void RegisterDialogOptionsEditorEvents(DialogOptionEditor dialogOptionEditor)
        {
            foreach (Control c in dialogOptionEditor.Controls)
            {
                TextBox textBox = c as TextBox;
                if (textBox != null)
                {
                    textBox.GotFocus += dialogOptionsEditorTextBox_GotFocus;
                    textBox.MouseUp += dialogOptionsEditorTextBox_MouseUp;
                    textBox.TextChanged += dialogOptionsEditorTextBox_TextChanged;
                    textBox.KeyUp += dialogOptionsEditorTextBox_KeyUp;
                }
            }
        }

        private void UnregisterDialogOptionsEditorEvents(DialogOptionEditor dialogOptionEditor)
        {
            foreach (Control c in dialogOptionEditor.Controls)
            {
                TextBox textBox = c as TextBox;
                if (textBox != null)
                {
                    textBox.GotFocus -= dialogOptionsEditorTextBox_GotFocus;
                    textBox.MouseUp -= dialogOptionsEditorTextBox_MouseUp;
                    textBox.TextChanged -= dialogOptionsEditorTextBox_TextChanged;
                    textBox.KeyUp -= dialogOptionsEditorTextBox_KeyUp;
                }
            }
        }

        private void DialogEditor_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }

        protected override void RegisterEvents()
        {
        }

        protected override void UnregisterEvents()
        {
            foreach(Control c in flowLayoutPanel1.Controls)
            {
                DialogOptionEditor dialogOptionEditor = c as DialogOptionEditor;
                if(dialogOptionEditor != null)
                {
                    UnregisterDialogOptionsEditorEvents(dialogOptionEditor);
                }
            }
        }

        private void InitScintilla()
        {
            scintillaEditor.DialogScriptStyle = true;
            scintillaEditor.AutoCompleteEnabled = true;
            scintillaEditor.IgnoreLinesWithoutIndent = true;
            scintillaEditor.AutoSpaceAfterComma = false;
            scintillaEditor.CallTipsEnabled = true;
            scintillaEditor.FixedTypeForThisKeyword = "Dialog";
            scintillaEditor.SetFillupKeys(Constants.AUTOCOMPLETE_ACCEPT_KEYS);
            //scintillaEditor.SetKeyWords(dialogKeyWords);
            scintillaEditor.SetKeyWords(Constants.SCRIPT_KEY_WORDS);
            scintillaEditor.SetKeyWords(BuildCharacterKeywords(), ScintillaWrapper.WordListType.GlobalClasses, true);
            scintillaEditor.SetAutoCompleteSource(_script);
            scintillaEditor.SetText(_script.Text);
            scintillaEditor.EnableLineNumbers();

            // Assign Scintilla reference to the base class
            Scintilla = scintillaEditor;
        }

        public ScintillaWrapper ScriptEditor
        {
            get { return scintillaEditor; }
        }

        public Dialog ItemToEdit
        {
            get { return _dialog; }
        }

        public override string GetScriptTabName()
        {
            return _dialog.WindowTitle + (IsModified ? " *" : "");
        }

        protected override void OnKeyPressed(Keys keyData)
        {
            if (keyData.Equals(Keys.Escape))
            {
                FindReplace.CloseDialogIfNeeded();
            }
        }

        protected override string OnGetHelpKeyword()
        {
            if (scintillaEditor.ContainsFocus)
            {
                var keyword = scintillaEditor.GetFullTypeNameAtCursor();
                if (string.IsNullOrEmpty(keyword))
                    return "Dialog Script";
                return keyword;
            }
            else
            {
                return "Dialog Editor";
            }
        }

        // TODO: find a way to merge this with ScriptEditor.OnPanelClosing and move to ScriptEditorBase
        protected override void OnPanelClosing(bool canCancel, ref bool cancelClose)
        {
            if ((canCancel) && (scintillaEditor.IsModified))
            {
                DialogResult answer = MessageBox.Show("Do you want to save your changes in script before closing?", "Save changes?", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
                if (answer == DialogResult.Cancel)
                {
                    cancelClose = true;
                }
                else if (answer == DialogResult.Yes)
                {
                    UnregisterEvents();
                    SaveChanges();
                    return;
                }
                else if (File.Exists(_script.FileName))
                {
                    // Revert back to saved version
                    _script.LoadFromDisk();
                    scintillaEditor.SetText(_script.Text);
                }
            }

            // Do anything necessary before the panel is closed
            if (!cancelClose)
            {
                UnregisterEvents();
            }
        }

        private String BuildCharacterKeywords()
        {
            StringBuilder sb = new StringBuilder(300);
            foreach (Character c in _agsEditor.CurrentGame.RootCharacterFolder.AllItemsFlat)
            {
                sb.Append(c.ScriptName.TrimStart('c') + " ");
                sb.Append(c.ScriptName.TrimStart('c').ToLowerInvariant() + " ");
            }
            return sb.ToString();
        }

        public void SaveData()
        {
            _script.Text = scintillaEditor.GetText();
            scintillaEditor.SetSavePoint();
        }

        // TODO: find a way to merge this with ScriptEditor.SaveChanges and move to ScriptEditorBase
        public override void SaveChanges()
        {
            if (!scintillaEditor.IsDisposed && scintillaEditor.IsModified)
            {
                _script.Text = scintillaEditor.GetText();
                BeforeSave();
                _script.SaveToDisk();
                AfterSave();
                scintillaEditor.SetSavePoint();
            }
        }

        public void GoToScriptLine(ZoomToFileEventArgs evArgs)
        {
            if (evArgs.ZoomType == ZoomToFileZoomType.ZoomToCharacterPosition)
            {
                scintillaEditor.GoToPosition(evArgs.ZoomPosition);
            }
            else if (evArgs.ZoomType == ZoomToFileZoomType.ZoomToLineNumber)
            {
                scintillaEditor.GoToLine(evArgs.ZoomPosition);
            }
            scintillaEditor.Focus();
            if(evArgs.SelectLine)
            {
                scintillaEditor.SelectCurrentLine();
            }

            if (evArgs.IsDebugExecutionPoint)
            {
                scintillaEditor.ShowCurrentExecutionPoint(evArgs.ZoomPosition);
                if (evArgs.ErrorMessage != null)
                {
                    scintillaEditor.ShowErrorMessagePopup(evArgs.ErrorMessage);
                }
            }
        }

        public void RemoveExecutionPointMarker()
        {
            scintillaEditor.HideCurrentExecutionPoint();
            scintillaEditor.HideErrorMessagePopup();
        }

        private void btnDeleteOption_Click(object sender, EventArgs e)
        {
            if (Factory.GUIController.ShowQuestion("Are you sure you want to delete the last option?") == DialogResult.Yes)
            {
                _dialog.Options.RemoveAt(_dialog.Options.Count - 1);
                DialogOptionEditor editorToDelete = _optionPanes[_optionPanes.Count - 1];
                UnregisterDialogOptionsEditorEvents(editorToDelete);
                flowLayoutPanel1.Controls.Remove(editorToDelete);
                _optionPanes.RemoveAt(_optionPanes.Count - 1);

                SaveData();

                if (_dialog.Options.Count < 1)
                {
                    btnDeleteOption.Visible = false;
                }
                else
                {
                    btnNewOption.Visible = true;
                }
            }
        }

        private void updateEditMenuForTextbox(TextBox tbox)
        {
            bool can_copy_cut = tbox.SelectionLength > 0;
            EnableStandardEditCommands(copy: can_copy_cut, cut: can_copy_cut, paste: true, undo: tbox.CanUndo, redo: false);
        }

        private void dialogOptionsEditorTextBox_Event(object sender, EventArgs e)
        {
            TextBox tbox = sender as TextBox;
            if (tbox == null) return;
            updateEditMenuForTextbox(tbox);
        }

        private void dialogOptionsEditorTextBox_MouseUp(object sender, EventArgs e)
        {
            dialogOptionsEditorTextBox_Event(sender, e);
        }

        private void dialogOptionsEditorTextBox_GotFocus(object sender, EventArgs e)
        {
            dialogOptionsEditorTextBox_Event(sender, e);
        }

        private void dialogOptionsEditorTextBox_TextChanged(object sender, EventArgs e)
        {
            dialogOptionsEditorTextBox_Event(sender, e);
        }

        private void dialogOptionsEditorTextBox_KeyUp(object sender, EventArgs e)
        {
            dialogOptionsEditorTextBox_Event(sender, e);
        }

        protected override void OnCommandClick(string command)
        {
            if (IsStandardEditCommand(command))
            {
                Control c = Utilities.GetControlThatHasFocus();
                TextBox tbox = c as TextBox;
                if (tbox != null)
                {
                    if (command == CUT_COMMAND) tbox.Cut();
                    else if (command == COPY_COMMAND) tbox.Copy();
                    else if (command == PASTE_COMMAND) tbox.Paste();
                    else if (command == UNDO_COMMAND) tbox.Undo();

                    updateEditMenuForTextbox(tbox);

                    return;
                }
            }

            base.OnCommandClick(command);
        }

        private void btnNewOption_Click(object sender, EventArgs e)
        {
            DialogOption newOption = new DialogOption();
            newOption.ID = _dialog.Options.Count + 1;
            if (_dialog.Options.Count > 0)
            {
                // Copy Show & Say settings from previous option
                newOption.Say = _dialog.Options[_dialog.Options.Count - 1].Say;
                newOption.Show = _dialog.Options[_dialog.Options.Count - 1].Show;
            }
            else
            {
                newOption.Say = true;
                newOption.Show = true;
            }
            _dialog.Options.Add(newOption);
            DialogOptionEditor newEditor = new DialogOptionEditor(newOption, DialogOptionChanged);
            RegisterDialogOptionsEditorEvents(newEditor);

            _optionPanes.Add(newEditor);
            flowLayoutPanel1.Controls.Remove(btnNewOption);
            flowLayoutPanel1.Controls.Remove(btnDeleteOption);
            flowLayoutPanel1.Controls.Add(newEditor);
            flowLayoutPanel1.Controls.Add(btnNewOption);
            flowLayoutPanel1.Controls.Add(btnDeleteOption);
            newEditor.Focus();

            btnDeleteOption.Visible = true;

            SaveData();

            // Ensure there is an entry point in the script for this
            if (!_script.Text.Contains(Environment.NewLine + "@" + newOption.ID))
            {
                if (!_script.Text.EndsWith(Environment.NewLine))
                {
                    _script.Text += Environment.NewLine;
                }
                _script.Text += "@" + newOption.ID + Environment.NewLine + "return" + Environment.NewLine;
                scintillaEditor.SetText(_script.Text);
            }
        }

        private void DialogOptionChanged(object sender, EventArgs e)
        {
            // --- disabled until Dialog.DisplayOptions(eSayAlways/eSayNever) question is resolved ---
            //_dialog.ScriptChangedSinceLastConverted = true;
        }

        private void OnFileChangedExternally(object sender, EventArgs e)
        {
            _script.LoadFromDisk();
            scintillaEditor.SetText(_script.Text);
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.ControlHelper(this, "dialog-editor");
            t.ButtonHelper(btnDeleteOption, "dialog-editor/btn-delete-option");
            t.ButtonHelper(btnNewOption, "dialog-editor/btn-new-option");
        }
    }
}
