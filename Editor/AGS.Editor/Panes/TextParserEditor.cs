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
    public partial class TextParserEditor : EditorContentPanel
    {
        private const string MENU_ITEM_EDIT_WORD = "EditWord";
        private const string MENU_ITEM_ADD_SYNONYM = "AddSynonym";
        private const string MENU_ITEM_DELETE_WORD = "DeleteWord";
        private const string MENU_ITEM_ADD_WORD = "AddWord";
        private const string MENU_ITEM_FIND_WORD = "FindWord";
        private const int SUB_ITEM_INDEX_WORD_TEXT = 1;

        private TextParser _parser;

        public TextParserEditor(TextParser parser)
        {
            InitializeComponent();
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            lvwWords.ListViewItemSorter = new TextParserWordComparer();
            _parser = parser;

            lvwWords.Sorting = SortOrder.Ascending;
            foreach (TextParserWord word in parser.Words)
            {
                lvwWords.Items.Add(CreateListItemFromWord(word));
            }
            lvwWords.Sort();
        }


        protected override string OnGetHelpKeyword()
        {
            return "Text parser";
        }

        protected override void OnPropertyChanged(string propertyName, object oldValue)
        {
            if (lvwWords.SelectedItems.Count > 0)
            {
                UpdateListItemFromWordObject(lvwWords.SelectedItems[0]);
            }
        }

        private ListViewItem CreateListItemFromWord(TextParserWord word)
        {
            ListViewItem newItem = new ListViewItem(new string[] { word.WordGroup.ToString(), word.Word, word.Type.ToString() });
            newItem.Tag = word;
            return newItem;
        }

        private void UpdateListItemFromWordObject(ListViewItem listItem)
        {
            TextParserWord selectedWord = ((TextParserWord)listItem.Tag);
            listItem.SubItems[SUB_ITEM_INDEX_WORD_TEXT].Text = selectedWord.Word;
        }

        private void TextParserEditor_SizeChanged(object sender, EventArgs e)
        {
            if (this.ClientSize.Height > mainFrame.Top + 40)
            {
                mainFrame.Size = new Size(mainFrame.Width, this.ClientSize.Height - mainFrame.Top - 5);
                lvwWords.Size = new Size(lvwWords.Width, mainFrame.ClientSize.Height - lvwWords.Top - 10);
            }
        }

        private void EditSelectedWord()
        {
            Factory.GUIController.SelectPropertyByName("Word");
        }

        private void ContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            TextParserWord selectedWord = null;
            if (lvwWords.SelectedItems.Count > 0)
            {
                selectedWord = ((TextParserWord)lvwWords.SelectedItems[0].Tag);
            }
            if (item.Name == MENU_ITEM_EDIT_WORD)
            {
                EditSelectedWord();
            }
            else if (item.Name == MENU_ITEM_ADD_SYNONYM)
            {
                TextParserWord word = new TextParserWord(selectedWord.WordGroup, "", selectedWord.Type);
                AddNewWordToList(word);
            }
            else if (item.Name == MENU_ITEM_ADD_WORD)
            {
                TextParserWord word = new TextParserWord(_parser.GetAvailableWordGroupID(), "", TextParserWordType.Normal);
                AddNewWordToList(word);
            }
            else if (item.Name == MENU_ITEM_DELETE_WORD)
            {
                lvwWords.Items.RemoveAt(lvwWords.SelectedIndices[0]);
                _parser.Words.Remove(selectedWord);
            }
            else if (item.Name == MENU_ITEM_FIND_WORD)
            {
                string searchFor = TextEntryDialog.Show("Find word", "Type the word that you want to find into the list below.", "");
                if (searchFor != null)
                {
                    searchFor = searchFor.ToLower();
                    bool found = false;
                    foreach (ListViewItem listItem in lvwWords.Items)
                    {
                        if (listItem.SubItems[SUB_ITEM_INDEX_WORD_TEXT].Text.ToLower() == searchFor)
                        {
                            listItem.Selected = true;
                            listItem.EnsureVisible();
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        Factory.GUIController.ShowMessage("The word '" + searchFor + "' could not be found.", MessageBoxIcon.Information);
                    }
                }
            }
        }

        private void AddNewWordToList(TextParserWord word)
        {
            _parser.Words.Add(word);
            int addAtIndex = 0;
            if (lvwWords.SelectedIndices.Count > 0)
            {
                addAtIndex = lvwWords.SelectedIndices[0] + 1;
            }
            ListViewItem addedItem = lvwWords.Items.Insert(addAtIndex, CreateListItemFromWord(word));
            addedItem.Selected = true;
            addedItem.EnsureVisible();
            EditSelectedWord();
        }

        private void ShowContextMenu(Point menuPosition)
        {
            EventHandler onClick = new EventHandler(ContextMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            if (lvwWords.SelectedItems.Count > 0)
            {
                int selectedWordGroup = Convert.ToInt32(lvwWords.SelectedItems[0].Text);
                menu.Items.Add(new ToolStripMenuItem("Edit word", null, onClick, MENU_ITEM_EDIT_WORD));
                menu.Items.Add(new ToolStripMenuItem("Add synonym", null, onClick, MENU_ITEM_ADD_SYNONYM));
                menu.Items.Add(new ToolStripSeparator());
                bool canDelete = true;
                if ((selectedWordGroup == TextParser.WORD_GROUP_ID_ANYWORD) ||
                    (selectedWordGroup == TextParser.WORD_GROUP_ID_IGNORE) ||
                    (selectedWordGroup == TextParser.WORD_GROUP_ID_REST_OF_LINE))
                {
                    if (_parser.CountWordsInWordGroup(selectedWordGroup) <= 1)
                    {
                        canDelete = false;
                    }
                }
                if (canDelete)
                {
                    menu.Items.Add(new ToolStripMenuItem("Delete word", null, onClick, MENU_ITEM_DELETE_WORD));
                    menu.Items.Add(new ToolStripSeparator());
                }
            }
            menu.Items.Add(new ToolStripMenuItem("Add new word", null, onClick, MENU_ITEM_ADD_WORD));
            menu.Items.Add(new ToolStripMenuItem("Find word...", null, onClick, MENU_ITEM_FIND_WORD));

            menu.Show(lvwWords, menuPosition);
        }

        private void lvwWords_ItemActivate(object sender, EventArgs e)
        {
            EditSelectedWord();
        }

        private void lvwWords_SelectedIndexChanged(object sender, EventArgs e)
        {
            TextParserWord selected = null;
            if (lvwWords.SelectedItems.Count > 0)
            {
                selected = ((TextParserWord)lvwWords.SelectedItems[0].Tag);
            }
            Factory.GUIController.SetPropertyGridObject(selected);
        }

        private class TextParserWordComparer : IComparer
        {
            public int Compare(object x, object y)
            {
                return ((TextParserWord)((ListViewItem)x).Tag).WordGroup - ((TextParserWord)((ListViewItem)y).Tag).WordGroup;
            }
        }

        private void lvwWords_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                ShowContextMenu(e.Location);
            }
        }

        private void lvwWords_MouseClick(object sender, MouseEventArgs e)
        {
        }

        private void LoadColorTheme(ColorTheme t)
        {
            BackColor = t.GetColor("text-parser-editor/background");
            ForeColor = t.GetColor("text-parser-editor/foreground");
            mainFrame.BackColor = t.GetColor("text-parser-editor/box/background");
            mainFrame.ForeColor = t.GetColor("text-parser-editor/box/foreground");
            lvwWords.BackColor = t.GetColor("text-parser-editor/list-view/background");
            lvwWords.ForeColor = t.GetColor("text-parser-editor/list-view/foreground");
            lvwWords.OwnerDraw = t.GetBool("text-parser-editor/list-view/owner-draw");
            lvwWords.GridLines = t.GetBool("text-parser-editor/list-view/grid-lines");
            lvwWords.Layout += (s, a) =>
            {
                lvwWords.Columns[lvwWords.Columns.Count - 1].Width = t.GetInt("text-parser-editor/list-view/last-column-width");
            };
            lvwWords.DrawItem += (s, a) => a.DrawDefault = t.GetBool("text-parser-editor/list-view/draw-item");
            lvwWords.DrawSubItem += (s, a) => a.DrawDefault = t.GetBool("text-parser-editor/list-view/draw-sub-item");
            lvwWords.DrawColumnHeader += (s, a) =>
            {
                a.Graphics.FillRectangle(new SolidBrush(t.GetColor("text-parser-editor/list-view/column-header/background")), a.Bounds);
                a.Graphics.DrawString(a.Header.Text, lvwWords.Font, new SolidBrush(t.GetColor("text-parser-editor/list-view/column-header/foreground")), a.Bounds.X + 5, a.Bounds.Y + a.Bounds.Size.Height / 5);
                a.Graphics.DrawRectangle(new Pen(new SolidBrush(t.GetColor("text-parser-editor/list-view/column-header/border"))), a.Bounds.X - 1, a.Bounds.Y - 1, a.Bounds.Size.Width, a.Bounds.Size.Height);
            };
        }
    }
}
