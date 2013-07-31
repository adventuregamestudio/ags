using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;

namespace AGS.Types
{
    [PropertyTab(typeof(PropertyTabInteractions), PropertyTabScope.Component)]
    public class Room : UnloadedRoom, IChangeNotification, ILoadedRoom
    {
        // These constants are also defined in the native code, if you change
        // them you need to update the Native DLL as well
        public const int LEGACY_MAX_BACKGROUNDS = 5;
        public const int LEGACY_MAX_OBJECTS = 40;
        public const int LEGACY_MAX_HOTSPOTS = 50;
        public const int LEGACY_MAX_WALKABLE_AREAS = 16;
        public const int LEGACY_MAX_WALK_BEHINDS = 16;
        public const int LEGACY_MAX_REGIONS = 16;
        public const int MAX_8BIT_MASK_REGIONS = 256;

        public const string EVENT_SUFFIX_ROOM_LOAD = "Load";

        private static InteractionSchema _interactionSchema;

        public delegate void RoomModifiedChangedHandler(bool isModified);
        public event RoomModifiedChangedHandler RoomModifiedChanged;
        public delegate void RoomRegionCountChangedHandler(RoomAreaMaskType maskType);
        public event RoomRegionCountChangedHandler RoomRegionCountChanged;

        private int _leftEdgeX;
        private int _rightEdgeX;
        private int _topEdgeY;
        private int _bottomEdgeY;
        private bool _stateSaving;
        private int _playMusicOnRoomLoad;
        private bool _saveLoadEnabled = true;
        private bool _showPlayerCharacter = true;
        private int _playerCharacterView;
        private RoomVolumeAdjustment _musicVolumeAdjustment;
        private RoomResolution _resolution = RoomResolution.LowRes;
        private int _colorDepth;
        private int _width;
        private int _height;
        private int _backgroundAnimationDelay;
        private int _backgroundCount;
        private int _gameId;
        private bool _modified;
        private CustomProperties _properties = new CustomProperties();
        private Interactions _interactions = new Interactions(_interactionSchema);
        private IList<RoomMessage> _messages = new List<RoomMessage>();
        private IList<RoomObject> _objects = new List<RoomObject>();
        private IList<RoomHotspot> _hotspots = new List<RoomHotspot>();
        private IList<RoomWalkableArea> _walkableAreas = new List<RoomWalkableArea>();
        private IList<RoomWalkBehind> _walkBehinds = new List<RoomWalkBehind>();
        private IList<RoomRegion> _regions = new List<RoomRegion>();
        private IList<OldInteractionVariable> _oldInteractionVariables = new List<OldInteractionVariable>();
        public IntPtr _roomStructPtr;

        static Room()
        {
            _interactionSchema = new InteractionSchema(new string[] {
                "Walks off left edge",
                "Walks off right edge",
                "Walks off bottom edge",
                "Walks off top edge",
                "First time enters room",
                "Enters room before fade-in",
                "Repeatedly execute",
                "Enters room after fade-in",
                "Leaves room",
            },
                new string[] { "LeaveLeft", "LeaveRight", "LeaveBottom", "LeaveTop", 
                    "FirstLoad", EVENT_SUFFIX_ROOM_LOAD, "RepExec", "AfterFadeIn", "Leave" });
        }

        public Room(int roomNumber) : base(roomNumber)
        {
            _script = new Script("room" + roomNumber + ".asc", "// Room script file", false);
        }

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

