namespace AGS.Types
{
    partial class FlagsUIEditorControl
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;
        private System.Windows.Forms.CheckedListBox lvwItems = null;

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
            lvwItems = new System.Windows.Forms.CheckedListBox();
            SuspendLayout();

            lvwItems.Dock = System.Windows.Forms.DockStyle.Fill;
            lvwItems.FormattingEnabled = true;
            lvwItems.IntegralHeight = false;
            lvwItems.BorderStyle = System.Windows.Forms.BorderStyle.None;
            lvwItems.CheckOnClick = true;
            lvwItems.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(lvwItems_ItemCheck);
            Controls.Add(this.lvwItems);

            Font = new System.Drawing.Font("Tahoma", 8.25F);
            ResumeLayout(false);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        }

        #endregion
    }
}
