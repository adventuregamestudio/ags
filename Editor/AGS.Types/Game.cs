using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Resources;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class Game : IGame
    {
		private readonly string[] RESERVED_SCRIPT_NAMES = { "inventory", "character", 
			"views", "player", "object", "mouse", "system", "game", "palette",
			"hotspot", "region", "dialog", "gui", "GUI"};
        private const int PALETTE_SIZE = 256;
        private const int NUMBER_OF_GLOBAL_MESSAGES = 500;
        private const int GLOBAL_MESSAGE_ID_START = 500;

        public const int MAX_CURSORS = 20;
        public const int MAX_DIALOGS = 500;
        public const int MAX_INV_ITEMS = 300;

        public delegate void GUIAddedOrRemovedHandler(GUI theGUI);
        public delegate void GUIControlAddedOrRemovedHandler(GUI owningGUI, GUIControl control);
        public delegate void ViewListUpdatedHandler();
        public event GUIAddedOrRemovedHandler GUIAddedOrRemoved;
        public event GUIControlAddedOrRemovedHandler GUIControlAddedOrRemoved;
        /// <summary>
        /// Fired when an external client adds/removes views
        /// </summary>
        public event ViewListUpdatedHandler ViewListUpdated;

        private GUIFolders _guis;
        private InventoryItemFolders _inventoryItems;
        private CharacterFolders _characters;
        private List<MouseCursor> _cursors;
        private List<Font> _fonts;
        private DialogFolders _dialogs;
        private List<Plugin> _plugins;
        private List<Translation> _translations;
        private UnloadedRoomFolders _rooms;
        private List<OldInteractionVariable> _oldInteractionVariables;
        private string[] _globalMessages;
        private Character _playerCharacter;
        private Settings _settings;
        private RuntimeSetup _defaultSetup;
        private WorkspaceState _workspaceState;
        private PaletteEntry[] _palette;
        private SpriteFolder _sprites;
        private ViewFolders _views;
        private AudioClipFolders _audioClips;
        private List<AudioClipType> _audioClipTypes;
        private ScriptFolders _scripts;
        private ScriptsAndHeaders _scriptsToCompile;
        private TextParser _textParser;
        private LipSync _lipSync;
        private CustomPropertySchema _propertySchema;
        private GlobalVariables _globalVariables;
        private IList<AudioClip> _cachedAudioClipListForCompile;
        private Dictionary<int, int> _cachedAudioClipIndexMapping;
		private string _directoryPath;
		private bool _roomsAddedOrRemoved = false;
		private Dictionary<int, object> _deletedViewIDs;
		private string _savedXmlVersion = null;
        private int? _savedXmlVersionIndex = null;
        private string _savedXmlEditorVersion = null;

        public Game()
        {
            _guis = new GUIFolders(GUIFolder.MAIN_GUI_FOLDER_NAME);
            _inventoryItems = new InventoryItemFolders(InventoryItemFolder.MAIN_INVENTORY_ITEM_FOLDER_NAME);
            _cursors = new List<MouseCursor>();
            _dialogs = new DialogFolders(DialogFolder.MAIN_DIALOG_FOLDER_NAME);
            _fonts = new List<Font>();
            _characters = new CharacterFolders(CharacterFolder.MAIN_CHARACTER_FOLDER_NAME);
            _plugins = new List<Plugin>();
            _translations = new List<Translation>();
            _rooms = new UnloadedRoomFolders(UnloadedRoomFolder.MAIN_UNLOADED_ROOM_FOLDER_NAME);
            _oldInteractionVariables = new List<OldInteractionVariable>();
            _settings = new Settings();
            _defaultSetup = new RuntimeSetup(_settings);
            _workspaceState = new WorkspaceState();
            _palette = new PaletteEntry[PALETTE_SIZE];
            _sprites = new SpriteFolder("Main");
            _views = new ViewFolders("Main");
            _audioClips = new AudioClipFolders("Main");
            _audioClipTypes = new List<AudioClipType>();
            _textParser = new TextParser();
            _lipSync = new LipSync();
            _propertySchema = new CustomPropertySchema();
            _globalVariables = new GlobalVariables();
            _globalMessages = new string[NUMBER_OF_GLOBAL_MESSAGES];
			_deletedViewIDs = new Dictionary<int, object>();
            _scripts = new ScriptFolders(ScriptFolder.MAIN_SCRIPT_FOLDER_NAME);
            _scriptsToCompile = new ScriptsAndHeaders();
            ScriptAndHeader globalScript = new ScriptAndHeader(
                new Script(Script.GLOBAL_HEADER_FILE_NAME, "// script header\r\n", true),
                new Script(Script.GLOBAL_SCRIPT_FILE_NAME, "// global script\r\n", false));
            ((IList<ScriptAndHeader>)_scripts).Add(globalScript);            
            _playerCharacter = null;

            for (int i = 0; i < _globalMessages.Length; i++)
            {
                _globalMessages[i] = string.Empty;
            }

            InitializeDefaultPalette();
        }

        public string[] GlobalMessages
        {
            get { return _globalMessages; }
        }

        public IList<GUI> GUIs
        {
            get { return _guis; }
        }

        public IList<InventoryItem> InventoryItems
        {
            get { return _inventoryItems; }
        }

        public IList<Character> Characters
        {
            get { return _characters; }
        }

        public IList<Dialog> Dialogs
        {
            get { return _dialogs; }            
        }

        public IList<MouseCursor> Cursors
        {
            get { return _cursors; }
        }

        public IList<Font> Fonts
        {
            get { return _fonts; }
        }

        public IList<Translation> Translations
        {
            get { return _translations; }
        }

        public List<Plugin> Plugins
        {
            get { return _plugins; }
        }

        public IList<IRoom> Rooms
        {
            get { return _rooms; }
        }

        public List<OldInteractionVariable> OldInteractionVariables
        {
            get { return _oldInteractionVariables; }
        }

        public CustomPropertySchema PropertySchema
        {
            get { return _propertySchema; }
        }

        public Character PlayerCharacter
        {
            get { return _playerCharacter; }
            set { _playerCharacter = value; }
        }

        public Settings Settings
        {
            get { return _settings; }
        }

        public RuntimeSetup DefaultSetup
        {
            get { return _defaultSetup; }
        }

        public WorkspaceState WorkspaceState
        {
            get { return _workspaceState; }
        }

        public PaletteEntry[] Palette
        {
            get { return _palette; }
        }

        public TextParser TextParser
        {
            get { return _textParser; }
        }

        public LipSync LipSync
        {
            get { return _lipSync; }
        }

        public SpriteFolder RootSpriteFolder
        {
            get { return _sprites; }
            set { _sprites = value; }
        }

        public CharacterFolder RootCharacterFolder
        {
            get { return _characters.RootFolder; }
        }

        public DialogFolder RootDialogFolder
        {
            get { return _dialogs.RootFolder; }
        }

        public ViewFolder RootViewFolder
        {
            get { return _views.RootFolder; }
            set { _views = new ViewFolders(value); }
        }

        public ScriptFolder RootScriptFolder
        {
            get { return _scripts.RootFolder; }
        }

        public InventoryItemFolder RootInventoryItemFolder
        {
            get { return _inventoryItems.RootFolder; }
        }

        public GUIFolder RootGUIFolder
        {
            get { return _guis.RootFolder; }
        }

        public UnloadedRoomFolder RootRoomFolder
        {
            get { return _rooms.RootFolder; }
        }

        public AudioClipFolder RootAudioClipFolder
        {
            get { return _audioClips.RootFolder; }
        }

        public IList<AudioClipType> AudioClipTypes
        {
            get { return _audioClipTypes; }
        }

        public Scripts Scripts
        {
            get 
            {
                Scripts scripts = new Scripts(_scripts);
                return scripts;
            }
        }

        public ScriptsAndHeaders ScriptsAndHeaders
        {
            get
            {
                ScriptsAndHeaders scriptsAndHeaders = new ScriptsAndHeaders(_scripts);
                return scriptsAndHeaders;
            }
        }

        // Used by the AGF->DTA compiler to bring in any extra modules
        public ScriptsAndHeaders ScriptsToCompile
        {
            get { return _scriptsToCompile; }
            set { _scriptsToCompile = value; }
        }

        public IList<AudioClip> CachedAudioClipListForCompile
        {
            get { return _cachedAudioClipListForCompile; }
        }

        public GlobalVariables GlobalVariables
        {
            get { return _globalVariables; }
        }

		IViewFolder IGame.Views
		{
			get { return _views.RootFolder; }
		}

        ISpriteFolder IGame.Sprites
        {
            get { return _sprites; }
        }

		/// <summary>
		/// The version of the Game.agf file that was loaded from disk.
		/// This is null if the game has not yet been saved.
		/// </summary>
		public string SavedXmlVersion
		{
			get { return _savedXmlVersion; }
			set { _savedXmlVersion = value; }
		}

        /// <summary>
        /// The editor version read from the Game.agf file that was loaded from disk.
        /// This is null if the game has not yet been saved or is an older version.
        /// </summary>
        public string SavedXmlEditorVersion
        {
            get { return _savedXmlEditorVersion; }
            set { _savedXmlEditorVersion = value; }
        }

        /// <summary>
        /// The version-index of the Game.agf file that was loaded from disk.
        /// This is null if the game has not yet been saved.
        /// </summary>
        public int? SavedXmlVersionIndex
        {
            get { return _savedXmlVersionIndex; }
            set { _savedXmlVersionIndex = value; }
        }

		/// <summary>
		/// Full path to the directory where the game is located
		/// </summary>
		public string DirectoryPath
		{
			get { return _directoryPath; }
			set { _directoryPath = value; }
		}

		/// <summary>
		/// If this is set, then the editor is more forceful about making
		/// the user save the game on exit.
		/// </summary>
		public bool FilesAddedOrRemoved
		{
			get { return _roomsAddedOrRemoved; }
			set { _roomsAddedOrRemoved = value; }
		}

        /// <summary>
        /// Causes the ViewListUpdated event to be fired. You should call this
        /// if you add/remove views and need the views component to update
        /// to reflect the changes.
        /// </summary>
        public void NotifyClientsViewsUpdated()
        {
            if (ViewListUpdated != null)
            {
                ViewListUpdated();
            }
        }

        public void NotifyClientsGUIAddedOrRemoved(GUI theGUI)
        {
            if (GUIAddedOrRemoved != null)
            {
                GUIAddedOrRemoved(theGUI);
            }
        }

        public void NotifyClientsGUIControlAddedOrRemoved(GUI owningGUI, GUIControl control)
        {
            if (GUIControlAddedOrRemoved != null)
            {
                GUIControlAddedOrRemoved(owningGUI, control);
            }
        }

		/// <summary>
		/// Returns the minimum height of the room background
		/// for the current game resolution.
		/// </summary>
        public int MinRoomHeight
        {
            get
            {
                return 1;
            }
        }

		/// <summary>
		/// Returns the minimum width of the room background
		/// for the current game resolution.
		/// </summary>
        public int MinRoomWidth
        {
            get
            {
                return 1;
            }
        }

		/// <summary>
		/// Returns the highest numbered View in the game
		/// </summary>
        public int ViewCount
        {
            get
            {
                return FindHighestViewNumber(RootViewFolder);
            }
        }

		/// <summary>
		/// Marks the view as deleted and available for re-creation
		/// </summary>
		public void ViewDeleted(int viewNumber)
		{
			_deletedViewIDs.Add(viewNumber, null);
		}

		public View CreateNewView(IViewFolder createInFolder)
		{
			if (createInFolder == null)
			{
				createInFolder = _views.RootFolder;
			}
			View newView = new View();
			newView.ID = FindAndAllocateAvailableViewID();
			newView.Name = "View" + newView.ID;
			createInFolder.Views.Add(newView);
			NotifyClientsViewsUpdated();
			return newView;
		}

		/// <summary>
		/// Returns an unused View ID and allocates it as in use
		/// </summary>
		public int FindAndAllocateAvailableViewID()
        {
			if (_deletedViewIDs.Count > 0)
			{
				foreach (int availableID in _deletedViewIDs.Keys)
				{
					_deletedViewIDs.Remove(availableID);
					return availableID;
				}
			}
            return FindHighestViewNumber(_views.RootFolder) + 1;
        }

        private int FindHighestViewNumber(ViewFolder folder)
        {
            int highest = 0;
            foreach (View view in folder.Views)
            {
                highest = Math.Max(view.ID, highest);
            }

            foreach (ViewFolder subFolder in folder.SubFolders)
            {
                int highestInSubFolder = FindHighestViewNumber(subFolder);
                highest = Math.Max(highest, highestInSubFolder);
            }

            return highest;
        }

		/// <summary>
		/// Returns the View object associated with the supplied ID
		/// </summary>
		public View FindViewByID(int viewNumber)
        {           
            return this.RootViewFolder.FindViewByID(viewNumber, true);
        }

		public Character FindCharacterByID(int charID)
		{
            return _characters.RootFolder.FindCharacterByID(charID, true);			
		}

		public UnloadedRoom FindRoomByID(int roomNumber)
        {
            return (UnloadedRoom)_rooms.RootFolder.FindUnloadedRoomByID(roomNumber, true);
        }

		public bool DoesRoomNumberAlreadyExist(int roomNumber)
		{
			return (FindRoomByID(roomNumber) != null);
		}

		public int FindFirstAvailableRoomNumber(int startingFromNumber)
		{
			do
			{
				startingFromNumber++;
			}
			while (DoesRoomNumberAlreadyExist(startingFromNumber));
			return startingFromNumber;
		}

        public AudioClip FindAudioClipForOldSoundNumber(IList<AudioClip> allAudio, int soundNumber)
        {
            if (allAudio == null)
            {
                allAudio = _audioClips.RootFolder.GetAllAudioClipsFromAllSubFolders();
            }
            string searchForName = string.Format("aSound{0}", soundNumber);
            foreach (AudioClip clip in allAudio)
            {
                if (clip.ScriptName == searchForName)
                {
                    return clip;
                }
            }
            return null;
        }

        public AudioClip FindAudioClipForOldMusicNumber(IList<AudioClip> allAudio, int musicNumber)
        {
            if (allAudio == null)
            {
                allAudio = _audioClips.RootFolder.GetAllAudioClipsFromAllSubFolders();
            }
            string searchForName = string.Format("aMusic{0}", musicNumber);
            foreach (AudioClip clip in allAudio)
            {
                if (clip.ScriptName == searchForName)
                {
                    return clip;
                }
            }
            return null;
        }

        // CLNUP this is a little confusing, IsHighResolution depends on GUIScaleFactor which depends on LowResolution
        // TODO: remove this after we have proper zoom controls in all editors;
        // default zoom-in should be relied on the actual image size if on anything
        public int GUIScaleFactor
        {
            get { return IsHighResolution ? 1 : 2; }
        }

        public int GetNextAudioIndex()
        {
            return ++_settings.AudioIndexer;
        }

        public PaletteEntry[] ReadPaletteFromXML(XmlNode parentOfPaletteNode)
        {
            PaletteEntry[] palette = new PaletteEntry[PALETTE_SIZE];
            for (int i = 0; i < palette.Length; i++)
            {
                palette[i] = new PaletteEntry(i, _palette[i].Colour);
                palette[i].ColourType = _palette[i].ColourType;
            }
            foreach (XmlNode palNode in SerializeUtils.GetChildNodes(parentOfPaletteNode, "Palette"))
            {
                PaletteEntry paletteEntry = palette[Convert.ToInt32(palNode.Attributes["Index"].InnerText)];
                paletteEntry.Colour = Color.FromArgb(
                    Convert.ToInt32(palNode.Attributes["Red"].InnerText),
                    Convert.ToInt32(palNode.Attributes["Green"].InnerText),
                    Convert.ToInt32(palNode.Attributes["Blue"].InnerText));
                paletteEntry.ColourType = (PaletteColourType)Enum.Parse(typeof(PaletteColourType), palNode.Attributes["Type"].InnerText);
            }
            return palette;
        }

        public void WritePaletteToXML(XmlTextWriter writer)
        {
            writer.WriteStartElement("Palette");

            int i = 0;
            foreach (PaletteEntry entry in _palette)
            {
                writer.WriteStartElement("PaletteEntry");
                writer.WriteAttributeString("Index", i.ToString());
                writer.WriteAttributeString("Type", entry.ColourType.ToString());
                writer.WriteAttributeString("Red", entry.Colour.R.ToString());
                writer.WriteAttributeString("Green", entry.Colour.G.ToString());
                writer.WriteAttributeString("Blue", entry.Colour.B.ToString());
                writer.WriteEndElement();
                i++;
            }

            writer.WriteEndElement();
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Game");

            _settings.ToXml(writer);
            _defaultSetup.ToXml(writer);

            _lipSync.ToXml(writer);

            _propertySchema.ToXml(writer);

            writer.WriteStartElement("GlobalMessages");
            int messageIndex = GLOBAL_MESSAGE_ID_START;
            foreach (string message in _globalMessages)
            {
                writer.WriteStartElement("Message");
                writer.WriteAttributeString("ID", messageIndex.ToString());
                writer.WriteValue(message);
                writer.WriteEndElement();
                messageIndex++;
            }
            writer.WriteEndElement();

			// We need to serialize the interaction variables in case
			// they don't upgrade a room until later, and it might
			// use the global interaction variables
			writer.WriteStartElement("OldInteractionVariables");
			foreach (OldInteractionVariable var in _oldInteractionVariables)
			{
				writer.WriteStartElement("Variable");
				writer.WriteAttributeString("Name", var.Name);
				writer.WriteAttributeString("Value", var.Value.ToString());
				writer.WriteEndElement();
			}
			writer.WriteEndElement();

			writer.WriteStartElement("Plugins");
            foreach (Plugin plugin in _plugins)
            {
                plugin.ToXml(writer);
            }
            writer.WriteEndElement();

            writer.WriteStartElement("Rooms");
            _rooms.ToXml(writer);
            writer.WriteEndElement();

            writer.WriteStartElement("GUIs");
            _guis.ToXml(writer);
            writer.WriteEndElement();

            writer.WriteStartElement("InventoryItems");
            _inventoryItems.ToXml(writer);            
            writer.WriteEndElement();

            writer.WriteStartElement("TextParser");
            _textParser.ToXml(writer);
            writer.WriteEndElement();

            writer.WriteStartElement("Characters");
            _characters.ToXml(writer);
            writer.WriteEndElement();

            writer.WriteElementString("PlayerCharacter", (_playerCharacter == null) ? string.Empty : _playerCharacter.ID.ToString());

            writer.WriteStartElement("Dialogs");
            _dialogs.ToXml(writer);            
            writer.WriteEndElement();

            writer.WriteStartElement("Cursors");
            foreach (MouseCursor cursor in _cursors)
            {
                cursor.ToXml(writer);
            }
            writer.WriteEndElement();

            writer.WriteStartElement("Fonts");
            foreach (Font font in _fonts)
            {
                font.ToXml(writer);
            }
            writer.WriteEndElement();

            WritePaletteToXML(writer);

            writer.WriteStartElement("GlobalVariables");
            _globalVariables.ToXml(writer);
            writer.WriteEndElement();

            writer.WriteStartElement("Sprites");
            _sprites.ToXml(writer);
            writer.WriteEndElement();

            writer.WriteStartElement("Views");
            _views.ToXml(writer);
            writer.WriteEndElement();

			writer.WriteStartElement("DeletedViews");
			foreach (int viewID in _deletedViewIDs.Keys)
			{
				writer.WriteElementString("ViewID", viewID.ToString());
			}
			writer.WriteEndElement();

            writer.WriteStartElement("Scripts");
            _scripts.ToXml(writer);
            writer.WriteEndElement();

            writer.WriteStartElement("AudioClips");
            _audioClips.ToXml(writer);
            writer.WriteEndElement();

            writer.WriteStartElement("AudioClipTypes");
            foreach (AudioClipType audioClipType in _audioClipTypes)
            {
                audioClipType.ToXml(writer);
            }
            writer.WriteEndElement();

            writer.WriteStartElement("Translations");
            foreach (Translation translation in _translations)
            {
                translation.ToXml(writer);
            }            
            writer.WriteEndElement();

            writer.WriteEndElement();
        }

        public void FromXml(XmlNode node)
        {
            node = node.SelectSingleNode("Game");

            _settings.FromXml(node);

            if (node.SelectSingleNode(_defaultSetup.GetType().Name) != null)
            {
                // Only for >= 3.4.1
                _defaultSetup.FromXml(node);
            }

            _lipSync.FromXml(node);
            _propertySchema.FromXml(node);

            if (node.SelectSingleNode("InventoryHotspotMarker") != null)
            {
                // Pre-3.0.3
                InventoryHotspotMarker marker = new InventoryHotspotMarker();
                marker.FromXml(node);
                _settings.InventoryHotspotMarker = marker;
            }

            foreach (XmlNode msgNode in SerializeUtils.GetChildNodes(node, "GlobalMessages"))
            {
                _globalMessages[Convert.ToInt32(msgNode.Attributes["ID"].InnerText) - GLOBAL_MESSAGE_ID_START] = msgNode.InnerText;
            }

            _plugins.Clear();
            foreach (XmlNode pluginNode in SerializeUtils.GetChildNodes(node, "Plugins"))
            {
                _plugins.Add(new Plugin(pluginNode));
            }

            _rooms = new UnloadedRoomFolders(node.SelectSingleNode("Rooms").FirstChild, node);
            
            _guis = new GUIFolders(node.SelectSingleNode("GUIs").FirstChild, node);            

            _inventoryItems = new InventoryItemFolders(node.SelectSingleNode("InventoryItems").FirstChild, node);            

            _textParser = new TextParser(node.SelectSingleNode("TextParser"));

            _characters = new CharacterFolders(node.SelectSingleNode("Characters").FirstChild, node);            

            _playerCharacter = null;
            string playerCharText = SerializeUtils.GetElementString(node, "PlayerCharacter");
            if (playerCharText.Length > 0)
            {
                int playerCharID = Convert.ToInt32(playerCharText);
                foreach (Character character in RootCharacterFolder.AllItemsFlat)
                {
                    if (character.ID == playerCharID)
                    {
                        _playerCharacter = character;
                        break;
                    }
                }
            }

            _dialogs = new DialogFolders(node.SelectSingleNode("Dialogs").FirstChild, node);                                    

            _cursors.Clear();
            foreach (XmlNode cursNode in SerializeUtils.GetChildNodes(node, "Cursors"))
            {
                _cursors.Add(new MouseCursor(cursNode));
            }

            _fonts.Clear();
            foreach (XmlNode fontNode in SerializeUtils.GetChildNodes(node, "Fonts"))
            {
                _fonts.Add(new Font(fontNode));
            }

            _palette = ReadPaletteFromXML(node);

            _sprites = new SpriteFolder(node.SelectSingleNode("Sprites").FirstChild);

            _views = new ViewFolders(node.SelectSingleNode("Views").FirstChild);

            _deletedViewIDs.Clear();
			if (node.SelectSingleNode("DeletedViews") != null)
			{
				foreach (XmlNode transNode in SerializeUtils.GetChildNodes(node, "DeletedViews"))
				{
					_deletedViewIDs.Add(Convert.ToInt32(transNode.InnerText), null);
				}
			}

            _scripts = new ScriptFolders(node.SelectSingleNode("Scripts").FirstChild, node);

            if (node.SelectSingleNode("AudioClips") != null)
            {
                _audioClips = new AudioClipFolders(node.SelectSingleNode("AudioClips").FirstChild);
            }
            else
            {
                _audioClips = new AudioClipFolders("Main");
                _audioClips.RootFolder.DefaultPriority = AudioClipPriority.Normal;
                _audioClips.RootFolder.DefaultRepeat = InheritableBool.False;
                _audioClips.RootFolder.DefaultVolume = 100;
            }

            _audioClipTypes.Clear();
            if (node.SelectSingleNode("AudioClipTypes") != null)
            {
                foreach (XmlNode clipTypeNode in SerializeUtils.GetChildNodes(node, "AudioClipTypes"))
                {
                    _audioClipTypes.Add(new AudioClipType(clipTypeNode));
                }
            }

            _translations.Clear();
            if (node.SelectSingleNode("Translations") != null)
            {
                foreach (XmlNode transNode in SerializeUtils.GetChildNodes(node, "Translations"))
                {
                    _translations.Add(new Translation(transNode));
                }
            }

            if (node.SelectSingleNode("GlobalVariables") != null)
            {
                _globalVariables = new GlobalVariables(node.SelectSingleNode("GlobalVariables"));
            }
            else
            {
                _globalVariables = new GlobalVariables();
            }

			_oldInteractionVariables.Clear();
			if (node.SelectSingleNode("OldInteractionVariables") != null)
			{
				foreach (XmlNode varNode in SerializeUtils.GetChildNodes(node, "OldInteractionVariables"))
				{
					_oldInteractionVariables.Add(new OldInteractionVariable(SerializeUtils.GetAttributeString(varNode, "Name"), SerializeUtils.GetAttributeInt(varNode, "Value")));
				}
			}

        }

        public bool IsScriptNameAlreadyUsed(string tryName, object ignoreObject)
        {
            if (tryName == string.Empty)
            {
                return false;
            }

			foreach (string name in RESERVED_SCRIPT_NAMES)
			{
				if (tryName == name)
				{
					return true;
				}
			}

            foreach (GUI gui in this.RootGUIFolder.AllItemsFlat)
            {
                if (gui != ignoreObject)
                {
                    if (gui.Name == tryName)
                    {
                        return true;
                    }

                    if (gui.Name.StartsWith("g") &&
                        (gui.Name.Length > 1) &&
                        (gui.Name.Substring(1).ToUpper() == tryName))
                    {
                        return true;
                    }
                }

                foreach (GUIControl control in gui.Controls)
                {
                    if ((control.Name == tryName) && (control != ignoreObject))
                    {
                        return true;
                    }
                }
            }

            foreach (InventoryItem item in this.RootInventoryItemFolder.AllItemsFlat)
            {
                if ((item.Name == tryName) && (item != ignoreObject))
                {
                    return true;
                }
            }

            foreach (Character character in this.RootCharacterFolder.AllItemsFlat)
            {
                if (character != ignoreObject)
                {
                    if (character.ScriptName == tryName)
                    {
                        return true;
                    }

                    if (character.ScriptName.StartsWith("c") &&
                        (character.ScriptName.Length > 1) &&
                        (character.ScriptName.Substring(1).ToUpper() == tryName))
                    {
                        return true;
                    }
                }
            }

            foreach (Dialog dialog in this.RootDialogFolder.AllItemsFlat)
            {
                if ((dialog.Name == tryName) && (dialog != ignoreObject))
                {
                    return true;
                }
            }

            if (IsNameUsedByAudioClip(tryName, this.RootAudioClipFolder, ignoreObject))
            {
                return true;
            }

            if (IsNameUsedByView(tryName, this.RootViewFolder, ignoreObject))
            {
                return true;
            }

            if ((_globalVariables[tryName] != null) &&
                (_globalVariables[tryName] != ignoreObject))
            {
                return true;
            }

            return false;
        }

        private bool IsNameUsedByAudioClip(string name, AudioClipFolder folderToCheck, object ignoreObject)
        {
            foreach (AudioClip clip in folderToCheck.Items)
            {
                if ((clip.ScriptName == name) && (clip != ignoreObject))
                {
                    return true;
                }
            }

            foreach (AudioClipFolder subFolder in folderToCheck.SubFolders)
            {
                if (IsNameUsedByAudioClip(name, subFolder, ignoreObject))
                {
                    return true;
                }
            }

            return false;
        }

        private bool IsNameUsedByView(string name, ViewFolder folderToCheck, object ignoreObject)
        {
            foreach (View view in folderToCheck.Views)
            {
                if ((view.Name.ToUpper() == name) && (view != ignoreObject))
                {
                    return true;
                }
            }

            foreach (ViewFolder subFolder in folderToCheck.SubFolders)
            {
                if (IsNameUsedByView(name, subFolder, ignoreObject))
                {
                    return true;
                }
            }

            return false;
        }

        public List<Script> GetAllGameAndLoadedRoomScripts()
        {
            List<Script> scripts = new List<Script>();
            foreach (Script script in this.RootScriptFolder.AllScriptsFlat)
            {
                scripts.Add(script);
            }
            foreach (UnloadedRoom room in this.RootRoomFolder.AllItemsFlat)
            {
                if (room.Script != null)
                {
                    scripts.Add(room.Script);
                }
            }
            return scripts;
        }

        public void UpdateCachedAudioClipList()
        {
            _cachedAudioClipListForCompile = _audioClips.RootFolder.GetAllAudioClipsFromAllSubFolders();
            _cachedAudioClipIndexMapping = new Dictionary<int, int>();
            for (int i = 0; i < _cachedAudioClipListForCompile.Count; i++)
            {
                _cachedAudioClipIndexMapping.Add(_cachedAudioClipListForCompile[i].Index, i);
            }
        }

        public int GetAudioArrayIndexFromAudioClipIndex(int audioClipIndex)
        {
            if (audioClipIndex > 0)
            {
                if (_cachedAudioClipIndexMapping.ContainsKey(audioClipIndex))
                {
                    return _cachedAudioClipIndexMapping[audioClipIndex];
                }
            }
            return -1;
        }

        public byte[] GetPaletteAsRawPAL()
        {
            byte[] rawPalette = new byte[768];
            for (int i = 0; i < _palette.Length; i++)
            {
                rawPalette[i * 3] = (byte)(_palette[i].Colour.R / 4);
                rawPalette[i * 3 + 1] = (byte)(_palette[i].Colour.G / 4);
                rawPalette[i * 3 + 2] = (byte)(_palette[i].Colour.B / 4);
            }
            return rawPalette;
        }

        public void SetPaletteFromRawPAL(byte[] rawPalette, bool resetColourTypes)
        {
            for (int i = 0; i < _palette.Length; i++)
            {
                _palette[i] = new PaletteEntry(i, Color.FromArgb(rawPalette[i * 3] * 4, rawPalette[i * 3 + 1] * 4, rawPalette[i * 3 + 2] * 4));
                if (resetColourTypes)
                {
                    if (i <= 41)
                    {
                        _palette[i].ColourType = PaletteColourType.Gamewide;
                    }
                    else
                    {
                        _palette[i].ColourType = PaletteColourType.Background;
                    }
                }
            }
        }

        private void InitializeDefaultPalette()
        {
            Stream palInput = GetType().Assembly.GetManifestResourceStream("AGS.Types.Resources.roomdef.pal");
            byte[] rawPalette = new byte[768];
            palInput.Read(rawPalette, 0, 768);
            palInput.Close();

            SetPaletteFromRawPAL(rawPalette, true);
        }

        public void SetScriptAPIForOldProject()
        {
            System.Version firstCompatibleVersion = new System.Version("3.4.0");
            System.Version firstVersionWithHighestConst = new System.Version("3.4.1");
            // Try to find corresponding ScriptAPI for older version game project that did not have such setting
            System.Version projectVersion = _savedXmlEditorVersion != null ? Utilities.TryParseVersion(_savedXmlEditorVersion) : null;

            if (projectVersion == null)
            {
                _settings.ScriptCompatLevel = ScriptAPIVersion.v321;
            }
            else
            {
                if (projectVersion < firstCompatibleVersion)
                {
                    // Devise the API version from enum values Description attribute
                    // and find the first version equal or higher than project's one.
                    Type t = typeof(ScriptAPIVersion);
                    string[] names = Enum.GetNames(t);
                    foreach (string n in names)
                    {
                        FieldInfo fi = t.GetField(n);
                        DescriptionAttribute[] attributes =
                          (DescriptionAttribute[])fi.GetCustomAttributes(
                          typeof(DescriptionAttribute), false);
                        if (attributes.Length == 0)
                            continue;
                        System.Version v = new System.Version(attributes[0].Description);
                        if (projectVersion <= v)
                        {
                            _settings.ScriptCompatLevel = (ScriptAPIVersion)Enum.Parse(t, n);
                            break;
                        }
                    }
                }
                if (projectVersion < firstVersionWithHighestConst && _settings.ScriptAPIVersion == ScriptAPIVersion.v340)
                {
                    // Convert API 3.4.0 into Highest constant if the game was made in AGS 3.4.0
                    _settings.ScriptAPIVersion = ScriptAPIVersion.Highest;
                }
            }
        }

    }
}
