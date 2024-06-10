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
    public partial class WatchVariablesPanel : DockContent
    {
        private Dictionary<uint, string> _varRequests = new Dictionary<uint, string>();

        // TODO: there should be a working thread for updating items instead,
        // with a list of items to update that may be added and removed async.
        private readonly Timer _updateItemTimer = new Timer();
        private readonly object _updateItemLock = new object();
        private readonly List<ListViewItem> _itemsToUpdate = new List<ListViewItem>();

        public WatchVariablesPanel()
        {
            InitializeComponent();

            listView1.AfterLabelEdit += ListView1_AfterLabelEdit;
            _updateItemTimer.Interval = 100;
            _updateItemTimer.Tick += _updateItemTimer_Tick;

            EnsureEmptyItem();
        }

        private ListViewItem CreateItem(string text)
        {
            var item = new ListViewItem();
            item.Text = text;
            item.SubItems.Add(new ListViewItem.ListViewSubItem());
            item.SubItems.Add(new ListViewItem.ListViewSubItem());
            return item;
        }

        /// <summary>
        /// Ensures that there's always a single ready empty item in the end of the list,
        /// which user may edit right away without calling a context menu.
        /// </summary>
        private void EnsureEmptyItem()
        {
            // remove trailing empty items, except the last empty item
            for (int i = listView1.Items.Count - 1;
                    i > 0 && listView1.Items[i].Text.Length == 0 && listView1.Items[i - 1].Text.Length == 0; --i)
            {
                listView1.Items.RemoveAt(i);
            }

            // add one empty item, if there's none at the end
            if ((listView1.Items.Count == 0) ||
                (listView1.Items.Count > 0 && listView1.Items[listView1.Items.Count - 1].Text.Length > 0))
            {
                listView1.Items.Add(CreateItem(""));
            }
        }

        private void _updateItemTimer_Tick(object sender, EventArgs e)
        {
            _updateItemTimer.Enabled = false;
            lock(_updateItemLock)
            {
                foreach (var item in _itemsToUpdate)
                {
                    UpdateSingleWatch(item);
                }
                _itemsToUpdate.Clear();
            }

            EnsureEmptyItem();
        }

        private void ListView1_AfterLabelEdit(object sender, LabelEditEventArgs e)
        {
            lock (_updateItemLock)
                _itemsToUpdate.Add(listView1.Items[e.Item]);
            _updateItemTimer.Start();
        }

        private void WatchVariablesPanel_Shown(object sender, EventArgs e)
        {
            AGSEditor.Instance.Debugger.ReceiveVariable += Debugger_ReceiveVariable;
        }

        private void WatchVariablesPanel_FormClosed(object sender, FormClosedEventArgs e)
        {
            AGSEditor.Instance.Debugger.ReceiveVariable -= Debugger_ReceiveVariable;
        }

        private void WatchVariablesPanel_VisibleChanged(object sender, EventArgs e)
        {
            if (Visible)
                AGSEditor.Instance.Debugger.ReceiveVariable += Debugger_ReceiveVariable;
            else
                AGSEditor.Instance.Debugger.ReceiveVariable -= Debugger_ReceiveVariable;
        }

        private void Debugger_ReceiveVariable(uint requestID, DebugController.VariableInfo info)
        {
            if (this.InvokeRequired)
            {
                this.Invoke(new DebugController.ReceiveVariableHandler(Debugger_ReceiveVariable), requestID, info);
                return;
            }

            // FIXME: protect _varRequests by a mutex

            string varName;
            if (_varRequests.TryGetValue(requestID, out varName))
            {
                string typeName = info.Type ?? "";
                string value = "";

                if (string.IsNullOrEmpty(info.TypeHint) || info.Value == null)
                {
                    value = info.ErrorText ?? "unknown data";
                }
                else if (info.TypeHint == "s")
                {
                    value = string.Format("\"{0}\"", info.Value);
                }
                else
                {
                    // This is a byte array, so decode from base64
                    byte[] bytes = null;
                    try
                    {
                        bytes = System.Convert.FromBase64String(info.Value);
                        if (bytes == null || bytes.Length == 0)
                            value = "unknown data";
                        else if (info.TypeHint == "i1")
                            value = string.Format("{0} ('{1}')", bytes[0], (char)bytes[0]);
                        else if (info.TypeHint == "i2")
                            value = string.Format("{0}", BitConverter.ToInt16(bytes, 0));
                        else if (info.TypeHint == "i4")
                            value = string.Format("{0}", BitConverter.ToInt32(bytes, 0));
                        else if (info.TypeHint == "f4")
                            value = string.Format("{0}", BitConverter.ToSingle(bytes, 0));
                        else if (info.TypeHint == "h")
                            value = string.Format("{0} (memory handle)", BitConverter.ToInt32(bytes, 0));
                        else
                            value = "unknown data";
                    }
                    catch (Exception)
                    {
                        value = "error";
                    }
                }

                for (var item = listView1.FindItemWithText(varName, false, 0, false);
                    item != null;
                    item = item.Index < listView1.Items.Count - 1 ? listView1.FindItemWithText(varName, false, item.Index + 1, false) : null)
                {
                    item.SubItems[1].Text = typeName;
                    item.SubItems[2].Text = value;
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

            // FIXME: it's currently potentially possible that reply comes faster than reqID gets into the _varRequests?!
            //        maybe block replies until registering of requests is complete?
            uint reqKey;
            if (AGSEditor.Instance.Debugger.QueryVariable(varName, out reqKey))
                _varRequests.Add(reqKey, varName);
        }

        public void UpdateAllWatches()
        {
            _varRequests.Clear();
            foreach (ListViewItem item in listView1.Items)
            {
                UpdateSingleWatch(item);
            }
        }

        private void listView1_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.F2)
            {
                if (listView1.SelectedItems.Count > 0)
                {
                    listView1.SelectedItems[0].BeginEdit();
                }
            }
        }

        private void listView1_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                removeToolStripMenuItem.Enabled = listView1.SelectedItems.Count > 0;
                clearToolStripMenuItem.Enabled = listView1.Items.Count > 0;
                contextMenuStrip1.Show(listView1, e.Location);
            }
        }

        private void addToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ListViewItem item;
            if (listView1.Items.Count > 0 && listView1.Items[listView1.Items.Count - 1].Text.Length == 0)
            {
                item = listView1.Items[listView1.Items.Count - 1];
            }
            else
            {
                item = listView1.Items.Add(CreateItem("VARIABLE NAME"));
            }
            item.BeginEdit();
        }

        private void removeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            foreach (var item in listView1.SelectedItems)
                listView1.Items.Remove(item as ListViewItem);
            EnsureEmptyItem();
        }

        private void clearToolStripMenuItem_Click(object sender, EventArgs e)
        {
            listView1.Items.Clear();
            EnsureEmptyItem();
        }
    }
}
