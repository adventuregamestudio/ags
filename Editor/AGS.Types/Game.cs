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

        public const int MAX_INV_ITEMS = 300;
        public const int MAX_SOUND_CHANNELS = 16;
        public const int MAX_USER_SOUND_CHANNELS = MAX_SOUND_CHANNELS - 1; // 1 reserved for Speech

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
        // Maps AudioClip.Index (fixed ID) to AudioClip.ID
        private Dictionary<int, int> _audioClipIndexMapping;
        private string _directoryPath;
		private bool _roomsAddedOrRemoved = false;
		private SortedDictionary<int, object> _deletedViewIDs;
		private string _savedXmlVersion = null;
        private int? _savedXmlVersionIndex = null;
        private string _savedXmlEditorVersion = null;
        private int? _savedXmlEncodingCP = null;

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
			_deletedViewIDs = new SortedDictionary<int, object>();
            _scripts = new ScriptFolders(ScriptFolder.MAIN_SCRIPT_FOLDER_NAME);
            _scriptsToCompile = new ScriptsAndHeaders();
            ScriptAndHeader globalScript = new ScriptAndHeader(
                new Script(Script.GLOBAL_HEADER_FILE_NAME, "// script header\r\n", true),
                new Script(Script.GLOBAL_SCRIPT_FILE_NAME, "// global script\r\n", false));
            ((IList<ScriptAndHeader>)_scripts).Add(globalScript);            
            _playerCharacter = null;

            InitializeDefaultPalette();
        }

        public Encoding TextEncoding
        {
            get {  return Utilities.EncodingFromName(_settings.GameTextEncoding); }
        }

        public bool UnicodeMode
        {
            get { return string.Compare(_settings.GameTextEncoding, "UTF-8", true) == 0; }
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

        public IList<Character> CharacterFlatList
        {
            get { return _characters.FlatList; }
        }

        public DialogFolder RootDialogFolder
        {
            get { return _dialogs.RootFolder; }
        }

        public IList<Dialog> DialogFlatList
        {
            get { return _dialogs.FlatList; }
        }

        public ViewFolder RootViewFolder
        {
            get { return _views.RootFolder; }
            set { _views = new ViewFolders(value); }
        }

        public IList<View> ViewFlatList
        {
            get { return _views.FlatList; }
        }

        public ScriptFolder RootScriptFolder
        {
            get { return _scripts.RootFolder; }
        }

        public InventoryItemFolder RootInventoryItemFolder
        {
            get { return _inventoryItems.RootFolder; }
        }

        public IList<InventoryItem> InventoryFlatList
        {
            get { return _inventoryItems.FlatList; }
        }

        public GUIFolder RootGUIFolder
        {
            get { return _guis.RootFolder; }
        }

        public IList<GUI> GUIFlatList
        {
            get { return _guis.FlatList; }
        }

        public UnloadedRoomFolder RootRoomFolder
        {
            get { return _rooms.RootFolder; }
        }

        public AudioClipFolder RootAudioClipFolder
        {
            get { return _audioClips.RootFolder; }
        }

        public IList<AudioClip> AudioClipFlatList
        {
            get { return _audioClips.FlatList; }
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

        public IList<AudioClip> AudioClips
        {
            get { return _audioClips; }
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
        /// The code page of the Game.agf file that was loaded from disk.
        /// </summary>
        public int? SavedXmlEncodingCodePage
        {
            get { return _savedXmlEncodingCP; }
            set { _savedXmlEncodingCP = value; }
        }

		/// <summary>		/// Full path to the directory where the game is located		/// </summary>
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

        /// <summary>
		/// Returns specific View ID and allocates it as in use
		/// </summary>
        public int GetAndAllocateViewID(int id)
        {
            if (_deletedViewIDs.ContainsKey(id))
            {
                _deletedViewIDs.Remove(id);
            }
            return id;
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

        // CLNUP
        // TODO: remove this after we have proper zoom controls in all editors;
        // default zoom-in should be relied on the actual image size if on anything
        public int GUIScaleFactor
        {
            get { return _settings.HighResolution ? 1 : 2; }
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

            if (parentOfPaletteNode.SelectSingleNode("Palette") == null)
                return palette;

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
            // Sort of a hack: ensure that text encoding is set for Scripts
            // early, in case any scripts are preloaded during project load.
            Script.TextEncoding = TextEncoding;

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

            _plugins.Clear();
            if (node.SelectSingleNode("Plugins") != null)
            {
                foreach (XmlNode pluginNode in SerializeUtils.GetChildNodes(node, "Plugins"))
                {
                    _plugins.Add(new Plugin(pluginNode));
                }
            }

            _rooms = new UnloadedRoomFolders(SerializeUtils.GetFirstChild(node, "Rooms"), node);
            
            _guis = new GUIFolders(SerializeUtils.GetFirstChild(node, "GUIs"), node);            

            _inventoryItems = new InventoryItemFolders(SerializeUtils.GetFirstChild(node, "InventoryItems"), node);

            if (node.SelectSingleNode("TextParser") != null)
                _textParser = new TextParser(node.SelectSingleNode("TextParser"));
            else
                _textParser = new TextParser();

            _characters = new CharacterFolders(SerializeUtils.GetFirstChild(node, "Characters"), node);            

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

            _dialogs = new DialogFolders(SerializeUtils.GetFirstChild(node, "Dialogs"), node);                                    

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

            var spriteNode = SerializeUtils.GetFirstChild(node, "Sprites");
            if (spriteNode != null)
                _sprites = new SpriteFolder(spriteNode);
            else
                _sprites = new SpriteFolder("Main");

            var viewNode = SerializeUtils.GetFirstChild(node, "Views");
            if (viewNode != null)
                _views = new ViewFolders(viewNode);
            else
                _views = new ViewFolders("Main");

            _deletedViewIDs.Clear();
			if (node.SelectSingleNode("DeletedViews") != null)
			{
				foreach (XmlNode transNode in SerializeUtils.GetChildNodes(node, "DeletedViews"))
				{
					_deletedViewIDs.Add(Convert.ToInt32(transNode.InnerText), null);
				}
			}

            _scripts = new ScriptFolders(SerializeUtils.GetFirstChild(node, "Scripts"), node);

            if (node.SelectSingleNode("AudioClips") != null)
            {
                _audioClips = new AudioClipFolders(SerializeUtils.GetFirstChild(node, "AudioClips"));
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
                    _translations.Add(new Translation(transNode, Settings.GameTextLanguage));
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

        // NOTE: when to update FixedID->ID map is mostly a question of
        // when and how is it used. As of now it is only called when writing game data, -
        // that is where we need to know real ordered clip ID. But in universal case
        // the map will have to be updated whenever a sound is added, deleted
        // or have its ID changed.
        public void UpdateAudioClipMap()    
        {
            _audioClipIndexMapping = new Dictionary<int, int>();
            foreach (AudioClip clip in _audioClips)
            {
                _audioClipIndexMapping.Add(clip.Index, clip.ID);
            }
        }

        public int GetAudioArrayIDFromFixedIndex(int fixedIndex)
        {
            int id;
            if (fixedIndex >= AudioClip.FixedIndexBase &&
                _audioClipIndexMapping.TryGetValue(fixedIndex, out id))
            {
                return id;
            }
            return AudioClip.IDNoValue;
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
                // We adjust compatibility setting in following situations:
                // *) project is older than the compatibility setting itself
                if (projectVersion < firstCompatibleVersion)
                {
                    // TODO: Description attribute of ScriptAPIVersion enum is not reliable
                    // source of the corresponding project version. Find some other way to
                    // automate this, like a custom attribute (otherwise it may be difficult
                    // to maintain in long term).
                    string[] versions = new string[] { "3.2.1", "3.3.0", "3.3.4", "3.3.5", "3.4.0" };
                    // Find the first version equal or higher than project's one.
                    int last_value = (int)ScriptAPIVersion.Highest;
                    for (int i = 0; i < versions.Length; ++i)
                    {
                        System.Version v = new System.Version(versions[i]);
                        if (projectVersion < v)
                            break;
                        last_value = i;
                        if (projectVersion == v)
                            break;
                    }
                    _settings.ScriptCompatLevel = (ScriptAPIVersion)last_value;
                }
                // Convert API 3.4.0 into Highest constant if the game was made in AGS 3.4.0
                if (projectVersion < firstVersionWithHighestConst && _settings.ScriptAPIVersion == ScriptAPIVersion.v340)
                {
                    _settings.ScriptAPIVersion = ScriptAPIVersion.Highest;
                }
            }
        }

    }
}
