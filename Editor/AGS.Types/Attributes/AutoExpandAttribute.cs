using System;
using System.ComponentModel;

namespace AGS.Types
{
    /// <summary>
    /// Marks the property, or the class of the property, as one that
    /// has to be automatically expanded whenever the object, containing such
    /// property, is selected on the property grid.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Property)]
    public class AutoExpandAttribute : Attribute
    {
    }
}
