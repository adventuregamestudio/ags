using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class ZoomTrackbar : UserControl
    {
        private const int DEFAULT_ZOOM_STEP_VALUE = 25;
        private const int DEFAULT_ZOOM_MAX_VALUE = 600;
        private const int DEFAULT_ZOOM_MIN_VALUE = DEFAULT_ZOOM_STEP_VALUE;
        private const int DEFAULT_ZOOM_INITIAL_VALUE = 100;

        private int _zoom_max_value = DEFAULT_ZOOM_MAX_VALUE;
        private int _zoom_step_value = DEFAULT_ZOOM_STEP_VALUE;
        private int _zoom_min_value = DEFAULT_ZOOM_MIN_VALUE;
        private float _zoom_value = DEFAULT_ZOOM_INITIAL_VALUE;

        public event EventHandler ValueChanged;

        private void RaiseEventValueChanged(EventArgs e)
        {
            // Make a temporary copy of the event to avoid possibility of
            // a race condition if the last subscriber unsubscribes
            // immediately after the null check and before the event is raised.
            EventHandler raiseEvent = ValueChanged;

            // Event will be null if there are no subscribers
            if (raiseEvent != null)
            {
                // Call to raise the event.
                raiseEvent(this, e);
            }
        }
        protected virtual void OnValueChanged(EventArgs e)
        {
            RaiseEventValueChanged(e);
        }

        private void _set_trackbar_limits()
        {
            sldZoomLevel.Maximum = _zoom_max_value / _zoom_step_value;
            sldZoomLevel.Minimum = _zoom_min_value / _zoom_step_value;
            sldZoomLevel.Value = (int)_zoom_value / _zoom_step_value;
        }

        private void SetZoomValue(float value)
        {
            _zoom_value = MathExtra.Clamp(value, _zoom_min_value, _zoom_max_value); ;
            sldZoomLevel.Value = (int) (_zoom_value / _zoom_step_value);
            textBoxZoomValue.Text = Convert.ToInt32((int)_zoom_value).ToString() + "%";
            RaiseEventValueChanged(new EventArgs());
        }

        public ZoomTrackbar()
        {
            InitializeComponent();
            _set_trackbar_limits();
        }

        private void textBoxZoomValue_Validating(object sender, CancelEventArgs e)
        {
           string text = textBoxZoomValue.Text.Replace("%", string.Empty).Replace(" ", string.Empty);
            int value = 0;
            try
            {
                value = Convert.ToInt32(text);
            } 
            catch
            {
                value = 100;
            }
            value = MathExtra.Clamp(value / _zoom_step_value, sldZoomLevel.Minimum, sldZoomLevel.Maximum);
            textBoxZoomValue.Text = (value * _zoom_step_value).ToString();
        }

        private void textBoxZoomValue_Validated(object sender, EventArgs e)
        {
            SetZoomValue(Convert.ToInt32(textBoxZoomValue.Text));
        }

        private void sldZoomLevel_ValueChanged(object sender, EventArgs e)
        {
            SetZoomValue(sldZoomLevel.Value * _zoom_step_value);
        }


        public void Increase()
        {
            if (sldZoomLevel.Value < sldZoomLevel.Maximum)
            {
                sldZoomLevel.Value++;
            }
        }

        public void Decrease()
        {
            if (sldZoomLevel.Value > sldZoomLevel.Minimum)
            {
                sldZoomLevel.Value--;
            }
        }

        public int Maximum
        {
            get
            {
                return _zoom_max_value;
            }
            set
            {
                _zoom_max_value = value;
                sldZoomLevel.Maximum = _zoom_max_value / _zoom_step_value;
            }
        }

        public int Minimum
        {
            get
            {
                return _zoom_min_value;
            }
            set
            {
                _zoom_min_value = value;
                sldZoomLevel.Minimum = _zoom_min_value / _zoom_step_value;
            }
        }

        public int Value
        {
            get
            {
                return sldZoomLevel.Value * _zoom_step_value;
            }
            set
            {
                SetZoomValue(value);
            }
        }

        public float ZoomScale
        {
            get
            {
                return _zoom_value * 0.01f;
            }
            set
            {
                SetZoomValue(value * 100f);
            }
        }

        public int Step
        {
            get
            {
                return _zoom_step_value;
            }
            set
            {
                _zoom_step_value = value;
                _set_trackbar_limits();
            }
        }

        private void textBoxZoomValue_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (char)Keys.Return)
            {
                textBoxZoomValue_Validating(this, new CancelEventArgs());
                textBoxZoomValue_Validated(this, new CancelEventArgs());
            }
        }
    }
}
