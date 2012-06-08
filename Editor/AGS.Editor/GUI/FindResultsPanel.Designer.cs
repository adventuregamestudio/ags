namespace AGS.Editor
{
    partial class FindResultsPanel
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FindResultsPanel));
            this.pnlTitle = new System.Windows.Forms.Panel();
            this.btnClose = new System.Windows.Forms.Button();
            this.lblTitle = new System.Windows.Forms.Label();
            this.lvwResults = new System.Windows.Forms.ListView();
            this.fileColumnHeader = new System.Windows.Forms.ColumnHeader();
            this.lineNumberColumnHeader = new System.Windows.Forms.ColumnHeader();
            this.lineTextColumnHeader = new System.Windows.Forms.ColumnHeader();
            this.pnlTitle.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlTitle
            // 
            this.pnlTitle.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(185)))), ((int)(((byte)(185)))), ((int)(((byte)(185)))));
            this.pnlTitle.Controls.Add(this.btnClose);
            this.pnlTitle.Controls.Add(this.lblTitle);
            this.pnlTitle.Dock = System.Windows.Forms.DockStyle.Top;
            this.pnlTitle.ForeColor = System.Drawing.SystemColors.ControlText;
            this.pnlTitle.Location = new System.Drawing.Point(0, 0);
            this.pnlTitle.Name = "pnlTitle";
            this.pnlTitle.Size = new System.Drawing.Size(521, 18);
            this.pnlTitle.TabIndex = 1;
            // 
            // btnClose
            // 
            this.btnClose.Anchor = System.Windows.Forms.AnchorStyles.Right;
            this.btnClose.FlatAppearance.BorderColor = System.Drawing.Color.White;
            this.btnClose.FlatAppearance.BorderSize = 0;
            this.btnClose.FlatAppearance.MouseOverBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(192)))), ((int)(((byte)(192)))), ((int)(((byte)(255)))));
            this.btnClose.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnClose.ForeColor = System.Drawing.SystemColors.Control;
            this.btnClose.Location = new System.Drawing.Point(502, 2);
            this.btnClose.Margin = new System.Windows.Forms.Padding(0);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(14, 14);
            this.btnClose.TabIndex = 1;
            this.btnClose.UseVisualStyleBackColor = true;
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // lblTitle
            // 
            this.lblTitle.AutoSize = true;
            this.lblTitle.Location = new System.Drawing.Point(3, 1);
            this.lblTitle.Name = "lblTitle";
            this.lblTitle.Size = new System.Drawing.Size(130, 17);
            this.lblTitle.TabIndex = 0;
            this.lblTitle.Text = "Find Symbol Results";
            // 
            // lvwResults
            // 
            this.lvwResults.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.fileColumnHeader,
            this.lineNumberColumnHeader,
            this.lineTextColumnHeader});
            this.lvwResults.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lvwResults.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lvwResults.FullRowSelect = true;
            this.lvwResults.GridLines = true;
            this.lvwResults.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.lvwResults.HideSelection = false;
            this.lvwResults.Location = new System.Drawing.Point(0, 18);
            this.lvwResults.MultiSelect = false;
            this.lvwResults.Name = "lvwResults";
            this.lvwResults.Size = new System.Drawing.Size(521, 113);
            this.lvwResults.TabIndex = 2;
            this.lvwResults.UseCompatibleStateImageBehavior = false;
            this.lvwResults.View = System.Windows.Forms.View.Details;
            this.lvwResults.ItemActivate += new System.EventHandler(this.lvwResults_ItemActivate);
            this.lvwResults.MouseUp += new System.Windows.Forms.MouseEventHandler(this.lvwResults_MouseUp);
            this.lvwResults.Click += new System.EventHandler(this.lvwResults_Click);
            // 
            // fileColumnHeader
            // 
            this.fileColumnHeader.Text = "File";
            this.fileColumnHeader.Width = 200;
            // 
            // lineNumberColumnHeader
            // 
            this.lineNumberColumnHeader.Text = "Line";
            // 
            // lineTextColumnHeader
            // 
            this.lineTextColumnHeader.Text = "Text";
            this.lineTextColumnHeader.Width = 260;
            // 
            // FindResultsPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 17F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(521, 131);
            this.Controls.Add(this.lvwResults);
            this.Controls.Add(this.pnlTitle);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "FindResultsPanel";
            this.pnlTitle.ResumeLayout(false);
            this.pnlTitle.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel pnlTitle;
        private System.Windows.Forms.Label lblTitle;
        private System.Windows.Forms.ListView lvwResults;
        private System.Windows.Forms.ColumnHeader fileColumnHeader;
        private System.Windows.Forms.ColumnHeader lineNumberColumnHeader;
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.ColumnHeader lineTextColumnHeader;

    }
}
