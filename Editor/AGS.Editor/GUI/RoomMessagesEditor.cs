using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class RoomMessagesEditor : Form
    {
        private const string MENU_ITEM_ADD = "AddRoomMessage";
        private const string MENU_ITEM_DELETE = "DeleteRoomMessage";

        private List<RoomMessage> _messages;

        public RoomMessagesEditor(List<RoomMessage> messages)
        {
            InitializeComponent();
            _messages = messages;
            foreach (RoomMessage message in _messages)
            {
                AddMessageToList(message);
            }
            cmbDisplayAs.Items.Add("Normal text");
            foreach (Character ch in Factory.AGSEditor.CurrentGame.Characters)
            {
                cmbDisplayAs.Items.Add("Character " + ch.RealName);
            }
            grpMessageDetails.Enabled = false;
        }

        public static void ShowEditor(List<RoomMessage> messages)
        {
            RoomMessagesEditor editor = new RoomMessagesEditor(messages);
            editor.ShowDialog();
            editor.Dispose();
        }

        private void AddMessageToList(RoomMessage message)
        {
            lstList.Items.Add(message.ID.ToString()).SubItems.Add(message.Text);
        }

        private void lstList_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lstList.SelectedIndices.Count == 1)
            {
                RoomMessage message = _messages[lstList.SelectedIndices[0]];
                txtMessage.Text = message.Text;
                chkAutoRemove.Checked = message.AutoRemoveAfterTime;
                chkShowNextMessage.Checked = message.DisplayNextMessageAfter;
                cmbDisplayAs.SelectedIndex = (message.ShowAsSpeech) ? message.CharacterID + 1 : 0;
                grpMessageDetails.Enabled = true;
            }
            else
            {
                grpMessageDetails.Enabled = false;
            }
        }

        private void txtMessage_TextChanged(object sender, EventArgs e)
        {
            _messages[lstList.SelectedIndices[0]].Text = txtMessage.Text;
            lstList.SelectedItems[0].SubItems[1].Text = txtMessage.Text;
        }

        private void chkAutoRemove_CheckedChanged(object sender, EventArgs e)
        {
            _messages[lstList.SelectedIndices[0]].AutoRemoveAfterTime = chkAutoRemove.Checked;
        }

        private void chkShowNextMessage_CheckedChanged(object sender, EventArgs e)
        {
            _messages[lstList.SelectedIndices[0]].DisplayNextMessageAfter = chkShowNextMessage.Checked;
        }

        private void cmbDisplayAs_SelectedIndexChanged(object sender, EventArgs e)
        {
            _messages[lstList.SelectedIndices[0]].ShowAsSpeech = (cmbDisplayAs.SelectedIndex > 0);
            _messages[lstList.SelectedIndices[0]].CharacterID = cmbDisplayAs.SelectedIndex - 1;
        }

        private void lstList_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                ShowContextMenu(e.Location);
            }
        }

        private void ContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem menuItem = (ToolStripMenuItem)sender;
            if (menuItem.Name == MENU_ITEM_ADD)
            {
                _messages.Add(new RoomMessage(_messages.Count));
                AddMessageToList(_messages[_messages.Count - 1]);
                lstList.SelectedIndices.Clear();
                lstList.SelectedIndices.Add(lstList.Items.Count - 1);
                lstList.SelectedItems[0].EnsureVisible();
                txtMessage.Focus();
            }
            else if (menuItem.Name == MENU_ITEM_DELETE)
            {
                if (Factory.GUIController.ShowQuestion("Are you sure you want to delete this message?") == DialogResult.Yes)
                {
                    _messages.RemoveAt(_messages.Count - 1);
                    lstList.Items.RemoveAt(lstList.Items.Count - 1);
                }
            }
        }

        private void ShowContextMenu(Point location)
        {
            EventHandler onClick = new EventHandler(ContextMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            if (lstList.SelectedItems.Count == 1)
            {
                menu.Items.Add(new ToolStripMenuItem("Delete message", null, onClick, MENU_ITEM_DELETE));
                if (lstList.SelectedIndices[0] < lstList.Items.Count - 1)
                {
                    menu.Items[menu.Items.Count - 1].Enabled = false;
                }

                menu.Items.Add(new ToolStripSeparator());
            }
            menu.Items.Add(new ToolStripMenuItem("Add message", null, onClick, MENU_ITEM_ADD));

            menu.Show(lstList, location);
        }

    }
}