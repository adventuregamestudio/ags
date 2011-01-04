using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class SpriteChooser : Form
    {
		private const int NO_SPRITE_ID = 0;

		private static int WindowWidth = 0, WindowHeight = 0;
		private static bool WindowMaximised = false;

        private Sprite _selectedSprite;
        private int _startingSpriteNumber;

        public SpriteChooser(int existingSprite)
        {
            InitializeComponent();
			this.Icon = Factory.GUIController.StandardEditorIcon;

            btnOK.Enabled = false;
            spriteSelector1.SetDataSource(Factory.AGSEditor.CurrentGame.RootSpriteFolder);
            spriteSelector1.ShowUseThisSpriteOption = true;
			spriteSelector1.SendUpdateNotifications = true;
            if (existingSprite > 0) 
            {
                spriteSelector1.OpenFolderForSprite(existingSprite);
            }
            _startingSpriteNumber = existingSprite;
        }

        public Sprite SelectedSprite
        {
            get { return _selectedSprite; }
        }

        public static Sprite ShowSpriteChooser(int currentSprite)
        {
            Sprite selectedSprite = null;
            SpriteChooser chooser = new SpriteChooser(currentSprite);
            if (chooser.ShowDialog() == DialogResult.OK)
            {
                selectedSprite = chooser.SelectedSprite;
            }
            chooser.Dispose();
            return selectedSprite;
        }

        private void spriteSelector1_OnSpriteActivated(Sprite activatedSprite)
        {
            btnOK_Click(null, null);
        }

		private void SaveWindowSizeAndClose()
		{
			if (this.WindowState == FormWindowState.Maximized)
			{
				WindowMaximised = true;
			}
			else
			{
				WindowMaximised = false;
				WindowWidth = this.Width;
				WindowHeight = this.Height;
			}
			this.DialogResult = DialogResult.OK;
			this.Close();
		}

        private void btnOK_Click(object sender, EventArgs e)
        {
            _selectedSprite = spriteSelector1.SelectedSprite;
            if (_selectedSprite != null)
            {
				SaveWindowSizeAndClose();
            }
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void spriteSelector1_OnSelectionChanged(Sprite[] newSelection)
        {
            if (newSelection.Length == 1)
            {
                btnOK.Enabled = true;
            }
            else
            {
                btnOK.Enabled = false;
            }
        }

        private void SpriteChooser_Load(object sender, EventArgs e)
        {
			if (WindowWidth > 0)
			{
				this.Size = new Size(WindowWidth, WindowHeight);
				this.CenterToParent();
			}
			if (WindowMaximised)
			{
				this.WindowState = FormWindowState.Maximized;
			}

			Cursor.Current = Cursors.WaitCursor;

            spriteSelector1.SelectSprite(_startingSpriteNumber);

			Cursor.Current = Cursors.Default;
		}

		private void btnUseNoSprite_Click(object sender, EventArgs e)
		{
			_selectedSprite = Factory.AGSEditor.CurrentGame.RootSpriteFolder.FindSpriteByID(NO_SPRITE_ID, true);
			SaveWindowSizeAndClose();
		}

		private void SpriteChooser_Resize(object sender, EventArgs e)
		{
			spriteSelector1.Height = btnOK.Top - 10;

			if (this.Height < 300)
			{
				this.Height = 300;
			}
			if (this.Width < 300)
			{
				this.Width = 300;
			}
		}
    }
}