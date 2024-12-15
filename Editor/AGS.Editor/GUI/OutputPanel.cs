using AGS.Types;
using System;
using System.Collections;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public partial class OutputPanel : DockContent
    {
        private const string MENU_ITEM_COPYSEL_TO_CLIPBOARD = "CopySelectedToClipboard";
        private const string MENU_ITEM_COPYALL_TO_CLIPBOARD = "CopyAllToClipboard";
        private CompileMessages _errors;
        private OutputPanelItem _rightClickItem;

        public OutputPanel()
        {
            InitializeComponent();
        }

		public void SetImageList(ImageList list)
		{
			lvwResults.SmallImageList = list;
		}

        public void SetMessages(string[] messages, string imageKey)
        {
            foreach (string message in messages)
            {
                SetMessage(message, imageKey);
            }
        }

        public void SetMessage(string message, string imageKey)
        {
            ListViewItem newItem = lvwResults.Items.Add(new OutputPanelItem(message, imageKey));
        }

        /// <summary>
        /// Comparison method that puts errors first in the message list.
        /// </summary>
        private static int SortErrorsFirst(CompileMessage message1, CompileMessage message2)
        {
            bool is_error1 = (message1 is CompileError);
            bool is_error2 = (message2 is CompileError);
            if (is_error1 && !is_error2)
                return -1;
            if (!is_error1 && is_error2)
                return 1;
            return message1.Index - message2.Index;
        }

        public CompileMessages ErrorsToList
        {
            get { return _errors; }
            set
            {
                _errors = value;
                // For user convenience: mention errors first;
                // This is a temporary measure, as there's no "message type" column in the list
                if (_errors != null)
                    _errors.Sort(SortErrorsFirst);
                RefreshList();
            }
        }

        private void RefreshList()
        {
            lvwResults.Items.Clear();
            if (_errors != null)
            {
                foreach (CompileMessage error in _errors)
                {
                    ListViewItem newItem = lvwResults.Items.Add(new OutputPanelItem(error));
                }
            }
        }

        private void lvwResults_ItemActivate(object sender, EventArgs e)
        {
			if (lvwResults.SelectedItems.Count > 0)
			{
                OutputPanelItem selectedItem = lvwResults.SelectedItems[0] as OutputPanelItem;
                selectedItem.DefaultAction();
            }
        }

        private string ListItemsToString(IEnumerable list)
        {
            StringBuilder sb = new StringBuilder();
            foreach(ListViewItem item in list)
            {
                sb.Append(item.ToString());
                sb.Append(Environment.NewLine);
            }
            return sb.ToString();
        }

		private void ContextMenuEventHandler(object sender, EventArgs e)
		{
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            if (item.Name == MENU_ITEM_COPYSEL_TO_CLIPBOARD)
            {
                string result = ListItemsToString(lvwResults.SelectedItems);
                if (!string.IsNullOrEmpty(result))
                    Utilities.CopyTextToClipboard(result);
            }
            else if (item.Name == MENU_ITEM_COPYALL_TO_CLIPBOARD)
            {
                string result = ListItemsToString(lvwResults.Items);
                if (!string.IsNullOrEmpty(result))
                    Utilities.CopyTextToClipboard(result);
            }
            else if (_rightClickItem != null)
            {
                _rightClickItem.Action(item.Name);
            }
		}

		private void ShowContextMenu(Point menuPosition)
		{
			ContextMenuStrip menu = new ContextMenuStrip();
            menu.Items.Add(new ToolStripMenuItem("Copy selection to clipboard", null, ContextMenuEventHandler, MENU_ITEM_COPYSEL_TO_CLIPBOARD));
            menu.Items.Add(new ToolStripMenuItem("Copy all to clipboard", null, ContextMenuEventHandler, MENU_ITEM_COPYALL_TO_CLIPBOARD));

            var hitInfo = lvwResults.HitTest(menuPosition);
            _rightClickItem = hitInfo.Item as OutputPanelItem;
            if (_rightClickItem != null)
            {
                var actions = _rightClickItem.GetActions();
                foreach (var action in actions)
                {
                    menu.Items.Add(new ToolStripMenuItem(action.Title, null, ContextMenuEventHandler, action.Name));
                }
            }

            menu.Show(lvwResults, menuPosition);
		}

		private void lvwResults_MouseUp(object sender, MouseEventArgs e)
		{
			if (e.Button == MouseButtons.Right)
			{
				ShowContextMenu(e.Location);
			}
		}

        private void LoadColorTheme(ColorTheme t)
        {
            t.SetColor("global/pane/background", c => BackColor = c);
            t.ListViewHelper(lvwResults, "output-panel");
        }

        private void OutputPanel_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }
    }
}
