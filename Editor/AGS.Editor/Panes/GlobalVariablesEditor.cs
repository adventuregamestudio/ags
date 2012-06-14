using AGS.Types;
using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class GlobalVariablesEditor : EditorContentPanel
    {
        private const string MENU_ITEM_EDIT_WORD = "EditWord";
        private const string MENU_ITEM_ADD_SYNONYM = "AddSynonym";
        private const string MENU_ITEM_DELETE_WORD = "DeleteWord";
        private const string MENU_ITEM_ADD_WORD = "AddWord";
        private const string MENU_ITEM_FIND_WORD = "FindWord";
        private const int SUB_ITEM_INDEX_WORD_TEXT = 1;

        public delegate void GlobalVariableChangedHandler();
        public event GlobalVariableChangedHandler GlobalVariableChanged;

        private GlobalVariables _variables;
        private Game _game;

        public GlobalVariablesEditor(Game game)
        {
            InitializeComponent();
            lvwWords.ListViewItemSorter = new GlobalVariableComparer();
            _game = game;
            _variables = game.GlobalVariables;

            lvwWords.Sorting = SortOrder.Ascending;
            foreach (GlobalVariable variable in _variables.ToList())
            {
                lvwWords.Items.Add(CreateListItemFromVariable(variable));
            }
            lvwWords.Sort();
        }

        protected override string OnGetHelpKeyword()
        {
            return "Global variables";
        }
        
        protected override void OnPropertyChanged(string propertyName, object oldValue)
        {
            if (lvwWords.SelectedItems.Count > 0)
            {
                UpdateListItemFromVariableObject(lvwWords.SelectedItems[0]);
            }
        }
        
        private ListViewItem CreateListItemFromVariable(GlobalVariable variable)
        {
            ListViewItem newItem = new ListViewItem(new string[] { variable.Name, variable.Type, variable.DefaultValue });
            newItem.Tag = variable;
            return newItem;
        }

        private void UpdateListItemFromVariableObject(ListViewItem listItem)
        {
            GlobalVariable selectedWord = ((GlobalVariable)listItem.Tag);
            //listItem.SubItems[SUB_ITEM_INDEX_WORD_TEXT].Text = selectedWord.Name;
        }

        private void OnGlobalVariableChanged()
        {
            if (GlobalVariableChanged != null)
            {
                GlobalVariableChanged();
            }
        }
        
        private void TextParserEditor_SizeChanged(object sender, EventArgs e)
        {
            if (this.ClientSize.Height > mainFrame.Top + 40)
            {
                mainFrame.Size = new Size(mainFrame.Width, this.ClientSize.Height - mainFrame.Top - 5);
                lvwWords.Size = new Size(lvwWords.Width, mainFrame.ClientSize.Height - lvwWords.Top - 10);
            }
        }

        private void ContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            GlobalVariable selectedVariable = null;
            if (lvwWords.SelectedItems.Count > 0)
            {
                selectedVariable = ((GlobalVariable)lvwWords.SelectedItems[0].Tag);
            }
            if ((item.Name == MENU_ITEM_EDIT_WORD) && (selectedVariable != null))
            {
                EditSelectedVariable(selectedVariable);
            }
            else if ((item.Name == MENU_ITEM_FIND_WORD) && (selectedVariable != null))
            {
                FindSelectedVariable(selectedVariable);
            }
            else if (item.Name == MENU_ITEM_ADD_WORD)
            {
                GlobalVariable variable = new GlobalVariable();
                if (GlobalVariableDialog.Show(variable, _game) == DialogResult.OK)
                {
                    AddNewVariableToList(variable);
                }
            }
            else if (item.Name == MENU_ITEM_DELETE_WORD)
            {
                if (Factory.GUIController.ShowQuestion("Are you sure you want to remove the variable '" + selectedVariable.Name + "'? If you are using it in your scripts, your game will no longer compile.", MessageBoxIcon.Warning) == DialogResult.Yes)
                {
                    lvwWords.Items.RemoveAt(lvwWords.SelectedIndices[0]);
                    _variables.Remove(selectedVariable);
                    OnGlobalVariableChanged();
                }
            }
        }

        private void EditSelectedVariable(GlobalVariable variable)
        {
            string nameWas = variable.Name;
            if (GlobalVariableDialog.Show(variable, _game) == DialogResult.OK)
            {
                if (variable.Name != nameWas)
                {
                    _variables.VariableRenamed(variable, nameWas);
                }

                lvwWords.SelectedItems[0].SubItems[0].Text = variable.Name;
                lvwWords.SelectedItems[0].SubItems[1].Text = variable.Type;
                lvwWords.SelectedItems[0].SubItems[2].Text = variable.DefaultValue;
                OnGlobalVariableChanged();
            }
        }

        private void FindSelectedVariable(GlobalVariable variable)
        {
            TextProcessing.FindAllUsages findAllUsages = new TextProcessing.FindAllUsages(null,
                null, null, AGSEditor.Instance);
            findAllUsages.Find(null, variable.Name);
        }

        private void AddNewVariableToList(GlobalVariable variable)
        {
            _variables.Add(variable);
            int addAtIndex = 0;
            if (lvwWords.SelectedIndices.Count > 0)
            {
                addAtIndex = lvwWords.SelectedIndices[0] + 1;
            }
            ListViewItem addedItem = lvwWords.Items.Insert(addAtIndex, CreateListItemFromVariable(variable));
            addedItem.Selected = true;
            addedItem.EnsureVisible();
            OnGlobalVariableChanged();
        }

        private void ShowContextMenu(Point menuPosition)
        {
            EventHandler onClick = new EventHandler(ContextMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            if (lvwWords.SelectedItems.Count > 0)
            {
                menu.Items.Add(new ToolStripMenuItem("Edit...", null, onClick, MENU_ITEM_EDIT_WORD));
                menu.Items.Add(new ToolStripMenuItem("Delete", null, onClick, MENU_ITEM_DELETE_WORD));
                menu.Items.Add(new ToolStripMenuItem("Find All Usages", null, onClick, MENU_ITEM_FIND_WORD));
                menu.Items.Add(new ToolStripSeparator());
            }
            menu.Items.Add(new ToolStripMenuItem("Add new variable...", null, onClick, MENU_ITEM_ADD_WORD));

            menu.Show(lvwWords, menuPosition);
        }

        private void lvwWords_ItemActivate(object sender, EventArgs e)
        {
            if (lvwWords.SelectedItems.Count > 0)
            {
                EditSelectedVariable((GlobalVariable)lvwWords.SelectedItems[0].Tag);
            }
        }

        private class GlobalVariableComparer : IComparer
        {
            public int Compare(object x, object y)
            {
                return ((GlobalVariable)((ListViewItem)x).Tag).Name.CompareTo(((GlobalVariable)((ListViewItem)y).Tag).Name);
            }
        }

        private void lvwWords_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                ShowContextMenu(e.Location);
            }
        }

    }
}
