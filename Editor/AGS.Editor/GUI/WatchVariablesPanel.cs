using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public partial class WatchVariablesPanel : DockContent
    {
        private object _requestsLock = new object();
        private List<string> _varsToSend = new List<string>();
        private Dictionary<uint, string> _varRequests = new Dictionary<uint, string>();
        private List<Tuple<uint, DebugController.VariableInfo>> _varResults = new List<Tuple<uint, DebugController.VariableInfo>>();
        private System.Threading.Thread _requestProcThread;

        // TODO: there should be a working thread for updating items instead,
        // with a list of items to update that may be added and removed async.
        private readonly Timer _updateItemTimer = new Timer();
        private readonly object _updateItemLock = new object();
        private readonly List<ListViewItem> _itemsToUpdate = new List<ListViewItem>();
        private bool _autoWatchLocalVars = true;

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
            lock (_updateItemLock)
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
            AGSEditor.Instance.Debugger.DebugStateChanged += Debugger_DebugStateChanged;
            AGSEditor.Instance.Debugger.ReceiveVariable += Debugger_ReceiveVariable;
        }

        private void WatchVariablesPanel_FormClosed(object sender, FormClosedEventArgs e)
        {
            AGSEditor.Instance.Debugger.DebugStateChanged -= Debugger_DebugStateChanged;
            AGSEditor.Instance.Debugger.ReceiveVariable -= Debugger_ReceiveVariable;
        }

        private void WatchVariablesPanel_VisibleChanged(object sender, EventArgs e)
        {
            if (Visible)
                AGSEditor.Instance.Debugger.ReceiveVariable += Debugger_ReceiveVariable;
            else
                AGSEditor.Instance.Debugger.ReceiveVariable -= Debugger_ReceiveVariable;
        }

        /// <summary>
        /// The thread that communicates with the Debugger:
        /// * sends accumulated variable requests;
        /// * parses accumulated received answers.
        /// </summary>
        private void RequestProcThread()
        {
            while (AGSEditor.Instance.Debugger.IsActive)
            {
                // Make a local copy of all scheduled sends and results
                List<string> newVarsToSend = null;
                List<Tuple<uint, DebugController.VariableInfo>> newVarRes = null;

                lock (_requestsLock)
                {
                    if (_varsToSend.Count > 0)
                        newVarsToSend = new List<string>(_varsToSend);
                    _varsToSend.Clear();
                    if (_varResults.Count > 0)
                        newVarRes = new List<Tuple<uint, DebugController.VariableInfo>>(_varResults);
                    _varResults.Clear();
                }

                // Process incoming results
                if (newVarRes != null)
                {
                    List<Tuple<string, DebugController.VariableInfo>> infos = new List<Tuple<string, DebugController.VariableInfo>>();
                    foreach (var res in newVarRes)
                    {
                        string varName;
                        bool requestFound = false;
                        lock (_requestsLock)
                        {
                            requestFound = _varRequests.TryGetValue(res.Item1, out varName);
                            if (requestFound)
                                _varRequests.Remove(res.Item1);
                        }

                        if (requestFound)
                            infos.Add(new Tuple<string, DebugController.VariableInfo>(varName, res.Item2));
                    }
                    this.Invoke(new ParseAndSetVariableValuesHandler(ParseAndSetVariableValues), infos);
                }

                // Process outcoming requests
                if (newVarsToSend != null)
                {
                    foreach (var varname in newVarsToSend)
                    {
                        uint reqKey;
                        if (AGSEditor.Instance.Debugger.QueryVariable(varname, out reqKey))
                            _varRequests.Add(reqKey, varname);
                    }
                }

                // Sleep
                System.Threading.Thread.Sleep(10);
            }
        }

        private void Debugger_DebugStateChanged(DebugState newState)
        {
            if (newState == DebugState.NotRunning)
            {
                ClearWatchRequests();
                _requestProcThread = null; // the thread proc should stop on its own detecting inactive debugger
            }
            else if (_requestProcThread == null)
            {
                ClearWatchRequests();
                _requestProcThread = new System.Threading.Thread(new System.Threading.ThreadStart(RequestProcThread));
                _requestProcThread.Name = "Variable request thread";
                _requestProcThread.Start();
            }
        }

        private void Debugger_ReceiveVariable(uint requestID, DebugController.VariableInfo info)
        {
            lock (_requestsLock)
            {
                _varResults.Add(new Tuple<uint, DebugController.VariableInfo>(requestID, info));
            }
        }

        private delegate void ParseAndSetVariableValuesHandler(List<Tuple<string, DebugController.VariableInfo>> infos);

        private void ParseAndSetVariableValues(List<Tuple<string, DebugController.VariableInfo>> infos)
        {
            foreach (var item in infos)
                ParseAndSetVariableValue(item.Item1, item.Item2);
        }

        private void ParseAndSetVariableValue(string varName, DebugController.VariableInfo info)
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
        }

        private void UpdateSingleWatch(ListViewItem item)
        {
            if (!AGSEditor.Instance.Debugger.IsActive)
                return;

            string varName = item.Text;
            if (string.IsNullOrEmpty(varName))
                return;

            lock (_requestsLock)
            {
                _varsToSend.Add(varName);
            }
        }

        private int PerceivedBrightness(Color c)
        {
            return (int)Math.Sqrt(
            c.R * c.R * .299 +
            c.G * c.G * .587 +
            c.B * c.B * .114);
        }

        private void ClearAllAutoLocalVariables()
        {
            foreach (ListViewItem itm in listView1.Items)
            {
                if (itm.Tag as string == "autolocal")
                    itm.Remove();
            }
            _itemsToUpdate.RemoveAll(itm => itm.Tag as string == "autolocal");
        }

        public void SetAutoLocalVariables(DebugCallStack callStack)
        {
            if (!AGSEditor.Instance.Debugger.IsActive || !_autoWatchLocalVars)
                return;

            string scriptName = callStack.Lines[0].ScriptName;
            int lineNumber = callStack.Lines[0].LineNumber;
            ScintillaWrapper scintilla = Factory.GUIController.GetScriptEditorControl(scriptName, true) as ScintillaWrapper;
            if (scintilla == null)
                return;

            if (scintilla.CurrentLine != lineNumber)
            {
                scintilla.GoToLine(lineNumber);
            }

            List<string> varnames = scintilla.GetListOfLocalVariablesForCurrentPosition(false)
                .Select(v => v.VariableName).Distinct().ToList();

            listView1.BeginUpdate();

            // We will avoid blinking of all "autolocal" by removing the ones not in varnames
            // so we can keep the ones that already were added in a previous cycle
            // later we will add ONLY the ones that weren't already added
            // We also need to keep sync in the items to update.
            foreach (ListViewItem itm in listView1.Items)
            {
                if (itm.Tag as string == "autolocal" && !varnames.Contains(itm.Text))
                {
                    itm.Remove();
                }
            }

            lock (_updateItemLock)
            {
                _itemsToUpdate.RemoveAll(itm => itm.Tag as string == "autolocal" && !varnames.Contains(itm.Text));

                Color c = Color.Empty;
                foreach (var v in varnames)
                {
                    if (!listView1.Items.Cast<ListViewItem>().Any(itm => itm.Text == v && itm.Tag as string == "autolocal"))
                    {
                        var itm = CreateItem(v);
                        itm.Tag = "autolocal";
                        listView1.Items.Insert(0, itm);

                        if (c == Color.Empty)
                        {
                            int brightness = PerceivedBrightness(itm.ForeColor);
                            c = brightness > 128 ? Color.LightBlue : Color.DarkBlue;
                        }
                        itm.ForeColor = c;

                        _itemsToUpdate.Add(itm);
                    }
                }
            }

            listView1.EndUpdate();
            _updateItemTimer.Start();
        }

        private bool AutoWatchLocalVariables
        {
            get { return _autoWatchLocalVars; }
            set {
                    _autoWatchLocalVars = value;
                    if(!_autoWatchLocalVars)
                        ClearAllAutoLocalVariables();
                }
        }

        public void UpdateAllWatches()
        {
            if (!AGSEditor.Instance.Debugger.IsActive)
                return;

            lock (_requestsLock)
            {
                // Clear requests, then add new ones
                _varsToSend.Clear();
                _varRequests.Clear();
                _varResults.Clear();
                foreach (ListViewItem item in listView1.Items)
                {
                    if (!string.IsNullOrEmpty(item.Text))
                        _varsToSend.Add(item.Text);
                }
            }
        }

        private void ClearWatchRequests()
        {
            lock (_requestsLock)
            {
                _varsToSend.Clear();
                _varRequests.Clear();
                _varResults.Clear();
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

        public void AddVariableToWatchList(string var_name)
        {
            ListViewItem item = listView1.Items.Add(CreateItem(var_name));
            lock (_updateItemLock)
                _itemsToUpdate.Add(item);
            _updateItemTimer.Start();
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

        private void localVarToggleToolStripMenuItem_Click(object sender, EventArgs e)
        {
            localVarToggleToolStripMenuItem.Checked = !localVarToggleToolStripMenuItem.Checked;
            AutoWatchLocalVariables = localVarToggleToolStripMenuItem.Checked;
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.SetColor("global/pane/background", c => BackColor = c);
            t.ListViewHelper(listView1, "watch-variables-panel");
        }

        private void WatchVariablesPanel_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }
    }
}
