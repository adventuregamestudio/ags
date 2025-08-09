using System;

namespace AGS.Types
{
    /// <summary>
    /// BrowsableMultiedit attribute defines whether the property should be
    /// displayed in the PropertyGrid when the multiple objects are edited at once.
    /// Default is true (BrowsableMultieditAttribute.Yes).
    /// Setting [BrowsableMultiedit(false)] will make properties hidden
    /// in case multiple objects are selected for edit.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property)]
    public class BrowsableMultieditAttribute : Attribute
    {
        public static BrowsableMultieditAttribute Yes = new BrowsableMultieditAttribute(true);
        public static BrowsableMultieditAttribute No = new BrowsableMultieditAttribute(false);
        public static BrowsableMultieditAttribute Default = Yes;

        private bool _isBrowseableWhenMultiedit;

        public BrowsableMultieditAttribute(bool isBrowseableWhenMultiedit)
        {
            _isBrowseableWhenMultiedit = isBrowseableWhenMultiedit;
        }

        public bool Browsable { get { return _isBrowseableWhenMultiedit; } }

        public override bool Equals(object obj)
        {
            var otherAttr = obj as BrowsableMultieditAttribute;
            return otherAttr != null && otherAttr.Browsable == Browsable;
        }

        public override int GetHashCode()
        {
            return _isBrowseableWhenMultiedit.GetHashCode();
        }

        public override bool IsDefaultAttribute()
        {
            return _isBrowseableWhenMultiedit == true;
        }
    }
}
