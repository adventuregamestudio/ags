namespace AddressBarExt.Controls
{
    partial class AddressBarExt
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

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
            this.ts_bar = new System.Windows.Forms.ToolStrip();
            this.SuspendLayout();
            // 
            // ts_bar
            // 
            this.ts_bar.AutoSize = false;
            this.ts_bar.BackColor = System.Drawing.SystemColors.Window;
            this.ts_bar.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.ts_bar.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.ts_bar.LayoutStyle = System.Windows.Forms.ToolStripLayoutStyle.HorizontalStackWithOverflow;
            this.ts_bar.Location = new System.Drawing.Point(0, 0);
            this.ts_bar.Name = "ts_bar";
            this.ts_bar.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.ts_bar.ShowItemToolTips = false;
            this.ts_bar.Size = new System.Drawing.Size(529, 25);
            this.ts_bar.Stretch = true;
            this.ts_bar.TabIndex = 0;
            this.ts_bar.Text = "AddressBarExt";
            this.ts_bar.DoubleClick += new System.EventHandler(this.BarDoubleClickHandler);
            // 
            // AddressBarExt
            // 
            this.BackColor = System.Drawing.Color.White;
            this.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.Controls.Add(this.ts_bar);
            this.ForeColor = System.Drawing.SystemColors.Window;
            this.MinimumSize = new System.Drawing.Size(331, 23);
            this.Name = "AddressBarExt";
            this.Size = new System.Drawing.Size(529, 22);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ToolStrip ts_bar;
    }
}
