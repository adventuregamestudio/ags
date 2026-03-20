using System;
using System.ComponentModel;

namespace AGS.Types
{
    [TypeConverter(typeof(GraphicAnchorTypeConverter))]
    public struct GraphicAnchor
    {
        private FrameAlignment _type;
        private float _x;
        private float _y;

        public GraphicAnchor(FrameAlignment type)
        {
            _type = type;
            _x = 0F;
            _y = 0F;
            UpdateXY();
        }

        public GraphicAnchor(float x, float y)
        {
            _type = FrameAlignment.None;
            _x = Math.Max(0F, Math.Min(1F, x));
            _y = Math.Max(0F, Math.Min(1F, y));
            UpdateType();
        }

        [TypeConverter(typeof(EnumTypeConverter))]
        public FrameAlignment Type
        {
            get { return _type; }
            set
            {
                if (value == FrameAlignment.None)
                {
                    // "None" type must not be set directly, so recover a
                    // proper type from the X, Y properties
                    UpdateType();
                }
                else
                {
                    _type = value;
                    UpdateXY();
                }
                
            }
        }

        public float X
        {
            get { return _x; }
            set
            {
                _x = Math.Max(0F, Math.Min(1F, value));
                UpdateType();
            }
        }

        public float Y
        {
            get { return _y; }
            set
            {
                _y = Math.Max(0F, Math.Min(1F, value));
                UpdateType();
            }
        }

        private void UpdateType()
        {
            if ((_x == 0F) && (_y == 0F))
                _type = FrameAlignment.TopLeft;
            else if ((_x == .5F) && (_y == 0F))
                _type = FrameAlignment.TopCenter;
            else if ((_x == 1F) && (_y == 0F))
                _type = FrameAlignment.TopRight;
            else if ((_x == 0F) && (_y == .5F))
                _type = FrameAlignment.MiddleLeft;
            else if ((_x == .5F) && (_y == .5F))
                _type = FrameAlignment.MiddleCenter;
            else if ((_x == 1F) && (_y == .5F))
                _type = FrameAlignment.MiddleRight;
            else if ((_x == 0F) && (_y == 1F))
                _type = FrameAlignment.BottomLeft;
            else if ((_x == .5F) && (_y == 1F))
                _type = FrameAlignment.BottomCenter;
            else if ((_x == 1F) && (_y == 1F))
                _type = FrameAlignment.BottomRight;
            else
                _type = FrameAlignment.None;
        }

        private void UpdateXY()
        {
            switch (_type)
            {
                case FrameAlignment.TopLeft:
                    _x = 0F; _y = 0F; break;
                case FrameAlignment.TopCenter:
                    _x = .5F; _y = 0F; break;
                case FrameAlignment.TopRight:
                    _x = 1F; _y = 0F; break;
                case FrameAlignment.MiddleLeft:
                    _x = 0F; _y = .5F; break;
                case FrameAlignment.MiddleCenter:
                    _x = .5F; _y = .5F; break;
                case FrameAlignment.MiddleRight:
                    _x = 1F; _y = .5F; break;
                case FrameAlignment.BottomLeft:
                    _x = 0F; _y = 1F; break;
                case FrameAlignment.BottomCenter:
                    _x = .5F; _y = 1F; break;
                case FrameAlignment.BottomRight:
                    _x = 1F; _y = 1F; break;
                default: break;
            }
        }
    }
}
