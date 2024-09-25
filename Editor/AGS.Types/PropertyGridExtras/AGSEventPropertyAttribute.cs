using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// AGSEventsTabProperty attribute marks a property that belong
    /// to the Events tab on the Property Grid pane.
    /// This attribute is meant for generic properties, not necessarily
    /// a object's event.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property)]
    public class AGSEventsTabPropertyAttribute : Attribute
    {
        public AGSEventsTabPropertyAttribute()
        {
        }
    }

    /// <summary>
    /// AGSEventProperty attribute marks a property that defines
    /// object's event, possible to attach a script function to.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property)]
    public class AGSEventPropertyAttribute : AGSEventsTabPropertyAttribute
    {
        public AGSEventPropertyAttribute()
        {
        }
    }
}
