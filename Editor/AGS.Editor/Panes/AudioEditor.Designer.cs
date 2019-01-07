namespace AGS.Editor
{
    partial class AudioEditor
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AudioEditor));
            this.grpAudioClip = new System.Windows.Forms.GroupBox();
            this.btnStop = new System.Windows.Forms.Button();
            this.lblCurrentPosition = new System.Windows.Forms.Label();
            this.btnPause = new System.Windows.Forms.Button();
            this.lblSoundName = new System.Windows.Forms.Label();
            this.btnPlay = new System.Windows.Forms.Button();
            this.grpFolder = new System.Windows.Forms.GroupBox();
            this.label1 = new System.Windows.Forms.Label();
            this.lblFolderTitle = new System.Windows.Forms.Label();
            this.grpAudioType = new System.Windows.Forms.GroupBox();
            this.label2 = new System.Windows.Forms.Label();
            this.lblAudioTypeTitle = new System.Windows.Forms.Label();
            this.lblClipLength = new System.Windows.Forms.Label();
            this.grpAudioClip.SuspendLayout();
            this.grpFolder.SuspendLayout();
            this.grpAudioType.SuspendLayout();
            this.SuspendLayout();
            // 
            // grpAudioClip
            // 
            this.grpAudioClip.Controls.Add(this.lblClipLength);
            this.grpAudioClip.Controls.Add(this.btnStop);
            this.grpAudioClip.Controls.Add(this.lblCurrentPosition);
            this.grpAudioClip.Controls.Add(this.btnPause);
            this.grpAudioClip.Controls.Add(this.lblSoundName);
            this.grpAudioClip.Controls.Add(this.btnPlay);
            this.grpAudioClip.Location = new System.Drawing.Point(16, 9);
            this.grpAudioClip.Name = "grpAudioClip";
            this.grpAudioClip.Size = new System.Drawing.Size(424, 219);
            this.grpAudioClip.TabIndex = 0;
            this.grpAudioClip.TabStop = false;
            this.grpAudioClip.Text = "Audio Clip";
            // 
            // btnStop
            // 
            this.btnStop.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnStop.Image = ((System.Drawing.Image)(resources.GetObject("btnStop.Image")));
            this.btnStop.Location = new System.Drawing.Point(128, 158);
            this.btnStop.Name = "btnStop";
            this.btnStop.Size = new System.Drawing.Size(42, 30);
            this.btnStop.TabIndex = 4;
            this.btnStop.UseVisualStyleBackColor = true;
            this.btnStop.Click += new System.EventHandler(this.btnStop_Click);
            // 
            // lblCurrentPosition
            // 
            this.lblCurrentPosition.AutoSize = true;
            this.lblCurrentPosition.Font = new System.Drawing.Font("Tahoma", 15.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblCurrentPosition.Location = new System.Drawing.Point(176, 160);
            this.lblCurrentPosition.Name = "lblCurrentPosition";
            this.lblCurrentPosition.Size = new System.Drawing.Size(63, 25);
            this.lblCurrentPosition.TabIndex = 3;
            this.lblCurrentPosition.Text = "00:00";
            // 
            // btnPause
            // 
            this.btnPause.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnPause.Image = ((System.Drawing.Image)(resources.GetObject("btnPause.Image")));
            this.btnPause.Location = new System.Drawing.Point(80, 158);
            this.btnPause.Name = "btnPause";
            this.btnPause.Size = new System.Drawing.Size(42, 30);
            this.btnPause.TabIndex = 2;
            this.btnPause.UseVisualStyleBackColor = true;
            this.btnPause.Click += new System.EventHandler(this.btnPause_Click);
            // 
            // lblSoundName
            // 
            this.lblSoundName.AutoSize = true;
            this.lblSoundName.Location = new System.Drawing.Point(24, 22);
            this.lblSoundName.Name = "lblSoundName";
            this.lblSoundName.Size = new System.Drawing.Size(35, 13);
            this.lblSoundName.TabIndex = 1;
            this.lblSoundName.Text = "label1";
            // 
            // btnPlay
            // 
            this.btnPlay.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnPlay.Image = ((System.Drawing.Image)(resources.GetObject("btnPlay.Image")));
            this.btnPlay.Location = new System.Drawing.Point(32, 158);
            this.btnPlay.Name = "btnPlay";
            this.btnPlay.Size = new System.Drawing.Size(42, 30);
            this.btnPlay.TabIndex = 0;
            this.btnPlay.UseVisualStyleBackColor = true;
            this.btnPlay.Click += new System.EventHandler(this.btnPlay_Click);
            // 
            // grpFolder
            // 
            this.grpFolder.Controls.Add(this.label1);
            this.grpFolder.Controls.Add(this.lblFolderTitle);
            this.grpFolder.Location = new System.Drawing.Point(16, 9);
            this.grpFolder.Name = "grpFolder";
            this.grpFolder.Size = new System.Drawing.Size(424, 219);
            this.grpFolder.TabIndex = 1;
            this.grpFolder.TabStop = false;
            this.grpFolder.Text = "Audio Folder";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(24, 63);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(354, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "Use the Properties Pane to the right to set the properties for this folder.";
            // 
            // lblFolderTitle
            // 
            this.lblFolderTitle.AutoSize = true;
            this.lblFolderTitle.Location = new System.Drawing.Point(24, 25);
            this.lblFolderTitle.Name = "lblFolderTitle";
            this.lblFolderTitle.Size = new System.Drawing.Size(62, 13);
            this.lblFolderTitle.TabIndex = 1;
            this.lblFolderTitle.Text = "folderName";
            // 
            // grpAudioType
            // 
            this.grpAudioType.Controls.Add(this.label2);
            this.grpAudioType.Controls.Add(this.lblAudioTypeTitle);
            this.grpAudioType.Location = new System.Drawing.Point(15, 9);
            this.grpAudioType.Name = "grpAudioType";
            this.grpAudioType.Size = new System.Drawing.Size(424, 219);
            this.grpAudioType.TabIndex = 3;
            this.grpAudioType.TabStop = false;
            this.grpAudioType.Text = "Audio Type";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(24, 63);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(377, 13);
            this.label2.TabIndex = 2;
            this.label2.Text = "Use the Properties Pane to the right to set the properties for this audio type.";
            // 
            // lblAudioTypeTitle
            // 
            this.lblAudioTypeTitle.AutoSize = true;
            this.lblAudioTypeTitle.Location = new System.Drawing.Point(24, 25);
            this.lblAudioTypeTitle.Name = "lblAudioTypeTitle";
            this.lblAudioTypeTitle.Size = new System.Drawing.Size(62, 13);
            this.lblAudioTypeTitle.TabIndex = 1;
            this.lblAudioTypeTitle.Text = "folderName";
            // 
            // lblClipLength
            // 
            this.lblClipLength.AutoSize = true;
            this.lblClipLength.Font = new System.Drawing.Font("Tahoma", 15.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblClipLength.Location = new System.Drawing.Point(236, 160);
            this.lblClipLength.Name = "lblClipLength";
            this.lblClipLength.Size = new System.Drawing.Size(78, 25);
            this.lblClipLength.TabIndex = 5;
            this.lblClipLength.Text = "/ 00:00";
            // 
            // AudioEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.grpAudioClip);
            this.Controls.Add(this.grpAudioType);
            this.Controls.Add(this.grpFolder);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "AudioEditor";
            this.Size = new System.Drawing.Size(865, 509);
            this.grpAudioClip.ResumeLayout(false);
            this.grpAudioClip.PerformLayout();
            this.grpFolder.ResumeLayout(false);
            this.grpFolder.PerformLayout();
            this.grpAudioType.ResumeLayout(false);
            this.grpAudioType.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox grpAudioClip;
        private System.Windows.Forms.Button btnPlay;
        private System.Windows.Forms.Label lblSoundName;
        private System.Windows.Forms.Button btnPause;
        private System.Windows.Forms.Label lblCurrentPosition;
        private System.Windows.Forms.Button btnStop;
        private System.Windows.Forms.GroupBox grpFolder;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label lblFolderTitle;
        private System.Windows.Forms.GroupBox grpAudioType;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label lblAudioTypeTitle;
        private System.Windows.Forms.Label lblClipLength;
    }
}
