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
using AGS.Editor;
using AGS.Types.Enums;
using AGS.Types;

namespace AGS.Editor
{
    public partial class LogPanel : EditorContentPanel
    {
        private LogBuffer _logBuffer = new LogBuffer();
        private bool _bufferNeedsSync = false;
        private bool _run = true;

        public LogPanel(GUIController guiController)
        {
            InitializeComponent();
            timerLogBufferSync.Start();
            Run();
        }

        delegate void SetTextCallback(string text);

        private void SetText(string text)
        {
            // InvokeRequired required compares the thread ID of the
            // calling thread to the thread ID of the creating thread.
            // If these threads are different, it returns true.
            if (this.logTextBox.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetText);
                this.Invoke(d, new object[] { text });
            }
            else
            {
                this.logTextBox.Text = text;
                this.logTextBox.SelectionStart = this.logTextBox.TextLength;
                this.logTextBox.ScrollToCaret();
                Show();
            }
        }

        protected override string OnGetHelpKeyword()
        {
            return "Logging";
        }

        private void BufferChanged(object sender, EventArgs e)
        {
            _bufferNeedsSync = true;
        }

        public void Run()
        {
            _run = true;
            _bufferNeedsSync = true;
            _logBuffer.ValueChanged += new System.EventHandler(this.BufferChanged);
            btnRun.Enabled = false;
            btnPause.Enabled = true;
        }

        public void Pause()
        {
            _run = false;
            _bufferNeedsSync = false;
            _logBuffer.ValueChanged -= new System.EventHandler(this.BufferChanged);
            btnRun.Enabled = true;
            btnPause.Enabled = false;
        }

        public void Clear()
        {
            _logBuffer.Clear();
        }

        public void WriteLogMessage(string message, LogGroup group, LogLevel level)
        {
            if (!_run) return;
            _logBuffer.Append(message, group, level);
        }

        public void ApplyFilters(DebugLog debugLog)
        {
            for(int i=0; i<(int)LogGroup.NumGroups; i++)
            {
                LogGroup group = (LogGroup)i;
                _logBuffer.SetLogLevel(group, debugLog.LogFilter.GetGroupLevel(group));
            }
            _bufferNeedsSync = true;
        }

        private void timerLogBufferSync_Tick(object sender, EventArgs e)
        {
            if (!_run) return;
            if (!_bufferNeedsSync) return;

            SetText(_logBuffer.ToString());
            _bufferNeedsSync = false;
        }

        private void btnRun_Click(object sender, EventArgs e)
        {
            Run();
        }

        private void btnPause_Click(object sender, EventArgs e)
        {
            Pause();
        }
    }
}
