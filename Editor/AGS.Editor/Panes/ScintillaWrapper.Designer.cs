namespace AGS.Editor
{
    partial class ScintillaWrapper
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;
        private ScintillaNET.Scintilla scintillaControl1;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.scintillaControl1 = new ScintillaNET.Scintilla();
            this.SuspendLayout();
            // 
            // scintillaControl1
            // 
            this.scintillaControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.scintillaControl1.AnchorPosition = 0;
            this.scintillaControl1.AutoCMaxHeight = 5;
            this.scintillaControl1.AutoCMaxWidth = 0;
            this.scintillaControl1.AutoCSeparator = (char)32;
            this.scintillaControl1.AutoCTypeSeparator = (char)63;
            this.scintillaControl1.CaretForeColor = System.Drawing.Color.Black;
            this.scintillaControl1.CaretLineBackColor = System.Drawing.Color.White;
            this.scintillaControl1.CaretLineBackColorAlpha = 256;
            this.scintillaControl1.CaretPeriod = 500;
            this.scintillaControl1.CaretWidth = 1;
            this.scintillaControl1.CurrentPosition = 0;
            this.scintillaControl1.UseWaitCursor = false;
            this.scintillaControl1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.scintillaControl1.EdgeColumn = 0;
            this.scintillaControl1.EdgeMode = 0;
            this.scintillaControl1.EolMode = 0;
            this.scintillaControl1.HighlightGuide = 0;
            this.scintillaControl1.IndentWidth = 0;
            this.scintillaControl1.AutoCAutoHide = true;
            this.scintillaControl1.AutoCCancelAtStart = true;
            this.scintillaControl1.AutoCChooseSingle = false;
            this.scintillaControl1.AutoCDropRestOfWord = false;
            this.scintillaControl1.AutoCIgnoreCase = false;
            this.scintillaControl1.BufferedDraw = true;
            this.scintillaControl1.CaretLineVisible = false;
            this.scintillaControl1.EndAtLastLine = true;
            this.scintillaControl1.HScrollBar = true;
            this.scintillaControl1.IndentationGuides = ScintillaNET.IndentView.None;
            this.scintillaControl1.Overtype = false;
            this.scintillaControl1.PasteConvertEndings = true;
            this.scintillaControl1.ReadOnly = false;
            this.scintillaControl1.UseTabs = true;
            this.scintillaControl1.ViewEol = false;
            this.scintillaControl1.VScrollBar = true;
            this.scintillaControl1.Lexer = 0;
            this.scintillaControl1.Location = new System.Drawing.Point(0, 0);
            this.scintillaControl1.Margins.Left = 0;
            this.scintillaControl1.Margins.Right = 0;
            this.scintillaControl1.MouseDwellTime = 10000000;
            this.scintillaControl1.Name = "scintillaControl1";
            this.scintillaControl1.ScrollWidth = 2000;
            this.scintillaControl1.SearchFlags = 0;
            this.scintillaControl1.AdditionalSelAlpha = 256;
            this.scintillaControl1.SelectionEnd = 0;
            this.scintillaControl1.SelectionStart = 0;
            this.scintillaControl1.Size = new System.Drawing.Size(733, 694);
            this.scintillaControl1.Status = 0;
            this.scintillaControl1.TabIndex = 0;
            this.scintillaControl1.TabWidth = 8;
            this.scintillaControl1.TargetEnd = 0;
            this.scintillaControl1.TargetStart = 0;
            this.scintillaControl1.Text = "scintillaControl1";
            this.scintillaControl1.ViewWhitespace = 0;
            this.scintillaControl1.WrapMode = 0;
            this.scintillaControl1.WrapStartIndent = 0;
            this.scintillaControl1.WrapVisualFlags = 0;
            this.scintillaControl1.WrapVisualFlagLocation = 0;
            this.scintillaControl1.XOffset = 0;
            this.scintillaControl1.Zoom = 0;
            this.Controls.Add(this.scintillaControl1);
            // 
            // ScriptEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Name = "ScriptEditor";
            this.Size = new System.Drawing.Size(574, 316);
            this.ResumeLayout(false);
        }

        #endregion
    }
}
