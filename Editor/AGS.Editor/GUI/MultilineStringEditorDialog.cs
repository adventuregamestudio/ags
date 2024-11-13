using AGS.Types;
using System;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class MultilineStringEditorDialog : Form
    {
        private static System.Drawing.Font _font = null;

        private MultilineStringEditorDialog(String text)
        {
            InitializeComponent();
            AdjustFont(); // make text slightly bigger
            MultilineString = text;
            UpdateTextStatus();
        }

        public static String ShowEditor(String text)
        {
            MultilineStringEditorDialog dialog = new MultilineStringEditorDialog(text);
            bool ok = dialog.ShowDialog() == DialogResult.OK;
            String result = dialog.MultilineString;
            dialog.Dispose();
            return ok ? result : null;
        }

        public String MultilineString
        {
            set { textBox1.Text = UnescapeToTextbox(value); }
            get { return EscapeToProperty(textBox1.Text); }
        }

        // textbox in winforms uses CR+LF for newlines
        private String UnescapeToTextbox(String text)
        {
            return text.Replace("\\r", "\r").Replace("\\n", "\n").Replace("\r\n", "\n").Replace("\n","\r\n");
        }

        // we need to convert from CR+LF to "\n" for AGS, but account for a "\n" that was copy-pasted somehow
        private String EscapeToProperty(String text)
        {
            return text.Replace("\r\n", "\n").Replace("\n", "\\n");
        }

        private void UpdateTextStatus()
        {
            int charCount = textBox1.Text.Length;
            int lineCount = textBox1.Lines.Length;
            string charText = charCount == 1 ? "character" : "characters";
            string lineText = lineCount == 1 ? "line" : "lines";
            labelStatus.Text = $"{charCount} {charText}, {lineCount} {lineText}";
        }

        private void AdjustFont()
        {
            if (_font == null)
                _font = textBox1.Font;
            textBox1.Font = new System.Drawing.Font(_font.Name, _font.SizeInPoints * 1.2f);
        }

        private void textBox1_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter && (Control.ModifierKeys == Keys.Control || Control.ModifierKeys == Keys.Shift))
            {
                e.Handled = true;
                this.DialogResult = DialogResult.OK; // either ctrl + return or shift + return confirms the dialog
                this.Close();
            }
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {
            UpdateTextStatus();
        }

        protected override void OnClosed(EventArgs e)
        {
            if (!DesignMode)
            {
                var config = GUIController.Instance.WindowConfig;
                ConfigUtils.WriteFormPosition(config, "MultilineStringEditorDialog", this);
            }
        }

        private void MultilineStringEditorDialog_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
                var config = GUIController.Instance.WindowConfig;
                ConfigUtils.ReadFormPosition(config, "MultilineStringEditorDialog", this);
            }
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.SetColor("global/background", c => BackColor = c);
            t.SetColor("global/foreground", c => ForeColor = c);
            t.SetColor("global/foreground", c => labelStatus.ForeColor = c);
            t.ButtonHelper(btnOk, "global/button");
            t.ButtonHelper(btnCancel, "global/button");
            t.TextBoxHelper(textBox1, "global/text-box");
        }
    }
}
