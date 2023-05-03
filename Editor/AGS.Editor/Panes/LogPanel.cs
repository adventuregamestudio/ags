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
        private bool _glue = true; // stick to the log's end
        private bool _autoGlue = true; // force glue, regardless of user's actions
        private bool _noScrollCheck = false; // temp disable checking scrolling event
        // Styling
        private string _logFont = Factory.AGSEditor.Settings.LogFont;
        private int _logFontSize = Factory.AGSEditor.Settings.LogFontSize;

        public LogPanel(GUIController guiController)
        {
            InitializeComponent();
            UpdateStyling();

            timerLogBufferSync.Start();
            Run();
        }

        public string LogFont
        {
            set { _logFont = value; logTextBox.Font = new System.Drawing.Font(_logFont, _logFontSize); }
            get { return _logFont; }
        }

        public int LogFontSize
        {
            set { _logFontSize = value; }
            get { return _logFontSize; }
        }

        public bool AutoGlue
        {
            get { return _autoGlue; }
            set
            {
                _autoGlue = value;
                if (_autoGlue)
                    _glue = true;
            }
        }

        public void UpdateStyling()
        {
            logTextBox.Font = new System.Drawing.Font(_logFont, _logFontSize);
        }

        delegate void SetTextCallback(string text);

        private void SetText(string text)
        {
            // InvokeRequired required compares the thread ID of the
            // calling thread to the thread ID of the creating thread.
            // If these threads are different, it returns true.
            if (logTextBox.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetText);
                Invoke(d, new object[] { text });
            }
            else
            {
                _noScrollCheck = true;
                logTextBox.SuspendDrawing();

                // Remember current view pos and the caret, we will use these
                // to restore the scroll & selection after setting a new text.
                var old_pos = logTextBox.GetPositionFromCharIndex(0);
                var old_sel_start = logTextBox.SelectionStart;
                var old_sel_end = logTextBox.SelectionStart + logTextBox.SelectionLength;

                // Set new text; this resets everything!
                logTextBox.Text = text;

                // In glue mode: scroll to the end
                if (_glue)
                {
                    logTextBox.SelectionStart = logTextBox.TextLength;
                    logTextBox.ScrollToCaret();
                }
                // otherwise: try to scroll to the previous pos
                else
                {
                    // FIXME: this is some vodoo magic here, investigate if there
                    // are better solutions that this...
                    var new_index = logTextBox.GetCharIndexFromPosition(new Point(old_pos.X, -old_pos.Y));
                    var new_line = logTextBox.GetLineFromCharIndex(new_index);
                    var char_to_scroll = logTextBox.GetFirstCharIndexFromLine(new_line + 1);
                    logTextBox.SelectionStart = char_to_scroll;
                    logTextBox.ScrollToCaret();

                    // Restore old selection
                    logTextBox.SelectionStart = old_sel_start;
                    logTextBox.SelectionLength = old_sel_end - old_sel_start;
                }

                logTextBox.ResumeDrawing();
                _noScrollCheck = false;

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

        private void btnGlue_Click(object sender, EventArgs e)
        {
            AutoGlue = btnGlue.Checked;
        }

        private bool IsScrollAtBottom()
        {
            // Found this idea here:
            // https://stackoverflow.com/questions/2986408/richtextbox-vertical-scrollbar-manipulation-in-visual-studio
            if (logTextBox.TextLength == 0)
                return true;

            var p = logTextBox.GetPositionFromCharIndex(logTextBox.TextLength - 1);
            return p.Y >= 0 && p.Y <= logTextBox.ClientSize.Height;
        }

        private void logTextBox_VScroll(object sender, EventArgs e)
        {
            if (_noScrollCheck)
                return;

            if (!_autoGlue)
            {
                _glue = IsScrollAtBottom();
            }
        }
    }
}
