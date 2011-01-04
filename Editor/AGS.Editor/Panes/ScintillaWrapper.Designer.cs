namespace AGS.Editor
{
    partial class ScintillaWrapper
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;
        private Scintilla.ScintillaControl scintillaControl1;

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
            this.scintillaControl1 = new Scintilla.ScintillaControl();
            this.SuspendLayout();
            // 
            // scintillaControl1
            // 
            this.scintillaControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.scintillaControl1.Anchor_ = 0;
            this.scintillaControl1.AutoCMaxHeight = 5;
            this.scintillaControl1.AutoCMaxWidth = 0;
            this.scintillaControl1.AutoCSeparator = 32;
            this.scintillaControl1.AutoCTypeSeparator = 63;
            this.scintillaControl1.CaretFore = 0;
            this.scintillaControl1.CaretLineBack = 65535;
            this.scintillaControl1.CaretLineBackAlpha = 256;
            this.scintillaControl1.CaretPeriod = 500;
            this.scintillaControl1.CaretWidth = 1;
            this.scintillaControl1.CodePage = 0;
            this.scintillaControl1.ControlCharSymbol = 0;
            this.scintillaControl1.CurrentPos = 0;
            this.scintillaControl1.Cursor_ = -1;
            this.scintillaControl1.DocPointer = 257856192;
            this.scintillaControl1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.scintillaControl1.EdgeColumn = 0;
            this.scintillaControl1.EdgeMode = 0;
            this.scintillaControl1.EOLMode = 0;
            this.scintillaControl1.HighlightGuide = 0;
            this.scintillaControl1.Indent = 0;
            this.scintillaControl1.IsAutoCAutoHide = true;
            this.scintillaControl1.IsAutoCCancelAtStart = true;
            this.scintillaControl1.IsAutoCChooseSingle = false;
            this.scintillaControl1.IsAutoCDropRestOfWord = false;
            this.scintillaControl1.IsAutoCIgnoreCase = false;
            this.scintillaControl1.IsBackSpaceUnIndents = false;
            this.scintillaControl1.IsBufferedDraw = true;
            this.scintillaControl1.IsCaretLineVisible = false;
            this.scintillaControl1.IsCaretSticky = false;
            this.scintillaControl1.IsEndAtLastLine = true;
            this.scintillaControl1.IsFocus = false;
            this.scintillaControl1.IsHScrollBar = true;
            this.scintillaControl1.IsIndentationGuides = false;
            this.scintillaControl1.IsMouseDownCaptures = true;
            this.scintillaControl1.IsOvertype = false;
            this.scintillaControl1.IsPasteConvertEndings = true;
            this.scintillaControl1.IsReadOnly = false;
            this.scintillaControl1.IsTabIndents = true;
            this.scintillaControl1.IsTwoPhaseDraw = true;
            this.scintillaControl1.IsUndoCollection = true;
            this.scintillaControl1.IsUsePalette = false;
            this.scintillaControl1.IsUseTabs = true;
            this.scintillaControl1.IsViewEOL = false;
            this.scintillaControl1.IsVScrollBar = true;
            this.scintillaControl1.LayoutCache = 1;
            this.scintillaControl1.Lexer = 0;
            this.scintillaControl1.Location = new System.Drawing.Point(0, 0);
            this.scintillaControl1.MarginLeft = 0;
            this.scintillaControl1.MarginRight = 0;
            this.scintillaControl1.ModEventMask = 8191;
            this.scintillaControl1.MouseDwellTime = 10000000;
            this.scintillaControl1.Name = "scintillaControl1";
            this.scintillaControl1.PrintMagnification = 0;
            this.scintillaControl1.PrintWrapMode = 1;
            this.scintillaControl1.ScrollWidth = 2000;
            this.scintillaControl1.SearchFlags = 0;
            this.scintillaControl1.SelAlpha = 256;
            this.scintillaControl1.SelectionEnd = 0;
            this.scintillaControl1.SelectionMode = 0;
            this.scintillaControl1.SelectionStart = 0;
            this.scintillaControl1.Size = new System.Drawing.Size(733, 694);
            this.scintillaControl1.Status = 0;
            this.scintillaControl1.StyleBits = 5;
            this.scintillaControl1.TabIndex = 0;
            this.scintillaControl1.TabWidth = 8;
            this.scintillaControl1.TargetEnd = 0;
            this.scintillaControl1.TargetStart = 0;
            this.scintillaControl1.Text = "scintillaControl1";
            this.scintillaControl1.ViewWS = 0;
            this.scintillaControl1.WrapMode = 0;
            this.scintillaControl1.WrapStartIndent = 0;
            this.scintillaControl1.WrapVisualFlags = 0;
            this.scintillaControl1.WrapVisualFlagsLocation = 0;
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
