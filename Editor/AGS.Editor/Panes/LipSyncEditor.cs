using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class LipSyncEditor : EditorContentPanel
    {
        private LipSync _lipSync;

        public LipSyncEditor(LipSync lipSync)
        {
            InitializeComponent();

            // This is required for the splitter to look good at both 100% and 200% scaling
            splitContainerMain.SplitterDistance = 300; // some arbitrary large number so text gets a proper height in flow layout
            int topSplitHeight = 0;
            foreach (Control control in flowLayoutPanelTop.Controls)
            {
                topSplitHeight += control.Height;
            }
            splitContainerMain.SplitterDistance = topSplitHeight + splitContainerMain.SplitterWidth +
                flowLayoutPanelTop.Margin.Top + flowLayoutPanelTop.Margin.Bottom; // compress the splitter size

            _lipSync = lipSync;

            int textBoxWidth = 150;
            int labelWidth = 48;
            int rowHeight = 24;

            int columns = 4;
            int rows = LipSync.MAX_LIP_SYNC_FRAMES / 2;
            int cells = columns * rows;
            tableLayoutPanelLP.ColumnCount = columns;
            tableLayoutPanelLP.RowCount = rows;
            tableLayoutPanelLP.SuspendDrawing();
            tableLayoutPanelLP.SuspendLayout();

            for (int i = 0; i < _lipSync.CharactersPerFrame.Length; i++)
            {
                Label label = new Label();
                label.Left = 0;
                label.Top = 0;
                label.Size = new Size(labelWidth, rowHeight);
                label.Text = i.ToString();
                label.TextAlign = ContentAlignment.MiddleRight;

                TextBox textBox = new TextBox();
                textBox.Left = 0;
                textBox.Top = 0;
                textBox.Size = new Size(textBoxWidth, rowHeight);
                textBox.Tag = i;
                textBox.Text = _lipSync.CharactersPerFrame[i];
                textBox.TextChanged += new EventHandler(textBox_TextChanged);

                int row = i;
                int column = 0;
                if (i >= rows)
                {
                    row -= rows;
                    column = 2;
                }
                tableLayoutPanelLP.Controls.Add(label, column, row);
                tableLayoutPanelLP.Controls.Add(textBox, column+1, row);
            }
            tableLayoutPanelLP.ResumeLayout();
            tableLayoutPanelLP.ResumeDrawing();
            UpdateControlsEnabled();
        }

        public LipSync EditingLipSync
        {
            get { return _lipSync; }
        }

        protected override string OnGetHelpKeyword()
        {
            return "Lip sync";
        }

        protected override void OnPropertyChanged(string propertyName, object oldValue)
        {
            UpdateControlsEnabled();
        }

        private void textBox_TextChanged(object sender, EventArgs e)
        {
            int frameIndex = (int)((TextBox)sender).Tag;
            _lipSync.CharactersPerFrame[frameIndex] = ((TextBox)sender).Text;
        }

        private void UpdateControlsEnabled()
        {
            bool shouldBeEnabled = true;
            if (_lipSync.Type == LipSyncType.None)
            {
                shouldBeEnabled = false;
            }
            foreach (Control control in tableLayoutPanelLP.Controls)
            {
                if (control is TextBox)
                {
                    control.Enabled = shouldBeEnabled;
                }
            }
        }

        public void LoadColorTheme(ColorTheme t)
        {
            t.ControlHelper(this, "lip-sync-editor");

            foreach (Control control in tableLayoutPanelLP.Controls)
            {
                TextBox textBox = control as TextBox;

                if (textBox != null)
                {
                    t.TextBoxHelper(textBox, "lip-sync-editor/text-boxes");
                }
            }
        }

        private void LipSyncEditor_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }
    }
}
