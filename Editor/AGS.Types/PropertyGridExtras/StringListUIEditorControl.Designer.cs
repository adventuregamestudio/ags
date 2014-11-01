namespace AGS.Types
{
    partial class StringListUIEditorControl
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
            components = new System.ComponentModel.Container();
            this.clbItems = new System.Windows.Forms.CheckedListBox();
            this.SuspendLayout();
            // 
            // clbItems
            // 
            this.clbItems.Dock = System.Windows.Forms.DockStyle.Fill;
            this.clbItems.FormattingEnabled = true;
            this.clbItems.IntegralHeight = false;
            this.clbItems.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.clbItems.CheckOnClick = true;
            this.clbItems.Location = new System.Drawing.Point(0, 0);
            this.clbItems.Name = "clbItems";
            this.clbItems.Size = new System.Drawing.Size(147, 154);
            this.clbItems.TabIndex = 0;
            this.clbItems.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.clbItems_ItemCheck);
            // 
            // StringListUIEditorControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.clbItems);
            this.Name = "StringListUIEditorControl";
            this.Font = new System.Drawing.Font("Tahoma", 8.25f);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.CheckedListBox clbItems;
    }
}
