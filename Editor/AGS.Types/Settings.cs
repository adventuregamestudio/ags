using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace AGS.Types
{
    [DeserializeIgnore("LastBuildConfiguration")]
    [DeserializeIgnore("GraphicsDriver")]
    [DeserializeIgnore("LeftToRightPrecedence")]
    [DeserializeIgnore("UseLowResCoordinatesInScript")]
    [DeserializeIgnore("GUIAlphaStyle")]
    [DeserializeIgnore("SpriteAlphaStyle")]
    [DefaultProperty("DebugMode")]
    public class Settings : ICustomTypeDescriptor
    {
        public const string PROPERTY_GAME_NAME = "Game name";
        public const string PROPERTY_COLOUR_DEPTH = "Colour depth";
        public const string PROPERTY_RESOLUTION = "Resolution";
        public const string PROPERTY_SCALE_FONTS = "Fonts designed for high resolution";
		public const string PROPERTY_ANTI_ALIAS_FONTS = "Anti-alias TTF fonts";
        public const string PROPERTY_LETTERBOX_MODE = "Enable letterbox mode";
        public const string PROPERTY_BUILD_TARGETS = "Build target platforms";
        public const string PROPERTY_RENDERATSCREENRES = "Render sprites at screen resolution";
		public const string REGEX_FOUR_PART_VERSION = @"^(\d+)\.(\d+)\.(\d+)\.(\d+)$";

		private const string DEFAULT_GENRE = "Adventure";
        private const string DEFAULT_VERSION = "1.0.0.0";

        public Settings()
        {
			GenerateNewGameID();
        }

        private string _gameName = "New game";
        private Size _resolution = new Size(320, 200);
        private GameColorDepth _colorDepth = GameColorDepth.TrueColor;
        private bool _debugMode = true;
        private bool _antiGlideMode = true;
        private bool _walkInLookMode = false;
        private InterfaceDisabledAction _whenInterfaceDisabled = InterfaceDisabledAction.GreyOut;
        private bool _pixelPerfect = true;
        private bool _autoMoveInWalkMode = true;
        private bool _letterboxMode = false;
        private RenderAtScreenResolution _renderAtScreenRes = RenderAtScreenResolution.UserDefined;
        private int _splitResources = 0;
        private bool _turnBeforeWalking = true;
        private bool _turnBeforeFacing = true;
        private bool _mouseWheelEnabled = true;
        private RoomTransitionStyle _roomTransition = RoomTransitionStyle.FadeOutAndIn;
        private bool _saveScreenshots = false;
        private bool _compressSprites = false;
        private bool _inventoryCursors = true;
        private bool _handleInvInScript = false;
        private bool _displayMultipleInv = false;
        private ScriptAPIVersion _scriptAPIVersion = ScriptAPIVersion.Highest;
        private ScriptAPIVersion _scriptCompatLevel = ScriptAPIVersion.Highest;
        private ScriptAPIVersion _scriptAPIVersionReal = Utilities.GetActualAPI(ScriptAPIVersion.Highest);
        private ScriptAPIVersion _scriptCompatLevelReal = Utilities.GetActualAPI(ScriptAPIVersion.Highest);
        private bool _enforceObjectScripting = true;
        private bool _enforceNewStrings = true;
        private bool _enforceNewAudio = true;
        private bool _oldCustomDlgOptsAPI = false;
        private int _playSoundOnScore = 0;
        private CrossfadeSpeed _crossfadeMusic = CrossfadeSpeed.No;
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
        private int _textWindowGUI = 0;
        private bool _alwaysDisplayTextAsSpeech = false;
        private bool _fontsAreHiRes = false;
        private bool _antiAliasFonts = false;
        private int _thoughtGUI = 0;
        private bool _backwardsText = false;
        private int _uniqueID;
		private Guid _guid;
        private bool _hasMODMusic = false;
        private int _totalScore = 0;
        private bool _binaryFilesInSourceControl = false;
        private bool _runGameLoopsWhileDialogOptionsDisplayed = false;
        private InventoryHotspotMarker _inventoryHotspotMarker = new InventoryHotspotMarker();
        // Windows game explorer fields
		private bool _enableGameExplorer = false;
		private string _description = string.Empty;
		private DateTime _releaseDate = DateTime.Now;
		private string _genre = DEFAULT_GENRE;
		private string _version = DEFAULT_VERSION;
		private int _windowsExperienceIndex = 1;
		private string _developerName = string.Empty;
		private string _developerURL = string.Empty;
		private string _saveGameExtension = string.Empty;
		private bool _enhancedSaveGames = false;
        private string _saveGamesFolderName = string.Empty;
        private int _audioIndexer = 0;
        private string _buildTargets = GetBuildTargetsString(BuildTargetsInfo.GetAvailableBuildTargetNames(), false);

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

		public void GenerateNewGameID()
		{
			_uniqueID = Environment.TickCount;
			_guid = Guid.NewGuid();
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

        [AGSNoSerialize]
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

        /// <summary>
        /// Tells if the game should be considered low-resolution.
        /// For backwards-compatble logic only.
        /// The "low resolution" assumes that game does not exceed
        /// 320x240 pixels.
        /// </summary>
        [Browsable(false)]
        // CLNUP I think this also tells the editor if scaling up GUIs in low res games
        public bool LowResolution
        {
            get
            {
                return (CustomResolution.Width * CustomResolution.Height) <= (320 * 240);
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

        [DisplayName("Compress the sprite file")]
        [Description("Compress the sprite file to reduce its size, at the expense of performance")]
        [DefaultValue(false)]
        [Category("Compiler")]
        public bool CompressSprites
        {
            get { return _compressSprites; }
            set { _compressSprites = value; }
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

        [DisplayName("Enable mouse wheel support")]
        [Description("Enable mouse wheel events to be sent to on_mouse_click")]
        [DefaultValue(true)]
        [Category("Backwards Compatibility")]
        public bool MouseWheelEnabled
        {
            get { return _mouseWheelEnabled; }
            set { _mouseWheelEnabled = value; }
        }

        [DisplayName("Characters turn to face direction")]
        [Description("Characters will turn on the spot to face their new direction when FaceLocation is used")]
        [DefaultValue(true)]
        [Category("Character movement")]
        public bool TurnBeforeFacing
        {
            get { return _turnBeforeFacing; }
            set { _turnBeforeFacing = value; }
        }

        [DisplayName("Characters turn before walking")]
        [Description("Characters will turn on the spot to face their new direction before starting to move")]
        [DefaultValue(true)]
        [Category("Character movement")]
        public bool TurnBeforeWalking
        {
            get { return _turnBeforeWalking; }
            set { _turnBeforeWalking = value; }
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

        [DisplayName("Old-style letterbox mode")]
        [Description("Game will run at 320x240 or 640x480 with top and bottom black borders to give a square aspect ratio. Not recommended unless importing an old project.")]
        [DefaultValue(false)]
        [Category("(Basic properties)")]
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
        [DefaultValue(true)]
        [Category("Character movement")]
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
        [Category("Character movement")]
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

        [DisplayName("Enforce object-based scripting")]
        [Description("Disable old-style AGS 2.62 script commands")]
        [DefaultValue(true)]
        [Category("Backwards Compatibility")]
        public bool EnforceObjectBasedScript
        {
            get { return _enforceObjectScripting; }
            set { _enforceObjectScripting = value; }
        }

        [DisplayName("Enforce new-style strings")]
        [Description("Disable old-style strings from AGS 2.70 and before")]
        [DefaultValue(true)]
        [Category("Backwards Compatibility")]
        public bool EnforceNewStrings
        {
            get { return _enforceNewStrings; }
            set { _enforceNewStrings = value; }
        }

        [DisplayName("Enforce new-style audio scripting")]
        [Description("Disable old-style audio commands like PlaySound, IsChannelPlaying, etc")]
        [DefaultValue(true)]
        [Category("Backwards Compatibility")]
        public bool EnforceNewAudio
        {
            get { return _enforceNewAudio; }
            set { _enforceNewAudio = value; }
        }

        [DisplayName("Use old-style custom dialog options API")]
        [Description("Use pre-3.4.0 callback functions to handle custom dialog options GUI")]
        [DefaultValue(false)]
        [Category("Backwards Compatibility")]
        public bool UseOldCustomDialogOptionsAPI
        {
            get { return _oldCustomDlgOptsAPI; }
            set { _oldCustomDlgOptsAPI = value; }
        }

        [DisplayName("Play sound when the player gets points")]
        [Description("This sound number will be played whenever the player scores points (0 to disable)")]
        [DefaultValue(0)]
        [Category("Sound")]
        [TypeConverter(typeof(AudioClipTypeConverter))]
        public int PlaySoundOnScore
        {
            get { return _playSoundOnScore; }
            set { _playSoundOnScore = value; }
        }

        [DisplayName("Crossfade music tracks")]
        [Description("When going from one track to another, they can be crossfaded")]
        [DefaultValue(CrossfadeSpeed.No)]
        [Category("Sound")]
        [Browsable(false)]
        public CrossfadeSpeed CrossfadeMusic
        {
            get { return _crossfadeMusic; }
            set { _crossfadeMusic = value; }
        }

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

        [DisplayName(PROPERTY_SCALE_FONTS)]
        [Description("Tells AGS that your fonts are designed for high resolution (higher than 320x240), and therefore not to scale them up in hi-res game")]
        [DefaultValue(false)]
        [Category("Text output")]
        public bool FontsForHiRes
        {
            get { return _fontsAreHiRes; }
            set { _fontsAreHiRes = value; }
        }

        [DisplayName(PROPERTY_ANTI_ALIAS_FONTS)]
        [Description("True-type fonts will be anti-aliased in-game, but there is a performance penalty")]
        [DefaultValue(false)]
        [Category("Text output")]
        public bool AntiAliasFonts
        {
            get { return _antiAliasFonts; }
            set { _antiAliasFonts = value; }
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

        [Browsable(false)]
        public bool HasMODMusic
        {
            get { return _hasMODMusic; }
            set { _hasMODMusic = value; }
        }

		[DisplayName("Enable Game Explorer integration")]
		[Description("Whether or not this game can be added to the Vista Game Explorer")]
		[Category("Windows Game Explorer")]
		public bool GameExplorerEnabled
		{
			get { return _enableGameExplorer; }
			set { _enableGameExplorer = value; }
		}

		[DisplayName("Game description")]
		[Description("The Description displayed in the Game Explorer")]
		[Category("Windows Game Explorer")]
		public string Description
		{
			get { return _description; }
			set { _description = value; }
		}

		[DisplayName("Release date")]
		[Description("Date on which this game is first released")]
		[Category("Windows Game Explorer")]
		public DateTime ReleaseDate
		{
			get { return _releaseDate; }
			set { _releaseDate = value; }
		}

		[DisplayName("Genre")]
		[Description("The Genre displayed in the Game Explorer")]
		[Category("Windows Game Explorer")]
		public string Genre
		{
			get { return _genre; }
			set { _genre = value; }
		}

		[DisplayName("Version")]
		[Description("The Version displayed in the Game Explorer")]
		[Category("Windows Game Explorer")]
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

		[DisplayName("Windows Experience Index")]
		[Description("The minimum Windows Experience Index necessary to play the game")]
		[Category("Windows Game Explorer")]
		public int WindowsExperienceIndex
		{
			get { return _windowsExperienceIndex; }
			set { _windowsExperienceIndex = value; }
		}

		[DisplayName("Developer name")]
		[Description("The name of the game developer (you!). Displayed on the game EXE in Explorer, and in the Windows Game Explorer.")]
        [Category("(Basic properties)")]
		public string DeveloperName
		{
			get { return _developerName; }
			set { _developerName = value; }
		}

		[DisplayName("Developer website")]
		[Description("URL of game developer's website")]
		[Category("Windows Game Explorer")]
		public string DeveloperURL
		{
			get { return _developerURL; }
			set { _developerURL = value; }
		}

		[DisplayName("Enhanced save games")]
		[Description("Whether to enable enhanced save games, where the user can double-click on one in Explorer to start the game and load it. If enabled, you must set the Save Game File Extension. Please see the manual for important information.")]
		[Category("Saved Games")]
		public bool EnhancedSaveGames
		{
			get { return _enhancedSaveGames; }
			set { _enhancedSaveGames = value; }
		}

		[DisplayName("Save game file extension")]
		[Description("The file extension to give save game files (if Enhanced Save Games are enabled)")]
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

        [DisplayName("Put sound and sprite files in source control")]
        [Description("If you are using a source control provider, this controls whether the sound and sprite files are added to source control. With large games, these files can become extremely large and therefore you may wish to exclude them from source control.")]
        [Category("(Basic properties)")]
        public bool BinaryFilesInSourceControl
        {
            get { return _binaryFilesInSourceControl; }
            set { _binaryFilesInSourceControl = value; }
        }

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
			_enableGameExplorer = false;
			_description = string.Empty;
			_releaseDate = DateTime.Now;
			_genre = DEFAULT_GENRE;
			_version = DEFAULT_VERSION;
			_windowsExperienceIndex = 1;
			_developerName = string.Empty;
			_developerURL = string.Empty;
			_enhancedSaveGames = false;
			_saveGameExtension = string.Empty;
            _saveGamesFolderName = null;
            _binaryFilesInSourceControl = false;
            _runGameLoopsWhileDialogOptionsDisplayed = false;
            _inventoryHotspotMarker = new InventoryHotspotMarker();
            _audioIndexer = 0;
            _enforceNewAudio = false;

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
