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
            this.lvwResults = new System.Windows.Forms.ListView();
            this.fileColumnHeader = new System.Windows.Forms.ColumnHeader();
            this.lineNumberColumnHeader = new System.Windows.Forms.ColumnHeader();
            this.lineTextColumnHeader = new System.Windows.Forms.ColumnHeader();
            this.SuspendLayout();
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
            this.lvwResults.Location = new System.Drawing.Point(1, 20);
            this.lvwResults.MultiSelect = false;
            this.lvwResults.Name = "lvwResults";
            this.lvwResults.Size = new System.Drawing.Size(519, 110);
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
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(521, 131);
            this.Controls.Add(this.lvwResults);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "FindResultsPanel";
            this.Padding = new System.Windows.Forms.Padding(1, 20, 1, 1);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ListView lvwResults;
        private System.Windows.Forms.ColumnHeader fileColumnHeader;
        private System.Windows.Forms.ColumnHeader lineNumberColumnHeader;
        private System.Windows.Forms.ColumnHeader lineTextColumnHeader;

    }
}
