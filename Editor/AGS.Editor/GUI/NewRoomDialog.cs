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
	public partial class NewRoomDialog : Form
	{
		private Game _game;
		private ImageList _imageList = new ImageList();
		private int _chosenRoomNumber;
		private List<RoomTemplate> _templates;

		internal NewRoomDialog(Game game, List<RoomTemplate> templates)
		{
			InitializeComponent();
			_templates = templates;
			_imageList.ImageSize = new Size(32, 32);
			_imageList.Images.Add("_default", Resources.ResourceManager.GetIcon("template_noicon.ico"));
			foreach (RoomTemplate template in templates)
			{
				ListViewItem newItem = lstRoomTemplates.Items.Add(template.FriendlyName);
				if (template.Icon != null)
				{
					_imageList.Images.Add(template.FileName, template.Icon);
					newItem.ImageKey = template.FileName;
				}
				else
				{
					newItem.ImageKey = "_default";
				}
			}
			lstRoomTemplates.LargeImageList = _imageList;
			lstRoomTemplates.Items[0].Selected = true;
			lstRoomTemplates.Items[0].Focused = true;
			_game = game;
			_chosenRoomNumber = -1;
			PickFirstAvailableRoomNumber();
		}

		internal int ChosenRoomNumber
		{
			get { return _chosenRoomNumber; }
		}

		internal RoomTemplate ChosenTemplate
		{
			get { return _templates[lstRoomTemplates.SelectedIndices[0]]; }
		}

		private void PickFirstAvailableRoomNumber()
		{
			int startingRoomNumber;
			if (chkNonStateSaving.Checked)
			{
				startingRoomNumber = UnloadedRoom.NON_STATE_SAVING_INDEX;
				udRoomNumber.Minimum = UnloadedRoom.NON_STATE_SAVING_INDEX + 1;
				udRoomNumber.Maximum = UnloadedRoom.HIGHEST_ROOM_NUMBER_ALLOWED;
			}
			else
			{
				startingRoomNumber = 0;
				udRoomNumber.Minimum = 0;
				udRoomNumber.Maximum = UnloadedRoom.NON_STATE_SAVING_INDEX;
			}
			int newNumber = _game.FindFirstAvailableRoomNumber(startingRoomNumber);
			if (newNumber > udRoomNumber.Maximum)
			{
				MessageBox.Show("There are no more room numbers available for this type of room. Consider changing some of your rooms to be non state saving.", "Room limit exceeded", MessageBoxButtons.OK, MessageBoxIcon.Warning);
			}
			else
			{
				udRoomNumber.Value = newNumber;
			}
		}

		private void chkNonStateSaving_CheckedChanged(object sender, EventArgs e)
		{
			PickFirstAvailableRoomNumber();
		}

		private void btnOk_Click(object sender, EventArgs e)
		{
			if (_game.DoesRoomNumberAlreadyExist((int)udRoomNumber.Value))
			{
				MessageBox.Show("You already have a room number " + udRoomNumber.Value + ". Please choose another number.", "Room already exists", MessageBoxButtons.OK, MessageBoxIcon.Warning);
				PickFirstAvailableRoomNumber();
				this.DialogResult = DialogResult.None;
				return;
			}
			if (lstRoomTemplates.SelectedIndices.Count != 1)
			{
				MessageBox.Show("You must select a template.", "Select template", MessageBoxButtons.OK, MessageBoxIcon.Warning);
				this.DialogResult = DialogResult.None;
				return;
			}
			_chosenRoomNumber = (int)udRoomNumber.Value;
			_imageList.Dispose();
			this.DialogResult = DialogResult.OK;
			this.Close();
		}

		private void btnCancel_Click(object sender, EventArgs e)
		{
			this.DialogResult = DialogResult.Cancel;
			_imageList.Dispose();
			this.Close();
		}
	}
}