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
        private const int TEXT_BOX_START_X = 10;
        private const int TEXT_BOX_START_Y = 65;
        private LipSync _lipSync;

        public LipSyncEditor(LipSync lipSync)
        {
            InitializeComponent();
            _lipSync = lipSync;
            this.AutoScroll = true;

            int x = TEXT_BOX_START_X, y = TEXT_BOX_START_Y;

            for (int i = 0; i < _lipSync.CharactersPerFrame.Length; i++)
            {
                Label label = new Label();
                label.Left = x;
                label.Top = y + 2;
                label.AutoSize = true;
                label.Text = i.ToString();
                this.Controls.Add(label);

                TextBox textBox = new TextBox();
                textBox.Left = x + 20;
                textBox.Top = y;
                textBox.Size = new Size(150, 23);
                textBox.Tag = i;
                textBox.Text = _lipSync.CharactersPerFrame[i];
                textBox.TextChanged += new EventHandler(textBox_TextChanged);

                this.Controls.Add(textBox);
                y += 25;
                if (i % 10 == 9)
                {
                    x += 200;
                    y = TEXT_BOX_START_Y;
                }
            }
            UpdateControlsEnabled();
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
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
            foreach (Control control in this.Controls)
            {
                if (control is TextBox)
                {
                    control.Enabled = shouldBeEnabled;
                }
            }
        }

        public void LoadColorTheme(ColorTheme t)
        {
            BackColor = t.GetColor("lip-sync-editor/background");
            ForeColor = t.GetColor("lip-sync-editor/foreground");

            foreach (Control control in Controls)
            {
                TextBox textBox = control as TextBox;

                if (textBox != null)
                {
                    textBox.BackColor = t.GetColor("lip-sync-editor/text-boxes/background");
                    textBox.ForeColor = t.GetColor("lip-sync-editor/text-boxes/foreground");
                    textBox.BorderStyle = (BorderStyle)t.GetInt("lip-sync-editor/text-boxes/border-style");
                }
            }
        }
    }
}
