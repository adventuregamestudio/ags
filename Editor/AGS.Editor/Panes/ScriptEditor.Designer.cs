namespace AGS.Editor
{
    partial class ScriptEditor
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;
        private ScintillaWrapper scintilla = null;

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
			this.scintilla = new AGS.Editor.ScintillaWrapper();
			this.panel1 = new System.Windows.Forms.Panel();
			this.cmbFunctions = new System.Windows.Forms.ComboBox();
			this.panel1.SuspendLayout();
			this.SuspendLayout();
			// 
			// scintilla
			// 
			this.scintilla.AutoCompleteEnabled = true;
			this.scintilla.AutoSpaceAfterComma = true;
			this.scintilla.CallTipsEnabled = true;
			this.scintilla.Dock = System.Windows.Forms.DockStyle.Fill;
			this.scintilla.Location = new System.Drawing.Point(0, 31);
			this.scintilla.Name = "scintilla";
			this.scintilla.Size = new System.Drawing.Size(574, 285);
			this.scintilla.TabIndex = 0;
			// 
			// panel1
			// 
			this.panel1.Controls.Add(this.cmbFunctions);
			this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
			this.panel1.Location = new System.Drawing.Point(0, 0);
			this.panel1.Name = "panel1";
			this.panel1.Size = new System.Drawing.Size(574, 31);
			this.panel1.TabIndex = 1;
			// 
			// cmbFunctions
			// 
			this.cmbFunctions.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.cmbFunctions.FormattingEnabled = true;
			this.cmbFunctions.Location = new System.Drawing.Point(5, 5);
			this.cmbFunctions.MaxDropDownItems = 20;
			this.cmbFunctions.Name = "cmbFunctions";
			this.cmbFunctions.Size = new System.Drawing.Size(285, 21);
			this.cmbFunctions.TabIndex = 0;
			this.cmbFunctions.SelectedIndexChanged += new System.EventHandler(this.cmbFunctions_SelectedIndexChanged);
            this.cmbFunctions.MouseEnter += new System.EventHandler(this.cmbFunctions_MouseEnter);
            this.cmbFunctions.MouseLeave += new System.EventHandler(this.cmbFunctions_MouseLeave);
			// 
			// ScriptEditor
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.Controls.Add(this.scintilla);
			this.Controls.Add(this.panel1);
			this.Name = "ScriptEditor";
			this.Size = new System.Drawing.Size(574, 316);
			this.panel1.ResumeLayout(false);
			this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.ComboBox cmbFunctions;
    }
}
