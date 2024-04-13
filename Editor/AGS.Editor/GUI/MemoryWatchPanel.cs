using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public partial class MemoryWatchPanel : DockContent
    {
        private delegate void ReceiveMemoryHandler(uint requestID, string value);

        private Dictionary<uint, string> _varRequests = new Dictionary<uint, string>();

        // TODO: there should be a working thread for updating items instead,
        // with a list of items to update that may be added and removed async.
        private Timer _updateItemTimer = new Timer();
        private ListViewItem _itemToUpdate;

        public MemoryWatchPanel()
        {
            InitializeComponent();

            listView1.AfterLabelEdit += ListView1_AfterLabelEdit;
            _updateItemTimer.Interval = 100;
            _updateItemTimer.Tick += _updateItemTimer_Tick;
        }

        private void _updateItemTimer_Tick(object sender, EventArgs e)
        {
            _updateItemTimer.Enabled = false;
            if (_itemToUpdate != null)
            {
                UpdateSingleWatch(_itemToUpdate);
            }
        }

        private void ListView1_AfterLabelEdit(object sender, LabelEditEventArgs e)
        {
            _itemToUpdate = listView1.Items[e.Item];
            _updateItemTimer.Start();
        }

        private void MemoryWatchPanel_Shown(object sender, EventArgs e)
        {
            AGSEditor.Instance.Debugger.ReceiveMemory += Debugger_ReceiveMemory;
        }

        private void MemoryWatchPanel_FormClosed(object sender, FormClosedEventArgs e)
        {
            AGSEditor.Instance.Debugger.ReceiveMemory -= Debugger_ReceiveMemory;
        }

        private void MemoryWatchPanel_VisibleChanged(object sender, EventArgs e)
        {
            if (Visible)
                AGSEditor.Instance.Debugger.ReceiveMemory += Debugger_ReceiveMemory;
            else
                AGSEditor.Instance.Debugger.ReceiveMemory -= Debugger_ReceiveMemory;
        }

        private void Debugger_ReceiveMemory(uint requestID, string value)
        {
            if (this.InvokeRequired)
            {
                this.Invoke(new ReceiveMemoryHandler(Debugger_ReceiveMemory), requestID, value);
                return;
            }

            string varName;
            if (_varRequests.TryGetValue(requestID, out varName))
            {
                for (var item = listView1.FindItemWithText(varName, false, 0, false);
                    item != null;
                    item = item.Index < listView1.Items.Count - 1 ? listView1.FindItemWithText(varName, false, item.Index + 1, false) : null)
                {
                    item.SubItems[1].Text = value;
                }
                _varRequests.Remove(requestID);
            }
        }

        private void UpdateSingleWatch(ListViewItem item)
        {
            if (!AGSEditor.Instance.Debugger.CanUseDebugger)
                return;

            string varName = item.Text;
            if (string.IsNullOrEmpty(varName))
                return;

            // FIXME: temporary logic for easier testing:
            // if user input begins with '+' then use direct mem query,
            // otherwise rely on variable names
            if (varName[0] == '+')
            {
                _varRequests.Add(
                    AGSEditor.Instance.Debugger.QueryMemoryDirect(varName.Substring(1)),
                    varName);
            }
            else
            {
                _varRequests.Add(
                    AGSEditor.Instance.Debugger.QueryMemory(varName),
                    varName);
            }
        }

        public void UpdateAllWatches()
        {
            _varRequests.Clear();
            foreach (ListViewItem item in listView1.Items)
            {
                UpdateSingleWatch(item);
            }
        }

        private void listView1_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                contextMenuStrip1.Show(listView1, e.Location);
            }
        }

        private void addToolStripMenuItem_Click(object sender, EventArgs e)
        {
            var item = new ListViewItem();
            item.Text = "VARIABLE NAME";
            item.SubItems.Add(new ListViewItem.ListViewSubItem());
            item = listView1.Items.Add(item);
            item.BeginEdit();
        }

        private void removeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            foreach (var item in listView1.SelectedItems)
                listView1.Items.Remove(item as ListViewItem);
        }

        private void clearToolStripMenuItem_Click(object sender, EventArgs e)
        {
            listView1.Items.Clear();
        }
    }
}
