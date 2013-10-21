using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace AGS.Types
{
    public class Settings : ICustomTypeDescriptor
    {
        public const string PROPERTY_GAME_NAME = "Game name";
        public const string PROPERTY_COLOUR_DEPTH = "Colour depth";
        public const string PROPERTY_RESOLUTION = "Resolution";
        public const string PROPERTY_SCALE_FONTS = "Fonts designed for 640x480";
		public const string PROPERTY_ANTI_ALIAS_FONTS = "Anti-alias TTF fonts";
        public const string PROPERTY_LETTERBOX_MODE = "Enable letterbox mode";
		public const string REGEX_FOUR_PART_VERSION = @"^(\d+)\.(\d+)\.(\d+)\.(\d+)$";

		private const string DEFAULT_GENRE = "Adventure";
        private const string DEFAULT_VERSION = "1.0.0.0";

        public Settings()
        {
			GenerateNewGameID();
        }

        private string _gameName = "New game";
        private GameResolutions _resolution = GameResolutions.R320x200;
        private GameColorDepth _colorDepth = GameColorDepth.HighColor;
		private GraphicsDriver _graphicsDriver = GraphicsDriver.DX5;
        private bool _debugMode = true;
        private bool _antiGlideMode = true;
        private bool _walkInLookMode = false;
        private InterfaceDisabledAction _whenInterfaceDisabled = InterfaceDisabledAction.GreyOut;
        private bool _pixelPerfect = true;
        private bool _autoMoveInWalkMode = true;
        private bool _letterboxMode = false;
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
        private bool _enforceObjectScripting = true;
        private bool _leftToRightPrecedence = true;
        private bool _enforceNewStrings = true;
        private bool _enforceNewAudio = true;
        private int _playSoundOnScore = 0;
        private CrossfadeSpeed _crossfadeMusic = CrossfadeSpeed.No;
        private int _dialogOptionsGUI = 0;
        private int _dialogOptionsGap = 0;
        private int _dialogBulletImage = 0;
        private SkipSpeechStyle _skipSpeech = SkipSpeechStyle.MouseOrKeyboardOrTimer;
        private SpeechStyle _speechStyle = SpeechStyle.Lucasarts;
        private int _globalSpeechAnimationDelay = 5;
        private bool _useGlobalSpeechAnimationDelay = false;
        private bool _numberDialogOptions = false;
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
        private GUIAlphaStyle _guiAlphaStyle = GUIAlphaStyle.MultiplyTranslucenceSrcBlend;
        private SpriteAlphaStyle _spriteAlphaStyle = SpriteAlphaStyle.Improved;
        private bool _runGameLoopsWhileDialogOptionsDisplayed = false;
        private InventoryHotspotMarker _inventoryHotspotMarker = new InventoryHotspotMarker();
        private bool _useLowResCoordinatesInScript = true;
        // Vista game explorer fields
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

		public void GenerateNewGameID()
		{
			_uniqueID = Environment.TickCount;
			_guid = Guid.NewGuid();
		}

		[DisplayName(PROPERTY_GAME_NAME)]
        [Description("The game's name (for display in the title bar)")]
        [Category("(Setup)")]
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
        [Category("(Setup)")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public GameColorDepth ColorDepth
        {
            get { return _colorDepth; }
            set { _colorDepth = value; }
        }

        [DisplayName(PROPERTY_RESOLUTION)]
        [Description("The graphics resolution of the game (higher allows more detail, but slower performance and larger file size)")]
        [Category("(Setup)")]
        [TypeConverter(typeof(EnumTypeConverter))]
        [RefreshProperties(RefreshProperties.All)]
        public GameResolutions Resolution
        {
            get { return _resolution; }
            set { _resolution = value; }
        }

		[DisplayName("Default graphics driver")]
		[Description("The default graphics driver that your game will use. Direct3D allows fast high-resolution alpha-blended sprites, but DirectDraw is better at RawDrawing.")]
		[Category("(Setup)")]
		[TypeConverter(typeof(EnumTypeConverter))]
		public GraphicsDriver GraphicsDriver
		{
			get { return _graphicsDriver; }
			set { _graphicsDriver = value; }
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

        [DisplayName("Use low-resolution co-ordinates in script")]
        [Description("Backwards-compatible option to always use low-res co-ordinates in the script. This is how previous versions of AGS always worked. WARNING: Changing this setting could break your current scripts.")]
        [DefaultValue(false)]
        [Category("Backwards Compatibility")]
        public bool UseLowResCoordinatesInScript
        {
            get { return _useLowResCoordinatesInScript; }
            set { _useLowResCoordinatesInScript = value; }
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

        [DisplayName("Enable letterbox mode")]
        [Description("Game will run at 320x240 or 640x480 with top and bottom black borders to give a square aspect ratio")]
        [DefaultValue(false)]
        [Category("Visual")]
        public bool LetterboxMode
        {
            get { return _letterboxMode; }
            set { _letterboxMode = value; }
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

        [DisplayName("GUI alpha rendering style")]
        [Description("When using 32-bit alpha-channel images, should GUIs be drawn with the new improved alpha method, or the backwards-compatible method?")]
        [DefaultValue(GUIAlphaStyle.MultiplyTranslucenceSrcBlend)]
        [Category("Visual")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public GUIAlphaStyle GUIAlphaStyle
        {
            get { return _guiAlphaStyle; }
            set { _guiAlphaStyle = value; }
        }

        [DisplayName("Sprite alpha rendering style")]
        [Description("When using 32-bit alpha-channel images, should sprites be drawn with the new improved alpha method, or the backwards-compatible method?")]
        [DefaultValue(SpriteAlphaStyle.Improved)]
        [Category("Visual")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public SpriteAlphaStyle SpriteAlphaStyle
        {
            get { return _spriteAlphaStyle; }
            set { _spriteAlphaStyle = value; }
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

        [DisplayName("Left-to-right operator precedence")]
        [Description("Expressions like (5 - 3 - 2) will equal 0 rather than 4")]
        [DefaultValue(true)]
        [Category("Backwards Compatibility")]
        public bool LeftToRightPrecedence
        {
            get { return _leftToRightPrecedence; }
            set { _leftToRightPrecedence = value; }
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
        [DefaultValue(false)]
        [Category("Dialog")]
        public bool NumberDialogOptions
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
        [Description("Tells AGS that your fonts are designed for 640x480, and therefore not to scale them up at this resolution")]
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
        [Category("(Setup)")]
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
		[Category("Windows Vista Game Explorer")]
		public bool GameExplorerEnabled
		{
			get { return _enableGameExplorer; }
			set { _enableGameExplorer = value; }
		}

		[DisplayName("Game description")]
		[Description("The Description displayed in the Game Explorer")]
		[Category("Windows Vista Game Explorer")]
		public string Description
		{
			get { return _description; }
			set { _description = value; }
		}

		[DisplayName("Release date")]
		[Description("Date on which this game is first released")]
		[Category("Windows Vista Game Explorer")]
		public DateTime ReleaseDate
		{
			get { return _releaseDate; }
			set { _releaseDate = value; }
		}

		[DisplayName("Genre")]
		[Description("The Genre displayed in the Game Explorer")]
		[Category("Windows Vista Game Explorer")]
		public string Genre
		{
			get { return _genre; }
			set { _genre = value; }
		}

		[DisplayName("Version")]
		[Description("The Version displayed in the Game Explorer")]
		[Category("Windows Vista Game Explorer")]
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
		[Description("The minimum Vista Experience Index necessary to play the game")]
		[Category("Windows Vista Game Explorer")]
		public int WindowsExperienceIndex
		{
			get { return _windowsExperienceIndex; }
			set { _windowsExperienceIndex = value; }
		}

		[DisplayName("Developer name")]
		[Description("The name of the game developer (you!). Displayed on the game EXE in Explorer, and in the Vista Game Explorer.")]
        [Category("(Setup)")]
		public string DeveloperName
		{
			get { return _developerName; }
			set { _developerName = value; }
		}

		[DisplayName("Developer website")]
		[Description("URL of game developer's website")]
		[Category("Windows Vista Game Explorer")]
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
        [Description("If set, creates a folder of this name inside the user's Saved Games folder in Vista (or My Documents in XP) to store the save games in.")]
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
        [Category("(Setup)")]
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

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

        public void FromXml(XmlNode node)
        {
			_graphicsDriver = GraphicsDriver.DX5;
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
            _guiAlphaStyle = GUIAlphaStyle.Classic;
            _spriteAlphaStyle = SpriteAlphaStyle.Classic;
            _runGameLoopsWhileDialogOptionsDisplayed = false;
            _inventoryHotspotMarker = new InventoryHotspotMarker();
            _useLowResCoordinatesInScript = true;
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
                else if ((_resolution != GameResolutions.R320x200) && 
                         (_resolution != GameResolutions.R640x400) &&
                         (property.Name == "LetterboxMode"))
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
