using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public partial class CustomResolutionDialog : Form
    {
        private static List<Size> _sizePresets = new List<Size>();

        private int _customItemIndex = -1;
        private bool _updatingControls = false;

        public delegate string SizeToStringDelegate(Size size);
        public delegate Size StringToSizeDelegate(string s);
        public static SizeToStringDelegate SizeToString = new SizeToStringDelegate(Types.Utilities.ResolutionToUserString);
        public static StringToSizeDelegate StringToSize = new StringToSizeDelegate(Types.Utilities.UserStringToResolution);

        static CustomResolutionDialog()
        {
            Size[] def_resolutions = new Size[]
            {
                new Size (320, 200),
                new Size (320, 240),
                new Size (640, 360),
                new Size (640, 400),
                new Size (640, 480),
                new Size (800, 600),
                new Size (1024, 768),
                new Size (1280, 720),
                new Size (1280, 800),
                new Size (1280, 1024),
                new Size (1440, 900),
                new Size (1600, 900),
                new Size (1600, 1200),
                new Size (1680, 1050),
                new Size (1920, 1080),
                new Size (1920, 1200)
            };

            _sizePresets.AddRange(def_resolutions);
        }

        public static Size Show(Size initialSize)
        {
            using (CustomResolutionDialog dialog = new CustomResolutionDialog(initialSize))
            {
                if (dialog.ShowDialog() == DialogResult.OK)
                    return dialog.Value;
                return initialSize;
            }
        }

        public CustomResolutionDialog(Size initialSize)
        {
            InitializeComponent();
            foreach (Size preset in _sizePresets)
            {
                cbResolutionPreset.Items.Add(SizeToString(preset));
            }
            Value = initialSize;
        }

        public Size Value
        {
            get { return new Size((int)udWidth.Value, (int)udHeight.Value); }
            set
            {
                _updatingControls = true;
                udWidth.Value = value.Width;
                udHeight.Value = value.Height;
                _updatingControls = false;
                SelectPresetIfExists();
            }
        }

        private void EnableCustomItem(bool enable)
        {
            if (enable)
            {
                if (_customItemIndex < 0)
                    _customItemIndex = cbResolutionPreset.Items.Add("Custom resolution");
            }
            else
            {
                if (_customItemIndex >= 0)
                {
                    cbResolutionPreset.Items.RemoveAt(_customItemIndex);
                    _customItemIndex = -1;
                }
            }
        }

        private void SelectPresetIfExists()
        {
            _updatingControls = true;
            Size wanted_size = new Size((int)udWidth.Value, (int)udHeight.Value);
            int preset_index = _sizePresets.IndexOf(wanted_size);
            if (preset_index >= 0)
            {
                cbResolutionPreset.SelectedIndex = preset_index;
                EnableCustomItem(false);
            }
            else
            {
                EnableCustomItem(true);
                cbResolutionPreset.SelectedIndex = _customItemIndex;
            }
            _updatingControls = false;
        }

        private void cbResolutionPreset_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (_updatingControls)
                return;
            if (cbResolutionPreset.SelectedIndex == _customItemIndex)
                return;

            _updatingControls = true;
            EnableCustomItem(false);
            Size preset = _sizePresets[cbResolutionPreset.SelectedIndex];
            udWidth.Value = preset.Width;
            udHeight.Value = preset.Height;
            _updatingControls = false;
        }

        private void udWidthHeight_ValueChanged(object sender, EventArgs e)
        {
            if (_updatingControls)
                return;
            SelectPresetIfExists();
        }

        private void udWidthHeight_Validating(object sender, CancelEventArgs e)
        {
            NumericUpDown num = (NumericUpDown)sender;
            num.Value = Math.Max(1, num.Value);
        }
    }
}
