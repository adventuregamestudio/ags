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
            this.lvwWords = new System.Windows.Forms.ListView();
            this.clmName = new System.Windows.Forms.ColumnHeader();
            this.clmType = new System.Windows.Forms.ColumnHeader();
            this.clmDefaultValue = new System.Windows.Forms.ColumnHeader();
            this.label1 = new System.Windows.Forms.Label();
            this.mainFrame.SuspendLayout();
            this.SuspendLayout();
            // 
            // mainFrame
            // 
            this.mainFrame.Controls.Add(this.lvwWords);
            this.mainFrame.Controls.Add(this.label1);
            this.mainFrame.Location = new System.Drawing.Point(5, 4);
            this.mainFrame.Name = "mainFrame";
            this.mainFrame.Size = new System.Drawing.Size(510, 514);
            this.mainFrame.TabIndex = 0;
            this.mainFrame.TabStop = false;
            this.mainFrame.Text = "Global Variables";
            // 
            // lvwWords
            // 
            this.lvwWords.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.clmName,
            this.clmType,
            this.clmDefaultValue});
            this.lvwWords.FullRowSelect = true;
            this.lvwWords.HideSelection = false;
            this.lvwWords.Location = new System.Drawing.Point(17, 50);
            this.lvwWords.MultiSelect = false;
            this.lvwWords.Name = "lvwWords";
            this.lvwWords.Size = new System.Drawing.Size(478, 458);
            this.lvwWords.TabIndex = 1;
            this.lvwWords.UseCompatibleStateImageBehavior = false;
            this.lvwWords.View = System.Windows.Forms.View.Details;
            this.lvwWords.ItemActivate += new System.EventHandler(this.lvwWords_ItemActivate);
            this.lvwWords.MouseUp += new System.Windows.Forms.MouseEventHandler(this.lvwWords_MouseUp);
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
            this.label1.Location = new System.Drawing.Point(14, 17);
            this.label1.MaximumSize = new System.Drawing.Size(480, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(433, 26);
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
            this.Size = new System.Drawing.Size(531, 519);
            this.SizeChanged += new System.EventHandler(this.TextParserEditor_SizeChanged);
            this.mainFrame.ResumeLayout(false);
            this.mainFrame.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox mainFrame;
        private System.Windows.Forms.ListView lvwWords;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ColumnHeader clmName;
        private System.Windows.Forms.ColumnHeader clmType;
        private System.Windows.Forms.ColumnHeader clmDefaultValue;
    }
}
