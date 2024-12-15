namespace AGS.Editor
{
    partial class CustomPropertySchemaItemEditor
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.txtName = new System.Windows.Forms.TextBox();
            this.txtDescription = new System.Windows.Forms.TextBox();
            this.txtDefaultValue = new System.Windows.Forms.TextBox();
            this.cmbType = new System.Windows.Forms.ComboBox();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.chkWalkareas = new System.Windows.Forms.CheckBox();
            this.chkRegions = new System.Windows.Forms.CheckBox();
            this.chkAudioClips = new System.Windows.Forms.CheckBox();
            this.chkGUIs = new System.Windows.Forms.CheckBox();
            this.chkDialogs = new System.Windows.Forms.CheckBox();
            this.chkRooms = new System.Windows.Forms.CheckBox();
            this.label5 = new System.Windows.Forms.Label();
            this.chkInventory = new System.Windows.Forms.CheckBox();
            this.chkHotspots = new System.Windows.Forms.CheckBox();
            this.chkObjects = new System.Windows.Forms.CheckBox();
            this.chkCharacters = new System.Windows.Forms.CheckBox();
            this.chkGUIControls = new System.Windows.Forms.CheckBox();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 18);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(38, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Name:";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 46);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(64, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "Description:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(12, 74);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(35, 13);
            this.label3.TabIndex = 2;
            this.label3.Text = "Type:";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(12, 102);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(75, 13);
            this.label4.TabIndex = 3;
            this.label4.Text = "Default value:";
            // 
            // txtName
            // 
            this.txtName.Location = new System.Drawing.Point(96, 18);
            this.txtName.Name = "txtName";
            this.txtName.Size = new System.Drawing.Size(194, 21);
            this.txtName.TabIndex = 4;
            // 
            // txtDescription
            // 
            this.txtDescription.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtDescription.Location = new System.Drawing.Point(96, 45);
            this.txtDescription.Name = "txtDescription";
            this.txtDescription.Size = new System.Drawing.Size(441, 21);
            this.txtDescription.TabIndex = 5;
            // 
            // txtDefaultValue
            // 
            this.txtDefaultValue.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtDefaultValue.Location = new System.Drawing.Point(96, 99);
            this.txtDefaultValue.Name = "txtDefaultValue";
            this.txtDefaultValue.Size = new System.Drawing.Size(441, 21);
            this.txtDefaultValue.TabIndex = 7;
            // 
            // cmbType
            // 
            this.cmbType.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbType.FormattingEnabled = true;
            this.cmbType.Items.AddRange(new object[] {
            "Boolean",
            "Number",
            "Text"});
            this.cmbType.Location = new System.Drawing.Point(96, 73);
            this.cmbType.Name = "cmbType";
            this.cmbType.Size = new System.Drawing.Size(146, 21);
            this.cmbType.TabIndex = 6;
            // 
            // btnOK
            // 
            this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnOK.Location = new System.Drawing.Point(15, 264);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(92, 26);
            this.btnOK.TabIndex = 8;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(113, 264);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(92, 26);
            this.btnCancel.TabIndex = 9;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox1.Controls.Add(this.chkGUIControls);
            this.groupBox1.Controls.Add(this.chkWalkareas);
            this.groupBox1.Controls.Add(this.chkRegions);
            this.groupBox1.Controls.Add(this.chkAudioClips);
            this.groupBox1.Controls.Add(this.chkGUIs);
            this.groupBox1.Controls.Add(this.chkDialogs);
            this.groupBox1.Controls.Add(this.chkRooms);
            this.groupBox1.Controls.Add(this.label5);
            this.groupBox1.Controls.Add(this.chkInventory);
            this.groupBox1.Controls.Add(this.chkHotspots);
            this.groupBox1.Controls.Add(this.chkObjects);
            this.groupBox1.Controls.Add(this.chkCharacters);
            this.groupBox1.Location = new System.Drawing.Point(15, 129);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(522, 119);
            this.groupBox1.TabIndex = 11;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Applies To";
            // 
            // chkWalkareas
            // 
            this.chkWalkareas.AutoSize = true;
            this.chkWalkareas.Location = new System.Drawing.Point(346, 64);
            this.chkWalkareas.Name = "chkWalkareas";
            this.chkWalkareas.Size = new System.Drawing.Size(100, 17);
            this.chkWalkareas.TabIndex = 21;
            this.chkWalkareas.Text = "Walkable Areas";
            this.chkWalkareas.UseVisualStyleBackColor = true;
            // 
            // chkRegions
            // 
            this.chkRegions.AutoSize = true;
            this.chkRegions.Location = new System.Drawing.Point(253, 64);
            this.chkRegions.Name = "chkRegions";
            this.chkRegions.Size = new System.Drawing.Size(64, 17);
            this.chkRegions.TabIndex = 20;
            this.chkRegions.Text = "Regions";
            this.chkRegions.UseVisualStyleBackColor = true;
            // 
            // chkAudioClips
            // 
            this.chkAudioClips.AutoSize = true;
            this.chkAudioClips.Location = new System.Drawing.Point(13, 87);
            this.chkAudioClips.Name = "chkAudioClips";
            this.chkAudioClips.Size = new System.Drawing.Size(78, 17);
            this.chkAudioClips.TabIndex = 19;
            this.chkAudioClips.Text = "Audio Clips";
            this.chkAudioClips.UseVisualStyleBackColor = true;
            // 
            // chkGUIs
            // 
            this.chkGUIs.AutoSize = true;
            this.chkGUIs.Location = new System.Drawing.Point(182, 41);
            this.chkGUIs.Name = "chkGUIs";
            this.chkGUIs.Size = new System.Drawing.Size(49, 17);
            this.chkGUIs.TabIndex = 18;
            this.chkGUIs.Text = "GUIs";
            this.chkGUIs.UseVisualStyleBackColor = true;
            // 
            // chkDialogs
            // 
            this.chkDialogs.AutoSize = true;
            this.chkDialogs.Location = new System.Drawing.Point(98, 41);
            this.chkDialogs.Name = "chkDialogs";
            this.chkDialogs.Size = new System.Drawing.Size(60, 17);
            this.chkDialogs.TabIndex = 17;
            this.chkDialogs.Text = "Dialogs";
            this.chkDialogs.UseVisualStyleBackColor = true;
            // 
            // chkRooms
            // 
            this.chkRooms.AutoSize = true;
            this.chkRooms.Location = new System.Drawing.Point(13, 64);
            this.chkRooms.Name = "chkRooms";
            this.chkRooms.Size = new System.Drawing.Size(58, 17);
            this.chkRooms.TabIndex = 16;
            this.chkRooms.Text = "Rooms";
            this.chkRooms.UseVisualStyleBackColor = true;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(10, 19);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(294, 13);
            this.label5.TabIndex = 15;
            this.label5.Text = "Which types of thing do you want this property to apply to?";
            // 
            // chkInventory
            // 
            this.chkInventory.AutoSize = true;
            this.chkInventory.Location = new System.Drawing.Point(346, 41);
            this.chkInventory.Name = "chkInventory";
            this.chkInventory.Size = new System.Drawing.Size(104, 17);
            this.chkInventory.TabIndex = 14;
            this.chkInventory.Text = "Inventory Items";
            this.chkInventory.UseVisualStyleBackColor = true;
            // 
            // chkHotspots
            // 
            this.chkHotspots.AutoSize = true;
            this.chkHotspots.Location = new System.Drawing.Point(182, 64);
            this.chkHotspots.Name = "chkHotspots";
            this.chkHotspots.Size = new System.Drawing.Size(69, 17);
            this.chkHotspots.TabIndex = 13;
            this.chkHotspots.Text = "Hotspots";
            this.chkHotspots.UseVisualStyleBackColor = true;
            // 
            // chkObjects
            // 
            this.chkObjects.AutoSize = true;
            this.chkObjects.Location = new System.Drawing.Point(98, 64);
            this.chkObjects.Name = "chkObjects";
            this.chkObjects.Size = new System.Drawing.Size(63, 17);
            this.chkObjects.TabIndex = 12;
            this.chkObjects.Text = "Objects";
            this.chkObjects.UseVisualStyleBackColor = true;
            // 
            // chkCharacters
            // 
            this.chkCharacters.AutoSize = true;
            this.chkCharacters.Location = new System.Drawing.Point(13, 41);
            this.chkCharacters.Name = "chkCharacters";
            this.chkCharacters.Size = new System.Drawing.Size(79, 17);
            this.chkCharacters.TabIndex = 11;
            this.chkCharacters.Text = "Characters";
            this.chkCharacters.UseVisualStyleBackColor = true;
            // 
            // chkGUIControls
            // 
            this.chkGUIControls.AutoSize = true;
            this.chkGUIControls.Location = new System.Drawing.Point(253, 41);
            this.chkGUIControls.Name = "chkGUIControls";
            this.chkGUIControls.Size = new System.Drawing.Size(87, 17);
            this.chkGUIControls.TabIndex = 22;
            this.chkGUIControls.Text = "GUI Controls";
            this.chkGUIControls.UseVisualStyleBackColor = true;
            // 
            // CustomPropertySchemaItemEditor
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(551, 302);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.cmbType);
            this.Controls.Add(this.txtDefaultValue);
            this.Controls.Add(this.txtDescription);
            this.Controls.Add(this.txtName);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "CustomPropertySchemaItemEditor";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Property settings";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.CustomPropertySchemaItemEditor_FormClosed);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox txtName;
        private System.Windows.Forms.TextBox txtDescription;
        private System.Windows.Forms.TextBox txtDefaultValue;
        private System.Windows.Forms.ComboBox cmbType;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.CheckBox chkInventory;
        private System.Windows.Forms.CheckBox chkHotspots;
        private System.Windows.Forms.CheckBox chkObjects;
        private System.Windows.Forms.CheckBox chkCharacters;
        private System.Windows.Forms.CheckBox chkRooms;
        private System.Windows.Forms.CheckBox chkGUIs;
        private System.Windows.Forms.CheckBox chkDialogs;
        private System.Windows.Forms.CheckBox chkAudioClips;
        private System.Windows.Forms.CheckBox chkWalkareas;
        private System.Windows.Forms.CheckBox chkRegions;
        private System.Windows.Forms.CheckBox chkGUIControls;
    }
}