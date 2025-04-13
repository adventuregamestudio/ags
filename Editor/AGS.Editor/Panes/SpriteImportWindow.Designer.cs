namespace AGS.Editor
{
    partial class SpriteImportWindow
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SpriteImportWindow));
            this.groupImportOptions = new System.Windows.Forms.GroupBox();
            this.chkUseAlphaChannel = new System.Windows.Forms.CheckBox();
            this.chkRoomBackground = new System.Windows.Forms.CheckBox();
            this.chkRemapCols = new System.Windows.Forms.CheckBox();
            this.groupTransColour = new System.Windows.Forms.GroupBox();
            this.udTransColorIndex = new System.Windows.Forms.NumericUpDown();
            this.panelBottomRight = new System.Windows.Forms.Panel();
            this.panelTopRight = new System.Windows.Forms.Panel();
            this.panelBottomLeft = new System.Windows.Forms.Panel();
            this.panelIndex0 = new System.Windows.Forms.Panel();
            this.panelTopLeft = new System.Windows.Forms.Panel();
            this.radTransColourNone = new System.Windows.Forms.RadioButton();
            this.radTransColourLeaveAsIs = new System.Windows.Forms.RadioButton();
            this.radTransColourBottomRightPixel = new System.Windows.Forms.RadioButton();
            this.radTransColourTopRightPixel = new System.Windows.Forms.RadioButton();
            this.radTransColourBottomLeftPixel = new System.Windows.Forms.RadioButton();
            this.radTransColourTopLeftPixel = new System.Windows.Forms.RadioButton();
            this.radTransColourIndex = new System.Windows.Forms.RadioButton();
            this.btnClose = new System.Windows.Forms.Button();
            this.btnImport = new System.Windows.Forms.Button();
            this.chkTiled = new System.Windows.Forms.CheckBox();
            this.cmbFilenames = new System.Windows.Forms.ComboBox();
            this.lblImageDescription = new System.Windows.Forms.Label();
            this.zoomSlider = new System.Windows.Forms.TrackBar();
            this.lblZoom = new System.Windows.Forms.Label();
            this.groupSelection = new System.Windows.Forms.GroupBox();
            this.cmbTileDirection = new System.Windows.Forms.ComboBox();
            this.numMaxTiles = new System.Windows.Forms.NumericUpDown();
            this.lblY = new System.Windows.Forms.Label();
            this.lblMaxTiles = new System.Windows.Forms.Label();
            this.lblX = new System.Windows.Forms.Label();
            this.lblTileDirection = new System.Windows.Forms.Label();
            this.lblMargin = new System.Windows.Forms.Label();
            this.numMarginX = new System.Windows.Forms.NumericUpDown();
            this.numMarginY = new System.Windows.Forms.NumericUpDown();
            this.lblSize = new System.Windows.Forms.Label();
            this.lblOffset = new System.Windows.Forms.Label();
            this.numSizeY = new System.Windows.Forms.NumericUpDown();
            this.numSizeX = new System.Windows.Forms.NumericUpDown();
            this.numOffsetX = new System.Windows.Forms.NumericUpDown();
            this.numOffsetY = new System.Windows.Forms.NumericUpDown();
            this.previewPanel = new AGS.Editor.BufferedPanel();
            this.btnImportAll = new System.Windows.Forms.Button();
            this.groupImportOptions.SuspendLayout();
            this.groupTransColour.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.udTransColorIndex)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.zoomSlider)).BeginInit();
            this.groupSelection.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numMaxTiles)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numMarginX)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numMarginY)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numSizeY)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numSizeX)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numOffsetX)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numOffsetY)).BeginInit();
            this.SuspendLayout();
            // 
            // groupImportOptions
            // 
            this.groupImportOptions.Controls.Add(this.chkUseAlphaChannel);
            this.groupImportOptions.Controls.Add(this.chkRoomBackground);
            this.groupImportOptions.Controls.Add(this.chkRemapCols);
            this.groupImportOptions.Location = new System.Drawing.Point(12, 12);
            this.groupImportOptions.Name = "groupImportOptions";
            this.groupImportOptions.Size = new System.Drawing.Size(224, 90);
            this.groupImportOptions.TabIndex = 3;
            this.groupImportOptions.TabStop = false;
            this.groupImportOptions.Text = "Import options";
            // 
            // chkUseAlphaChannel
            // 
            this.chkUseAlphaChannel.AutoSize = true;
            this.chkUseAlphaChannel.Checked = true;
            this.chkUseAlphaChannel.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkUseAlphaChannel.Location = new System.Drawing.Point(6, 20);
            this.chkUseAlphaChannel.Name = "chkUseAlphaChannel";
            this.chkUseAlphaChannel.Size = new System.Drawing.Size(189, 17);
            this.chkUseAlphaChannel.TabIndex = 3;
            this.chkUseAlphaChannel.Text = "Import alpha channel (if available)";
            this.chkUseAlphaChannel.UseVisualStyleBackColor = true;
            // 
            // chkRoomBackground
            // 
            this.chkRoomBackground.AutoSize = true;
            this.chkRoomBackground.Location = new System.Drawing.Point(6, 66);
            this.chkRoomBackground.Name = "chkRoomBackground";
            this.chkRoomBackground.Size = new System.Drawing.Size(215, 17);
            this.chkRoomBackground.TabIndex = 5;
            this.chkRoomBackground.Text = "Use room background (8-bit game only)";
            this.chkRoomBackground.UseVisualStyleBackColor = true;
            // 
            // chkRemapCols
            // 
            this.chkRemapCols.AutoSize = true;
            this.chkRemapCols.Checked = true;
            this.chkRemapCols.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkRemapCols.Location = new System.Drawing.Point(6, 43);
            this.chkRemapCols.Name = "chkRemapCols";
            this.chkRemapCols.Size = new System.Drawing.Size(183, 17);
            this.chkRemapCols.TabIndex = 4;
            this.chkRemapCols.Text = "Remap palette (8-bit image only)";
            this.chkRemapCols.UseVisualStyleBackColor = true;
            // 
            // groupTransColour
            // 
            this.groupTransColour.Controls.Add(this.udTransColorIndex);
            this.groupTransColour.Controls.Add(this.panelBottomRight);
            this.groupTransColour.Controls.Add(this.panelTopRight);
            this.groupTransColour.Controls.Add(this.panelBottomLeft);
            this.groupTransColour.Controls.Add(this.panelIndex0);
            this.groupTransColour.Controls.Add(this.panelTopLeft);
            this.groupTransColour.Controls.Add(this.radTransColourNone);
            this.groupTransColour.Controls.Add(this.radTransColourLeaveAsIs);
            this.groupTransColour.Controls.Add(this.radTransColourBottomRightPixel);
            this.groupTransColour.Controls.Add(this.radTransColourTopRightPixel);
            this.groupTransColour.Controls.Add(this.radTransColourBottomLeftPixel);
            this.groupTransColour.Controls.Add(this.radTransColourTopLeftPixel);
            this.groupTransColour.Controls.Add(this.radTransColourIndex);
            this.groupTransColour.Location = new System.Drawing.Point(12, 108);
            this.groupTransColour.Name = "groupTransColour";
            this.groupTransColour.Size = new System.Drawing.Size(224, 192);
            this.groupTransColour.TabIndex = 6;
            this.groupTransColour.TabStop = false;
            this.groupTransColour.Text = "Transparent colour";
            // 
            // udTransColorIndex
            // 
            this.udTransColorIndex.Location = new System.Drawing.Point(99, 64);
            this.udTransColorIndex.Maximum = new decimal(new int[] {
            255,
            0,
            0,
            0});
            this.udTransColorIndex.Name = "udTransColorIndex";
            this.udTransColorIndex.Size = new System.Drawing.Size(48, 21);
            this.udTransColorIndex.TabIndex = 13;
            this.udTransColorIndex.ValueChanged += new System.EventHandler(this.udTransColorIndex_ValueChanged);
            // 
            // panelBottomRight
            // 
            this.panelBottomRight.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panelBottomRight.Location = new System.Drawing.Point(152, 158);
            this.panelBottomRight.Name = "panelBottomRight";
            this.panelBottomRight.Size = new System.Drawing.Size(62, 17);
            this.panelBottomRight.TabIndex = 12;
            // 
            // panelTopRight
            // 
            this.panelTopRight.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panelTopRight.Location = new System.Drawing.Point(152, 135);
            this.panelTopRight.Name = "panelTopRight";
            this.panelTopRight.Size = new System.Drawing.Size(62, 17);
            this.panelTopRight.TabIndex = 11;
            // 
            // panelBottomLeft
            // 
            this.panelBottomLeft.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panelBottomLeft.Location = new System.Drawing.Point(152, 112);
            this.panelBottomLeft.Name = "panelBottomLeft";
            this.panelBottomLeft.Size = new System.Drawing.Size(62, 17);
            this.panelBottomLeft.TabIndex = 10;
            // 
            // panelIndex0
            // 
            this.panelIndex0.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panelIndex0.Location = new System.Drawing.Point(152, 66);
            this.panelIndex0.Name = "panelIndex0";
            this.panelIndex0.Size = new System.Drawing.Size(62, 17);
            this.panelIndex0.TabIndex = 8;
            // 
            // panelTopLeft
            // 
            this.panelTopLeft.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panelTopLeft.Location = new System.Drawing.Point(152, 89);
            this.panelTopLeft.Name = "panelTopLeft";
            this.panelTopLeft.Size = new System.Drawing.Size(62, 17);
            this.panelTopLeft.TabIndex = 9;
            // 
            // radTransColourNone
            // 
            this.radTransColourNone.AutoSize = true;
            this.radTransColourNone.Location = new System.Drawing.Point(6, 43);
            this.radTransColourNone.Name = "radTransColourNone";
            this.radTransColourNone.Size = new System.Drawing.Size(105, 17);
            this.radTransColourNone.TabIndex = 7;
            this.radTransColourNone.Text = "No transparency";
            this.radTransColourNone.UseVisualStyleBackColor = true;
            // 
            // radTransColourLeaveAsIs
            // 
            this.radTransColourLeaveAsIs.AutoSize = true;
            this.radTransColourLeaveAsIs.Checked = true;
            this.radTransColourLeaveAsIs.Location = new System.Drawing.Point(6, 20);
            this.radTransColourLeaveAsIs.Name = "radTransColourLeaveAsIs";
            this.radTransColourLeaveAsIs.Size = new System.Drawing.Size(79, 17);
            this.radTransColourLeaveAsIs.TabIndex = 6;
            this.radTransColourLeaveAsIs.TabStop = true;
            this.radTransColourLeaveAsIs.Text = "Leave as-is";
            this.radTransColourLeaveAsIs.UseVisualStyleBackColor = true;
            // 
            // radTransColourBottomRightPixel
            // 
            this.radTransColourBottomRightPixel.AutoSize = true;
            this.radTransColourBottomRightPixel.Location = new System.Drawing.Point(5, 158);
            this.radTransColourBottomRightPixel.Name = "radTransColourBottomRightPixel";
            this.radTransColourBottomRightPixel.Size = new System.Drawing.Size(110, 17);
            this.radTransColourBottomRightPixel.TabIndex = 12;
            this.radTransColourBottomRightPixel.Text = "Bottom-right pixel";
            this.radTransColourBottomRightPixel.UseVisualStyleBackColor = true;
            // 
            // radTransColourTopRightPixel
            // 
            this.radTransColourTopRightPixel.AutoSize = true;
            this.radTransColourTopRightPixel.Location = new System.Drawing.Point(5, 135);
            this.radTransColourTopRightPixel.Name = "radTransColourTopRightPixel";
            this.radTransColourTopRightPixel.Size = new System.Drawing.Size(94, 17);
            this.radTransColourTopRightPixel.TabIndex = 11;
            this.radTransColourTopRightPixel.Text = "Top-right pixel";
            this.radTransColourTopRightPixel.UseVisualStyleBackColor = true;
            // 
            // radTransColourBottomLeftPixel
            // 
            this.radTransColourBottomLeftPixel.AutoSize = true;
            this.radTransColourBottomLeftPixel.Location = new System.Drawing.Point(5, 112);
            this.radTransColourBottomLeftPixel.Name = "radTransColourBottomLeftPixel";
            this.radTransColourBottomLeftPixel.Size = new System.Drawing.Size(104, 17);
            this.radTransColourBottomLeftPixel.TabIndex = 10;
            this.radTransColourBottomLeftPixel.Text = "Bottom-left pixel";
            this.radTransColourBottomLeftPixel.UseVisualStyleBackColor = true;
            // 
            // radTransColourTopLeftPixel
            // 
            this.radTransColourTopLeftPixel.AutoSize = true;
            this.radTransColourTopLeftPixel.Location = new System.Drawing.Point(5, 89);
            this.radTransColourTopLeftPixel.Name = "radTransColourTopLeftPixel";
            this.radTransColourTopLeftPixel.Size = new System.Drawing.Size(88, 17);
            this.radTransColourTopLeftPixel.TabIndex = 9;
            this.radTransColourTopLeftPixel.Text = "Top-left pixel";
            this.radTransColourTopLeftPixel.UseVisualStyleBackColor = true;
            // 
            // radTransColourIndex
            // 
            this.radTransColourIndex.AutoSize = true;
            this.radTransColourIndex.Location = new System.Drawing.Point(5, 66);
            this.radTransColourIndex.Name = "radTransColourIndex";
            this.radTransColourIndex.Size = new System.Drawing.Size(92, 17);
            this.radTransColourIndex.TabIndex = 8;
            this.radTransColourIndex.Text = "Palette index:";
            this.radTransColourIndex.UseVisualStyleBackColor = true;
            // 
            // btnClose
            // 
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnClose.Location = new System.Drawing.Point(12, 569);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(70, 30);
            this.btnClose.TabIndex = 23;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // btnImport
            // 
            this.btnImport.Location = new System.Drawing.Point(89, 569);
            this.btnImport.Name = "btnImport";
            this.btnImport.Size = new System.Drawing.Size(70, 30);
            this.btnImport.TabIndex = 0;
            this.btnImport.Text = "Import";
            this.btnImport.UseVisualStyleBackColor = true;
            this.btnImport.Click += new System.EventHandler(this.btnImport_Click);
            // 
            // chkTiled
            // 
            this.chkTiled.AutoSize = true;
            this.chkTiled.Location = new System.Drawing.Point(63, 20);
            this.chkTiled.Name = "chkTiled";
            this.chkTiled.Size = new System.Drawing.Size(111, 17);
            this.chkTiled.TabIndex = 13;
            this.chkTiled.Text = "Tiled sprite import";
            this.chkTiled.UseVisualStyleBackColor = true;
            this.chkTiled.CheckedChanged += new System.EventHandler(this.chkTiled_CheckedChanged);
            // 
            // cmbFilenames
            // 
            this.cmbFilenames.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbFilenames.FormattingEnabled = true;
            this.cmbFilenames.Location = new System.Drawing.Point(12, 519);
            this.cmbFilenames.Name = "cmbFilenames";
            this.cmbFilenames.Size = new System.Drawing.Size(224, 21);
            this.cmbFilenames.TabIndex = 22;
            this.cmbFilenames.SelectedIndexChanged += new System.EventHandler(this.cmbFilenames_SelectedIndexChanged);
            // 
            // lblImageDescription
            // 
            this.lblImageDescription.AutoSize = true;
            this.lblImageDescription.Location = new System.Drawing.Point(13, 547);
            this.lblImageDescription.Name = "lblImageDescription";
            this.lblImageDescription.Size = new System.Drawing.Size(69, 13);
            this.lblImageDescription.TabIndex = 23;
            this.lblImageDescription.Text = "image details";
            // 
            // zoomSlider
            // 
            this.zoomSlider.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.zoomSlider.LargeChange = 2;
            this.zoomSlider.Location = new System.Drawing.Point(245, 15);
            this.zoomSlider.Maximum = 20;
            this.zoomSlider.Minimum = 1;
            this.zoomSlider.Name = "zoomSlider";
            this.zoomSlider.Size = new System.Drawing.Size(567, 42);
            this.zoomSlider.TabIndex = 2;
            this.zoomSlider.Value = 1;
            this.zoomSlider.Scroll += new System.EventHandler(this.zoomSlider_Scroll);
            // 
            // lblZoom
            // 
            this.lblZoom.AutoSize = true;
            this.lblZoom.Location = new System.Drawing.Point(242, 47);
            this.lblZoom.Name = "lblZoom";
            this.lblZoom.Size = new System.Drawing.Size(55, 13);
            this.lblZoom.TabIndex = 24;
            this.lblZoom.Text = "Zoom: x 1";
            // 
            // groupSelection
            // 
            this.groupSelection.Controls.Add(this.cmbTileDirection);
            this.groupSelection.Controls.Add(this.numMaxTiles);
            this.groupSelection.Controls.Add(this.lblY);
            this.groupSelection.Controls.Add(this.lblMaxTiles);
            this.groupSelection.Controls.Add(this.lblX);
            this.groupSelection.Controls.Add(this.lblTileDirection);
            this.groupSelection.Controls.Add(this.lblMargin);
            this.groupSelection.Controls.Add(this.numMarginX);
            this.groupSelection.Controls.Add(this.numMarginY);
            this.groupSelection.Controls.Add(this.lblSize);
            this.groupSelection.Controls.Add(this.lblOffset);
            this.groupSelection.Controls.Add(this.numSizeY);
            this.groupSelection.Controls.Add(this.numSizeX);
            this.groupSelection.Controls.Add(this.numOffsetX);
            this.groupSelection.Controls.Add(this.numOffsetY);
            this.groupSelection.Controls.Add(this.chkTiled);
            this.groupSelection.Location = new System.Drawing.Point(12, 306);
            this.groupSelection.Name = "groupSelection";
            this.groupSelection.Size = new System.Drawing.Size(224, 207);
            this.groupSelection.TabIndex = 13;
            this.groupSelection.TabStop = false;
            this.groupSelection.Text = "Selection";
            // 
            // cmbTileDirection
            // 
            this.cmbTileDirection.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbTileDirection.Enabled = false;
            this.cmbTileDirection.FormattingEnabled = true;
            this.cmbTileDirection.Items.AddRange(new object[] {
            "To the right",
            "Downwards"});
            this.cmbTileDirection.Location = new System.Drawing.Point(63, 43);
            this.cmbTileDirection.Name = "cmbTileDirection";
            this.cmbTileDirection.Size = new System.Drawing.Size(152, 21);
            this.cmbTileDirection.TabIndex = 14;
            this.cmbTileDirection.SelectedIndexChanged += new System.EventHandler(this.cmbTileDirection_SelectedIndexChanged);
            // 
            // numMaxTiles
            // 
            this.numMaxTiles.Enabled = false;
            this.numMaxTiles.Location = new System.Drawing.Point(152, 74);
            this.numMaxTiles.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numMaxTiles.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.numMaxTiles.Name = "numMaxTiles";
            this.numMaxTiles.Size = new System.Drawing.Size(63, 21);
            this.numMaxTiles.TabIndex = 15;
            this.numMaxTiles.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.numMaxTiles.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numMaxTiles.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // lblY
            // 
            this.lblY.AutoSize = true;
            this.lblY.Location = new System.Drawing.Point(151, 102);
            this.lblY.Name = "lblY";
            this.lblY.Size = new System.Drawing.Size(13, 13);
            this.lblY.TabIndex = 16;
            this.lblY.Text = "Y";
            // 
            // lblMaxTiles
            // 
            this.lblMaxTiles.AutoSize = true;
            this.lblMaxTiles.Location = new System.Drawing.Point(63, 76);
            this.lblMaxTiles.Name = "lblMaxTiles";
            this.lblMaxTiles.Size = new System.Drawing.Size(79, 13);
            this.lblMaxTiles.TabIndex = 15;
            this.lblMaxTiles.Text = "Number of tiles";
            // 
            // lblX
            // 
            this.lblX.AutoSize = true;
            this.lblX.Location = new System.Drawing.Point(63, 102);
            this.lblX.Name = "lblX";
            this.lblX.Size = new System.Drawing.Size(13, 13);
            this.lblX.TabIndex = 16;
            this.lblX.Text = "X";
            // 
            // lblTileDirection
            // 
            this.lblTileDirection.AutoSize = true;
            this.lblTileDirection.Location = new System.Drawing.Point(10, 46);
            this.lblTileDirection.Name = "lblTileDirection";
            this.lblTileDirection.Size = new System.Drawing.Size(49, 13);
            this.lblTileDirection.TabIndex = 14;
            this.lblTileDirection.Text = "Direction";
            // 
            // lblMargin
            // 
            this.lblMargin.AutoSize = true;
            this.lblMargin.Location = new System.Drawing.Point(18, 178);
            this.lblMargin.Name = "lblMargin";
            this.lblMargin.Size = new System.Drawing.Size(39, 13);
            this.lblMargin.TabIndex = 20;
            this.lblMargin.Text = "Margin";
            // 
            // numMarginX
            // 
            this.numMarginX.Enabled = false;
            this.numMarginX.Location = new System.Drawing.Point(63, 176);
            this.numMarginX.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numMarginX.Name = "numMarginX";
            this.numMarginX.Size = new System.Drawing.Size(62, 21);
            this.numMarginX.TabIndex = 20;
            this.numMarginX.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numMarginX.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // numMarginY
            // 
            this.numMarginY.Enabled = false;
            this.numMarginY.Location = new System.Drawing.Point(152, 175);
            this.numMarginY.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numMarginY.Name = "numMarginY";
            this.numMarginY.Size = new System.Drawing.Size(62, 21);
            this.numMarginY.TabIndex = 21;
            this.numMarginY.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numMarginY.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // lblSize
            // 
            this.lblSize.AutoSize = true;
            this.lblSize.Location = new System.Drawing.Point(31, 150);
            this.lblSize.Name = "lblSize";
            this.lblSize.Size = new System.Drawing.Size(26, 13);
            this.lblSize.TabIndex = 18;
            this.lblSize.Text = "Size";
            // 
            // lblOffset
            // 
            this.lblOffset.AutoSize = true;
            this.lblOffset.Location = new System.Drawing.Point(20, 123);
            this.lblOffset.Name = "lblOffset";
            this.lblOffset.Size = new System.Drawing.Size(38, 13);
            this.lblOffset.TabIndex = 16;
            this.lblOffset.Text = "Offset";
            // 
            // numSizeY
            // 
            this.numSizeY.Enabled = false;
            this.numSizeY.Location = new System.Drawing.Point(152, 148);
            this.numSizeY.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numSizeY.Name = "numSizeY";
            this.numSizeY.Size = new System.Drawing.Size(62, 21);
            this.numSizeY.TabIndex = 19;
            this.numSizeY.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numSizeY.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // numSizeX
            // 
            this.numSizeX.Enabled = false;
            this.numSizeX.Location = new System.Drawing.Point(64, 148);
            this.numSizeX.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numSizeX.Name = "numSizeX";
            this.numSizeX.Size = new System.Drawing.Size(62, 21);
            this.numSizeX.TabIndex = 18;
            this.numSizeX.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numSizeX.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // numOffsetX
            // 
            this.numOffsetX.Enabled = false;
            this.numOffsetX.Location = new System.Drawing.Point(64, 121);
            this.numOffsetX.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numOffsetX.Name = "numOffsetX";
            this.numOffsetX.Size = new System.Drawing.Size(62, 21);
            this.numOffsetX.TabIndex = 16;
            this.numOffsetX.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numOffsetX.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // numOffsetY
            // 
            this.numOffsetY.Enabled = false;
            this.numOffsetY.Location = new System.Drawing.Point(152, 121);
            this.numOffsetY.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numOffsetY.Name = "numOffsetY";
            this.numOffsetY.Size = new System.Drawing.Size(62, 21);
            this.numOffsetY.TabIndex = 17;
            this.numOffsetY.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numOffsetY.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // previewPanel
            // 
            this.previewPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.previewPanel.AutoScroll = true;
            this.previewPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.previewPanel.Location = new System.Drawing.Point(245, 66);
            this.previewPanel.Name = "previewPanel";
            this.previewPanel.Size = new System.Drawing.Size(570, 533);
            this.previewPanel.TabIndex = 25;
            this.previewPanel.Scroll += new System.Windows.Forms.ScrollEventHandler(this.previewPanel_Scroll);
            this.previewPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.previewPanel_Paint);
            this.previewPanel.MouseDown += new System.Windows.Forms.MouseEventHandler(this.previewPanel_MouseDown);
            this.previewPanel.MouseMove += new System.Windows.Forms.MouseEventHandler(this.previewPanel_MouseMove);
            this.previewPanel.MouseUp += new System.Windows.Forms.MouseEventHandler(this.previewPanel_MouseUp);
            // 
            // btnImportAll
            // 
            this.btnImportAll.Location = new System.Drawing.Point(166, 569);
            this.btnImportAll.Name = "btnImportAll";
            this.btnImportAll.Size = new System.Drawing.Size(70, 30);
            this.btnImportAll.TabIndex = 1;
            this.btnImportAll.Text = "Import All";
            this.btnImportAll.UseVisualStyleBackColor = true;
            this.btnImportAll.Click += new System.EventHandler(this.btnImportAll_Click);
            // 
            // SpriteImportWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnClose;
            this.ClientSize = new System.Drawing.Size(832, 618);
            this.Controls.Add(this.lblZoom);
            this.Controls.Add(this.zoomSlider);
            this.Controls.Add(this.btnImportAll);
            this.Controls.Add(this.groupSelection);
            this.Controls.Add(this.btnImport);
            this.Controls.Add(this.groupTransColour);
            this.Controls.Add(this.lblImageDescription);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.cmbFilenames);
            this.Controls.Add(this.groupImportOptions);
            this.Controls.Add(this.previewPanel);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(840, 650);
            this.Name = "SpriteImportWindow";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Import Sprite";
            this.groupImportOptions.ResumeLayout(false);
            this.groupImportOptions.PerformLayout();
            this.groupTransColour.ResumeLayout(false);
            this.groupTransColour.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.udTransColorIndex)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.zoomSlider)).EndInit();
            this.groupSelection.ResumeLayout(false);
            this.groupSelection.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numMaxTiles)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numMarginX)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numMarginY)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numSizeY)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numSizeX)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numOffsetX)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numOffsetY)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private BufferedPanel previewPanel;
        private System.Windows.Forms.GroupBox groupImportOptions;
        private System.Windows.Forms.Label lblImageDescription;
        private System.Windows.Forms.CheckBox chkRoomBackground;
        private System.Windows.Forms.CheckBox chkRemapCols;
        private System.Windows.Forms.CheckBox chkTiled;
        private System.Windows.Forms.TrackBar zoomSlider;
        private System.Windows.Forms.Label lblZoom;
        private System.Windows.Forms.Button btnImport;
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.ComboBox cmbFilenames;
        private System.Windows.Forms.CheckBox chkUseAlphaChannel;
        private System.Windows.Forms.GroupBox groupTransColour;
        private System.Windows.Forms.RadioButton radTransColourNone;
        private System.Windows.Forms.RadioButton radTransColourLeaveAsIs;
        private System.Windows.Forms.RadioButton radTransColourBottomRightPixel;
        private System.Windows.Forms.RadioButton radTransColourTopRightPixel;
        private System.Windows.Forms.RadioButton radTransColourBottomLeftPixel;
        private System.Windows.Forms.RadioButton radTransColourTopLeftPixel;
        private System.Windows.Forms.RadioButton radTransColourIndex;
        private System.Windows.Forms.Panel panelBottomRight;
        private System.Windows.Forms.Panel panelBottomLeft;
        private System.Windows.Forms.Panel panelIndex0;
        private System.Windows.Forms.Panel panelTopLeft;
        private System.Windows.Forms.GroupBox groupSelection;
        private System.Windows.Forms.NumericUpDown numSizeY;
        private System.Windows.Forms.NumericUpDown numSizeX;
        private System.Windows.Forms.NumericUpDown numOffsetX;
        private System.Windows.Forms.NumericUpDown numOffsetY;
        private System.Windows.Forms.Label lblTileDirection;
        private System.Windows.Forms.ComboBox cmbTileDirection;
        private System.Windows.Forms.Label lblMaxTiles;
        private System.Windows.Forms.NumericUpDown numMaxTiles;
        private System.Windows.Forms.Label lblSize;
        private System.Windows.Forms.Label lblOffset;
        private System.Windows.Forms.Label lblMargin;
        private System.Windows.Forms.NumericUpDown numMarginX;
        private System.Windows.Forms.NumericUpDown numMarginY;
        private System.Windows.Forms.Label lblX;
        private System.Windows.Forms.Label lblY;
        private System.Windows.Forms.Button btnImportAll;
        private System.Windows.Forms.Panel panelTopRight;
        private System.Windows.Forms.NumericUpDown udTransColorIndex;
    }
}