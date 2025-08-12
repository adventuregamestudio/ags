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
        // First (lowest) room number to suggest to the user
        private const int FIRST_SUGGESTED_ROOM_NUMBER = 1;

        private Game _game;
		private ImageList _imageList = new ImageList();
        private int _startingRoomNumber;
		private int _chosenRoomNumber;
		private List<RoomTemplate> _templates;

        /// <summary>
        /// Sets up a NewRoomDialog with full range of options, including a template selection.
        /// </summary>
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
            _startingRoomNumber = -1;
            _chosenRoomNumber = -1;
			PickFirstAvailableRoomNumber();
		}

        /// <summary>
        /// Sets up a NewRoomDialog without template selection, where
        /// user may only choose a new number for the existing room.
        /// TODO: perhaps move shared controls to a UserControl,
        /// and create a second dialog that does not have templates.
        /// </summary>
        internal NewRoomDialog(Game game, int existingRoomNumber)
        {
            InitializeComponent();

            this.Text = "Assign new Room Number";
            labelChooseRoomType.Text = "Please choose the new room number:";
            lstRoomTemplates.Visible = false;
            this.Height -= lstRoomTemplates.Height;

            _game = game;
            _startingRoomNumber = existingRoomNumber;
            _chosenRoomNumber = -1;
            chkNonStateSaving.Checked = _startingRoomNumber >= UnloadedRoom.NON_STATE_SAVING_INDEX;
            UpdateAvailableRoomNumber();
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
				udRoomNumber.Minimum = UnloadedRoom.NON_STATE_SAVING_INDEX;
				udRoomNumber.Maximum = UnloadedRoom.HIGHEST_ROOM_NUMBER_ALLOWED;
			}
			else
			{
                startingRoomNumber = FIRST_SUGGESTED_ROOM_NUMBER; //UnloadedRoom.FIRST_ROOM_NUMBER;
				udRoomNumber.Minimum = UnloadedRoom.FIRST_ROOM_NUMBER;
				udRoomNumber.Maximum = UnloadedRoom.NON_STATE_SAVING_INDEX - 1;
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

        private void UpdateAvailableRoomNumber()
        {
            int roomNumber;
            if (chkNonStateSaving.Checked)
            {
                roomNumber = _startingRoomNumber >= UnloadedRoom.NON_STATE_SAVING_INDEX ?
                    _startingRoomNumber : UnloadedRoom.NON_STATE_SAVING_INDEX;
                udRoomNumber.Minimum = UnloadedRoom.NON_STATE_SAVING_INDEX;
                udRoomNumber.Maximum = UnloadedRoom.HIGHEST_ROOM_NUMBER_ALLOWED;
            }
            else
            {
                roomNumber = _startingRoomNumber < UnloadedRoom.NON_STATE_SAVING_INDEX ?
                    _startingRoomNumber : FIRST_SUGGESTED_ROOM_NUMBER;//UnloadedRoom.FIRST_ROOM_NUMBER;
                udRoomNumber.Minimum = UnloadedRoom.FIRST_ROOM_NUMBER;
                udRoomNumber.Maximum = UnloadedRoom.NON_STATE_SAVING_INDEX - 1;
            }

            if (roomNumber != _startingRoomNumber)
            {
                roomNumber = _game.FindFirstAvailableRoomNumber(roomNumber);
                if (roomNumber > udRoomNumber.Maximum)
                {
                    MessageBox.Show("There are no more room numbers available for this type of room. Consider changing some of your rooms to be non state saving.", "Room limit exceeded", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }
            }
            udRoomNumber.Value = roomNumber;
        }

		private void chkNonStateSaving_CheckedChanged(object sender, EventArgs e)
		{
            if (_startingRoomNumber >= 0)
                UpdateAvailableRoomNumber();
            else
			    PickFirstAvailableRoomNumber();
		}

		private void btnOk_Click(object sender, EventArgs e)
		{
            int wantRoomNumber = (int)udRoomNumber.Value;
			if (wantRoomNumber != _startingRoomNumber &&
                _game.DoesRoomNumberAlreadyExist(wantRoomNumber))
			{
				MessageBox.Show("You already have a room number " + wantRoomNumber + ". Please choose another number.", "Room already exists", MessageBoxButtons.OK, MessageBoxIcon.Warning);
				PickFirstAvailableRoomNumber();
				this.DialogResult = DialogResult.None;
				return;
			}
			if (_startingRoomNumber < 0 &&
                lstRoomTemplates.SelectedIndices.Count != 1)
			{
				MessageBox.Show("You must select a template.", "Select template", MessageBoxButtons.OK, MessageBoxIcon.Warning);
				this.DialogResult = DialogResult.None;
				return;
			}
			_chosenRoomNumber = wantRoomNumber;
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