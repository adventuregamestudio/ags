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
        public const string PROPERTY_TEXT_LANGUAGE = "Text language";
        public const string PROPERTY_ANTI_ALIAS_FONTS = "Anti-alias TTF fonts";
        public const string PROPERTY_FONT_HEIGHT_IN_LOGIC = "TTF fonts height used in the game logic";
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
        private string _gameTextLanguage = "en_US";
        private bool _debugMode = true;
        private bool _antiGlideMode = true;
        private bool _walkInLookMode = false;
        private InterfaceDisabledAction _whenInterfaceDisabled = InterfaceDisabledAction.GreyOut;
        private bool _clipGuiControls = true;
        private bool _pixelPerfect = true;
        private bool _autoMoveInWalkMode = true;
        private RenderAtScreenResolution _renderAtScreenRes = RenderAtScreenResolution.UserDefined;
        private string _customDataDir = null;
        private int _splitResources = 0;
        private bool _attachDataToExe = false;
        private bool _turnBeforeWalking = true;
        private bool _turnBeforeFacing = true;
        private RoomTransitionStyle _roomTransition = RoomTransitionStyle.FadeOutAndIn;
        private bool _saveScreenshots = false;
        private SpriteCompression _compressSprites = SpriteCompression.None;
        private bool _optimizeSpriteStorage = true;
        private bool _experimentalCompiler = false;
        private bool _inventoryCursors = true;
        private bool _handleInvInScript = false;
        private bool _displayMultipleInv = false;
        private ScriptAPIVersion _scriptAPIVersion = ScriptAPIVersion.Highest;
        private ScriptAPIVersion _scriptCompatLevel = ScriptAPIVersion.Highest;
        private ScriptAPIVersion _scriptAPIVersionReal = Utilities.GetActualAPI(ScriptAPIVersion.Highest);
        private ScriptAPIVersion _scriptCompatLevelReal = Utilities.GetActualAPI(ScriptAPIVersion.Highest);
        private bool _oldKeyHandling = false;
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
        private bool _antiAliasFonts = false;
        private FontHeightDefinition _ttfHeightDefinedBy = FontHeightDefinition.NominalHeight;
        private FontMetricsFixup _ttfMetricsFixup = FontMetricsFixup.None;
        private int _thoughtGUI = 0;
        private bool _backwardsText = false;
        private int _uniqueID;
		private Guid _guid;
        private int _totalScore = 0;
        private bool _runGameLoopsWhileDialogOptionsDisplayed = false;
        private InventoryHotspotMarker _inventoryHotspotMarker = new InventoryHotspotMarker();
        private int _defRoomMaskResolution = 1;
        // Description fields (previously: made for Windows Game Explorer)
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
        public string GameFileName
        {
            get { return _gameFileName; }
            set
            {
                if (string.IsNullOrEmpty(value))
                {
                    throw new ArgumentException("Game file name cannot be empty");
                }

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
            set { _resolution = value; }
        }

        [DisplayName(PROPERTY_TEXT_FORMAT)]
        [Category("(Basic properties)")]
        [TypeConverter(typeof(TextEncodingTypeConverter))]
        public string GameTextEncoding
        {
            get { return _gameTextEncoding; }
            set { _gameTextEncoding = value; }
        }

        [DisplayName(PROPERTY_TEXT_LANGUAGE)]
        [Description("Defines current language for translations purposes (use standard locale strings, like 'en', 'en_US', etc)")]
        [DefaultValue("en_US")]
        [Category("(Basic properties)")]
        public string GameTextLanguage
        {
            get { return _gameTextLanguage; }
            set { _gameTextLanguage = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public bool AllowRelativeAssetResolutions { get { return false; } }
        
        /// <summary>
        /// Tells if the game should be considered high-resolution.
        /// For backwards-compatble logic only.
        /// The "high resolution" assumes that game exceeds 320x240 pixels.
        /// </summary>
        [Browsable(false)]
        // CLNUP I think this also tells the editor if scaling up GUIs in low res games
        public bool HighResolution
        {
            get
            {
                return (CustomResolution.Width * CustomResolution.Height) > (320 * 240);
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
        [DefaultValue(false)]
        [Category("Compiler")]
        public SpriteCompression CompressSpritesType
        {
            get { return _compressSprites; }
            set { _compressSprites = value; }
        }
        
        [DisplayName("Enable sprite storage optimization")]
        [Description("When possible save sprites in game files in a format that requires less storage space. This may reduce the compiled game size on disk, but effect may differ depending on number of colors used in sprites, and other factors.")]
        [DefaultValue(true)]
        [Category("Compiler")]
        public bool OptimizeSpriteStorage
        {
            get { return _optimizeSpriteStorage; }
            set { _optimizeSpriteStorage = value; }
        }

        [DisplayName("Use extended script compiler")]
        [Description("The extended script compiler has more features but may still have some bugs, too")]
        [DefaultValue(false)]
        [Category("Compiler")]
        public bool ExtendedCompiler
        {
            get { return _experimentalCompiler; }
            set { _experimentalCompiler = value; }
        }

        [DisplayName("Save screenshots in save games")]
        [Description("A screenshot of the player's current position will be saved into the save games")]
        [DefaultValue(false)]
        [Category("Saved Games")]
        public bool SaveScreenshots
        {
            get { return _saveScreenshots; }
            set { _saveScreenshots = value; }
        }

        [DisplayName("Default transition when changing rooms")]
        [Description("This transition will be used when the player exits one room and moves onto another")]
        [DefaultValue(RoomTransitionStyle.FadeOutAndIn)]
        [Category("Visual")]
        public RoomTransitionStyle RoomTransition
        {
            get { return _roomTransition; }
            set { _roomTransition = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public bool MouseWheelEnabled { get { return true; } }

        [DisplayName("Characters turn to face direction")]
        [Description("Characters will turn on the spot to face their new direction when FaceLocation is used")]
        [DefaultValue(true)]
        [Category("Character behavior")]
        public bool TurnBeforeFacing
        {
            get { return _turnBeforeFacing; }
            set { _turnBeforeFacing = value; }
        }

        [DisplayName("Characters turn before walking")]
        [Description("Characters will turn on the spot to face their new direction before starting to move")]
        [DefaultValue(true)]
        [Category("Character behavior")]
        public bool TurnBeforeWalking
        {
            get { return _turnBeforeWalking; }
            set { _turnBeforeWalking = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public bool ScaleMovementSpeedWithMaskResolution { get { return false; } }

        [DisplayName("Package custom data folder(s)")]
        [Description("A comma-separated list of folders; their contents will be added to the game resources")]
        [Category("Compiler")]
        public string CustomDataDir
        {
            get { return _customDataDir; }
            set { _customDataDir = GetCustomDirsString(value); }
        }

        [DisplayName("Split resource files into X MB-sized chunks")]
        [Description("Resources will be split into files sized with the number of megabytes you enter here (0 to disable)")]
        [DefaultValue(0)]
        [Category("Compiler")]
        public int SplitResources
        {
            get { return _splitResources; }
            set { _splitResources = value; }
        }

        [DisplayName("Attach game data to exe (Windows only)")]
        [Description("Main game data will be attached to game exe. Otherwise it will be in a separate file called GAMENAME.ags")]
        [DefaultValue(false)]
        [Category("Compiler")]
        public bool AttachDataToExe
        {
            get { return _attachDataToExe; }
            set { _attachDataToExe = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public bool LetterboxMode { get { return false; } }

        [DisplayName("Automatically move the player in Walk mode")]
        [Description("When the player clicks somewhere in Walk mode, the player character will be sent there rather than processing it as an interaction")]
        [DefaultValue(true)]
        [Category("Character behavior")]
        public bool AutoMoveInWalkMode
        {
            get { return _autoMoveInWalkMode; }
            set { _autoMoveInWalkMode = value; }
        }

        [DisplayName("Pixel-perfect click detection")]
        [Description("When the player clicks the mouse, a pixel-perfect check will be used to decide what they clicked on (if false, a simple rectangular check is used)")]
        [DefaultValue(true)]
        [Category("Visual")]
        public bool PixelPerfect
        {
            get { return _pixelPerfect; }
            set { _pixelPerfect = value; }
        }

        [DisplayName("When player interface is disabled, GUIs should")]
        [Description("When the player interface is disabled (eg. during a cutscene), GUIs on screen will take this action")]
        [DefaultValue(InterfaceDisabledAction.GreyOut)]
        [Category("Visual")]
		[TypeConverter(typeof(EnumTypeConverter))]
		public InterfaceDisabledAction WhenInterfaceDisabled
        {
            get { return _whenInterfaceDisabled; }
            set { _whenInterfaceDisabled = value; }
        }

        [DisplayName(PROPERTY_CLIPGUICONTROLS)]
        [Description("GUI controls will clip their graphical contents, such as text, preventing it from being drawn outside of their rectangle." +
            "\nNOTE: Button images are clipped using individual button's property.")]
        [DefaultValue(true)]
        [Category("Visual")]
        public bool ClipGUIControls
        {
            get { return _clipGuiControls; }
            set { _clipGuiControls = value; }
        }

        [DisplayName("Run game loops while dialog options are displayed")]
        [Description("Whether to allow game animations to continue in the background while waiting for the player to select a dialog option")]
        [DefaultValue(false)]
        [Category("Dialog")]
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
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int InventoryHotspotMarkerSprite
        {
            get { return _inventoryHotspotMarker.Image; }
            set { _inventoryHotspotMarker.Image = value; }
        }

        [DisplayName("Inventory item cursor hotspot marker dot colour")]
        [Description("The AGS Colour Number of the colour of the central dot of the crosshair")]
        [Category("Inventory")]
        public int InventoryHotspotMarkerDotColor
        {
            get { return _inventoryHotspotMarker.DotColor; }
            set { _inventoryHotspotMarker.DotColor = value; }
        }

        [DisplayName("Inventory item cursor hotspot marker crosshair colour")]
        [Description("The AGS Colour Number of the colour of the crosshair bars")]
        [Category("Inventory")]
        public int InventoryHotspotMarkerCrosshairColor
        {
            get { return _inventoryHotspotMarker.CrosshairColor; }
            set { _inventoryHotspotMarker.CrosshairColor = value; }
        }

        [DisplayName("Automatically walk to hotspots in Look mode")]
        [Description("Whenever the player clicks somewhere in Look mode, the player character will automatically be moved there")]
        [DefaultValue(false)]
        [Category("Character behavior")]
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
        [DefaultValue(true)]
        [Category("Character behavior")]
        public bool ScaleCharacterSpriteOffsets
        {
            get { return _scaleCharacterSpriteOffsets; }
            set { _scaleCharacterSpriteOffsets = value; }
        }

        [DisplayName("Enable Debug Mode")]
        [Description("Enable various debugging keys that help you while developing your game")]
        [DefaultValue(true)]
        [Category("Compiler")]
        public bool DebugMode
        {
            get { return _debugMode; }
            set { _debugMode = value; }
        }

        [DisplayName("Use selected inventory graphic for cursor")]
        [Description("When in Use Inventory mode, the mouse cursor will be the selected inventory item rather than a fixed cursor")]
        [DefaultValue(true)]
        [Category("Inventory")]
        public bool InventoryCursors
        {
            get { return _inventoryCursors; }
            set { _inventoryCursors = value; }
        }

        [DisplayName("Override built-in inventory window click handling")]
        [Description("When the mouse is clicked in an inventory window, on_mouse_click is called rather than using AGS's default processing")]
        [DefaultValue(true)]
        [Category("Inventory")]
        public bool HandleInvClicksInScript
        {
            get { return _handleInvInScript; }
            set { _handleInvInScript = value; }
        }

        [DisplayName("Display multiple icons for multiple items")]
        [Description("If the player has two or more of an item, it will be displayed multiple times in the inventory window")]
        [DefaultValue(false)]
        [Category("Inventory")]
        public bool DisplayMultipleInventory
        {
            get { return _displayMultipleInv; }
            set { _displayMultipleInv = value; }
        }

        [DisplayName("Script API version")]
        [Description("Choose the version of the script API to use in your scripts")]
        [DefaultValue(ScriptAPIVersion.Highest)]
        [Category("Backwards Compatibility")]
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
        [DefaultValue(ScriptAPIVersion.Highest)]
        [Category("Backwards Compatibility")]
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

        [Obsolete]
        [Browsable(false)]
        public bool EnforceObjectBasedScript { get { return true; } }

        [Obsolete]
        [Browsable(false)]
        public bool EnforceNewStrings { get { return true; } }

        [Obsolete]
        [Browsable(false)]
        public bool EnforceNewAudio { get { return true; } }

        [Obsolete]
        [Browsable(false)]
        public bool UseOldCustomDialogOptionsAPI { get { return false; } }

        [DisplayName("Use old-style keyboard handling")]
        [Description("Use pre-unicode mode key codes in 'on_key_press' function, where regular keys were merged with Ctrl and Alt modifiers.")]
        [DefaultValue(false)]
        [Category("Backwards Compatibility")]
        public bool UseOldKeyboardHandling
        {
            get { return _oldKeyHandling; }
            set { _oldKeyHandling = value; }
        }

        [DisplayName("Play sound when the player gets points")]
        [Description("This sound number will be played whenever the player scores points (0 to disable)")]
        [DefaultValue(AudioClip.FixedIndexNoValue)]
        [Category("Sound")]
        [TypeConverter(typeof(AudioClipTypeConverter))]
        public int PlaySoundOnScore
        {
            get { return _playSoundOnScore; }
            set { _playSoundOnScore = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public CrossfadeSpeed CrossfadeMusic { get; }

        [DisplayName("Use GUI for dialog options")]
        [Description("Dialog options can be drawn on a textwindow GUI (0 to just draw at bottom of screen instead)")]
        [DefaultValue(0)]
        [Category("Dialog")]
        public int DialogOptionsGUI
        {
            get { return _dialogOptionsGUI; }
            set { _dialogOptionsGUI = value; }
        }

        [DisplayName("Gap between dialog options (in pixels)")]
        [Description("Gap between dialog options (in pixels)")]
        [DefaultValue(0)]
        [Category("Dialog")]
        public int DialogOptionsGap
        {
            get { return _dialogOptionsGap; }
            set { _dialogOptionsGap = value; }
        }

        [DisplayName("Dialog bullet point image")]
        [Description("Sprite to use as a bullet point before each dialog option (0 for none)")]
        [DefaultValue(0)]
        [Category("Dialog")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int DialogOptionsBullet
        {
            get { return _dialogBulletImage; }
            set { _dialogBulletImage = value; }
        }

        [DisplayName("Allow speech to be skipped by which events")]
        [Description("Determines whether the mouse, keyboard or the auto-remove timer can remove the current speech line")]
        [DefaultValue(SkipSpeechStyle.MouseOrKeyboardOrTimer)]
        [Category("Dialog")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public SkipSpeechStyle SkipSpeech
        {
            get { return _skipSpeech; }
            set { _skipSpeech = value; }
        }

        [DisplayName("Speech style")]
        [Description("Determines whether speech is displayed in lucasarts or sierra-style")]
        [DefaultValue(SpeechStyle.Lucasarts)]
        [Category("Dialog")]
        public SpeechStyle SpeechStyle
        {
            get { return _speechStyle; }
            set { _speechStyle = value; }
        }

        [DisplayName("Use game-wide speech animation delay")]
        [Description("Determines whether to use game-wide speech animation delay or use the individual character settings.")]
        [DefaultValue(false)]
        [Category("Dialog")]
        [RefreshProperties(RefreshProperties.All)]
        public bool UseGlobalSpeechAnimationDelay
        {
            get { return _useGlobalSpeechAnimationDelay; }
            set { _useGlobalSpeechAnimationDelay = value; }
        }

        [DisplayName("Game-wide speech animation delay")]
        [Description("Sets the Speech.GlobalSpeechAnimationDelay setting to determine the animation speed of character speech; individual character SpeechAnimationDelay settings will be ignored.")]
        [DefaultValue(5)]
        [Category("Dialog")]
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
        [DefaultValue(DialogOptionsNumbering.None)]
        [Category("Dialog")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public DialogOptionsNumbering NumberDialogOptions
        {
            get { return _numberDialogOptions; }
            set { _numberDialogOptions = value; }
        }

        [DisplayName("Print dialog options upwards")]
        [Description("The first dialog option will be at the bottom, and the last at the top")]
        [DefaultValue(false)]
        [Category("Dialog")]
        public bool DialogOptionsBackwards
        {
            get { return _dialogOptionsBackwards; }
            set { _dialogOptionsBackwards = value; }
        }

        [DisplayName("Sierra-style portrait location")]
        [Description("Determines whether to draw the Sierra-style portrait on the left or right of the screen")]
        [DefaultValue(SpeechPortraitSide.Left)]
        [Category("Dialog")]
        public SpeechPortraitSide SpeechPortraitSide
        {
            get { return _speechPortraitSide; }
            set { _speechPortraitSide = value; }
        }

        [DisplayName("Custom Say function in dialog scripts")]
        [Description("Sets which function name to use in place of character.Say when running dialog scripts. Note that it must be an extension function of a Character class. Leave empty to use default (Character.Say).")]
        [DefaultValue("")]
        [Category("Dialog")]
        public string DialogScriptSayFunction
        {
            get { return _dialogScriptSayFunction; }
            set { _dialogScriptSayFunction = value; }
        }

        [DisplayName("Custom Narrate function in dialog scripts")]
        [Description("Sets which function name to use in place of narrator's speech when running dialog scripts. Note that it must be either regular function or a static struct function. Leave empty to use default (Display).")]
        [DefaultValue("")]
        [Category("Dialog")]
        public string DialogScriptNarrateFunction
        {
            get { return _dialogScriptNarrateFunction; }
            set { _dialogScriptNarrateFunction = value; }
        }

        [DisplayName("Custom text-window GUI")]
        [Description("Sets which text-window GUI is used for normal text in the game. You must use the GUI number, not name. You can't use GUI 0 for this, because 0 means that AGS will use its built-in text window instead.")]
        [DefaultValue(0)]
        [Category("Text output")]
        public int TextWindowGUI
        {
            get { return _textWindowGUI; }
            set { _textWindowGUI = value; }
        }

        [DisplayName("Always display text as speech")]
        [Description("Enables the lucasarts-style option where all default text in the game is spoken by the main character")]
        [DefaultValue(false)]
        [Category("Text output")]
        public bool AlwaysDisplayTextAsSpeech
        {
            get { return _alwaysDisplayTextAsSpeech; }
            set { _alwaysDisplayTextAsSpeech = value; }
        }

        [Browsable(false)]
        [Obsolete("FontsForHiRes property is replaced with individual SizeMultiplier in each font.")]
        public bool FontsForHiRes { get { return false; } }

        [DisplayName(PROPERTY_ANTI_ALIAS_FONTS)]
        [Description("True-type fonts will be anti-aliased in-game, but there is a performance penalty")]
        [DefaultValue(false)]
        [Category("Text output")]
        public bool AntiAliasFonts
        {
            get { return _antiAliasFonts; }
            set { _antiAliasFonts = value; }
        }

        [DisplayName(PROPERTY_FONT_HEIGHT_IN_LOGIC)]
        [Description("How the true-type font height will be defined whenever it is required by the script or game logic.")]
        [DefaultValue(FontHeightDefinition.NominalHeight)]
        [Category("Text output")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public FontHeightDefinition TTFHeightDefinedBy
        {
            get { return _ttfHeightDefinedBy; }
            set { _ttfHeightDefinedBy = value; }
        }

        [DisplayName("TTF fonts adjustment defaults")]
        [Description("Automatic adjustment of the true-type font metrics; primarily for backward compatibility." +
            "\nThis option will be used as a default value for each new imported font, but you may also customize it in the Font's properties.")]
        [DefaultValue(FontMetricsFixup.None)]
        [Category("Text output")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public FontMetricsFixup TTFMetricsFixup
        {
            get { return _ttfMetricsFixup; }
            set { _ttfMetricsFixup = value; }
        }

        [DisplayName("Custom thought bubble GUI")]
        [Description("Character.Think will use this custom text-window GUI")]
        [DefaultValue(0)]
        [Category("Text output")]
        public int ThoughtGUI
        {
            get { return _thoughtGUI; }
            set { _thoughtGUI = value; }
        }

        [DisplayName("Write game text Right-to-Left")]
        [Description("The game will draw text right-to-left, used by languages such as Hebrew")]
        [DefaultValue(false)]
        [Category("Text output")]
        public bool BackwardsText
        {
            get { return _backwardsText; }
            set { _backwardsText = value; }
        }

        [DisplayName("Maximum possible score")]
        [Description("The maximum score that the player can achieve (displayed by @TOTALSCORE@ on GUI labels)")]
        [Category("(Basic properties)")]
        public int MaximumScore
        {
            get { return _totalScore; }
            set { _totalScore = value; }
        }

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
        public string Genre
		{
			get { return _genre; }
			set { _genre = value; }
		}

		[DisplayName("Version")]
		[Description("Current game version")]
        [Category("(Information)")]
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
		public string DeveloperName
		{
			get { return _developerName; }
			set { _developerName = value; }
		}

		[DisplayName("Developer website")]
		[Description("URL of game developer's website")]
        [Category("(Information)")]
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
					throw new ArgumentException("Save game extension cannot be longer than 15 letters");
				}
				if ((value.Length > 0) && (value.Length < 5))
				{
					throw new ArgumentException("Save game extension must be at least 5 letters long");
				}
				_saveGameExtension = value; 
			}
		}

        [DisplayName("Save games folder name")]
        [Description("If set, creates a folder of this name inside the user's Saved Games folder in Windows Vista and higher (or My Documents in XP) to store the save games in.")]
        [Category("Saved Games")]
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
            _runGameLoopsWhileDialogOptionsDisplayed = false;
            _inventoryHotspotMarker = new InventoryHotspotMarker();
            _audioIndexer = AudioClip.FixedIndexBase;
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

        [DisplayName("Render sprites at screen resolution")]
        [Description("When drawing zoomed character and object sprites, AGS will take advantage of higher runtime resolution to give scaled images more detail, than it would be possible if the game was displayed in its native resolution. The effect is stronger for low-res games. Keep disabled for pixel-perfect output. Currently supported only by Direct3D and OpenGL renderers.")]
        [DefaultValue(RenderAtScreenResolution.UserDefined)]
        [Category("(Basic properties)")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public RenderAtScreenResolution RenderAtScreenResolution
        {
            get { return _renderAtScreenRes; }
            set { _renderAtScreenRes = value; }
        }

        [DisplayName(PROPERTY_ANDROID_APPLICATION_ID)]
        [Description("The application ID, used in app store. Also called package name, it's usually looks like com.mystudio.mygame, and it's used in store URLs. It must have at least two segments (one or more dots), and each segment must start with a letter.")]
        [DefaultValue("com.mystudio.mygame")]
        [Category("Android")]
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
        [DefaultValue("1")]
        [Category("Android")]
        public int AndroidAppVersionCode
        {
            get { return _androidAppVersionCode; }
            set { _androidAppVersionCode = value; }
        }

        [DisplayName(PROPERTY_ANDROID_APP_VERSION_NAME)]
        [Description("The version name visible to users in the stores, this can be anything. Leave empty to use the same version you set in desktop platforms.")]
        [DefaultValue("")]
        [Category("Android")]
        public string AndroidAppVersionName
        {
            get { return _androidAppVersionName; }
            set { _androidAppVersionName = value; }
        }

        [DisplayName("Build Format")]
        [Description("Use embedded formats when testing locally. Google Play only accepts AAB.")]
        [DefaultValue("Aab")]
        [Category("Android")]
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
        [Obsolete]
        [Browsable(false)]
        public bool LeftToRightPrecedence { get; }
        [Obsolete]
        [Browsable(false)]
        public bool UseLowResCoordinatesInScript { get; }
        [Obsolete]
        [Browsable(false)]
        public GUIAlphaStyle GUIAlphaStyle { get; }
        [Obsolete]
        [Browsable(false)]
        public SpriteAlphaStyle SpriteAlphaStyle { get; }

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