        [Description("Which resolution the background was imported for")]
        [Category("Visual")]
        [ReadOnly(true)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public RoomResolution Resolution
        {
            get { return _resolution; }
            set { _resolution = value; }
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
        /*
        [Description("Width of the background image, in real pixels")]
        [DisplayName("ImageWidth")]
        [Category("Visual")]
        [ReadOnly(true)]
        public int ImageWidth
        {
            get { return _width * (int)_resolution; }
        }

        [Description("Height of the background image, in real pixels")]
        [DisplayName("ImageHeight")]
        [Category("Visual")]
        [ReadOnly(true)]
        public int ImageHeight
        {
            get { return _height * (int)_resolution; }
        }*/

        [Description("Room-specific messages")]
        [Category("Messages")]
        [EditorAttribute(typeof(RoomMessagesUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public IList<RoomMessage> Messages
        {
            get { return _messages; }
        }

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
        public int HotspotCount
        {
            get { return _hotspots.Count; }
            set { SetHotspotCount(value); }
        }

        [Description("The number of walkable areas in the room")]
        [Category("Regions")]
        public int WalkableAreaCount
        {
            get { return _walkableAreas.Count; }
            set { SetWalkableAreaCount(value); }
        }

        [Description("The number of walk-behinds in the room")]
        [Category("Regions")]
        public int WalkBehindCount
        {
            get { return _walkBehinds.Count; }
            set { SetWalkBehindCount(value); }
        }

        [Description("The number of regions in the room")]
        [Category("Regions")]
        public int RegionCount
        {
            get { return _regions.Count; }
            set { SetRegionCount(value); }
        }

        [Browsable(false)]
        public IList<OldInteractionVariable> OldInteractionVariables
        {
            get { return _oldInteractionVariables; }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return _description + " (Room; number " + _number + ")"; }
        }

        [Description("Whether the state of the room is saved when the player leaves the room and comes back")]
        [DefaultValue(true)]
        [Category("Settings")]
        public bool StateSaving
        {
            get { return _stateSaving; }
            set { _stateSaving = value; }
        }

        [Description("The speed at which the backgrounds will rotate (only applicable if you have imported more than one)")]
        [DefaultValue(5)]
        [Category("Settings")]
        public int BackgroundAnimationDelay
        {
            get { return _backgroundAnimationDelay; }
            set { _backgroundAnimationDelay = value; }
        }

        [Browsable(false)]
        public int PlayMusicOnRoomLoad
        {
            get { return _playMusicOnRoomLoad; }
            set { _playMusicOnRoomLoad = value; }
        }

        [Browsable(false)]
        public bool SaveLoadEnabled
        {
            get { return _saveLoadEnabled; }
            set { _saveLoadEnabled = value; }
        }

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

        [Description("LEGACY PlayMusic COMMANDS ONLY: The music volume will be adjusted when in this room")]
        [DefaultValue(RoomVolumeAdjustment.Normal)]
        [Category("Settings")]
        [DisplayName("LegacyMusicVolumeAdjustment")]
        public RoomVolumeAdjustment MusicVolumeAdjustment
        {
            get { return _musicVolumeAdjustment; }
            set { _musicVolumeAdjustment = value; }
        }

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
			this.Modified = true;
		}

        void SetHotspotCount(int count)
        {
            if (count < 1 || count > MAX_8BIT_MASK_REGIONS)
            {
                throw new ArgumentOutOfRangeException("Hotspot count must be 1-256");
            }

            while (count < _hotspots.Count)
            {
                _hotspots.RemoveAt(_hotspots.Count - 1);
            }
            while (count > _hotspots.Count)
            {
                RoomHotspot hotspot = new RoomHotspot(this);
                hotspot.ID = _hotspots.Count;
                if (hotspot.ID == 0) hotspot.Description = "No hotspot";
                else hotspot.Description = "Hotspot " + hotspot.ID;
                _hotspots.Add(hotspot);
            }

            Modified = true;
            if (RoomRegionCountChanged != null)
            {
                RoomRegionCountChanged(RoomAreaMaskType.Hotspots);
            }
        }

        void SetWalkableAreaCount(int count)
        {
            if (count < 1 || count > MAX_8BIT_MASK_REGIONS)
            {
                throw new ArgumentOutOfRangeException("Walkable area count must be 1-256");
            }

            while (count < _walkableAreas.Count)
            {
                _walkableAreas.RemoveAt(_walkableAreas.Count - 1);
            }
            while (count > _walkableAreas.Count)
            {
                RoomWalkableArea area = new RoomWalkableArea();
                area.ID = _walkableAreas.Count;
                _walkableAreas.Add(area);
            }

            Modified = true;
            if (RoomRegionCountChanged != null)
            {
                RoomRegionCountChanged(RoomAreaMaskType.WalkableAreas);
            }
        }

        void SetWalkBehindCount(int count)
        {
            if (count < 1 || count > MAX_8BIT_MASK_REGIONS)
            {
                throw new ArgumentOutOfRangeException("Walk-behind count must be 1-256");
            }

            while (count < _walkBehinds.Count)
            {
                _walkBehinds.RemoveAt(_walkBehinds.Count - 1);
            }
            while (count > _walkBehinds.Count)
            {
                RoomWalkBehind area = new RoomWalkBehind();
                area.ID = _walkBehinds.Count;
                _walkBehinds.Add(area);
            }

            Modified = true;
            if (RoomRegionCountChanged != null)
            {
                RoomRegionCountChanged(RoomAreaMaskType.WalkBehinds);
            }
        }

        void SetRegionCount(int count)
        {
            if (count < 1 || count > MAX_8BIT_MASK_REGIONS)
            {
                throw new ArgumentOutOfRangeException("Region count must be 1-256");
            }

            while (count < _regions.Count)
            {
                _regions.RemoveAt(_regions.Count - 1);
            }
            while (count > _regions.Count)
            {
                RoomRegion area = new RoomRegion();
                area.ID = _regions.Count;
                _regions.Add(area);
            }

            Modified = true;
            if (RoomRegionCountChanged != null)
            {
                RoomRegionCountChanged(RoomAreaMaskType.Regions);
            }
        }
	}
}
