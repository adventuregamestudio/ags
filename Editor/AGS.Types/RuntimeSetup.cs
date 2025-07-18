using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class RuntimeSetup : ICustomTypeDescriptor
    {
        public const string DEFAULT_GFX_FILTER_NAME = "stdscale";

        private Settings _gameSettings;
        private GraphicsDriver _gfxDriver;
        // All possible gfx filters, for every gfx driver
        private Dictionary<GraphicsDriver, Dictionary<string, string>> _gfxFiltersAll;
        // Default filter per driver
        private Dictionary<GraphicsDriver, string> _gfxFiltersDefault;
        // Gfx filters for currently selected driver
        private Dictionary<string, string> _gfxFilters;
        private string _customSavePath;
        private string _customAppDataPath;

        public RuntimeSetup(Settings gameSettings)
        {
            _gameSettings = gameSettings;

            InitFilters();
            SetDefaults();
        }

        private void InitFilters()
        {
            _gfxFiltersAll = new Dictionary<GraphicsDriver, Dictionary<string, string>>();
            Dictionary<string, string> filters = new Dictionary<string, string>();
            filters[DEFAULT_GFX_FILTER_NAME] = "Nearest-neighbour";
            _gfxFiltersAll.Add(GraphicsDriver.Software, filters);
            filters = new Dictionary<string, string>();
            filters[DEFAULT_GFX_FILTER_NAME] = "Nearest-neighbour";
            filters["linear"] = "Linear interpolation (with anti-aliasing)";
            _gfxFiltersAll.Add(GraphicsDriver.D3D9, filters);
            _gfxFiltersAll.Add(GraphicsDriver.OpenGL, filters);

            _gfxFiltersDefault = new Dictionary<GraphicsDriver, string>();
            _gfxFiltersDefault[GraphicsDriver.Software] = DEFAULT_GFX_FILTER_NAME;
            _gfxFiltersDefault[GraphicsDriver.D3D9] = DEFAULT_GFX_FILTER_NAME;
            _gfxFiltersDefault[GraphicsDriver.OpenGL] = DEFAULT_GFX_FILTER_NAME;
        }

        public void SetDefaults()
        {
            GraphicsDriver = GraphicsDriver.D3D9;
            Windowed = false;
            FullscreenGameScaling = GameScaling.ProportionalStretch;
            GameScaling = GameScaling.MaxInteger;
            GameScalingMultiplier = 1;
            GraphicsFilter = DEFAULT_GFX_FILTER_NAME;
            VSync = false;
            AAScaledSprites = false;
            RenderAtScreenResolution = false;
            Rotation = ScreenRotationMode.Unlocked;
            DigitalSound = RuntimeAudioDriver.Default;
            UseVoicePack = true;
            Translation = "";
            AutoLockMouse = false;
            MouseSpeed = 1.0f;
            TouchToMouseEmulation = TouchToMouseEmulationType.OneFinger;
            SpriteCacheSize = 128;
            TextureCacheSize = 128;
            SoundCacheSize = 32;
            CompressSaves = false;
            UseCustomSavePath = false;
            CustomSavePath = "";
            TitleText = _gameSettings.GameName + " Setup";
        }

        [DisplayName("Graphics driver")]
        [Description("The default graphics driver that your game will use. Software renderer is slower at scaling images, but it is slightly faster with raw drawing, and only one supporting 8-bit games at the moment.")]
        [Category("Graphics")]
        [DefaultValue(GraphicsDriver.D3D9)]
        [TypeConverter(typeof(EnumTypeConverter))]
        [RefreshProperties(RefreshProperties.All)]
        public GraphicsDriver GraphicsDriver
        {
            get { return _gfxDriver; }
            set
            {
                _gfxDriver = value;

                string old_filter = GraphicsFilter;
                if (_gfxFiltersAll.ContainsKey(_gfxDriver))
                    _gfxFilters = _gfxFiltersAll[_gfxDriver];
                else
                    _gfxFilters = null;

                if (_gfxFilters == null)
                    GraphicsFilter = null; // something unexpected
                else if (old_filter != null && _gfxFilters.ContainsKey(old_filter))
                    GraphicsFilter = old_filter; // same filter
                else
                    GraphicsFilter = _gfxFiltersDefault[_gfxDriver]; // use default filter
            }
        }

        [DisplayName("Start in windowed mode")]
        [Description("Run the game in windowed as opposed to fullscreen mode.")]
        [Category("Graphics")]
        [DefaultValue(false)]
        public bool Windowed
        {
            get;
            set;
        }

        [DisplayName("Fullscreen as borderless window")]
        [Description("In fullscreen mode create a borderless window covering whole desktop as opposed to exclusive (real) fullscreen.")]
        [Category("Graphics")]
        [DefaultValue(true)]
        public bool FullscreenDesktop
        {
            get;
            set;
        }

        [DisplayName("Fullscreen scaling style")]
        [Description("Determines how the game frame is scaled in the fullscreen mode.")]
        [Category("Graphics")]
        [DefaultValue(GameScaling.ProportionalStretch)]
        [TypeConverter(typeof(FullscreenGameScalingConverter))]
        public GameScaling FullscreenGameScaling
        {
            get;
            set;
        }

        [DisplayName("Windowed scaling style")]
        [Description("Determines how the game frame is scaled in the windowed mode.")]
        [Category("Graphics")]
        [DefaultValue(GameScaling.MaxInteger)]
        [TypeConverter(typeof(EnumTypeConverter))]
        [RefreshProperties(RefreshProperties.All)]
        // TODO:  consider renaming to WindowedGameScaling and implement property-to-property deserialization attribute
        // similar to value-to-value DeserializeConvertValueAttribute
        public GameScaling GameScaling
        {
            get;
            set;
        }

        [DisplayName("Windowed game scaling multiplier")]
        [Description("A round multiplier to scale game window with.")]
        [Category("Graphics")]
        [DefaultValue(1)]
        // TODO: consider renaming to WindowedScalingMultiplier and implement property-to-property deserialization attribute
        // similar to value-to-value DeserializeConvertValueAttribute
        public int GameScalingMultiplier
        {
            get;
            set;
        }

        [DisplayName("Scaling filter")]
        [Description("Graphics filter to use when scaling the game.")]
        [Category("Graphics")]
        [DefaultValue(DEFAULT_GFX_FILTER_NAME)]
        [TypeConverter(typeof(GraphicsFilterTypeConverter))]
        public string GraphicsFilter
        {
            get;
            set;
        }

        [AGSNoSerialize]
        [Browsable(false)]
        public Dictionary<string, string> GraphicsFilterOptions
        {
            get { return _gfxFilters; }
        }

        [DisplayName("Vertical sync")]
        [Description("Enable vertical synchronization. This will prevent \"screen tearing\", but may lower game's FPS if the player's graphic card is not fast enough.")]
        [Category("Graphics")]
        [DefaultValue(false)]
        public bool VSync
        {
            get;
            set;
        }

        [DisplayName("Anti-alias scaled sprites")]
        [Description("Enable sprites anti-aliasing when scaling them in game. Is not recommended to use with the software graphics driver.")]
        [Category("Graphics")]
        [DefaultValue(false)]
        public bool AAScaledSprites
        {
            get;
            set;
        }

        [Obsolete]
        [Browsable(false)]
        public bool DowngradeTo16bit { get; }

        [DisplayName("Render sprites at screen resolution")]
        [Description("When drawing zoomed character and object sprites, AGS will take advantage of higher runtime resolution to give scaled images more detail, than it would be possible if the game was displayed in its native resolution. The effect is stronger for low-res games. Keep disabled for pixel-perfect output. Currently supported only by Direct3D and OpenGL renderers.")]
        [Category("Graphics")]
        [DefaultValue(false)]
        public bool RenderAtScreenResolution
        {
            get;
            set;
        }

        [DisplayName("Rotation mode")]
        [Description("Sets if the player can freely rotate the screen or if it should be locked to a specific orientation.")]
        [Category("Graphics")]
        [DefaultValue(ScreenRotationMode.Unlocked)]
        public ScreenRotationMode Rotation
        {
            get;
            set;
        }

        [DisplayName("Audio driver")]
        [Description("The suggested audio driver option. Normally you keep this at Default, but if your game does not use audio at all you may disable it to tell the engine to not initialize audio system at the runtime.")]
        [Category("Audio")]
        [DefaultValue(RuntimeAudioDriver.Default)]
        public RuntimeAudioDriver DigitalSound
        {
            get;
            set;
        }

        [Obsolete]
        [Browsable(false)]
        public RuntimeAudioDriver MidiSound { get; }

        [DisplayName("Use voice pack if available")]
        [Description("Enables the use of digital voice-over pack. Will be ignored if your game does not have one.")]
        [Category("Audio")]
        [DefaultValue(true)]
        public bool UseVoicePack
        {
            get;
            set;
        }

        [Obsolete]
        [Browsable(false)]
        public bool ThreadedAudio { get; }

        [DisplayName("Game language")]
        [Description("Use this translation when running the game.")]
        [Category("Gameplay")]
        [DefaultValue("")]
        [TypeConverter(typeof(TranslationListTypeConverter))]
        public string Translation
        {
            get;
            set;
        }

        [DisplayName("Auto-lock mouse to window")]
        [Description("When running the game in window, system mouse cursor will be automatically locked inside the window bounds. It will be unlocked whenever player switches out from the game or presses Ctrl+Alt key combo.")]
        [Category("Mouse")]
        [DefaultValue(false)]
        public bool AutoLockMouse
        {
            get;
            set;
        }
        
        [DisplayName("Mouse cursor speed")]
        [Description("Mouse speed multiplier in fullscreen mode")]
        [Category("Mouse")]
        [DefaultValue(1.0)]
        public float MouseSpeed
        {
            get;
            set;
        }

        [DisplayName("Touch to Mouse Emulation")]
        [Description("Tells AGS if touch events should create emulated mouse events and how this should work.")]
        [Category("Touch")]
        [DefaultValue(TouchToMouseEmulationType.OneFinger)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public TouchToMouseEmulationType TouchToMouseEmulation
        {
            get;
            set;
        }

        [DisplayName("Touch to Mouse motion mode")]
        [Description("Whether the emulated mouse should directly correspond to a touch position, or only count relative touch movements.")]
        [Category("Touch")]
        [DefaultValue(InputMotionMode.Direct)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public InputMotionMode TouchToMouseMotionMode
        {
            get;
            set;
        }

        [DisplayName("Display FPS on Screen")]
        [Description("Whether to display fps counter on screen, useful for debugging.")]
        [Category("Misc")]
        [DefaultValue(false)]
        public bool ShowFPS
        {
            get;
            set;
        }

        [DisplayName("Sprite cache size (in megabytes)")]
        [Description("The limit for runtime sprite cache. The cache is used to keep game graphics loaded even if they are not currently used. Bigger cache means faster room transitions and generally better performance if your game has lots of high-res animations.")]
        [Category("Performance")]
        [DefaultValue(128)]
        public int SpriteCacheSize
        {
            get;
            set;
        }

        [DisplayName("Texture cache size (in megabytes)")]
        [Description("The limit for runtime texture cache. The cache is used to keep textures loaded in VRAM (your graphics card memory), and improves performance.")]
        [Category("Performance")]
        [DefaultValue(128)]
        public int TextureCacheSize
        {
            get;
            set;
        }

        [DisplayName("Sound cache size (in megabytes)")]
        [Description("The limit for runtime sound cache. The cache is used to keep small sound clips loaded (typically the ones less than 1 MB each). Bigger cache means less loading of the same clips, such as frame-linked sounds, and improves performance.")]
        [Category("Performance")]
        [DefaultValue(32)]
        public int SoundCacheSize
        {
            get;
            set;
        }

        [DisplayName("Compress game saves")]
        [Description("Whether the game saves will be compressed. This reduces their size on disk, but may affect save's loading times.")]
        [Category("Performance")]
        [DefaultValue(false)]
        public bool CompressSaves
        {
            get;
            set;
        }

        private String TestCustomPath(string path)
        {
            if (!String.IsNullOrEmpty(path))
            {
                if (Path.IsPathRooted(path))
                    throw new ArgumentException("Absolute paths are not allowed.");
                // We test paths against current working dir here, but in practice
                // this will be applied to game's installation dir.
                if (!Utilities.IsPathOrSubDir(Directory.GetCurrentDirectory(), path))
                    throw new ArgumentException("Paths to the upper directory levels are not allowed.");
            }
            return path;
        }

        [DisplayName("Use custom game saves path")]
        [Description("Define your own location for saved games and individual player files created by game script. Players are able to change this option freely.")]
        [Category("Enviroment")]
        [DefaultValue(false)]
        [RefreshProperties(RefreshProperties.All)]
        public bool UseCustomSavePath
        {
            get;
            set;
        }

        [DisplayName("Custom game saves path")]
        [Description("Define your own location for saved games and individual player files created by game script. Leave empty to use game's directory. This option accepts only relative paths, and your players will be able to change it to their liking.")]
        [Category("Enviroment")]
        [DefaultValue("")]
        [EditorAttribute(typeof(MultiLineStringUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string CustomSavePath
        {
            get
            {
                return _customSavePath;
            }
            set
            {
                _customSavePath = TestCustomPath(value);
            }
        }

        [DisplayName("Use custom shared data path")]
        [Description("Define your own location for shared data files created by game script.")]
        [Category("Enviroment")]
        [DefaultValue(false)]
        [RefreshProperties(RefreshProperties.All)]
        public bool UseCustomAppDataPath
        {
            get;
            set;
        }

        [DisplayName("Custom shared data path")]
        [Description("Define your own location for shared data files created by game script. Leave empty to use game's directory. This option accepts only relative paths.")]
        [Category("Enviroment")]
        [DefaultValue("")]
        [EditorAttribute(typeof(MultiLineStringUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string CustomAppDataPath
        {
            get
            {
                return _customAppDataPath;
            }
            set
            {
                _customAppDataPath = TestCustomPath(value);
            }
        }

        [DisplayName("Title text")]
        [Description("Text shown at the title bar of the setup program.")]
        [Category("(Setup appearance)")]
        [DefaultValue("")]
        [EditorAttribute(typeof(MultiLineStringUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string TitleText
        {
            get;
            set;
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

        public void FromXml(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        #region ICustomTypeDescriptor Members

        public AttributeCollection GetAttributes()
        {
            return TypeDescriptor.GetAttributes(this, true);
        }

        public string GetClassName()
        {
            return TypeDescriptor.GetClassName(this, true);
        }

        public string GetComponentName()
        {
            return TypeDescriptor.GetComponentName(this, true);
        }

        public TypeConverter GetConverter()
        {
            return TypeDescriptor.GetConverter(this, true);
        }

        public EventDescriptor GetDefaultEvent()
        {
            return TypeDescriptor.GetDefaultEvent(this, true);
        }

        public PropertyDescriptor GetDefaultProperty()
        {
            return TypeDescriptor.GetDefaultProperty(this, true);
        }

        public object GetEditor(Type editorBaseType)
        {
            return TypeDescriptor.GetEditor(this, editorBaseType, true);
        }

        public EventDescriptorCollection GetEvents(Attribute[] attributes)
        {
            return TypeDescriptor.GetEvents(this, attributes, true);
        }

        public EventDescriptorCollection GetEvents()
        {
            return TypeDescriptor.GetEvents(this, true);
        }

        public PropertyDescriptorCollection GetProperties(Attribute[] attributes)
        {
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, attributes, true);
            List<PropertyDescriptor> wantProperties = new List<PropertyDescriptor>();
            foreach (PropertyDescriptor property in properties)
            {
                bool wantThisProperty = true;
                if (property.Name == "GameScalingMultiplier")
                    wantThisProperty = GameScaling == GameScaling.Integer;
                // Only display RenderAtScreenRes if game settings have this property set to UserDefined
                else if (property.Name == "RenderAtScreenResolution")
                    wantThisProperty = _gameSettings.RenderAtScreenResolution == AGS.Types.RenderAtScreenResolution.UserDefined;
                else if (property.Name == "CustomSavePath")
                    wantThisProperty = UseCustomSavePath;
                else if (property.Name == "CustomAppDataPath")
                    wantThisProperty = UseCustomAppDataPath;

                if (wantThisProperty)
                {
                    wantProperties.Add(property);
                }
            }
            return new PropertyDescriptorCollection(wantProperties.ToArray());
        }

        public PropertyDescriptorCollection GetProperties()
        {
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, true);
            return properties;
        }

        public object GetPropertyOwner(PropertyDescriptor pd)
        {
            return this;
        }

        #endregion
    }
}
