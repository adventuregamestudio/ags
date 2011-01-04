using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class DialogEditor : EditorContentPanel
    {
        private const string dialogKeyWords = "return stop";

        private Dialog _dialog;
        private List<DialogOptionEditor> _optionPanes = new List<DialogOptionEditor>();

        public DialogEditor(Dialog dialogToEdit)
        {
            InitializeComponent();
            _dialog = dialogToEdit;

            scintillaEditor.AutoCompleteEnabled = true;
            scintillaEditor.IgnoreLinesWithoutIndent = true;
			scintillaEditor.AutoSpaceAfterComma = false;
            scintillaEditor.CallTipsEnabled = true;
            scintillaEditor.FixedTypeForThisKeyword = "Dialog";
            scintillaEditor.SetFillupKeys(Constants.AUTOCOMPLETE_ACCEPT_KEYS);
            scintillaEditor.SetKeyWords(dialogKeyWords);
            scintillaEditor.SetAutoCompleteKeyWords(Constants.SCRIPT_KEY_WORDS);
            scintillaEditor.SetText(dialogToEdit.Script);

            flowLayoutPanel1.Controls.Remove(btnNewOption);
            foreach (DialogOption option in dialogToEdit.Options)
            {
                DialogOptionEditor optionEditor = new DialogOptionEditor(option);
                _optionPanes.Add(optionEditor);
                flowLayoutPanel1.Controls.Add(optionEditor);
            }
            flowLayoutPanel1.Controls.Add(btnNewOption);
            flowLayoutPanel1.Controls.Add(btnDeleteOption);

            if (_dialog.Options.Count >= Dialog.MAX_OPTIONS_PER_DIALOG)
            {
                btnNewOption.Visible = false;
            }
            if (_dialog.Options.Count < 1)
            {
                btnDeleteOption.Visible = false;
            }
        }

        public Dialog ItemToEdit
        {
            get { return _dialog; }
        }

        protected override string OnGetHelpKeyword()
        {
            return "Dialogs";
        }
        
        public void SaveData()
        {
            _dialog.Script = scintillaEditor.GetText();
        }

		public void GoToScriptLine(ZoomToFileEventArgs evArgs)
		{
			scintillaEditor.GoToLine(evArgs.ZoomPosition);
			scintillaEditor.Focus();

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
                flowLayoutPanel1.Controls.Remove(_optionPanes[_optionPanes.Count - 1]);
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
            DialogOptionEditor newEditor = new DialogOptionEditor(newOption);
            _optionPanes.Add(newEditor);
            flowLayoutPanel1.Controls.Remove(btnNewOption);
            flowLayoutPanel1.Controls.Remove(btnDeleteOption);
            flowLayoutPanel1.Controls.Add(newEditor);
            flowLayoutPanel1.Controls.Add(btnNewOption);
            flowLayoutPanel1.Controls.Add(btnDeleteOption);
            newEditor.Focus();

            if (_dialog.Options.Count >= Dialog.MAX_OPTIONS_PER_DIALOG)
            {
                btnNewOption.Visible = false;
            }
            else
            {
                btnDeleteOption.Visible = true;
            }

            SaveData();

            // Ensure there is an entry point in the script for this
            if (!_dialog.Script.Contains(Environment.NewLine + "@" + newOption.ID))
            {
                if (!_dialog.Script.EndsWith(Environment.NewLine))
                {
                    _dialog.Script += Environment.NewLine;
                }
                _dialog.Script += "@" + newOption.ID + Environment.NewLine + "return" + Environment.NewLine;
                scintillaEditor.SetText(_dialog.Script);
            }
        }


    }
}
