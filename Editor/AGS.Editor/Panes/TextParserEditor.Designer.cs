namespace AGS.Editor
{
    partial class TextParserEditor
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
            this.clmWordGroup = new System.Windows.Forms.ColumnHeader();
            this.clmWord = new System.Windows.Forms.ColumnHeader();
            this.clmWordType = new System.Windows.Forms.ColumnHeader();
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
            this.mainFrame.Size = new System.Drawing.Size(376, 514);
            this.mainFrame.TabIndex = 0;
            this.mainFrame.TabStop = false;
            this.mainFrame.Text = "Text Parser word list";
            // 
            // lvwWords
            // 
            this.lvwWords.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.clmWordGroup,
            this.clmWord,
            this.clmWordType});
            this.lvwWords.FullRowSelect = true;
            this.lvwWords.HideSelection = false;
            this.lvwWords.Location = new System.Drawing.Point(15, 42);
            this.lvwWords.MultiSelect = false;
            this.lvwWords.Name = "lvwWords";
            this.lvwWords.Size = new System.Drawing.Size(350, 458);
            this.lvwWords.TabIndex = 1;
            this.lvwWords.UseCompatibleStateImageBehavior = false;
            this.lvwWords.View = System.Windows.Forms.View.Details;
            this.lvwWords.ItemActivate += new System.EventHandler(this.lvwWords_ItemActivate);
            this.lvwWords.MouseClick += new System.Windows.Forms.MouseEventHandler(this.lvwWords_MouseClick);
            this.lvwWords.SelectedIndexChanged += new System.EventHandler(this.lvwWords_SelectedIndexChanged);
            this.lvwWords.MouseUp += new System.Windows.Forms.MouseEventHandler(this.lvwWords_MouseUp);
            // 
            // clmWordGroup
            // 
            this.clmWordGroup.Text = "Word Group";
            this.clmWordGroup.Width = 70;
            // 
            // clmWord
            // 
            this.clmWord.Text = "Word";
            this.clmWord.Width = 150;
            // 
            // clmWordType
            // 
            this.clmWordType.Text = "Type";
            this.clmWordType.Width = 110;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(14, 17);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(283, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "To modify an item or add new words, right-click in the list.";
            // 
            // TextParserEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.mainFrame);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "TextParserEditor";
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
        private System.Windows.Forms.ColumnHeader clmWordGroup;
        private System.Windows.Forms.ColumnHeader clmWord;
        private System.Windows.Forms.ColumnHeader clmWordType;
    }
}
