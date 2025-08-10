using System;
using System.Windows.Forms;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace AGS.Controls
{
    /// <summary>
    /// A read-only RichTextBox which hides its caret (in order to avoid user confusion).
    /// See https://stackoverflow.com/a/7362074
    ///
    /// Basically, use this as a rich text label.
    /// </summary>
    public class ReadOnlyRichTextBox : System.Windows.Forms.RichTextBox
    {
        [DllImport("user32.dll")]
        private static extern int HideCaret(IntPtr hwnd);

        public ReadOnlyRichTextBox()
        {
            this.MouseDown += ReadOnlyRichTextBox_Mouse;
            this.MouseUp += ReadOnlyRichTextBox_Mouse;
            base.ReadOnly = true;
            base.TabStop = false;
            HideCaret(this.Handle);
        }

        protected override void OnGotFocus(EventArgs e)
        {
            HideCaret(this.Handle);
        }

        protected override void OnEnter(EventArgs e)
        {
            HideCaret(this.Handle);
        }

        [DefaultValue(true)]
        public new bool ReadOnly
        {
            get { return true; }
            set { }
        }

        [DefaultValue(false)]
        public new bool TabStop
        {
            get { return false; }
            set { }
        }

        private void ReadOnlyRichTextBox_Mouse(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            HideCaret(this.Handle);
        }

        private void InitializeComponent()
        {
            this.Resize += ReadOnlyRichTextBox_Resize;
        }

        private void ReadOnlyRichTextBox_Resize(object sender, System.EventArgs e)
        {
            HideCaret(this.Handle);
        }
    }
}
