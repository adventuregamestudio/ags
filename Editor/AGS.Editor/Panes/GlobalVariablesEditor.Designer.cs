namespace AGS.Editor
{
    partial class GlobalVariablesEditor
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
            this.mainFrame = new System.Windows.Forms.GroupBox();
            this.lvwVars = new AGS.Editor.SortableListView();
            this.clmName = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.clmType = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.clmDefaultValue = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.label1 = new System.Windows.Forms.Label();
            this.mainFrame.SuspendLayout();
            this.SuspendLayout();
            // 
            // mainFrame
            // 
            this.mainFrame.Controls.Add(this.lvwVars);
            this.mainFrame.Controls.Add(this.label1);
            this.mainFrame.Location = new System.Drawing.Point(5, 4);
            this.mainFrame.Name = "mainFrame";
            this.mainFrame.Size = new System.Drawing.Size(510, 514);
            this.mainFrame.TabIndex = 0;
            this.mainFrame.TabStop = false;
            this.mainFrame.Text = "Global Variables";
            // 
            // lvwVars
            // 
            this.lvwVars.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.clmName,
            this.clmType,
            this.clmDefaultValue});
            this.lvwVars.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lvwVars.FullRowSelect = true;
            this.lvwVars.HideSelection = false;
            this.lvwVars.Location = new System.Drawing.Point(3, 64);
            this.lvwVars.MultiSelect = false;
            this.lvwVars.Name = "lvwVars";
            this.lvwVars.Size = new System.Drawing.Size(504, 447);
            this.lvwVars.TabIndex = 1;
            this.lvwVars.UseCompatibleStateImageBehavior = false;
            this.lvwVars.View = System.Windows.Forms.View.Details;
            this.lvwVars.ItemActivate += new System.EventHandler(this.lvwVars_ItemActivate);
            this.lvwVars.MouseUp += new System.Windows.Forms.MouseEventHandler(this.lvwVars_MouseUp);
            // 
            // clmName
            // 
            this.clmName.Text = "Name";
            this.clmName.Width = 150;
            // 
            // clmType
            // 
            this.clmType.Text = "Type";
            this.clmType.Width = 110;
            // 
            // clmDefaultValue
            // 
            this.clmDefaultValue.Text = "Initial Value";
            this.clmDefaultValue.Width = 190;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Dock = System.Windows.Forms.DockStyle.Top;
            this.label1.Location = new System.Drawing.Point(3, 17);
            this.label1.MaximumSize = new System.Drawing.Size(502, 0);
            this.label1.Name = "label1";
            this.label1.Padding = new System.Windows.Forms.Padding(3, 9, 3, 12);
            this.label1.Size = new System.Drawing.Size(501, 47);
            this.label1.TabIndex = 0;
            this.label1.Text = "Global variables allow you to create variables which you can access from all your" +
    " scripts. Right-click in the list to add, modify and delete variables.";
            // 
            // GlobalVariablesEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.mainFrame);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "GlobalVariablesEditor";
            this.Padding = new System.Windows.Forms.Padding(3);
            this.Size = new System.Drawing.Size(531, 519);
            this.Load += new System.EventHandler(this.GlobalVariablesEditor_Load);
            this.SizeChanged += new System.EventHandler(this.TextParserEditor_SizeChanged);
            this.mainFrame.ResumeLayout(false);
            this.mainFrame.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox mainFrame;
        private AGS.Editor.SortableListView lvwVars;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ColumnHeader clmName;
        private System.Windows.Forms.ColumnHeader clmType;
        private System.Windows.Forms.ColumnHeader clmDefaultValue;
    }
}
