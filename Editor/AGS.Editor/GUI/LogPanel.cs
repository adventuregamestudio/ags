using System;
using WeifenLuo.WinFormsUI.Docking;
using AGS.Types.Enums;
using AGS.Types;

namespace AGS.Editor
{
    public partial class LogPanel : DockContent
    {
        private LogBuffer _logBuffer = new LogBuffer();
        // Styling
        private string _logFont = Factory.AGSEditor.Settings.LogFont;
        private int _logFontSize = Factory.AGSEditor.Settings.LogFontSize;
        // State properties
        private DebugLog _logConfig;
        private bool _run = true;
        private bool _glue = true; // stick to the log's end
        private bool _autoGlue = true; // force glue, regardless of user's actions
        // Internal state
        private object _bufferLockObject = new object();
        private bool _bufferNeedsSync = false; // tells if must sync with the buffer contents
        private bool _bufferWasReset = false; // tell if buffer needs to be fully reapplied
        private int _bufferPopCount = 0; // accumulated record of discarded characters
        private int _bufferPushCount = 0; // accumulated record of added characters
        private bool _noScrollCheck = false; // temp disable checking scrolling event


        public LogPanel()
        {
            InitializeComponent();
            UpdateStyling();

            propertyGrid.PropertyValueChanged += PropertyGrid_PropertyValueChanged;

            timerLogBufferSync.Start();
            Run();
        }

        public DebugLog LogConfig
        {
            get { return _logConfig; }
            set
            {
                _logConfig = value;
                propertyGrid.SelectedObject = _logConfig;
                ApplyFilters(_logConfig);
            }
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

        /// <summary>
        /// Gets/sets AutoGlue mode in which text box always autoscrolls to the end of the text.
        /// </summary>
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

        private void PropertyGrid_PropertyValueChanged(object s, System.Windows.Forms.PropertyValueChangedEventArgs e)
        {
            ApplyFilters(_logConfig);
        }

        delegate void UpdateTextCallback(bool reset, int pop, int push);

        /// <summary>
        /// Syncs with the LogBuffer's contents in accordance to the accumulated changes.
        /// </summary>
        private void UpdateText(bool reset, int pop, int push)
        {
            // InvokeRequired required compares the thread ID of the
            // calling thread to the thread ID of the creating thread.
            // If these threads are different, it returns true.
            if (logTextBox.InvokeRequired)
            {
                UpdateTextCallback d = new UpdateTextCallback(UpdateText);
                Invoke(d, new object[] {});
            }
            else
            {
                _noScrollCheck = true;
                logTextBox.SuspendDrawing();

                // Remember current view pos and the caret, we will use these
                // to restore the scroll & selection after setting a new text.
                var old_pos = logTextBox.GetScrollPos();
                var old_sel_start = logTextBox.SelectionStart;
                var old_sel_end = logTextBox.SelectionStart + logTextBox.SelectionLength;

                //
                // Apply buffer contents depending on accumulated changes
                if (reset)
                {
                    // Set new text; this resets everything!
                    logTextBox.Text = _logBuffer.GetFullText();
                }
                else
                {
                    // Remove N first entries
                    if (pop > 0)
                    {
                        DeleteLines(0, _bufferPopCount);
                    }
                    // Append N last entries
                    if (push > 0)
                    {
                        logTextBox.ReplaceSelectedText(logTextBox.TextLength, 0, _logBuffer.QueryLastEntries());
                    }
                }
                // end apply buffer contents
                //

                // In glue mode: scroll to the end
                if (_glue)
                {
                    logTextBox.SelectionStart = logTextBox.TextLength;
                    logTextBox.ScrollToCaret();
                }
                // Otherwise: restore old state
                else
                {
                    // NOTE: we use extender method here, that sends control messages
                    // to scroll to the new position. If we use temporary selection
                    // and then ScrollToCaret(), then restoring old user's selection
                    // afterwards will cause RTB to scroll to the caret again.

                    // Restore old selection
                    logTextBox.SelectionStart = old_sel_start;
                    logTextBox.SelectionLength = old_sel_end - old_sel_start;
                    // Restore old scroll position
                    logTextBox.SetScrollPos(old_pos);
                }

                logTextBox.ResumeDrawing();
                _noScrollCheck = false;
            }
        }

        /*
        protected override string OnGetHelpKeyword()
        {
            return "Logging";
        }
        */

        private void BufferChanged(object sender, LogBufferEventArgs e)
        {
            lock (_bufferLockObject)
            {
                _bufferNeedsSync = true;
                _bufferWasReset |= e.Reset;
                _bufferPopCount += e.PopCount;
                _bufferPushCount += e.PushCount;
            }
        }

        public void Run()
        {
            _run = true;
            _bufferNeedsSync = true;
            _logBuffer.BufferChanged += BufferChanged;
            btnRun.Enabled = false;
            btnPause.Enabled = true;
        }

        public void Pause()
        {
            _run = false;
            _bufferNeedsSync = false;
            _logBuffer.BufferChanged -= BufferChanged;
            btnRun.Enabled = true;
            btnPause.Enabled = false;
        }

        public void Clear()
        {
            _logBuffer.Clear();
            logTextBox.Clear();
            _bufferNeedsSync = false;
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

            bool reset;
            int pop, push;
            lock (_bufferLockObject)
            {
                _bufferNeedsSync = false;
                reset = _bufferWasReset;
                pop = _bufferPopCount;
                push = _bufferPushCount;
                _bufferWasReset = false;
                _bufferPopCount = 0;
                _bufferPushCount = 0;
            }

            UpdateText(reset, pop, push);
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

        private void btnProperties_Click(object sender, EventArgs e)
        {
            splitContainer.Panel2Collapsed = !splitContainer.Panel2Collapsed;
        }
        
        private void btnCopySelected_Click(object sender, EventArgs e)
        {
            logTextBox.Copy();
        }

        private void btnCopyAll_Click(object sender, EventArgs e)
        {
            logTextBox.SelectAll();
            logTextBox.Copy();
        }

        private void btnClear_Click(object sender, EventArgs e)
        {
            Clear();
        }

        private void DeleteLines(int firstChar, int lastChar)
        {
            logTextBox.EraseSelectedText(firstChar, lastChar);
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

            // If auto-glue is off, then update glue mode state,
            // depending on where the user had scrolled the text box.
            if (!_autoGlue)
            {
                _glue = IsScrollAtBottom();
            }
        }

        private void LoadColorTheme(ColorTheme t)
        {
            
            t.SetColor("log-panel/background", c => BackColor = c);
            t.SetColor("log-panel/splitter", c => splitContainer.BackColor = c);
            t.SetColor("log-panel/background", c => logTextBox.BackColor = c);
            t.SetColor("log-panel/foreground", c => logTextBox.ForeColor = c);
            t.PropertyGridHelper(propertyGrid, "log-panel/grid");
            if (t.Has("tool-bar"))
            {
                t.SetColor("tool-bar/background", c => toolStrip.BackColor = c);
                toolStrip.Renderer = t.GetToolStripRenderer("tool-bar");
            }
        }

        private void LogPanel_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Hacks.SetRichTextBoxMargins(logTextBox, 12, 12);
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }
    }
}
