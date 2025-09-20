using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [PropertyTab(typeof(PropertyTabEvents), PropertyTabScope.Component)]
    [DefaultProperty("Description")]
    public class RoomHotspot : IChangeNotification, IToXml
	{
		public const string PROPERTY_NAME_SCRIPT_NAME = "Name";
        public const string PROPERTY_NAME_DESCRIPTION = "Description";

        private int _id;
        private string _name = string.Empty;
        private string _description = string.Empty;
        private Point _walkToPoint;
        private CustomProperties _properties = new CustomProperties(CustomPropertyAppliesTo.Hotspots);
        // Game Events
        private Interactions _interactions = new Interactions(InteractionSchema.Instance);
        private string _onAnyClick;
        private string _onWalkOn;
        private string _onMouseMove;
        //
        private Room _room;

		public RoomHotspot(Room room)
		{
			_room = room;
		}

        public RoomHotspot(Room room, XmlNode node) : this(room)
        {
            SerializeUtils.DeserializeFromXML(this, node);
            Interactions.FromXml(node);
        }

        [AGSNoSerialize()]
        [Browsable(false)]
        public Room Room
        {
            get { return _room; }
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

        #region Game Events

        [AGSSerializeClass()]
        [Browsable(false)]
        [Category("Cursor Events")]
        [ScriptFunction("Hotspot *theHotspot, CursorMode mode")]
        public Interactions Interactions
        {
            get { return _interactions; }
        }

        [DisplayName("Any click on")]
        [Category("Cursor Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("AnyClick", "Hotspot *theHotspot, CursorMode mode")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnAnyClick
        {
            get { return _onAnyClick; }
            set { _onAnyClick = value; }
        }

        [DisplayName("Player stands on hotspot")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("WalkOn", "Hotspot *theHotspot")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnWalkOn
        {
            get { return _onWalkOn; }
            set { _onWalkOn = value; }
        }

        [DisplayName("Mouse moves over hotspot")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("WalkOn", "Hotspot *theHotspot")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnMouseMove
        {
            get { return _onMouseMove; }
            set { _onMouseMove = value; }
        }

        #endregion // Game Events

        void IChangeNotification.ItemModified()
		{
			(_room as IChangeNotification).ItemModified();
		}

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer, false);
            Interactions.ToXml(writer);
            writer.WriteEndElement();
        }
    }
}
