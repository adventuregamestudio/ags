using System;

namespace AGS.Editor
{
    /// <summary>
    /// Class describes design-time properties of a room entity.
    /// </summary>
    public class DesignTimeProperties
    {
        /// <summary>
        /// Tells if the entity should be visible in the editor.
        /// </summary>
        public bool Visible { get; set; }
        /// <summary>
        /// Tells if the entity should be locked in the editor (preventing it to be moved around).
        /// </summary>
        public bool Locked { get; set; }

        public DesignTimeProperties()
        {
            Visible = true;
            Locked = false;
        }

        public DesignTimeProperties(bool visible, bool locked)
        {
            Visible = visible;
            Locked = locked;
        }
    }
}
