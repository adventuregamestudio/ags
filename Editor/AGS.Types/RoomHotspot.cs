using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [PropertyTab(typeof(PropertyTabInteractions), PropertyTabScope.Component)]
    [DefaultProperty("Description")]
    public class RoomHotspot : IChangeNotification, IToXml
	{
		public const string PROPERTY_NAME_SCRIPT_NAME = "Name";
        public const string PROPERTY_NAME_DESCRIPTION = "Description";

        private static InteractionSchema _interactionSchema;

        private int _id;
        private string _name = string.Empty;
        private string _description = string.Empty;
        private Point _walkToPoint;
        private CustomProperties _properties = new CustomProperties(CustomPropertyAppliesTo.Hotspots);
        private Interactions _interactions = new Interactions(_interactionSchema);
		private IChangeNotification _notifyOfModification;

        static RoomHotspot()
        {
            _interactionSchema = new InteractionSchema(string.Empty, true,
                new string[] {"Stands on hotspot",
                "$$01 hotspot","$$02 hotspot","Use inventory on hotspot",
                "$$03 hotspot", "Any click on hotspot","Mouse moves over hotspot", 
                "$$05 hotspot", "$$08 hotspot", "$$09 hotspot"},
                new string[] { "WalkOn", "Look", "Interact", "UseInv", "Talk", "AnyClick", "MouseMove", "PickUp", "Mode8", "Mode9" },
                new string[] { /*WalkOn*/"Hotspot *theHotspot", /*Look*/"Hotspot *theHotspot, CursorMode mode",
                    /*Interact*/"Hotspot *theHotspot, CursorMode mode", /*UseInv*/"Hotspot *theHotspot, CursorMode mode",
                    /*Talk*/"Hotspot *theHotspot, CursorMode mode", /*AnyClick*/"Hotspot *theHotspot, CursorMode mode",
                    /*MouseMove*/"Hotspot *theHotspot", /*PickUp*/"Hotspot *theHotspot, CursorMode mode",
                    /*Mode8*/"Hotspot *theHotspot, CursorMode mode", /*Mode9*/"Hotspot *theHotspot, CursorMode mode" });
        }

		public RoomHotspot(IChangeNotification changeNotifier)
		{
			_notifyOfModification = changeNotifier;
		}

        public RoomHotspot(IChangeNotification changeNotifier, XmlNode node) : this(changeNotifier)
        {
            SerializeUtils.DeserializeFromXML(this, node);
            Interactions.FromXml(node);
        }

        [AGSNoSerialize]
        [Description("The ID number of the hotspot")]
        [Category("Design")]
        [ReadOnly(true)]
        [BrowsableMultiedit(false)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("Description of the hotspot")]
        [Category("Appearance")]
        [EditorAttribute(typeof(MultiLineStringUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string Description
        {
            get { return _description; }
            set { _description = value; }
        }

		[DisplayName(PROPERTY_NAME_SCRIPT_NAME)]
		[Description("The script name of the hotspot")]
        [Category("Design")]
        [BrowsableMultiedit(false)]
        public string Name
        {
            get { return _name; }
            set { _name = Utilities.ValidateScriptName(value); }
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
            get { return TypesHelper.MakePropertyGridTitle("Hotspot", _name, _description, _id); }
        }

        [AGSSerializeClass()]
        [Description("Custom properties for this hotspot")]
        public CustomProperties Properties
        {
            get { return _properties; }
            protected set
            {
                _properties = value;
                _properties.AppliesTo = CustomPropertyAppliesTo.Hotspots;
            }
        }

        [AGSSerializeClass()]
        [Browsable(false)]
        public Interactions Interactions
        {
            get { return _interactions; }
        }

		void IChangeNotification.ItemModified()
		{
			_notifyOfModification.ItemModified();
		}

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer, false);
            Interactions.ToXml(writer);
            writer.WriteEndElement();
        }
    }
}
