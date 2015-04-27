using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;

namespace AGS.Types
{
    [PropertyTab(typeof(PropertyTabInteractions), PropertyTabScope.Component)]
    [DefaultProperty("Description")]
    public class RoomHotspot : IChangeNotification
	{
		public const string PROPERTY_NAME_SCRIPT_NAME = "Name";
		private const int MAX_NAME_LENGTH = 19;

        private static InteractionSchema _interactionSchema;

        private int _id;
        private string _name = string.Empty;
        private string _description = string.Empty;
        private Point _walkToPoint;
        private CustomProperties _properties = new CustomProperties();
        private Interactions _interactions = new Interactions(_interactionSchema);
		private IChangeNotification _notifyOfModification;

        static RoomHotspot()
        {
            _interactionSchema = new InteractionSchema(new string[] {"Stands on hotspot",
                "$$01 hotspot","$$02 hotspot","Use inventory on hotspot",
                "$$03 hotspot", "Any click on hotspot","Mouse moves over hotspot", 
                "$$05 hotspot", "$$08 hotspot", "$$09 hotspot"},
                new string[] { "WalkOn", "Look", "Interact", "UseInv", "Talk", "AnyClick", "MouseMove", "PickUp", "Mode8", "Mode9" });
        }

		public RoomHotspot(IChangeNotification changeNotifier)
		{
			_notifyOfModification = changeNotifier;
		}

        [Description("The ID number of the hotspot")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("Description of the hotspot")]
        [Category("Appearance")]
        public string Description
        {
            get { return _description; }
            set { _description = value; }
        }

		[DisplayName(PROPERTY_NAME_SCRIPT_NAME)]
		[Description("The script name of the hotspot")]
        [Category("Design")]
        public string Name
        {
            get { return _name; }
            set { _name = Utilities.ValidateScriptName(value, MAX_NAME_LENGTH); }
        }

        [Description("The player will walk to this spot when the hotspot is activated")]
        [Category("Design")]
        public Point WalkToPoint
        {
            get { return _walkToPoint; }
            set { _walkToPoint = value; }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return _name + " (Hotspot; ID " + _id + ")"; }
        }

        [AGSSerializeClass()]
        [Description("Custom properties for this hotspot")]
        [Category("Properties")]
        [EditorAttribute(typeof(CustomPropertiesUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public CustomProperties Properties
        {
            get { return _properties; }
            protected set { _properties = value; }
        }

        [AGSNoSerialize()]
        [Browsable(false)]
        public Interactions Interactions
        {
            get { return _interactions; }
        }

		void IChangeNotification.ItemModified()
		{
			_notifyOfModification.ItemModified();
		}
	}
}
