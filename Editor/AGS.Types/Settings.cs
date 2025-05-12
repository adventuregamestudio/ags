using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace AGS.Types
{
    [DefaultProperty("DebugMode")]
    public class Settings : ICustomTypeDescriptor
    {
        // TODO: reimplement the handling of property value changes in the Editor assembly
        // so that relying on property labels is no longer necessary!
        public const string PROPERTY_GAME_NAME = "Game title";
        public const string PROPERTY_COLOUR_DEPTH = "Colour depth";
        public const string PROPERTY_RESOLUTION = "Resolution";
        public const string PROPERTY_TEXT_FORMAT = "Text format";
        public const string PROPERTY_ALLOWRELATIVEASSETS = "Allow relative asset resolutions";
		public const string PROPERTY_ANTI_ALIAS_FONTS = "Anti-alias TTF fonts";
        public const string PROPERTY_FONT_HEIGHT_IN_LOGIC = "TTF fonts height used in the game logic";
        public const string PROPERTY_LETTERBOX_MODE = "Enable letterbox mode";
        public const string PROPERTY_BUILD_TARGETS = "Build target platforms";
        public const string PROPERTY_RENDERATSCREENRES = "Render sprites at screen resolution";
        public const string PROPERTY_CLIPGUICONTROLS = "GUI controls clip their contents";
        public const string PROPERTY_DIALOG_SCRIPT_SAYFN = "Custom Say function in dialog scripts";
        public const string PROPERTY_DIALOG_SCRIPT_NARRATEFN = "Custom Narrate function in dialog scripts";
        public const string REGEX_FOUR_PART_VERSION = @"^(\d+)\.(\d+)\.(\d+)\.(\d+)$";
        public const string PROPERTY_ANDROID_APPLICATION_ID = "App ID";
        public const string PROPERTY_ANDROID_APP_VERSION_CODE = "App Version Code";
        public const string PROPERTY_ANDROID_APP_VERSION_NAME = "App Version Name";

		private const string DEFAULT_GENRE = "Adventure";
        private const string DEFAULT_VERSION = "1.0.0.0";
        private const string DEFAULT_TEXTFORMAT = "utf-8";

        private const string DEFAULT_TARGET_NAMES = "DataFile, Windows";

        public Settings()
        {
			GenerateNewGameID();
        }

        private string _gameFileName = string.Empty;
        private string _gameName = "New game";
        private Size _resolution = new Size(320, 200);
        private GameColorDepth _colorDepth = GameColorDepth.TrueColor;
        private string _gameTextEncoding = Encoding.UTF8.WebName;
        private int _gameFPS = 60;
        private bool _allowRelativeAssetResolution = false;
        private bool _debugMode = true;
        private bool _antiGlideMode = true;
        private bool _walkInLookMode = false;
        private InterfaceDisabledAction _whenInterfaceDisabled = InterfaceDisabledAction.GreyOut;
        private bool _clipGuiControls = true;
        private bool _pixelPerfect = true;
        private bool _autoMoveInWalkMode = true;
        private bool _letterboxMode = false;
        private RenderAtScreenResolution _renderAtScreenRes = RenderAtScreenResolution.UserDefined;
        private string _customDataDir = null;
        private int _splitResources = 0;
        private bool _attachDataToExe = false;
        private bool _turnBeforeWalking = false;
        private bool _turnBeforeFacing = false;
        private bool _scaleMovementSpeedWithMaskRes = false;
        private bool _mouseWheelEnabled = true;
        private RoomTransitionStyle _roomTransition = RoomTransitionStyle.FadeOutAndIn;
        private bool _saveScreenshots = false;
        private SpriteCompression _compressSprites = SpriteCompression.None;
        private bool _optimizeSpriteStorage = true;
        private bool _inventoryCursors = true;
        private bool _handleInvInScript = true;
        private bool _displayMultipleInv = false;
        private ScriptAPIVersion _scriptAPIVersion = ScriptAPIVersion.Highest;
        private ScriptAPIVersion _scriptCompatLevel = ScriptAPIVersion.Highest;
        private ScriptAPIVersion _scriptAPIVersionReal = Utilities.GetActualAPI(ScriptAPIVersion.Highest);
        private ScriptAPIVersion _scriptCompatLevelReal = Utilities.GetActualAPI(ScriptAPIVersion.Highest);
        private bool _enforceObjectScripting = true;
        private bool _leftToRightPrecedence = true;
        private bool _enforceNewStrings = true;
        private bool _enforceNewAudio = true;
        private bool _oldCustomDlgOptsAPI = false;
        private bool _oldKeyHandling = false;
        private bool _oldVoiceClipNaming = false;
        private bool _scaleCharacterSpriteOffsets = true;
        private int _playSoundOnScore = -1;
        private int _dialogOptionsGUI = 0;
        private int _dialogOptionsGap = 0;
        private int _dialogBulletImage = 0;
        private SkipSpeechStyle _skipSpeech = SkipSpeechStyle.MouseOrKeyboardOrTimer;
        private SpeechStyle _speechStyle = SpeechStyle.Lucasarts;
        private int _globalSpeechAnimationDelay = 5;
        private bool _useGlobalSpeechAnimationDelay = false;
        private DialogOptionsNumbering _numberDialogOptions = DialogOptionsNumbering.KeyShortcutsOnly;
        private bool _dialogOptionsBackwards = false;
        private SpeechPortraitSide _speechPortraitSide = SpeechPortraitSide.Left;
        private string _dialogScriptSayFunction;
        private string _dialogScriptNarrateFunction;
        private int _textWindowGUI = 0;
        private bool _alwaysDisplayTextAsSpeech = false;
        private bool _fontsAreHiRes = false;
        private bool _antiAliasFonts = false;
        private FontHeightDefinition _ttfHeightDefinedBy = FontHeightDefinition.NominalHeight;
        private FontMetricsFixup _ttfMetricsFixup = FontMetricsFixup.None;
        private int _thoughtGUI = 0;
        private bool _backwardsText = false;
        private int _uniqueID;
		private Guid _guid;
        private int _totalScore = 0;
        private GUIAlphaStyle _guiAlphaStyle = GUIAlphaStyle.MultiplyTranslucenceSrcBlend;
        private SpriteAlphaStyle _spriteAlphaStyle = SpriteAlphaStyle.Improved;
        private bool _runGameLoopsWhileDialogOptionsDisplayed = true;
        private InventoryHotspotMarker _inventoryHotspotMarker = new InventoryHotspotMarker();
        private bool _useLowResCoordinatesInScript = true;
        private int _defRoomMaskResolution = 1;
        // Game Description fields
		private string _description = string.Empty;
		private DateTime _releaseDate = DateTime.Now;
		private string _genre = DEFAULT_GENRE;
		private string _version = DEFAULT_VERSION;
		private string _developerName = string.Empty;
		private string _developerURL = string.Empty;
		private string _saveGameExtension = string.Empty;
        private string _saveGamesFolderName = string.Empty;
        private int _audioIndexer = AudioClip.FixedIndexBase;
        private string _buildTargets = GetBuildTargetsString(DEFAULT_TARGET_NAMES, false);
        private string _androidApplicationId = "com.mystudio.mygame";
        private int _androidAppVersionCode = 1;
        private string _androidAppVersionName = DEFAULT_VERSION;
        private AndroidBuildFormat _androidBuildFormat = AndroidBuildFormat.ApkEmbedded;

        /// <summary>
        /// Helper function to validate the BuildTargets string. Excludes data file target
        /// from the string unless it is the only target, and optionally checks that the
        /// targets are available for building.
        /// </summary>
        private static string GetBuildTargetsString(string[] targets, bool checkAvailable)
        {
            if (targets.Length == 0) return BuildTargetsInfo.DATAFILE_TARGET_NAME;
            List<string> availableTargets = null; // only retrieve available targets on request
            if (checkAvailable) availableTargets = new List<string>(BuildTargetsInfo.GetAvailableBuildTargetNames());
            List<string> resultTargetList = new List<string>(targets.Length);
            foreach (string targ in targets)
            {
                if ((!checkAvailable) || (availableTargets.Contains(targ)))
                {
                    // only include data file target if it is the only target
                    if ((targ != BuildTargetsInfo.DATAFILE_TARGET_NAME) || (targets.Length == 1))
                    {
                        resultTargetList.Add(targ);
                    }
                }
            }
            return string.Join(BuildTargetUIEditor.Separators[0], resultTargetList.ToArray());
        }

        private static string GetBuildTargetsString(string targetList, bool checkAvailable)
        {
            return GetBuildTargetsString(targetList.Split(BuildTargetUIEditor.Separators,
                StringSplitOptions.RemoveEmptyEntries), checkAvailable);
        }

        private static string GetCustomDirsString(string input)
        {
            if (String.IsNullOrEmpty(input))
                return input;
            var dirs = input.Split(StringListUIEditor.Separators, StringSplitOptions.RemoveEmptyEntries).
                Select(d => d.Trim());
            return string.Join(",", dirs);
        }

        public void GenerateNewGameID()
		{
			_uniqueID = Environment.TickCount;
			_guid = Guid.NewGuid();
		}

        [DisplayName("Game file name")]
        [Description("The game's binary name (the name of the file AGS will create after compiling the game). Leave empty to use project folder's name.")]
        [Category("(Basic properties)")]
        [DefaultValue("")]
        public string GameFileName
        {
            get { return _gameFileName; }
            set
            {
                if (value.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0)
                {
                    throw new ArgumentException("Game file name contains invalid characters");
                }

                if (value.Any(c => c > 127))
                {
                    throw new ArgumentException("Game file name should only contain letters and numbers");
                }

                _gameFileName = value;
            }
        }

        [DisplayName(PROPERTY_GAME_NAME)]
        [Description("The game's name (for display in the title bar)")]
        [Category("(Basic properties)")]
        [DefaultValue("")]
        public string GameName
        {
            get { return _gameName; }
            set { _gameName = value; }
        }

        [Browsable(false)]
        public int UniqueID
        {
            get { return _uniqueID; }
            set { _uniqueID = value; }
        }

		[Browsable(false)]
		[AGSNoSerialize]
		public Guid GUID
		{
			get { return _guid; }
			set { _guid = value; }
		}

		[Browsable(false)]
		public string GUIDAsString
		{
			get { return _guid.ToString("B"); }
			set { _guid = new Guid(value); }
		}

        [DisplayName(PROPERTY_COLOUR_DEPTH)]
        [Description("The colour depth of the game (higher gives better colour quality, but slower performance)")]
        [Category("(Basic properties)")]
        [DefaultValue(GameColorDepth.TrueColor)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public GameColorDepth ColorDepth
        {
            get { return _colorDepth; }
            set { _colorDepth = value; }
        }

        [Browsable(false)]
        [Obsolete("Old Resolution property of Enum type is replaced by CustomResolution of Size type.")]
        public GameResolutions Resolution
        {
            get { return GameResolutions.Custom; }
            set
            {
                switch (value)
                {
                    case GameResolutions.R320x200:
                        CustomResolution = new Size(320, 200); break;
                    case GameResolutions.R320x240:
                        CustomResolution = new Size(320, 240); break;
                    case GameResolutions.R640x400:
                        CustomResolution = new Size(640, 400); break;
                    case GameResolutions.R640x480:
                        CustomResolution = new Size(640, 480); break;
                    case GameResolutions.R800x600:
                        CustomResolution = new Size(800, 600); break;
                    case GameResolutions.R1024x768:
                        CustomResolution = new Size(1024, 768); break;
                    case GameResolutions.R1280x720:
                        CustomResolution = new Size(1280, 720); break;
                    case GameResolutions.Custom:
                        throw new ArgumentOutOfRangeException("You are not allowed to explicitly set Custom resolution type to the deprecated Settings.Resolution property.");
                }
            }
        }

        [DisplayName(PROPERTY_RESOLUTION)]
        [Description("The graphics resolution of the game (higher allows more detail, but slower performance and larger file size)")]
        [Category("(Basic properties)")]
        [EditorAttribute(typeof(CustomResolutionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomResolutionTypeConverter))]
        [RefreshProperties(RefreshProperties.All)]
        public Size CustomResolution
        {
            get { return _resolution; }
            set
            {
                _resolution = value;
                if (LegacyLetterboxResolution == GameResolutions.Custom)
                    LetterboxMode = false;
            }
        }

        [DisplayName(PROPERTY_TEXT_FORMAT)]
        [Category("(Basic properties)")]
        [DefaultValue(DEFAULT_TEXTFORMAT)]
        [TypeConverter(typeof(TextEncodingTypeConverter))]
        public string GameTextEncoding
        {
            get { return _gameTextEncoding; }
            set { _gameTextEncoding = value; }
        }

        [DisplayName("Game Speed (FPS)")]
        [Category("(Basic properties)")]
        [DefaultValue(60)]
        public int GameFPS
        {
            get { return _gameFPS; }
            set
            {
                if (value <= 0) throw new ArgumentOutOfRangeException("Value must be greater than zero. Value was " + value.ToString() + ".");
                _gameFPS = value;
            }
        }

        [DisplayName("Allow relative asset resolutions")]
        [Description("Allow sprites and room backgrounds to define whether they are low- or high-resolution assets. If this does not match the game type then images will be scaled up or down in game." +
            "\nThis option will only be useful when importing games made before AGS 3.1.")]
        [Category("Backwards Compatibility")]
        [DefaultValue(false)]
        public bool AllowRelativeAssetResolutions
        {
            get { return _allowRelativeAssetResolution; }
            set { _allowRelativeAssetResolution = value; }
        }

        /// <summary>
        /// Tells if the game should be considered high-resolution.
        /// For backwards-compatble logic only.
        /// The "high resolution" assumes that game exceeds 320x240 pixels.
        /// </summary>
        [Browsable(false)]
        public bool HighResolution
        {
            get
            {
                return (CustomResolution.Width * CustomResolution.Height) > (320 * 240);
            }
        }

        /// <summary>
        /// Tells which of the legacy resolution types would be represented by
        /// the current game resolution if designed in letterboxed mode.
        /// Returns GameResolutions.Custom if current resolution cannot be used
        /// for letterboxed design.
        /// For backwards-compatible logic only.
        /// </summary>
        [Browsable(false)]
        public GameResolutions LegacyLetterboxResolution
        {
            get
            {
                if (CustomResolution.Width == 320 && CustomResolution.Height == 200)
                    return GameResolutions.R320x200;
                if (CustomResolution.Width == 640 && CustomResolution.Height == 400)
                    return GameResolutions.R640x400;
                return GameResolutions.Custom;
            }
        }

        [Browsable(false)]
        [Obsolete("Boolean CompressSprites is replaced with CompressSpritesType.")]
        public bool CompressSprites
        {
            get { return _compressSprites != SpriteCompression.None; }
            set { _compressSprites = value ? SpriteCompression.RLE : SpriteCompression.None; }
        }

        [DisplayName("Sprite file compression")]
        [Description("Compress the sprite file to reduce its size, at the expense of performance")]
        [Category("Compiler")]
        [DefaultValue(SpriteCompression.None)]
        public SpriteCompression CompressSpritesType
        {
            get { return _compressSprites; }
            set { _compressSprites = value; }
        }

        [DisplayName("Enable sprite storage optimization")]
        [Description("When possible save sprites in game files in a format that requires less storage space. This may reduce the compiled game size on disk, but effect may differ depending on number of colors used in sprites, and other factors.")]
        [Category("Compiler")]
        [DefaultValue(true)]
        public bool OptimizeSpriteStorage
        {
            get { return _optimizeSpriteStorage; }
            set { _optimizeSpriteStorage = value; }
        }

        [DisplayName("Save screenshots in save games")]
        [Description("A screenshot of the player's current position will be saved into the save games")]
        [Category("Saved Games")]
        [DefaultValue(false)]
        public bool SaveScreenshots
        {
            get { return _saveScreenshots; }
            set { _saveScreenshots = value; }
        }

        [DisplayName("Default transition when changing rooms")]
        [Description("This transition will be used when the player exits one room and moves onto another")]
        [Category("Visual")]
        [DefaultValue(RoomTransitionStyle.FadeOutAndIn)]
        public RoomTransitionStyle RoomTransition
        {
            get { return _roomTransition; }
            set { _roomTransition = value; }
        }

        [DisplayName("Enable mouse wheel support")]
        [Description("Enable mouse wheel events to be sent to on_mouse_click")]
        [Category("Backwards Compatibility")]
        [DefaultValue(true)]
        public bool MouseWheelEnabled
        {
            get { return _mouseWheelEnabled; }
            set { _mouseWheelEnabled = value; }
        }

        [DisplayName("Use low-resolution co-ordinates in script")]
        [Description("Backwards-compatible option to always use low-res co-ordinates in the script. This is how previous versions of AGS always worked. WARNING: Changing this setting could break your current scripts.")]
        [Category("Backwards Compatibility")]
        [DefaultValue(false)]
        public bool UseLowResCoordinatesInScript
        {
            get { return _useLowResCoordinatesInScript; }
            set { _useLowResCoordinatesInScript = value; }
        }

        [DisplayName("Characters turn to face direction")]
        [Description("Characters will turn on the spot to face their new direction when FaceLocation is used")]
        [Category("Character behavior")]
        [DefaultValue(false)]
        public bool TurnBeforeFacing
        {
            get { return _turnBeforeFacing; }
            set { _turnBeforeFacing = value; }
        }

        [DisplayName("Characters turn before walking")]
        [Description("Characters will turn on the spot to face their new direction before starting to move")]
        [Category("Character behavior")]
        [DefaultValue(false)]
        public bool TurnBeforeWalking
        {
            get { return _turnBeforeWalking; }
            set { _turnBeforeWalking = value; }
        }

        [DisplayName("Scale movement speed with room's mask resolution")]
        [Description("Character walking and object movement speeds will scale inversely in proportion to the current room's Mask Resolution, for example having 1:2 mask resolution will multiply speed by 2. " +
            "This is a backward compatible setting that should not be enabled without real need.")]
        [Category("Character behavior")]
        [DefaultValue(false)]
        public bool ScaleMovementSpeedWithMaskResolution
        {
            get { return _scaleMovementSpeedWithMaskRes; }
            set { _scaleMovementSpeedWithMaskRes = value; }
        }

        [DisplayName("Package custom data folder(s)")]
        [Description("A comma-separated list of folders; their contents will be added to the game resources")]
        [Category("Compiler")]
        [DefaultValue("")]
        public string CustomDataDir
        {
            get { return _customDataDir; }
            set { _customDataDir = GetCustomDirsString(value); }
        }

        [DisplayName("Split resource files into X MB-sized chunks")]
        [Description("Resources will be split into files sized with the number of megabytes you enter here (0 to disable)")]
        [Category("Compiler")]
        [DefaultValue(0)]
        public int SplitResources
        {
            get { return _splitResources; }
            set { _splitResources = value; }
        }

        [DisplayName("Attach game data to exe (Windows only)")]
        [Description("Main game data will be attached to game exe. Otherwise it will be in a separate file called GAMENAME.ags")]
        [Category("Compiler")]
        [DefaultValue(false)]
        public bool AttachDataToExe
        {
            get { return _attachDataToExe; }
            set { _attachDataToExe = value; }
        }

        [DisplayName("Old-style letterbox mode")]
        [Description("Game will run at 320x240 or 640x480 with top and bottom black borders to give a square aspect ratio. Not recommended unless importing an old project.")]
        [Category("Backwards Compatibility")]
        [DefaultValue(false)]
        public bool LetterboxMode
        {
            get { return _letterboxMode; }
            set
            {
                if (value == false || LegacyLetterboxResolution != GameResolutions.Custom)
                    _letterboxMode = value;
            }
        }

        [DisplayName("Automatically move the player in Walk mode")]
        [Description("When the player clicks somewhere in Walk mode, the player character will be sent there rather than processing it as an interaction")]
        [Category("Character behavior")]
        [DefaultValue(true)]
        public bool AutoMoveInWalkMode
        {
            get { return _autoMoveInWalkMode; }
            set { _autoMoveInWalkMode = value; }
        }

        [DisplayName("Pixel-perfect click detection")]
        [Description("When the player clicks the mouse, a pixel-perfect check will be used to decide what they clicked on (if false, a simple rectangular check is used)")]
        [Category("Visual")]
        [DefaultValue(true)]
        public bool PixelPerfect
        {
            get { return _pixelPerfect; }
            set { _pixelPerfect = value; }
        }

        [DisplayName("When player interface is disabled, GUIs should")]
        [Description("When the player interface is disabled (eg. during a cutscene), GUIs on screen will take this action")]
        [Category("Visual")]
        [DefaultValue(InterfaceDisabledAction.GreyOut)]
        [TypeConverter(typeof(EnumTypeConverter))]
		public InterfaceDisabledAction WhenInterfaceDisabled
        {
            get { return _whenInterfaceDisabled; }
            set { _whenInterfaceDisabled = value; }
        }

        [DisplayName(PROPERTY_CLIPGUICONTROLS)]
        [Description("GUI controls will clip their graphical contents, such as text, preventing it from being drawn outside of their rectangle." +
            "\nNOTE: Button images are clipped using individual button's property.")]
        [Category("Visual")]
        [DefaultValue(true)]
        public bool ClipGUIControls
        {
            get { return _clipGuiControls; }
            set { _clipGuiControls = value; }
        }

        [DisplayName("GUI alpha rendering style")]
        [Description("When using 32-bit alpha-channel images, should GUIs be drawn with the new improved alpha method, or the backwards-compatible method?")]
        [Category("Visual")]
        [DefaultValue(GUIAlphaStyle.MultiplyTranslucenceSrcBlend)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public GUIAlphaStyle GUIAlphaStyle
        {
            get { return _guiAlphaStyle; }
            set { _guiAlphaStyle = value; }
        }

        [DisplayName("Sprite alpha rendering style")]
        [Description("When using 32-bit alpha-channel images, should sprites be drawn with the new improved alpha method, or the backwards-compatible method?")]
        [Category("Visual")]
        [DefaultValue(SpriteAlphaStyle.Improved)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public SpriteAlphaStyle SpriteAlphaStyle
        {
            get { return _spriteAlphaStyle; }
            set { _spriteAlphaStyle = value; }
        }

        [DisplayName("Run game loops while dialog options are displayed")]
        [Description("Whether to allow game animations to continue in the background while waiting for the player to select a dialog option")]
        [Category("Dialog")]
        [DefaultValue(true)]
        public bool RunGameLoopsWhileDialogOptionsDisplayed
        {
            get { return _runGameLoopsWhileDialogOptionsDisplayed; }
            set { _runGameLoopsWhileDialogOptionsDisplayed = value; }
        }

        [AGSNoSerialize]
        [Browsable(false)]
        public InventoryHotspotMarker InventoryHotspotMarker
        {
            get { return _inventoryHotspotMarker; }
            set { _inventoryHotspotMarker = value; }
        }

        [DisplayName("Inventory item cursor hotspot marker")]
        [Description("AGS can automatically add a marker to inventory item cursors to help the player see where the active hotspot is on the cursor")]
        [Category("Inventory")]
        [DefaultValue(InventoryHotspotMarkerStyle.None)]
        [TypeConverter(typeof(EnumTypeConverter))]
        [RefreshProperties(RefreshProperties.All)]
        public InventoryHotspotMarkerStyle InventoryHotspotMarkerStyle
        {
            get { return _inventoryHotspotMarker.Style; }
            set { _inventoryHotspotMarker.Style = value; }
        }

        [DisplayName("Inventory item cursor hotspot marker sprite")]
        [Description("The sprite to draw on top of the inventory cursor at the hotspot position")]
        [Category("Inventory")]
        [DefaultValue(0)]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int InventoryHotspotMarkerSprite
        {
            get { return _inventoryHotspotMarker.Image; }
            set { _inventoryHotspotMarker.Image = value; }
        }

        [DisplayName("Inventory item cursor hotspot marker dot colour")]
        [Description("The AGS Colour Number of the colour of the central dot of the crosshair")]
        [Category("Inventory")]
        [DefaultValue(0)]
        public int InventoryHotspotMarkerDotColor
        {
            get { return _inventoryHotspotMarker.DotColor; }
            set { _inventoryHotspotMarker.DotColor = value; }
        }

        [DisplayName("Inventory item cursor hotspot marker crosshair colour")]
        [Description("The AGS Colour Number of the colour of the crosshair bars")]
        [Category("Inventory")]
        [DefaultValue(0)]
        public int InventoryHotspotMarkerCrosshairColor
        {
            get { return _inventoryHotspotMarker.CrosshairColor; }
            set { _inventoryHotspotMarker.CrosshairColor = value; }
        }

        [DisplayName("Automatically walk to hotspots in Look mode")]
        [Description("Whenever the player clicks somewhere in Look mode, the player character will automatically be moved there")]
        [Category("Character behavior")]
        [DefaultValue(false)]
        public bool WalkInLookMode
        {
            get { return _walkInLookMode; }
            set { _walkInLookMode = value; }
        }

        [Browsable(false)]
        public bool AntiGlideMode
	    {
		    get { return _antiGlideMode;}
		    set { _antiGlideMode = value;}
	    }

        [DisplayName("Scale Character sprite offsets")]
        [Description("Scale sprite offsets along with the character sprite, such as Character.z property, and offsets set by Character.LockViewOffset()")]
        [Category("Character behavior")]
        [DefaultValue(true)]
        public bool ScaleCharacterSpriteOffsets
        {
            get { return _scaleCharacterSpriteOffsets; }
            set { _scaleCharacterSpriteOffsets = value; }
        }

        [DisplayName("Enable Debug Mode")]
        [Description("Enable various debugging keys that help you while developing your game")]
        [Category("Compiler")]
        [DefaultValue(false)]
        public bool DebugMode
        {
            get { return _debugMode; }
            set { _debugMode = value; }
        }

        [DisplayName("Use selected inventory graphic for cursor")]
        [Description("When in Use Inventory mode, the mouse cursor will be the selected inventory item rather than a fixed cursor")]
        [Category("Inventory")]
        [DefaultValue(true)]
        public bool InventoryCursors
        {
            get { return _inventoryCursors; }
            set { _inventoryCursors = value; }
        }

        [DisplayName("Handle inventory window clicks in script")]
        [Description("When the mouse is clicked in an inventory window, on_mouse_click is called rather than using AGS's default processing")]
        [Category("Inventory")]
        [DefaultValue(true)]
        public bool HandleInvClicksInScript
        {
            get { return _handleInvInScript; }
            set { _handleInvInScript = value; }
        }

        [DisplayName("Display multiple icons for multiple items")]
        [Description("If the player has two or more of an item, it will be displayed multiple times in the inventory window")]
        [Category("Inventory")]
        [DefaultValue(false)]
        public bool DisplayMultipleInventory
        {
            get { return _displayMultipleInv; }
            set { _displayMultipleInv = value; }
        }

        [DisplayName("Script API version")]
        [Description("Choose the version of the script API to use in your scripts")]
        [Category("Backwards Compatibility")]
        [DefaultValue(ScriptAPIVersion.Highest)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public ScriptAPIVersion ScriptAPIVersion
        {
            get { return _scriptAPIVersion; }
            set
            {
                _scriptAPIVersion = value;
                _scriptAPIVersionReal = Utilities.GetActualAPI(_scriptAPIVersion);
                if (_scriptAPIVersion < _scriptCompatLevel)
                    ScriptCompatLevel = _scriptAPIVersion;
            }
        }

        [DisplayName("Script compatibility level")]
        [Description("Lowest version of the obsoleted script API to support in your script")]
        [Category("Backwards Compatibility")]
        [DefaultValue(ScriptAPIVersion.Highest)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public ScriptAPIVersion ScriptCompatLevel
        {
            get { return _scriptCompatLevel; }
            set
            {
                _scriptCompatLevel = value;
                _scriptCompatLevelReal = Utilities.GetActualAPI(_scriptCompatLevel);
                if (_scriptCompatLevel > _scriptAPIVersion)
                    ScriptAPIVersion = _scriptCompatLevel;
            }
        }

        /// <summary>
        /// Returns actual current API version, even if project is set to use Highest
        /// </summary>
        [AGSNoSerialize]
        [Browsable(false)]
        public ScriptAPIVersion ScriptAPIVersionReal
        {
            get { return _scriptAPIVersionReal; }
        }

        /// <summary>
        /// Returns actual current API compat level, even if project is set to use Highest
        /// </summary>
        [AGSNoSerialize]
        [Browsable(false)]
        public ScriptAPIVersion ScriptCompatLevelReal
        {
            get { return _scriptCompatLevelReal; }
        }

        [DisplayName("Left-to-right operator precedence")]
        [Description("Expressions like (5 - 3 - 2) will equal 0 rather than 4")]
        [Category("Backwards Compatibility")]
        [DefaultValue(true)]
        public bool LeftToRightPrecedence
        {
            get { return _leftToRightPrecedence; }
            set { _leftToRightPrecedence = value; }
        }

        [DisplayName("Enforce post-2.62 scripting")]
        [Description("Disable old-style AGS 2.62 script commands")]
        [Category("Backwards Compatibility")]
        [DefaultValue(true)]
        public bool EnforceObjectBasedScript
        {
            get { return _enforceObjectScripting; }
            set { _enforceObjectScripting = value; }
        }

        [DisplayName("Enforce new-style strings")]
        [Description("Disable old-style strings from AGS 2.70 and before")]
        [Category("Backwards Compatibility")]
        [DefaultValue(true)]
        public bool EnforceNewStrings
        {
            get { return _enforceNewStrings; }
            set { _enforceNewStrings = value; }
        }

        [DisplayName("Enforce new-style audio scripting")]
        [Description("Disable old-style audio commands like PlaySound, IsChannelPlaying, etc")]
        [Category("Backwards Compatibility")]
        [DefaultValue(true)]
        public bool EnforceNewAudio
        {
            get { return _enforceNewAudio; }
            set { _enforceNewAudio = value; }
        }

        [DisplayName("Use old-style custom dialog options API")]
        [Description("Use pre-3.4.0 callback functions to handle custom dialog options GUI")]
        [Category("Backwards Compatibility")]
        [DefaultValue(false)]
        public bool UseOldCustomDialogOptionsAPI
        {
            get { return _oldCustomDlgOptsAPI; }
            set { _oldCustomDlgOptsAPI = value; }
        }

        [DisplayName("Use old-style keyboard handling")]
        [Description("Use pre-unicode mode key codes in 'on_key_press' function, where regular keys were merged with Ctrl and Alt modifiers.")]
        [Category("Backwards Compatibility")]
        [DefaultValue(false)]
        public bool UseOldKeyboardHandling
        {
            get { return _oldKeyHandling; }
            set { _oldKeyHandling = value; }
        }

        [DisplayName("Use old-style voice clip naming rule")]
        [Description("Define voice clip name using only the first 4 letters from a Character's script name.")]
        [Category("Backwards Compatibility")]
        [DefaultValue(false)]
        public bool UseOldVoiceClipNaming
        {
            get { return _oldVoiceClipNaming; }
            set { _oldVoiceClipNaming = value; }
        }

        [DisplayName("Play sound when the player gets points")]
        [Description("This sound number will be played whenever the player scores points (0 to disable)")]
        [Category("Score")]
        [DefaultValue(AudioClip.FixedIndexNoValue)]
        [TypeConverter(typeof(AudioClipTypeConverter))]
        public int PlaySoundOnScore
        {
            get { return _playSoundOnScore; }
            set { _playSoundOnScore = value; }
        }

        [Obsolete]
        [Browsable(false)]
        // NOTE: have to keep setter here because we load old games before upgrading them
        public CrossfadeSpeed CrossfadeMusic { get; set; }

        [DisplayName("Use GUI for dialog options")]
        [Description("Dialog options can be drawn on a normal or textwindow GUI (0 to just draw at bottom of screen instead)")]
        [Category("Dialog")]
        [DefaultValue(0)]
        [TypeConverter(typeof(GUIIndexTypeConverter))]
        public int DialogOptionsGUI
        {
            get { return _dialogOptionsGUI; }
            set { _dialogOptionsGUI = value; }
        }

        [DisplayName("Gap between dialog options (in pixels)")]
        [Description("Gap between dialog options (in pixels)")]
        [Category("Dialog")]
        [DefaultValue(0)]
        public int DialogOptionsGap
        {
            get { return _dialogOptionsGap; }
            set { _dialogOptionsGap = value; }
        }

        [DisplayName("Dialog bullet point image")]
        [Description("Sprite to use as a bullet point before each dialog option (0 for none)")]
        [Category("Dialog")]
        [DefaultValue(0)]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int DialogOptionsBullet
        {
            get { return _dialogBulletImage; }
            set { _dialogBulletImage = value; }
        }

        [DisplayName("Allow speech to be skipped by which events")]
        [Description("Determines whether the mouse, keyboard or the auto-remove timer can remove the current speech line")]
        [Category("Dialog")]
        [DefaultValue(SkipSpeechStyle.MouseOrKeyboardOrTimer)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public SkipSpeechStyle SkipSpeech
        {
            get { return _skipSpeech; }
            set { _skipSpeech = value; }
        }

        [DisplayName("Speech style")]
        [Description("Determines whether speech is displayed in lucasarts or sierra-style")]
        [Category("Dialog")]
        [DefaultValue(SpeechStyle.Lucasarts)]
        public SpeechStyle SpeechStyle
        {
            get { return _speechStyle; }
            set { _speechStyle = value; }
        }

        [DisplayName("Use game-wide speech animation delay")]
        [Description("Determines whether to use game-wide speech animation delay or use the individual character settings.")]
        [Category("Dialog")]
        [DefaultValue(false)]
        [RefreshProperties(RefreshProperties.All)]
        public bool UseGlobalSpeechAnimationDelay
        {
            get { return _useGlobalSpeechAnimationDelay; }
            set { _useGlobalSpeechAnimationDelay = value; }
        }

        [DisplayName("Game-wide speech animation delay")]
        [Description("Sets the Speech.GlobalSpeechAnimationDelay setting to determine the animation speed of character speech; individual character SpeechAnimationDelay settings will be ignored.")]
        [Category("Dialog")]
        [DefaultValue(5)]
        public int GlobalSpeechAnimationDelay
        {
            get { return _globalSpeechAnimationDelay; }

            set
            {
                if (value < 0) throw new ArgumentOutOfRangeException("Value must be greater than or equal to zero. Value was " + value.ToString() + ".");
                _globalSpeechAnimationDelay = value;
            }
        }

        [Browsable(false)]
        [Obsolete("LegacySpeechAnimationSpeed has been replaced by UseGlobalSpeechAnimationDelay.")]
        public bool LegacySpeechAnimationSpeed
        {
            get { return UseGlobalSpeechAnimationDelay; }
            set { UseGlobalSpeechAnimationDelay = value; }
        }

        [DisplayName("Number dialog options")]
        [Description("Dialog options become numbered bullet points, and the numeric keys can be used to select them")]
        [Category("Dialog")]
        [DefaultValue(DialogOptionsNumbering.None)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public DialogOptionsNumbering NumberDialogOptions
        {
            get { return _numberDialogOptions; }
            set { _numberDialogOptions = value; }
        }

        [DisplayName("Print dialog options upwards")]
        [Description("The first dialog option will be at the bottom, and the last at the top")]
        [Category("Dialog")]
        [DefaultValue(false)]
        public bool DialogOptionsBackwards
        {
            get { return _dialogOptionsBackwards; }
            set { _dialogOptionsBackwards = value; }
        }

        [DisplayName("Sierra-style portrait location")]
        [Description("Determines whether to draw the Sierra-style portrait on the left or right of the screen")]
        [Category("Dialog")]
        [DefaultValue(SpeechPortraitSide.Left)]
        public SpeechPortraitSide SpeechPortraitSide
        {
            get { return _speechPortraitSide; }
            set { _speechPortraitSide = value; }
        }

        [DisplayName("Custom Say function in dialog scripts")]
        [Description("Sets which function name to use in place of character.Say when running dialog scripts. Note that it must be an extension function of a Character class. Leave empty to use default (Character.Say).")]
        [Category("Dialog")]
        [DefaultValue("")]
        public string DialogScriptSayFunction
        {
            get { return _dialogScriptSayFunction; }
            set { _dialogScriptSayFunction = value; }
        }

        [DisplayName("Custom Narrate function in dialog scripts")]
        [Description("Sets which function name to use in place of narrator's speech when running dialog scripts. Note that it must be either regular function or a static struct function. Leave empty to use default (Display).")]
        [Category("Dialog")]
        [DefaultValue("")]
        public string DialogScriptNarrateFunction
        {
            get { return _dialogScriptNarrateFunction; }
            set { _dialogScriptNarrateFunction = value; }
        }

        [DisplayName("Custom text-window GUI")]
        [Description("Sets which text-window GUI is used for normal text in the game. You must use the GUI number, not name. You can't use GUI 0 for this, because 0 means that AGS will use its built-in text window instead.")]
        [Category("Text output")]
        [DefaultValue(0)]
        [TypeConverter(typeof(GUITextWindowIndexTypeConverter))]
        public int TextWindowGUI
        {
            get { return _textWindowGUI; }
            set { _textWindowGUI = value; }
        }

        [DisplayName("Always display text as speech")]
        [Description("Enables the lucasarts-style option where all default text in the game is spoken by the main character")]
        [Category("Text output")]
        [DefaultValue(false)]
        public bool AlwaysDisplayTextAsSpeech
        {
            get { return _alwaysDisplayTextAsSpeech; }
            set { _alwaysDisplayTextAsSpeech = value; }
        }

        [Browsable(false)]
        [Obsolete("FontsForHiRes property is replaced with individual SizeMultiplier in each font.")]
        public bool FontsForHiRes
        {
            get { return _fontsAreHiRes; }
            set { _fontsAreHiRes = value; }
        }

        [DisplayName(PROPERTY_ANTI_ALIAS_FONTS)]
        [Description("True-type fonts will be anti-aliased in-game, but there is a performance penalty")]
        [Category("Text output")]
        [DefaultValue(false)]
        public bool AntiAliasFonts
        {
            get { return _antiAliasFonts; }
            set { _antiAliasFonts = value; }
        }

        [DisplayName(PROPERTY_FONT_HEIGHT_IN_LOGIC)]
        [Description("How the true-type font height will be defined whenever it is required by the script or game logic.")]
        [Category("Text output")]
        [DefaultValue(FontHeightDefinition.NominalHeight)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public FontHeightDefinition TTFHeightDefinedBy
        {
            get { return _ttfHeightDefinedBy; }
            set { _ttfHeightDefinedBy = value; }
        }

        [DisplayName("TTF fonts adjustment defaults")]
        [Description("Automatic adjustment of the true-type font metrics; primarily for backward compatibility." +
            "\nThis option will be used as a default value for each new imported font, but you may also customize it in the Font's properties.")]
        [Category("Text output")]
        [DefaultValue(FontMetricsFixup.None)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public FontMetricsFixup TTFMetricsFixup
        {
            get { return _ttfMetricsFixup; }
            set { _ttfMetricsFixup = value; }
        }

        [DisplayName("Custom thought bubble GUI")]
        [Description("Character.Think will use this custom text-window GUI")]
        [Category("Text output")]
        [DefaultValue(0)]
        [TypeConverter(typeof(GUITextWindowIndexTypeConverter))]
        public int ThoughtGUI
        {
            get { return _thoughtGUI; }
            set { _thoughtGUI = value; }
        }

        [DisplayName("Write game text Right-to-Left")]
        [Description("The game will draw text right-to-left, used by languages such as Hebrew")]
        [Category("Text output")]
        [DefaultValue(false)]
        public bool BackwardsText
        {
            get { return _backwardsText; }
            set { _backwardsText = value; }
        }

        [DisplayName("Maximum possible score")]
        [Description("The maximum score that the player can achieve (displayed by @TOTALSCORE@ on GUI labels)")]
        [Category("Score")]
        [DefaultValue(0)]
        public int MaximumScore
        {
            get { return _totalScore; }
            set { _totalScore = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public bool HasMODMusic { get; }

        [DisplayName("Default mask resolution")]
        [Description("What resolution do room region masks have relative to the room size")]
        [Category("Rooms")]
        [DefaultValue(1)]
        [TypeConverter(typeof(RoomMaskResolutionTypeConverter))]
        public int DefaultRoomMaskResolution
        {
            get { return _defRoomMaskResolution; }
            set { _defRoomMaskResolution = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public bool GameExplorerEnabled { get; }

		[DisplayName("Game description")]
		[Description("The game's description")]
		[Category("(Information)")]
        [DefaultValue("")]
        public string Description
		{
			get { return _description; }
			set { _description = value; }
		}

		[DisplayName("Release date")]
		[Description("Date on which this game is first released")]
        [Category("(Information)")]
        public DateTime ReleaseDate
		{
			get { return _releaseDate; }
			set { _releaseDate = value; }
		}

		[DisplayName("Genre")]
		[Description("The game's genre")]
        [Category("(Information)")]
        [DefaultValue("")]
        public string Genre
		{
			get { return _genre; }
			set { _genre = value; }
		}

		[DisplayName("Version")]
		[Description("Current game version")]
        [Category("(Information)")]
        [DefaultValue("")]
        public string Version
		{
			get { return _version; }
			set 
            {
                // The GDF Schema requires this type of version number
				if (!Regex.IsMatch(value, REGEX_FOUR_PART_VERSION))
                {
                    throw new ArgumentException("Version must be of form  a.b.c.d");
                }
                _version = value; 
            }
		}

        [Obsolete]
        [Browsable(false)]
        public int WindowsExperienceIndex { get; }

		[DisplayName("Developer name")]
		[Description("The name of the game developer (you!). On Windows assigned to the game exe properties.")]
        [Category("(Basic properties)")]
        [DefaultValue("")]
        public string DeveloperName
		{
			get { return _developerName; }
			set { _developerName = value; }
		}

		[DisplayName("Developer website")]
		[Description("URL of game developer's website")]
        [Category("(Information)")]
        [DefaultValue("")]
        public string DeveloperURL
		{
			get { return _developerURL; }
			set { _developerURL = value; }
		}

        [Obsolete]
        [Browsable(false)]
        public bool EnhancedSaveGames { get; }

		[DisplayName("Save game file extension")]
		[Description("The file extension to give save game files")]
		[Category("Saved Games")]
        [DefaultValue("")]
        public string SaveGameFileExtension
		{
			get { return _saveGameExtension; }
			set 
			{
				// Limit what they can set it to
				if ((value.Length > 0) && (!Regex.IsMatch(value, @"^([a-zA-Z0-9]+)$")))
				{
					throw new ArgumentException("Save game extension can only contain letters and numbers");
				}
				if (value.Length > 19)
				{
					throw new ArgumentException("Save game extension cannot be longer than 19 letters");
				}
				if ((value.Length > 0) && (value.Length < 5))
				{
					throw new ArgumentException("Save game extension must be at least 5 letters long");
				}
				_saveGameExtension = value; 
			}
		}

        [DisplayName("Save games folder name")]
        [Description("If set, creates a folder of this name inside the player's Saved Games folder to store the save games in.")]
        [Category("Saved Games")]
        [DefaultValue("")]
        public string SaveGameFolderName
        {
            get { return _saveGamesFolderName; }
            set
            {
                // Strip out any invalid path characters
                char[] invalidChars = System.IO.Path.GetInvalidFileNameChars();
                _saveGamesFolderName = string.Empty;
                foreach (char thisChar in value)
                {
                    bool charValid = true;
                    foreach (char invalidChar in invalidChars)
                    {
                        if (invalidChar == thisChar)
                        {
                            charValid = false;
                        }
                    }
                    if (charValid)
                    {
                        _saveGamesFolderName += thisChar;
                    }
                }
                _saveGamesFolderName = _saveGamesFolderName.Trim();
            }
        }

        [Obsolete]
        [Browsable(false)]
        public bool BinaryFilesInSourceControl { get; }

        // This is used to assign "fixed indices" to audio clips, which work as a stable reference the clip,
        // regardless of any clip list rearrangements.
        [Browsable(false)]
        public int AudioIndexer
        {
            get { return _audioIndexer; }
            set { _audioIndexer = value; }
        }

        [DisplayName(PROPERTY_BUILD_TARGETS)]
        [Description("Sets the platforms to compile your game for.")]
        [Category("Compiler")]
        [Editor(typeof(BuildTargetUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string BuildTargets
        {
            get { return _buildTargets; }

            set
            {
                _buildTargets = GetBuildTargetsString(value, true);
            }
        }

        [DisplayName("Render sprites at screen resolution")]
        [Description("When drawing zoomed character and object sprites, AGS will take advantage of higher runtime resolution to give scaled images more detail, than it would be possible if the game was displayed in its native resolution. The effect is stronger for low-res games. Keep disabled for pixel-perfect output. Currently supported only by Direct3D and OpenGL renderers.")]
        [Category("Visual")]
        [DefaultValue(RenderAtScreenResolution.UserDefined)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public RenderAtScreenResolution RenderAtScreenResolution
        {
            get { return _renderAtScreenRes; }
            set { _renderAtScreenRes = value; }
        }

        [DisplayName(PROPERTY_ANDROID_APPLICATION_ID)]
        [Description("The application ID, used in app store. Also called package name, it's usually looks like com.mystudio.mygame, and it's used in store URLs. It must have at least two segments (one or more dots), and each segment must start with a letter.")]
        [Category("Android")]
        [DefaultValue("com.mystudio.mygame")]
        public string AndroidApplicationId
        {
            get { return _androidApplicationId; }
            set
            {
                if (string.IsNullOrEmpty(value))
                {
                    throw new ArgumentException("Application ID cannot be empty");
                }
                if ((value.Length > 0) && (!Regex.IsMatch(value, @"^([a-zA-Z0-9\.\ ]+)$")))
                {
                    throw new ArgumentException("Application ID can only contain letters, number and dots.");
                }
                value = value.Replace(" ", "").ToLower().Trim();
                _androidApplicationId = value; 
            }
        }

        [DisplayName(PROPERTY_ANDROID_APP_VERSION_CODE)]
        [Description("The version ID used by Google Play Store and others - positive integer, must be different from the last one uploaded.")]
        [Category("Android")]
        [DefaultValue("1")]
        public int AndroidAppVersionCode
        {
            get { return _androidAppVersionCode; }
            set { _androidAppVersionCode = value; }
        }

        [DisplayName(PROPERTY_ANDROID_APP_VERSION_NAME)]
        [Description("The version name visible to users in the stores, this can be anything. Leave empty to use the same version you set in desktop platforms.")]
        [Category("Android")]
        [DefaultValue("")]
        public string AndroidAppVersionName
        {
            get { return _androidAppVersionName; }
            set { _androidAppVersionName = value; }
        }

        [DisplayName("Build Format")]
        [Description("Use embedded formats when testing locally. Google Play only accepts AAB.")]
        [Category("Android")]
        [DefaultValue(AndroidBuildFormat.Aab)]
        public AndroidBuildFormat AndroidBuildFormat
        {
            get { return _androidBuildFormat; }
            set { _androidBuildFormat = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public BuildConfiguration LastBuildConfiguration { get; }
        [Obsolete]
        [Browsable(false)]
        public GraphicsDriver GraphicsDriver { get; }


        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

        public void FromXml(XmlNode node)
        {
            _totalScore = 0;
            _guid = Guid.Empty;
            _description = string.Empty;
            _releaseDate = DateTime.Now;
            _genre = DEFAULT_GENRE;
            _version = DEFAULT_VERSION;
            _developerName = string.Empty;
            _developerURL = string.Empty;
            _saveGameExtension = string.Empty;
            _saveGamesFolderName = null;
            _guiAlphaStyle = GUIAlphaStyle.Classic;
            _spriteAlphaStyle = SpriteAlphaStyle.Classic;
            _runGameLoopsWhileDialogOptionsDisplayed = false;
            _inventoryHotspotMarker = new InventoryHotspotMarker();
            _useLowResCoordinatesInScript = true;
            _audioIndexer = AudioClip.FixedIndexBase;
            _enforceNewAudio = false;
            _gameFileName = string.Empty;

            SerializeUtils.DeserializeFromXML(this, node);

            if (_guid == Guid.Empty)
            {
                _guid = Guid.NewGuid();
            }
            if (_saveGamesFolderName == null)
            {
                this.SaveGameFolderName = _gameName;
            }
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
            // Show/hide the inventory marker details depending on whether it's
            // enabled or not.
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, attributes, true);
            List<PropertyDescriptor> wantProperties = new List<PropertyDescriptor>();
            foreach (PropertyDescriptor property in properties)
            {
                bool wantThisProperty = true;
                if ((_inventoryHotspotMarker.Style != InventoryHotspotMarkerStyle.Sprite) &&
                    (property.Name == "InventoryHotspotMarkerSprite"))
                {
                    wantThisProperty = false;
                }
                else if ((_inventoryHotspotMarker.Style != InventoryHotspotMarkerStyle.Crosshair) &&
                    ((property.Name == "InventoryHotspotMarkerDotColor") ||
                     (property.Name == "InventoryHotspotMarkerCrosshairColor")))
                {
                    wantThisProperty = false;
                }
                // TODO: this must be done other way; leaving for backwards-compatibility only
                else if (property.Name == "LetterboxMode" &&
                    LegacyLetterboxResolution == GameResolutions.Custom)
                {
                    // Only show letterbox option for 320x200 and 640x400 games
                    wantThisProperty = false;
                }
                else if ((property.Name == "GlobalSpeechAnimationDelay") && (!UseGlobalSpeechAnimationDelay))
                {
                    wantThisProperty = false;
                }

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
