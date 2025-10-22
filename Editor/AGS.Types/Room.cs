using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [PropertyTab(typeof(PropertyTabEvents), PropertyTabScope.Component)]
    [DefaultProperty("Description")]
    public class Room : UnloadedRoom, IChangeNotification, ILoadedRoom
    {
        // These constants are also defined in the native code, if you change
        // them you need to update the Native DLL as well
        public const int MAX_BACKGROUNDS = 5;
        public const int MAX_OBJECTS = 256;
        public const int MAX_HOTSPOTS = 50;
        public const int MAX_WALKABLE_AREAS = 16;
        public const int MAX_WALK_BEHINDS = 16;
        public const int MAX_REGIONS = 16;

        public const string EVENT_NAME_ROOM_LOAD = "Load";

        public const string PROPERTY_NAME_MASKRESOLUTION = "MaskResolution";
        /*
         * Room version history:
         * 
         * 1            - New XML room format introduced
         * 4.00.00.20   - Updated to a X.Y.Z.W version format.
         * 4.00.00.21   - New event tables.
        */
        public const string LATEST_XML_VERSION = "4.00.00.21";

        private const string FIRST_XML_VERSION = "3.99.99.01";

        public delegate void RoomModifiedChangedHandler(bool isModified);
        public event RoomModifiedChangedHandler RoomModifiedChanged;

        // The version this room was loaded from
        private System.Version _savedXmlVersion = null;
        private int _leftEdgeX;
        private int _rightEdgeX;
        private int _topEdgeY;
        private int _bottomEdgeY;
        private bool _showPlayerCharacter = true;
        private int _playerCharacterView;
        private float _faceDirectionRatio = 0.0f;
        private int _maskResolution = 1;
        private int _colorDepth;
        private int _width;
        private int _height;
        private int _backgroundAnimationDelay = 5;
        private bool _backgroundAnimEnabled = true;
        private int _backgroundCount;
        private int _gameId;
        private bool _modified;
        private CustomProperties _properties = new CustomProperties(CustomPropertyAppliesTo.Rooms);
        private readonly List<RoomObject> _objects = new List<RoomObject>();
        private readonly List<RoomHotspot> _hotspots = new List<RoomHotspot>();
        private readonly List<RoomWalkableArea> _walkableAreas = new List<RoomWalkableArea>();
        private readonly List<RoomWalkBehind> _walkBehinds = new List<RoomWalkBehind>();
        private readonly List<RoomRegion> _regions = new List<RoomRegion>();
        // Game Events
        private Interactions _interactions = new Interactions(InteractionSchema.Instance);
        private string _onLeaveLeft;
        private string _onLeaveRight;
        private string _onLeaveBottom;
        private string _onLeaveTop;
        private string _onLoad;
        private string _onFirstTimeEnter;
        private string _onAfterFadeIn;
        private string _onBeforeFadeOut;
        private string _onUnload;
        private string _onRepExec;

        public Room(int roomNumber) : base(roomNumber)
        {
            _script = new Script("room" + roomNumber + ".asc", "// Room script file", false);

            for (int i = 0; i < MAX_HOTSPOTS; i++)
            {
                RoomHotspot hotspot = new RoomHotspot(this);
                hotspot.ID = i;
                _hotspots.Add(hotspot);
            }

            for (int i = 0; i < MAX_WALKABLE_AREAS; i++)
            {
                RoomWalkableArea area = new RoomWalkableArea(this);
                area.ID = i;
                _walkableAreas.Add(area);
            }

            for (int i = 0; i < MAX_WALK_BEHINDS; i++)
            {
                RoomWalkBehind area = new RoomWalkBehind(this);
                area.ID = i;
                _walkBehinds.Add(area);
            }

            for (int i = 0; i < MAX_REGIONS; i++)
            {
                RoomRegion area = new RoomRegion(this);
                area.ID = i;
                _regions.Add(area);
            }
        }

        public Room(XmlNode node) : base(node)
        {
            LoadFromXml(node);
        }

        /// <summary>
        /// The version of the room file that was loaded from disk.
        /// This is null if the room has not yet been saved.
        /// </summary>
        [AGSNoSerialize]
        [Browsable(false)]
        public System.Version SavedXmlVersion
        {
            get { return _savedXmlVersion; }
            set { _savedXmlVersion = value; }
        }

        [AGSNoSerialize]
        [Browsable(false)]
        public bool Modified
        {
            get { return _modified; }
            set 
            {
                if (value != _modified)
                {
                    _modified = value;
                    if (RoomModifiedChanged != null)
                    {
                        RoomModifiedChanged(_modified);
                    }
                }
            }
        }

        [Description("What resolution do room region masks have relative to the room size")]
        [Category("Regions")]
        [DefaultValue(1)]
        [TypeConverter(typeof(RoomMaskResolutionTypeConverter))]
        public int MaskResolution
        {
            get { return _maskResolution; }
            set { _maskResolution = value; }
        }

        [Browsable(false)]
        public int BackgroundCount
        {
            get { return _backgroundCount; }
            set { _backgroundCount = value; }
        }

        [Browsable(false)]
        public int GameID
        {
            get { return _gameId; }
            set { _gameId = value; }
        }

        [Description("Colour depth of the room background, in bits per pixel")]
        [Category("Visual")]
        [ReadOnly(true)]
        public int ColorDepth
        {
            get { return _colorDepth; }
            set { _colorDepth = value; }
        }

        [Description("Width of the room, in game units")]
        //[DisplayName("WidthInGameUnits")]
        [Category("Visual")]
        [ReadOnly(true)]
        public int Width
        {
            get { return _width; }
            set { _width = value; }
        }

        [Description("Height of the room, in game units")]
        //[DisplayName("HeightInGameUnits")]
        [Category("Visual")]
        [ReadOnly(true)]
        public int Height
        {
            get { return _height; }
            set { _height = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public object Messages { get; }

        [Browsable(false)]
        public IList<RoomObject> Objects
        {
            get { return _objects; }
        }

        [Browsable(false)]
        public IList<RoomHotspot> Hotspots
        {
            get { return _hotspots; }
        }

        [Browsable(false)]
        public IList<RoomWalkableArea> WalkableAreas
        {
            get { return _walkableAreas; }
        }

        [Browsable(false)]
        public IList<RoomWalkBehind> WalkBehinds
        {
            get { return _walkBehinds; }
        }

        [Browsable(false)]
        public IList<RoomRegion> Regions
        {
            get { return _regions; }
        }

        [Description("The number of hotspots in the room")]
        [Category("Regions")]
        [Browsable(false)]
        public int HotspotCount
        {
            get { return _hotspots.Count; }
        }

        [Description("The number of walkable areas in the room")]
        [Category("Regions")]
        [Browsable(false)]
        public int WalkableAreaCount
        {
            get { return _walkableAreas.Count; }
        }

        [Description("The number of walk-behinds in the room")]
        [Category("Regions")]
        [Browsable(false)]
        public int WalkBehindCount
        {
            get { return _walkBehinds.Count; }
        }

        [Description("The number of regions in the room")]
        [Category("Regions")]
        [Browsable(false)]
        public int RegionCount
        {
            get { return _regions.Count; }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return $"{_description} (Room; number {_number})"; }
        }

        [Description("The speed at which the backgrounds will rotate (only applicable if you have imported more than one)")]
        [DefaultValue(5)]
        [Category("Settings")]
        public int BackgroundAnimationDelay
        {
            get { return _backgroundAnimationDelay; }
            set
            {
                // The animation delay value is restricted to 1 byte in the compiled room at the moment
                _backgroundAnimationDelay = Math.Max(0, Math.Min(255, value));
            }
        }

        [Description("Whether the background frames will switch automatically (only applicable if you have imported more than one)")]
        [DefaultValue(true)]
        [Category("Settings")]
        public bool BackgroundAnimationEnabled
        {
            get { return _backgroundAnimEnabled; }
            set { _backgroundAnimEnabled = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public int PlayMusicOnRoomLoad { get; }

        [Obsolete]
        [Browsable(false)]
        // TODO: check if it's okay to adjust ILoadedRoom interface and let remove the setter here
        public bool SaveLoadEnabled { get; set; }

        [Description("Whether the player character is visible on this screen")]
        [DefaultValue(true)]
        [Category("Settings")]
        public bool ShowPlayerCharacter
        {
            get { return _showPlayerCharacter; }
            set { _showPlayerCharacter = value; }
        }

        [Description("Override the player character's walking view for this room only")]
        [DefaultValue(0)]
        [Category("Settings")]
        [EditorAttribute(typeof(ViewUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int PlayerCharacterView
        {
            get { return _playerCharacterView; }
            set { _playerCharacterView = value; }
        }

        [Description("The Y/X relation of diagonal directions at which Character switches from horizontal to vertical walking loops. " +
            "Default is 1.0. < 1.0 would make diagonal direction more \"vertical\", > 1.0 would make it more \"horizontal\".\n" +
            "Negative values switch up and down loops.\n" +
            "This property is optional, and is disabled by assigning a 0. If non-zero, then it will override character behavior in this room.")]
        [DefaultValue(0.0f)]
        [Category("Settings")]
        public float FaceDirectionRatio
        {
            get { return _faceDirectionRatio; }
            set { _faceDirectionRatio = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public int MusicVolumeAdjustment { get; }

        [Description("The X co-ordinate of the left room edge")]
        [Category("Edges")]
        public int LeftEdgeX
        {
            get { return _leftEdgeX; }
            set { _leftEdgeX = value; }
        }

        [Description("The X co-ordinate of the right room edge")]
        [Category("Edges")]
        public int RightEdgeX
        {
            get { return _rightEdgeX; }
            set { _rightEdgeX = value; }
        }

        [Description("The Y co-ordinate of the top room edge")]
        [Category("Edges")]
        public int TopEdgeY
        {
            get { return _topEdgeY; }
            set { _topEdgeY = value; }
        }

        [Description("The Y co-ordinate of the bottom room edge")]
        [Category("Edges")]
        public int BottomEdgeY
        {
            get { return _bottomEdgeY; }
            set { _bottomEdgeY = value; }
        }

        [AGSSerializeClass()]
        [Description("Custom properties for this room")]
        public CustomProperties Properties
        {
            get { return _properties; }
            protected set 
            {
                _properties = value;
                _properties.AppliesTo = CustomPropertyAppliesTo.Rooms;
            }
        }

        #region Game Events

        [AGSNoSerialize()]
        [Browsable(false)]
        [Obsolete]
        public Interactions Interactions
        {
            get { return _interactions; }
        }

        [DisplayName("Walks off left edge")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("LeaveLeft", "")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnLeaveLeft
        {
            get { return _onLeaveLeft; }
            set { _onLeaveLeft = value; }
        }

        [DisplayName("Walks off right edge")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("LeaveRight", "")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnLeaveRight
        {
            get { return _onLeaveRight; }
            set { _onLeaveRight = value; }
        }

        [DisplayName("Walks off bottom edge")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("LeaveBottom", "")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnLeaveBottom
        {
            get { return _onLeaveBottom; }
            set { _onLeaveBottom = value; }
        }

        [DisplayName("Walks off top edge")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("LeaveTop", "")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnLeaveTop
        {
            get { return _onLeaveTop; }
            set { _onLeaveTop = value; }
        }

        [DisplayName("Enters room before fade-in")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("Load", "")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnLoad
        {
            get { return _onLoad; }
            set { _onLoad = value; }
        }

        [DisplayName("First time enters room")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("FirstTimeEnter", "")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnFirstTimeEnter
        {
            get { return _onFirstTimeEnter; }
            set { _onFirstTimeEnter = value; }
        }

        [DisplayName("Enters room after fade-in")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("AfterFadeIn", "")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnAfterFadeIn
        {
            get { return _onAfterFadeIn; }
            set { _onAfterFadeIn = value; }
        }

        [DisplayName("Leaves room before fade-out")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("Leave", "")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnLeave
        {
            get { return _onBeforeFadeOut; }
            set { _onBeforeFadeOut = value; }
        }

        [DisplayName("Leaves room after fade-out")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("Unload", "")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnUnload
        {
            get { return _onUnload; }
            set { _onUnload = value; }
        }

        [DisplayName("Repeatedly execute")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("RepExec", "")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnRepExec
        {
            get { return _onRepExec; }
            set { _onRepExec = value; }
        }

        #endregion // Game Events

        public static int GetMaskMaxColor(RoomAreaMaskType mask)
        {
            switch (mask)
            {
                case RoomAreaMaskType.WalkBehinds: return MAX_WALK_BEHINDS;
                case RoomAreaMaskType.Hotspots: return MAX_HOTSPOTS;
                case RoomAreaMaskType.WalkableAreas: return MAX_WALKABLE_AREAS;
                case RoomAreaMaskType.Regions: return MAX_REGIONS;
                default:
                    throw new ArgumentException($"Illegal mask type, mask {mask} doesn't have colors");
            }
        }

        public double GetMaskScale(RoomAreaMaskType mask)
        {
            switch (mask)
            {
                case RoomAreaMaskType.WalkBehinds:
                    return 1.0; // walk-behinds always 1:1 with room size
                case RoomAreaMaskType.Hotspots:
                case RoomAreaMaskType.WalkableAreas:
                case RoomAreaMaskType.Regions:
                    return 1.0 / MaskResolution;
                default:
                    throw new ArgumentException($"Illegal mask type, mask {mask} cannot be scaled.");
            }
        }

        void IChangeNotification.ItemModified()
		{
			this.Modified = true;
		}
		
        public bool IsScriptNameAlreadyUsed(string tryName, object ignoreObject)
        {
            if (tryName == string.Empty)
            {
                return false;
            }

            foreach (string name in Game.RESERVED_SCRIPT_NAMES)
            {
                if (tryName == name)
                {
                    return true;
                }
            }

            foreach (RoomHotspot hotspot in Hotspots)
            {
                if ((hotspot.Name == tryName) && (hotspot != ignoreObject))
                {
                    return true;
                }
            }
            foreach (RoomObject obj in Objects)
            {
                if ((obj.Name == tryName) && (obj != ignoreObject))
                {
                    return true;
                }
            }
            return false;
        }

        public override void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement(GetType().Name);
            writer.WriteAttributeString("Version", LATEST_XML_VERSION);
            SerializeUtils.SerializePropertiesToXML(this, writer);
            SerializeUtils.SerializeToXML(writer, "Objects", Objects);
            SerializeUtils.SerializeToXML(writer, "Hotspots", Hotspots);
            SerializeUtils.SerializeToXML(writer, "WalkableAreas", WalkableAreas);
            SerializeUtils.SerializeToXML(writer, "WalkBehinds", WalkBehinds);
            SerializeUtils.SerializeToXML(writer, "Regions", Regions);
            writer.WriteEndElement();
        }

        private static IEnumerable<XmlNode> GetXmlChildren(XmlNode node, string xpath, int maxLimit)
        {
            return node.SelectSingleNode(xpath).ChildNodes.Cast<XmlNode>().Take(maxLimit);
        }

        /// <summary>
        /// Loads Room contents from XML.
        /// Room's own properties (top level) are loaded automatically using DeserializeFromXML
        /// (see UnloadedRoom's constructor).
        /// </summary>
        private void LoadFromXml(XmlNode node)
        {
            var versionAttr = node.Attributes.GetNamedItem("Version");
            System.Version fileVersion = null;
            if (versionAttr != null)
            {
                // Try parsing as a decimal first, that was used in the very first version
                int versionIndex = 0;
                if (int.TryParse(versionAttr.InnerText, out versionIndex))
                {
                    fileVersion = new System.Version(FIRST_XML_VERSION);
                }
                else
                {
                    if (!System.Version.TryParse(versionAttr.InnerText, out fileVersion))
                    {
                        throw new AGSEditorException($"Room data file has an invalid version identifier.");
                    }
                }
            }

            LoadAndConvertInteractionEvents(node);

            _objects.AddRange(GetXmlChildren(node, "/Room/Objects", MAX_OBJECTS).Select((xml, i) => new RoomObject(this, xml) { ID = i }));
            _hotspots.AddRange(GetXmlChildren(node, "/Room/Hotspots", MAX_HOTSPOTS).Select((xml, i) => new RoomHotspot(this, xml) { ID = i }));
            _walkableAreas.AddRange(GetXmlChildren(node, "/Room/WalkableAreas", MAX_WALKABLE_AREAS).Select((xml, i) => new RoomWalkableArea(this, xml) { ID = i }));
            _walkBehinds.AddRange(GetXmlChildren(node, "/Room/WalkBehinds", MAX_WALK_BEHINDS).Select((xml, i) => new RoomWalkBehind(this, xml) { ID = i }));
            _regions.AddRange(GetXmlChildren(node, "/Room/Regions", MAX_REGIONS).Select((xml, i) => new RoomRegion(this, xml) { ID = i }));

            _savedXmlVersion = fileVersion;
        }

        private void LoadAndConvertInteractionEvents(XmlNode node)
        {
            if (node.SelectSingleNode("Interactions") == null)
                return;

            // Load old-style events into the temporary interactions object,
            // as the one that we are keeping is for compatibility only.
            Interactions interactions = new Interactions(null);
            interactions.FromXml(node);
            if (interactions.IndexedFunctionNames.Count == 0)
                return; // no old indexed events, bail out
            // Convert interaction events to our new event properties
            OnLeaveLeft = interactions.IndexedFunctionNames.TryGetValueOrDefault(0, string.Empty);
            OnLeaveRight = interactions.IndexedFunctionNames.TryGetValueOrDefault(1, string.Empty);
            OnLeaveBottom = interactions.IndexedFunctionNames.TryGetValueOrDefault(2, string.Empty);
            OnLeaveTop = interactions.IndexedFunctionNames.TryGetValueOrDefault(3, string.Empty);
            OnFirstTimeEnter = interactions.IndexedFunctionNames.TryGetValueOrDefault(4, string.Empty);
            OnLoad = interactions.IndexedFunctionNames.TryGetValueOrDefault(5, string.Empty);
            OnRepExec = interactions.IndexedFunctionNames.TryGetValueOrDefault(6, string.Empty);
            OnAfterFadeIn = interactions.IndexedFunctionNames.TryGetValueOrDefault(7, string.Empty);
            OnLeave = interactions.IndexedFunctionNames.TryGetValueOrDefault(8, string.Empty);
            OnUnload = interactions.IndexedFunctionNames.TryGetValueOrDefault(9, string.Empty);
        }
    }
}
