using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor.Panes.Room
{
    /// <summary>
    /// Since currently we can't have more than one areas filter visible,
    /// we need the visible buttons to be part of a radio group.
    /// </summary>
    public class VisibleLayerRadioGroup
    {
        List<RoomNodeControl> _controls;

        public VisibleLayerRadioGroup()
        {
            _controls = new List<RoomNodeControl>();
        }

        public void Register(RoomNodeControl control)
        {
            _controls.Add(control);
            control.OnIsVisibleChanged += (sender, eventArgs) => 
            {
                if (control.IsVisible)
                {
                    foreach (RoomNodeControl controlToHide in _controls)
                    {
                        if (controlToHide == control) continue;
                        controlToHide.IsVisible = false;
                    }
                }
            };
        }
    }
}
