// The available script API is determined by two bounds: upper bound
// determines which new API parts will be enabled, and lower bound
// determines which obsolete API parts will be disabled.
//
// Upper bound depends on defined SCRIPT_API_vXXX macros. For every macro
// defined the corresponding API contents should be enabled. If certain
// macro is not defined, then those API version contents stay disabled.
//
// Lower bound depends on defined SCRIPT_COMPAT_vXXX macros. For every
// macro defined the deprecated API contents that were still active in
// corresponding version are kept enabled; otherwise these are disabled.

#define function int  // $AUTOCOMPLETEIGNORE$
// CursorMode isn't actually defined yet, but int will do
#define CursorMode int
#define FontType int
#define AudioType int
// MAX_INV is a topmost index, AGS_MAX_INV_ITEMS is max count
#define MAX_INV 301
#define AGS_MAX_INV_ITEMS 300
#define AGS_MAX_OBJECTS   256
#define AGS_MAX_HOTSPOTS  50
#define AGS_MAX_REGIONS   16
#ifdef SCRIPT_API_v330
  // MAX_ROOM_OBJECTS is a duplicate and was added by an oversight
  #define MAX_ROOM_OBJECTS 40
#endif // SCRIPT_API_v330
#ifdef SCRIPT_COMPAT_v335
  #define AGS_MAX_CONTROLS_PER_GUI 30
#endif // SCRIPT_COMPAT_v335
#define MAX_LEGACY_GLOBAL_VARS  50
#define MAX_LEGACY_SAVED_GAMES  50
#define PALETTE_SIZE   256
#define FOLLOW_EXACTLY 32766
#define NARRATOR -1

#define OPT_DEBUGMODE        0
// OPT_SCORESOUND            1   // [HIDDEN], only read once on startup
#define OPT_WALKONLOOK       2
#define OPT_DIALOGOPTIONSGUI 3
#define OPT_ANTIGLIDE        4   // $AUTOCOMPLETEIGNORE$
#define OPT_TWCUSTOM         5
#define OPT_DIALOGOPTIONSGAP 6
#define OPT_NOSKIPTEXT       7   // $AUTOCOMPLETEIGNORE$
#define OPT_WHENGUIDISABLED  8
#define OPT_ALWAYSSPEECH     9
#define OPT_SPEECHTYPE      10   // $AUTOCOMPLETEIGNORE$
#define OPT_PIXELPERFECT    11
#define OPT_NOWALKMODE      12
#define OPT_LETTERBOX       13
#define OPT_FIXEDINVCURSOR  14
#define OPT_DONTLOSEINV     15   // $AUTOCOMPLETEIGNORE$
// OPT_HIRES_FONTS          16   // [HIDDEN]
// OPT_SPLITRESOURCES       17   // [HIDDEN]
#define OPT_TURNBEFOREWALK  18
// OPT_FADETYPE             19   // [HIDDEN], only read once on startup
#define OPT_HANDLEINVCLICKS 20
#define OPT_MOUSEWHEEL      21
#define OPT_DIALOGNUMBERED  22
#define OPT_DIALOGUPWARDS   23
#define OPT_CROSSFADEMUSIC  24
#define OPT_ANTIALIASFONTS  25
#define OPT_THOUGHTGUI      26
#define OPT_TURNWHENFACING  27
#define OPT_RIGHTTOLEFT     28
#define OPT_MULTIPLEINV     29
#define OPT_SAVEGAMESCREENSHOTS 30
#define OPT_PORTRAITPOSITION  31
// OPT_STRICTSCRIPTING        32 // [HIDDEN]
// OPT_LEFTTORIGHTEVAL        33 // [HIDDEN]
// OPT_COMPRESSSPRITES        34 // [HIDDEN]
// OPT_STRICTSTRINGS          35 // [HIDDEN]
#define OPT_GUIALPHABLEND     36
#define OPT_RUNGAMEINDLGOPTS  37
#define OPT_NATIVECOORDINATES 38
#define OPT_GLOBALTALKANIMSPD 39 // $AUTOCOMPLETEIGNORE$
#define OPT_SPRITEALPHABLEND  40
// OPT_SAFEFILEPATHS          41 // [HIDDEN]
#define OPT_DIALOGOPTIONSAPI  42
#define OPT_BASESCRIPTAPI     43
#define OPT_SCRIPTCOMPATLEV   44
// OPT_RENDERATSCREENRES      45 // [HIDDEN], only read once on startup
// OPT_RELATIVEASSETRES       46 // [HIDDEN]
#define OPT_WALKSPEEDABSOLUTE 47
#define OPT_CLIPGUICONTROLS   48
#define OPT_GAMETEXTENCODING  49
#define OPT_KEYHANDLEAPI      50
// OPT_CUSTOMENGINETAG        51 // [HIDDEN]
#define OPT_SCALECHAROFFSETS  52
#define OPT_SAVEGAMESCREENSHOTLAYER 53
#define OPT_VOICECLIPNAMERULE 54
#define OPT_SAVECOMPONENTSIGNORE 55
// OPT_GAMEFPS                56 // [HIDDEN], only read once on startup
#define OPT_GUICONTROLMOUSEBUT 57
#define OPT_LIPSYNCTEXT       99

#define COLOR_TRANSPARENT -1
#define DIALOG_PARSER_SELECTED -3053
#define RUN_DIALOG_RETURN        -1
#define RUN_DIALOG_STOP_DIALOG   -2
#define RUN_DIALOG_GOTO_PREVIOUS -4

#define SCR_NO_VALUE   31998   // $AUTOCOMPLETEIGNORE$

enum bool {
  false = 0,
  true = 1
};

enum eOperatingSystem {
  eOSDOS = 1,
  eOSWindows,
  eOSLinux,
  eOSMacOS,
  eOSAndroid,
  eOSiOS,
  eOSPSP,
  eOSWeb,
  eOSFreeBSD
};

enum TransitionStyle {
  eTransitionFade=0,
  eTransitionInstant,
  eTransitionDissolve,
  eTransitionBoxout,
  eTransitionCrossfade
};

enum MouseButton {
  eMouseNone = 0,
  eMouseLeft = 1,
  eMouseRight = 2,
  eMouseMiddle = 3,
  eMouseLeftInv = 5,
  eMouseRightInv = 6,
  eMouseMiddleInv = 7,
  eMouseWheelNorth = 8,
  eMouseWheelSouth = 9
};

enum RoundDirection {
  eRoundDown = 0,
  eRoundNearest = 1,
  eRoundUp = 2
};

enum RepeatStyle {
  eOnce = 0,
  eRepeat = 1
};

#ifdef SCRIPT_API_v350
enum Alignment {
  eAlignNone          = 0,

  eAlignTopLeft       = 1,
  eAlignTopCenter     = 2,
  eAlignTopRight      = 4,
  eAlignMiddleLeft    = 8,
  eAlignMiddleCenter  = 16,
  eAlignMiddleRight   = 32,
  eAlignBottomLeft    = 64,
  eAlignBottomCenter  = 128,
  eAlignBottomRight   = 256,

  // Masks are helping to determine whether alignment parameter contains
  // particular horizontal or vertical component (for example: left side
  // or bottom side)
  eAlignHasLeft       = 73,
  eAlignHasRight      = 292,
  eAlignHasTop        = 7,
  eAlignHasBottom     = 448,
  eAlignHasHorCenter  = 146,
  eAlignHasVerCenter  = 56
};

enum HorizontalAlignment {
  // eq eAlignTopLeft
  eAlignLeft = 1,
  // eq eAlignTopCenter
  eAlignCenter = 2,
#ifdef SCRIPT_COMPAT_v341
  eAlignCentre = 2,
#endif // SCRIPT_COMPAT_v341
  // eq eAlignTopRight
  eAlignRight = 4
};
#else // !SCRIPT_API_v350
// Pre-3.5.0 Alignment enum was horizontal-only
enum Alignment {
  eAlignLeft = 1,
  eAlignCentre = 2,
  eAlignRight = 3
};

#define HorizontalAlignment Alignment
#endif // !SCRIPT_API_v350

enum LocationType {
  eLocationNothing = 0,
  eLocationHotspot = 1,
  eLocationCharacter = 2,
  eLocationObject = 3
};

enum CutsceneSkipType {
  eSkipESCOnly = 1,
  eSkipAnyKey = 2,
  eSkipMouseClick = 3,
  eSkipAnyKeyOrMouseClick = 4,
  eSkipESCOrRightButton = 5,
  eSkipScriptOnly = 6
};

enum DialogOptionState {
  eOptionOff = 0,
  eOptionOn = 1,
  eOptionOffForever = 2
};

enum eSpeechStyle {
  eSpeechLucasarts = 0,
  eSpeechSierra = 1,
  eSpeechSierraWithBackground = 2,
  eSpeechFullScreen = 3
};

enum eVoiceMode {
  eSpeechTextOnly = 0,
  eSpeechVoiceAndText = 1,
  eSpeechVoiceOnly = 2
};

enum eFlipDirection {
  eFlipLeftToRight = 1,
  eFlipUpsideDown = 2,
  eFlipBoth = 3
};

enum eCDAudioFunction {
  eCDIsDriverPresent = 0,
  eCDGetPlayingStatus,
  eCDPlayTrack,
  eCDPausePlayback,
  eCDResumePlayback,
  eCDGetNumTracks,
  eCDEject,
  eCDCloseTray,
  eCDGetCDDriveCount,
  eCDSelectActiveCDDrive
};

enum DialogOptionSayStyle
{
  eSayUseOptionSetting = 1,
  eSayAlways = 2,
  eSayNever = 3
};

enum VideoSkipStyle
{
  eVideoSkipNotAllowed = 0,
  eVideoSkipEscKey = 1,
  eVideoSkipAnyKey = 2,
  eVideoSkipAnyKeyOrMouse = 3
};

#ifdef SCRIPT_API_v363
enum VideoPlayStyle
{
  eVideoPlayDefault = 0,
  eVideoPlayStretchToScreen = 1,
  eVideoPlayNoAudio = 10,
  eVideoPlayAudioAndDontMuteGame = 20
};
#endif // SCRIPT_API_v363

enum eKeyCode
{
#ifdef SCRIPT_API_v330
  eKeyNone  = 0,
#endif // SCRIPT_API_v330
  eKeyCtrlA = 1,
  eKeyCtrlB = 2,
  eKeyCtrlC = 3,
  eKeyCtrlD = 4,
  eKeyCtrlE = 5,
  eKeyCtrlF = 6,
  eKeyCtrlG = 7,
  eKeyCtrlH = 8,
  eKeyBackspace = 8,
  eKeyCtrlI = 9,
  eKeyTab = 9,
  eKeyCtrlJ = 10,
  eKeyCtrlK = 11,
  eKeyCtrlL = 12,
  eKeyCtrlM = 13,
  eKeyReturn = 13,
  eKeyCtrlN = 14,
  eKeyCtrlO = 15,
  eKeyCtrlP = 16,
  eKeyCtrlQ = 17,
  eKeyCtrlR = 18,
  eKeyCtrlS = 19,
  eKeyCtrlT = 20,
  eKeyCtrlU = 21,
  eKeyCtrlV = 22,
  eKeyCtrlW = 23,
  eKeyCtrlX = 24,
  eKeyCtrlY = 25,
  eKeyCtrlZ = 26,
  eKeyEscape = 27,
  eKeySpace = 32,
  eKeyExclamationMark = 33,
  eKeyDoubleQuote = 34,
  eKeyHash = 35,
  eKeyDollar = 36,
  eKeyPercent = 37,
  eKeyAmpersand = 38,
  eKeySingleQuote = 39,
  eKeyOpenParenthesis = 40,
  eKeyCloseParenthesis = 41,
  eKeyAsterisk = 42,
  eKeyPlus = 43,
  eKeyComma = 44,
  eKeyHyphen = 45,
  eKeyPeriod = 46,
  eKeyForwardSlash = 47,
  eKey0 = 48,
  eKey1 = 49,
  eKey2 = 50,
  eKey3 = 51,
  eKey4 = 52,
  eKey5 = 53,
  eKey6 = 54,
  eKey7 = 55,
  eKey8 = 56,
  eKey9 = 57,
  eKeyColon = 58,
  eKeySemiColon = 59,
  eKeyLessThan = 60,
  eKeyEquals = 61,
  eKeyGreaterThan = 62,
  eKeyQuestionMark = 63,
  eKeyAt = 64,
  eKeyA = 65,
  eKeyB = 66,
  eKeyC = 67,
  eKeyD = 68,
  eKeyE = 69,
  eKeyF = 70,
  eKeyG = 71,
  eKeyH = 72,
  eKeyI = 73,
  eKeyJ = 74,
  eKeyK = 75,
  eKeyL = 76,
  eKeyM = 77,
  eKeyN = 78,
  eKeyO = 79,
  eKeyP = 80,
  eKeyQ = 81,
  eKeyR = 82,
  eKeyS = 83,
  eKeyT = 84,
  eKeyU = 85,
  eKeyV = 86,
  eKeyW = 87,
  eKeyX = 88,
  eKeyY = 89,
  eKeyZ = 90,
  eKeyOpenBracket = 91,
  eKeyBackSlash = 92,
  eKeyCloseBracket = 93,
  eKeyUnderscore = 95,
  eKeyF1 = 359,
  eKeyF2 = 360,
  eKeyF3 = 361,
  eKeyF4 = 362,
  eKeyF5 = 363,
  eKeyF6 = 364,
  eKeyF7 = 365,
  eKeyF8 = 366,
  eKeyF9 = 367,
  eKeyF10 = 368,
  eKeyHome = 371,
  eKeyUpArrow = 372,
  eKeyPageUp = 373,
  eKeyLeftArrow = 375,
  eKeyNumPad5 = 376,
  eKeyRightArrow = 377,
  eKeyEnd = 379,
  eKeyDownArrow = 380,
  eKeyPageDown = 381,
  eKeyInsert = 382,
  eKeyDelete = 383,
#ifdef SCRIPT_API_v360
  eKeyShiftLeft = 403, 
  eKeyShiftRight = 404, 
  eKeyCtrlLeft = 405, 
  eKeyCtrlRight = 406,
  eKeyAltLeft = 407,
  eKeyAltRight = 420,
#endif // SCRIPT_API_v360
  eKeyF11 = 433,
  eKeyF12 = 434,

#ifdef SCRIPT_API_v36026
  eKeyCodeMask = 0x0FFF
#endif // SCRIPT_API_v36026
};

#ifdef SCRIPT_API_v360
enum eKeyMod
{
  eKeyModShiftLeft  = 0x00010000,
  eKeyModShiftRight = 0x00020000,
  eKeyModShift      = 0x00030000,
  eKeyModCtrlLeft   = 0x00040000,
  eKeyModCtrlRight  = 0x00080000,
  eKeyModCtrl       = 0x000C0000,
  eKeyModAltLeft    = 0x00100000,
  eKeyModAltRight   = 0x00200000,
  eKeyModAlt        = 0x00300000,
  eKeyModNum        = 0x00400000,
  eKeyModCaps       = 0x00800000,

  eKeyModMask       = 0x00FF0000
};
#endif // SCRIPT_API_v360

#ifdef SCRIPT_API_v3507
managed struct Point {
	int x, y;
};
#endif // SCRIPT_API_v3507

#define CHARID int  // $AUTOCOMPLETEIGNORE$
struct ColorType {
  char r,g,b;
  char filler;  // $AUTOCOMPLETEIGNORE$
  };

enum AudioFileType {
  eAudioFileOGG = 1,
  eAudioFileMP3 = 2,
  eAudioFileWAV = 3,
  eAudioFileVOC = 4,
  eAudioFileMIDI = 5,
  eAudioFileMOD = 6
};

enum AudioPriority {
  eAudioPriorityVeryLow = 1,
  eAudioPriorityLow = 25,
  eAudioPriorityNormal = 50,
  eAudioPriorityHigh = 75,
  eAudioPriorityVeryHigh = 100
};

enum ChangeVolumeType {
  eVolChangeExisting = 1678,
  eVolSetFutureDefault = 1679,
  eVolExistingAndFuture = 1680
};

#ifdef SCRIPT_API_v340
enum CharacterDirection {
  eDirectionDown = 0,
  eDirectionLeft,
  eDirectionRight,
  eDirectionUp,
  eDirectionDownRight,
  eDirectionUpRight,
  eDirectionDownLeft,
  eDirectionUpLeft,
  eDirectionNone = SCR_NO_VALUE
};
#endif // SCRIPT_API_v340

#ifdef SCRIPT_API_v350
enum StringCompareStyle
{
  eCaseInsensitive = 0,
  eCaseSensitive = 1
};

enum SortStyle
{
  eNonSorted = 0,
  eSorted = 1
};
#endif // SCRIPT_API_v350

#ifdef SCRIPT_API_v360
enum LogLevel
{
  eLogAlert = 1,
  eLogFatal = 2,
  eLogError = 3,
  eLogWarn = 4,
  eLogInfo = 5,
  eLogDebug = 6
};
#endif // SCRIPT_API_v360

#ifdef SCRIPT_API_v36026
enum InputType
{
  eInputNone     = 0x00000000,
  // 0x0100... is used internally to define Timeout (may be worked around in the future)
  eInputKeyboard = 0x02000000,
  eInputMouse    = 0x04000000,
  eInputAny      = 0xFF000000
};
#endif // SCRIPT_API_v36026

#ifdef SCRIPT_API_v362
enum RenderLayer
{
  eRenderLayerNone      = 0x00000000,
  eRenderLayerEngine    = 0x00000001,
  eRenderLayerCursor    = 0x00000002,
  eRenderLayerUI        = 0x00000004,
  eRenderLayerRoom      = 0x00000008,
  eRenderLayerAll       = 0xFFFFFFFF
};

enum FileSortStyle
{
  eFileSort_None		= 0,
  eFileSort_Name		= 1,
  eFileSort_Time		= 2
};

enum SaveGameSortStyle
{
  eSaveGameSort_None	= 0,
  eSaveGameSort_Number	= 1,
  eSaveGameSort_Time	= 2,
  eSaveGameSort_Description = 3
};

enum SortDirection
{
  eSortNoDirection		= 0,
  eSortAscending		= 1,
  eSortDescending		= 2
};
#endif // SCRIPT_API_v362

#ifdef SCRIPT_API_v363
enum DialogOptionsNumbering
{
  eDialogOptNumbers_Disabled  = -1,
  eDialogOptNumbers_KeysOnly  = 0,
  eDialogOptNumbers_Display   = 1
};
#endif // SCRIPT_API_v363

enum EventType {
  eEventLeaveRoom		= 1,
  eEventEnterRoomBeforeFadein = 2,
  // 3 is reserved by an obsolete "death" event
  eEventGotScore		= 4,
  eEventGUIMouseDown	= 5,
  eEventGUIMouseUp		= 6,
  eEventAddInventory	= 7,
  eEventLoseInventory	= 8,
  eEventRestoreGame		= 9,
#ifdef SCRIPT_API_v36026
  eEventEnterRoomAfterFadein = 10,
#endif // SCRIPT_API_v36026
#ifdef SCRIPT_API_v361
  eEventLeaveRoomAfterFadeout = 11,
  eEventGameSaved		= 12,
#endif // SCRIPT_API_v361
#ifdef SCRIPT_API_v362
  eEventDialogStart		= 13,
  eEventDialogStop		= 14,
  eEventDialogRun		= 15,
  eEventDialogOptionsOpen = 16,
  eEventDialogOptionsClose = 17,
  eEventSavesScanComplete = 18,
#endif // SCRIPT_API_v362

#ifdef SCRIPT_API_v362
  eEventUserEvent		= 10000
#endif // SCRIPT_API_v362
};


internalstring autoptr builtin managed struct String {
  /// Creates a formatted string using the supplied parameters.
  import static String Format(const string format, ...);    // $AUTOCOMPLETESTATICONLY$
  /// Checks whether the supplied string is null or empty.
  import static bool IsNullOrEmpty(String stringToCheck);  // $AUTOCOMPLETESTATICONLY$
  /// Returns a new string with the specified string appended to this string.
  import String  Append(const string appendText);
  /// Returns a new string that has the extra character appended.
  import String  AppendChar(int extraChar);
  import int     Contains(const string needle);   // $AUTOCOMPLETEIGNORE$
  /// Creates a copy of the string.
  import String  Copy();
  /// Returns the index of the first occurrence of the needle in this string.
  import int     IndexOf(const string needle);
  /// Returns a lower-cased version of this string.
  import String  LowerCase();
  /// Returns a new string, with the specified character changed.
  import String  ReplaceCharAt(int index, int newChar);
  /// Returns a portion of the string.
  import String  Substring(int index, int length);
  /// Truncates the string down to the specified length by removing characters from the end.
  import String  Truncate(int length);
  /// Returns an upper-cased version of this string.
  import String  UpperCase();
#ifdef SCRIPT_API_v350
  /// Compares this string to the other string.
  import int     CompareTo(const string otherString, StringCompareStyle style = eCaseInsensitive);
  /// Checks whether this string ends with the specified text.
  import bool    EndsWith(const string endsWithText, StringCompareStyle style = eCaseInsensitive);
  /// Returns a copy of this string with all occurrences of LookForText replaced with ReplaceWithText
  import String  Replace(const string lookForText, const string replaceWithText, StringCompareStyle style = eCaseInsensitive);
  /// Checks whether this string starts with the specified text.
  import bool    StartsWith(const string startsWithText, StringCompareStyle style = eCaseInsensitive);
#else // !SCRIPT_API_v350
  /// Compares this string to the other string.
  import int     CompareTo(const string otherString, bool caseSensitive = false);
  /// Checks whether this string ends with the specified text.
  import bool    EndsWith(const string endsWithText, bool caseSensitive = false);
  /// Returns a copy of this string with all occurrences of LookForText replaced with ReplaceWithText
  import String  Replace(const string lookForText, const string replaceWithText, bool caseSensitive = false);
  /// Checks whether this string starts with the specified text.
  import bool    StartsWith(const string startsWithText, bool caseSensitive = false);
#endif // !SCRIPT_API_v350
  /// Converts the string to a float.
  readonly import attribute float AsFloat;
  /// Converts the string to an integer.
  readonly import attribute int AsInt;
  /// Accesses individual characters of the string.
  readonly import attribute int Chars[];
  /// Returns the length of the string.
  readonly import attribute int Length;
};

#ifdef SCRIPT_API_v350
builtin managed struct Dictionary
{
  /// Creates a new empty Dictionary of the given properties.
  import static Dictionary* Create(SortStyle sortStyle = eNonSorted, StringCompareStyle compareStyle = eCaseInsensitive); // $AUTOCOMPLETESTATICONLY$

  /// Removes all items from the dictionary.
  import void Clear();
  /// Tells if given key is in the dictionary.
  import bool Contains(const string key);
  /// Gets value by the key; returns null if such key does not exist.
  import String Get(const string key);
  /// Removes key/value pair from the dictionary, fails if there was no such key.
  import bool Remove(const string key);
  /// Assigns a value to the given key, adds this key if it did not exist yet.
  import bool Set(const string key, const string value);

  /// Gets if this dictionary is case-sensitive.
  import readonly attribute StringCompareStyle CompareStyle;
  /// Gets the method items are arranged in this dictionary.
  import readonly attribute SortStyle SortStyle;
  /// Gets the number of key/value pairs currently in the dictionary.
  import readonly attribute int ItemCount;
  /// Creates a dynamic array filled with keys in same order as they are stored in the Dictionary.
  import String[] GetKeysAsArray();
  /// Creates a dynamic array filled with values in same order as they are stored in the Dictionary.
  import String[] GetValuesAsArray();
};

builtin managed struct Set
{
  /// Creates a new empty Set of the given properties.
  import static Set* Create(SortStyle sortStyle = eNonSorted, StringCompareStyle compareStyle = eCaseInsensitive); // $AUTOCOMPLETESTATICONLY$

  /// Adds item to the set, fails if such item was already existing.
  import bool Add(const string item);
  /// Removes all items from the set.
  import void Clear();
  /// Tells if given item is in the set.
  import bool Contains(const string item);
  /// Removes item from the set, fails if there was no such item.
  import bool Remove(const string item);

  /// Gets if this set is case-sensitive.
  import readonly attribute StringCompareStyle CompareStyle;
  /// Gets the method items are arranged in this set.
  import readonly attribute SortStyle SortStyle;
  /// Gets the number of items currently in the set.
  import readonly attribute int ItemCount;
  /// Creates a dynamic array filled with items in same order as they are stored in the Set.
  import String[] GetItemsAsArray();
};
#endif // SCRIPT_API_v350

builtin managed struct AudioClip;

builtin managed struct ViewFrame {
  /// Gets whether this frame is flipped.
  readonly import attribute bool Flipped;
  /// Gets the frame number of this frame.
  readonly import attribute int Frame;
  /// Gets/sets the sprite that is displayed by this frame.
  import attribute int Graphic;
  /// Gets/sets the audio that is played when this frame comes around.
  import attribute AudioClip* LinkedAudio;
  /// Gets the loop number of this frame.
  readonly import attribute int Loop;
  /// Gets/sets the sound that is played when this frame comes around.
  import attribute int Sound;    // $AUTOCOMPLETEIGNORE$
  /// Gets the delay of this frame.
  readonly import attribute int Speed;
  /// Gets the view number that this frame is part of.
  readonly import attribute int View;
};

builtin managed struct DrawingSurface {
  /// Clears the surface to the specified colour, or transparent if you do not specify a colour.
  import void Clear(int colour=COLOR_TRANSPARENT);
  /// Creates a copy of the surface.
  import DrawingSurface* CreateCopy();
  /// Draws a circle onto the surface with its centre at (x,y).
  import void DrawCircle(int x, int y, int radius);
#ifdef SCRIPT_API_v360
  /// Draws a sprite onto the surface with its top-left corner at (x,y).
  import void DrawImage(int x, int y, int spriteSlot, int transparency=0, int width=SCR_NO_VALUE, int height=SCR_NO_VALUE,
						int cut_x=0, int cut_y=0, int cut_width=SCR_NO_VALUE, int cut_height=SCR_NO_VALUE);
  /// Draws the specified surface onto this surface.
  import void DrawSurface(DrawingSurface *surfaceToDraw, int transparency=0, int x=0, int y=0, int width=SCR_NO_VALUE, int height=SCR_NO_VALUE,
						int cut_x=0, int cut_y=0, int cut_width=SCR_NO_VALUE, int cut_height=SCR_NO_VALUE);
#else // !SCRIPT_API_v360
  /// Draws a sprite onto the surface with its top-left corner at (x,y).
  import void DrawImage(int x, int y, int spriteSlot, int transparency=0, int width=SCR_NO_VALUE, int height=SCR_NO_VALUE);
  /// Draws the specified surface onto this surface.
  import void DrawSurface(DrawingSurface *surfaceToDraw, int transparency=0);
#endif // !SCRIPT_API_v360
  /// Draws a straight line between the two points on the surface.
  import void DrawLine(int x1, int y1, int x2, int y2, int thickness=1);
  /// Draws a message from the Room Message Editor, wrapping at the specified width.
  import void DrawMessageWrapped(int x, int y, int width, FontType, int messageNumber);
  /// Changes the colour of a single pixel on the surface.
  import void DrawPixel(int x, int y);
  /// Draws a filled rectangle to the surface.
  import void DrawRectangle(int x1, int y1, int x2, int y2);
  /// Draws the specified text to the surface.
  import void DrawString(int x, int y, FontType, const string text, ...);
#ifdef SCRIPT_API_v361
  /// Draws the text to the surface, wrapping it at the specified width.
  import void DrawStringWrapped(int x, int y, int width, FontType, HorizontalAlignment, const string text, ...);
#else // !SCRIPT_API_v361
  /// Draws the text to the surface, wrapping it at the specified width.
  import void DrawStringWrapped(int x, int y, int width, FontType, HorizontalAlignment, const string text);
#endif // !SCRIPT_API_v361
  /// Draws a filled triangle onto the surface.
  import void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3);
  /// Gets the colour of a single pixel on the surface.
  import int  GetPixel(int x, int y);
  /// Tells AGS that you have finished drawing onto the surface.
  import void Release();
  /// Gets/sets the current AGS Colour Number that will be used for drawing onto this surface.
  import attribute int DrawingColor;
  /// Gets the height of this surface.
  readonly import attribute int Height;
#ifdef SCRIPT_COMPAT_v341
  /// Determines whether you use high-res or low-res co-ordinates for drawing onto this surface.
  import attribute bool UseHighResCoordinates;
#endif // SCRIPT_COMPAT_v341
  /// Gets the width of the surface.
  readonly import attribute int Width;
};

#ifdef SCRIPT_API_v3507
builtin managed struct Camera;
builtin managed struct Viewport;
#endif // SCRIPT_API_v3507

builtin struct Room {
  /// Gets a custom text property associated with this room.
  import static String GetTextProperty(const string property);
  /// Gets a drawing surface that allows you to manipulate the room background.
  import static DrawingSurface* GetDrawingSurfaceForBackground(int backgroundNumber=SCR_NO_VALUE);
  /// Gets the Y co-ordinate of the bottom edge of the room.
  readonly import static attribute int BottomEdge;
  /// Gets the colour depth of the room background.
  readonly import static attribute int ColorDepth;
  /// Gets the height of the room background.
  readonly import static attribute int Height;
  /// Gets the X co-ordinate of the left edge of the room.
  readonly import static attribute int LeftEdge;
  /// Accesses room messages, as set up in the Room Message Editor.
  readonly import static attribute String Messages[];
  /// Gets the music that is played when the player enters this room.
  readonly import static attribute int MusicOnLoad;
  /// Gets the number of objects in this room.
  readonly import static attribute int ObjectCount;
  /// Gets the X co-ordinate of the right edge of the room.
  readonly import static attribute int RightEdge;
  /// Gets the Y co-ordinate of the top edge of the room.
  readonly import static attribute int TopEdge;
  /// Gets the width of the room background.
  readonly import static attribute int Width;
#ifdef SCRIPT_API_v340
  /// Gets a Custom Property associated with this room.
  import static int GetProperty(const string property);
  /// Sets an integer custom property associated with this room.
  import static bool SetProperty(const string property, int value);
  /// Sets a text custom property associated with this room.
  import static bool SetTextProperty(const string property, const string value);
  /// Performs default processing of a mouse click at the specified co-ordinates.
  import static void ProcessClick(int x, int y, CursorMode);
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v360
  /// Checks if the specified room exists
  import static bool Exists(int room);   // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v360
#ifdef SCRIPT_API_v362
  /// Finds the nearest position on any walkable area, close to the given coordinates. Returns null if no walkable area is found.
  import static Point* NearestWalkableArea(int x, int y);   // $AUTOCOMPLETESTATICONLY$
  /// Gets the number of backgrounds in this room.
  import static readonly attribute int BackgroundCount;
#endif
};

builtin struct Parser {
  /// Returns the parser dictionary word ID for the specified word
  import static int    FindWordID(const string wordToFind);
  /// Stores the supplied user text for later use with Said
  import static void   ParseText(const string text);
  /// Checks whether the player's input matched this text.
  import static bool   Said(const string text);
  /// Gets any word that the player typed that was not in the game dictionary.
  import static String SaidUnknownWord();
};

// standard functions
/// Displays the text in a standard text window.
import void Display(const string message, ...);
/// Displays the text in a standard text window at the specified location.
import void DisplayAt(int x, int y, int width, const string message, ...);
#ifdef SCRIPT_API_v361
/// Displays the text in a standard text window at the specified y-coordinate.
import void DisplayAtY (int y, const string message, ...);
#else // !SCRIPT_API_v361
/// Displays the text in a standard text window at the specified y-coordinate.
import void DisplayAtY (int y, const string message);
#endif // !SCRIPT_API_v361
/// Displays a message from the Room Message Editor.
import void DisplayMessage(int messageNumber);
/// Displays a message from the Room Message Editor at the specified y-coordinate.
import void DisplayMessageAtY(int messageNumber, int y);
/// Displays a message in a text window with a title, used for speech in SCI0 games.
import void DisplayTopBar(int y, int textColor, int backColor, const string title, const string text, ...);
/// Displays a Room Message Editor message in a text window with a title, used for speech in SCI0 games.
import void DisplayMessageBar(int y, int textColor, int backColor, const string title, int message);
/// Resets the room state back to how it was initially set up in the editor.
import void ResetRoom(int roomNumber);
/// Checks whether the player has been in the specified room yet.
import int  HasPlayerBeenInRoom(int roomNumber);
#ifdef SCRIPT_COMPAT_v335
/// Performs default processing of a mouse click at the specified co-ordinates.
import void ProcessClick(int x, int y, CursorMode);
#endif // SCRIPT_COMPAT_v335
/// Exits the game with an error message.
import void AbortGame(const string message, ...);
/// Quits the game, optionally showing a confirmation dialog.
import void QuitGame(int promptUser);
/// Changes the current game speed.
import void SetGameSpeed(int framesPerSecond);
/// Gets the current game speed.
import int  GetGameSpeed();
/// Changes a game option; see the manual for details.
import int  SetGameOption(int option, int value);
/// Gets the current value of a game option.
import int  GetGameOption(int option);
/// Performs various debugging commands.
import void Debug(int command, int data);
/// Calls the on_call function in the current room.
import void CallRoomScript(int value);
/// Transfers gameplay into a separate AGS game.
import int  RunAGSGame(const string filename, int mode, int data);
/// Gets the translated version of the specified text.
import const string GetTranslation (const string originalText);
/// Checks if a translation is currently in use.
import int  IsTranslationAvailable ();
/// Displays the default built-in Restore Game dialog.
import void RestoreGameDialog(int min_slot = 0, int max_slot = 99);
/// Displays the default built-in Save Game dialog.
import void SaveGameDialog(int min_slot = 0, int max_slot = 99);
/// Restarts the game from the restart point.
import void RestartGame();
/// Saves the current game position to the specified slot.
import void SaveGameSlot(int slot, const string description, int sprite = -1);
/// Restores the game saved to the specified game slot.
import void RestoreGameSlot(int slot);
#ifdef SCRIPT_API_v362
/// Copies the save game from one slot to another, overwriting a save if one was already present at a new slot.
import void CopySaveSlot(int old_slot, int new_slot);
/// Moves the save game from one slot to another, overwriting a save if one was already present at a new slot.
import void MoveSaveSlot(int old_slot, int new_slot);
#endif // SCRIPT_API_v362
/// Deletes the specified save game.
import void DeleteSaveSlot(int slot);
/// Sets this as the point at which the game will be restarted.
import void SetRestartPoint();
/// Gets what type of thing is in the room at the specified co-ordinates.
import LocationType GetLocationType(int x, int y);
#ifdef SCRIPT_COMPAT_v350
/// Returns which walkable area is at the specified position on screen.
import int  GetWalkableAreaAt(int screenX, int screenY);
#endif // SCRIPT_COMPAT_v350
#ifdef SCRIPT_API_v3507
/// Returns which walkable area is at the specified position on screen.
import int  GetWalkableAreaAtScreen(int screenX, int screenY);
/// Returns which walkable area is at the specified position within the room.
import int  GetWalkableAreaAtRoom(int roomX, int roomY);
#endif // SCRIPT_API_v3507
#ifdef SCRIPT_API_v360
/// Gets the drawing surface for the 8-bit walkable mask
import DrawingSurface* GetDrawingSurfaceForWalkableArea();
/// Gets the drawing surface for the 8-bit walk-behind mask
import DrawingSurface* GetDrawingSurfaceForWalkbehind();
#endif // SCRIPT_API_v360
/// Returns the scaling level at the specified position within the room.
import int  GetScalingAt (int x, int y);
#ifdef SCRIPT_COMPAT_v335
/// Gets the specified Custom Property for the current room.
import int  GetRoomProperty(const string property);
#endif // SCRIPT_COMPAT_v335
#ifdef SCRIPT_COMPAT_v350
/// Locks the viewport to stop the screen scrolling automatically.
import void SetViewport(int x, int y);
/// Allows AGS to scroll the screen automatically to follow the player character.
import void ReleaseViewport();
/// Gets the current X offset of the scrolled viewport.
import int  GetViewportX();
/// Gets the current Y offset of the scrolled viewport.
import int  GetViewportY();
#endif // SCRIPT_COMPAT_v350
/// Returns whether the game is currently paused.
import int  IsGamePaused();
/// Disables the player interface and activates the Wait cursor.
import void DisableInterface();
/// Re-enables the player interface.
import void EnableInterface();
/// Checks whether the player interface is currently enabled.
import int  IsInterfaceEnabled();
#ifdef SCRIPT_API_v362
/// Triggers the standard "on_event" script callback, passing EventType and a number of optional parameters
import void SendEvent(EventType, int data1=0, int data2=0, int data3=0, int data4=0);
#endif

builtin struct Mouse {
  /// Changes the sprite for the specified mouse cursor.
  import static void ChangeModeGraphic(CursorMode, int slot);
  /// Changes the active hotspot for the specified mouse cursor.
  import static void ChangeModeHotspot(CursorMode, int x, int y);
  /// Changes the view used to animate the specified mouse cursor.
  import static void ChangeModeView(CursorMode, int view, int delay = SCR_NO_VALUE);
  /// Disables the specified cursor mode.
  import static void DisableMode(CursorMode);
  /// Re-enables the specified cursor mode.
  import static void EnableMode(CursorMode);
  /// Gets the sprite used for the specified mouse cursor.
  import static int  GetModeGraphic(CursorMode);
  /// Checks whether the specified mouse button is currently pressed.
  import static bool IsButtonDown(MouseButton);
  /// Remembers the current mouse cursor and restores it when the mouse leaves the current area.
  import static void SaveCursorUntilItLeaves();
  /// Cycles to the next available mouse cursor.
  import static void SelectNextMode();
#ifdef SCRIPT_API_v341
  /// Cycles to the previous available mouse cursor.
  import static void SelectPreviousMode();
#endif // SCRIPT_API_v341
  /// Restricts the mouse movement to the specified area.
  import static void SetBounds(int left, int top, int right, int bottom);
  /// Moves the mouse cursor to the specified location.
  import static void SetPosition(int x, int y);
  /// Updates the X and Y co-ordinates to match the current mouse position.
  import static void Update();
  /// Changes the current mouse cursor back to the default for the current mode.
  import static void UseDefaultGraphic();
  /// Changes the mouse cursor to use the graphic for a different non-active cursor mode.
  import static void UseModeGraphic(CursorMode);
  /// Gets/sets the current mouse cursor mode.
  import static attribute CursorMode Mode;
  /// Gets/sets whether the mouse cursor is visible.
  import static attribute bool Visible;
#ifdef SCRIPT_API_v335
  /// Gets/sets whether the user-defined factors are applied to mouse movement
  import static attribute bool ControlEnabled;
  /// Gets/sets the mouse speed
  import static attribute float Speed;
#endif // SCRIPT_API_v335
#ifdef SCRIPT_API_v340
  /// Fires mouse click event at current mouse position.
  import static void Click(MouseButton);
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v341
  /// Returns whether the specified mouse cursor is currently enabled.
  import static bool IsModeEnabled(CursorMode);
#endif // SCRIPT_API_v341
#ifdef SCRIPT_API_v36026
  /// Gets/sets whether the mouse cursor will be automatically locked in the game window.
  import static attribute bool AutoLock;
#endif // SCRIPT_API_v36026
  /// Gets the current mouse position.
  readonly int  x,y;
};

#ifndef STRICT_STRINGS
// OLD STRING BUFFER FUNCTIONS
import void SetGlobalString(int stringID, const string newValue);
import void GetGlobalString(int stringID, string buffer);
import void InputBox(const string prompt, string buffer);
import int  GetTranslationName (string buffer);
import int  GetSaveSlotDescription(int slot, string buffer);
import void GetLocationName(int x, int y, string buffer);
import void GetRoomPropertyText(const string property, string buffer);
// string functions
import void StrCat(string main, const string newbit);
import int  StrCaseComp(const string str1, const string str2);
import int  StrComp(const string str1, const string str2);
import void StrCopy(string dest, const string source);
import void StrFormat(string dest, const string format, ...);
import int  StrLen(const string);
import int  StrGetCharAt (const string, int position);
import void StrSetCharAt (string, int position, int newChar);
import void StrToLowerCase (string);
import void StrToUpperCase (string);
import int  StrContains (const string haystack, const string needle);
import void ParseText (const string);
import int  SaidUnknownWord (string buffer);
import void GetMessageText (int messageNumber, string buffer);
import int  StringToInt(const string);
#define strcmp StrComp
#define strlen StrLen
#define strcpy StrCopy
#define strcat StrCat
#endif // !STRICT_STRINGS

import int  Said (const string text);

#ifndef STRICT
// OBSOLETE STUFF
#define SPEECH_SIERRA 1
#define SPEECH_SIERRABKGRND 2
#define SPEECH_LUCASARTS 0
#define SPEECH_FULLSCREEN 3
#define MODE_WALK   0
#define MODE_LOOK   1
#define MODE_USE    2
#define MODE_TALK   3
#define MODE_USEINV 4
#define MODE_PICKUP 5
#define TRANSITION_FADE     0
#define TRANSITION_INSTANT  1
#define TRANSITION_DISSOLVE 2
#define TRANSITION_BOXOUT   3
#define TRANSITION_CROSSFADE 4
#define ALIGN_LEFT    eAlignLeft
#define ALIGN_CENTRE  eAlignCenter
#define ALIGN_CENTER  eAlignCenter
#define ALIGN_RIGHT   eAlignRight
#define CHAR_IGNORESCALING 1
#define CHAR_NOINTERACTION 4
#define CHAR_NODIAGONAL    8
#define CHAR_IGNORELIGHT   32
#define CHAR_NOTURNING     64
#define CHAR_IGNOREWALKBEHINDS 128
#define CHAR_WALKTHROUGH       512
#define CHAR_SCALEMOVESPEED    1024
#define LEFT  1
#define RIGHT 2
#define MIDDLE 3
#define LEFTINV  5
#define RIGHTINV 6
#define WHEELNORTH  8
#define WHEELSOUTH  9

import void SetGlobalInt(int globalInt, int value);
import int  GetGlobalInt(int globalInt);
import int  GetGraphicalVariable (const string variableName);
import void SetGraphicalVariable (const string variableName, int value);

import int  GetHotspotAt(int x, int y);
import int  GetObjectAt(int x,int y);
import int  GetCharacterAt(int x,int y);
import int  GetRegionAt (int x, int y);
import int  GetInvAt(int x,int y);

import int  CreateGraphicOverlay(int x, int y, int slot, bool transparent);
import int  CreateTextOverlay(int x, int y, int width, FontType, int colour, const string text, ...);
import void SetTextOverlay(int overlayID, int x, int y, int width, FontType, int colour, const string text, ...);
import void RemoveOverlay(int overlayID);
import int  MoveOverlay(int overlayID, int x, int y);
import int  IsOverlayValid(int overlayID);

import int  InventoryScreen();
// mouse functions
import void ChangeCursorGraphic(int mode, int slot);
import void ChangeCursorHotspot(int mode, int x, int y);
import int  GetCursorMode();
import void SetCursorMode(CursorMode);
import void SetNextCursorMode();
import void SetDefaultCursor();
import void SetMouseCursor(CursorMode);
import void SetMouseBounds(int left, int top, int right, int bottom);
import void SetMousePosition(int x, int y);
import void ShowMouseCursor();
import void HideMouseCursor();
import void RefreshMouse();
import void DisableCursorMode(CursorMode);
import void EnableCursorMode(CursorMode);
import void SaveCursorForLocationChange();
import int  IsButtonDown(MouseButton);
// Obsolete functions for objects
import void MergeObject(int object);
import void SetObjectTint(int object, int red, int green, int blue, int saturation, int luminance);
import void RemoveObjectTint(int object);
import void StopObjectMoving(int object);
import void RunObjectInteraction (int object, CursorMode);
import int  GetObjectProperty(int object, const string property);
import void GetObjectPropertyText(int object, const string property, string buffer);
import void AnimateObject(int object, int loop, int delay, int repeat);
import void AnimateObjectEx(int object, int loop, int delay, int repeat, int direction, int blocking);
import void ObjectOff(int object);
import void ObjectOn(int object);
import void SetObjectBaseline(int object, int baseline);
import int  GetObjectBaseline(int object);
import void SetObjectFrame(int object, int view, int loop, int frame);
import void SetObjectGraphic(int object, int spriteSlot);
import void SetObjectView(int object, int view);
import void SetObjectTransparency(int object, int amount);
import void MoveObject(int object, int x, int y, int speed);
import void MoveObjectDirect(int object, int x, int y, int speed);
import void SetObjectPosition(int object, int x, int y);
import int  AreObjectsColliding(int object1, int object2);
import void GetObjectName(int object, string buffer);
import int  GetObjectX(int object);
import int  GetObjectY(int object);
import int  GetObjectGraphic(int object);
import int  IsObjectAnimating(int object);
import int  IsObjectMoving(int object);
import int  IsObjectOn (int object);
import void SetObjectClickable(int object, int clickable);
import void SetObjectIgnoreWalkbehinds (int object, int ignore);

// Obsolete Character functions
import void AddInventory(int item);
import void LoseInventory(int item);
import void SetActiveInventory(int item);
import void NewRoom(int roomNumber);
import void NewRoomEx(int roomNumber, int x, int y);
import void NewRoomNPC(CHARID, int roomNumber, int x, int y);
import int  GetCharacterProperty(CHARID, const string property);
import void GetCharacterPropertyText(CHARID, const string property, string buffer);
import void RunCharacterInteraction (CHARID, CursorMode);
import void DisplaySpeech (CHARID, const string message, ...);
import int  DisplaySpeechBackground(CHARID, const string message);
import void DisplaySpeechAt (int x, int y, int width, CHARID, const string message);
import void DisplayThought (CHARID, const string message, ...);
import void FollowCharacter(CHARID sheep, CHARID shepherd);
import void FollowCharacterEx(CHARID sheep, CHARID shepherd, int dist, int eagerness);
import void SetPlayerCharacter(CHARID);
import void AddInventoryToCharacter(CHARID, int item);
import void LoseInventoryFromCharacter(CHARID, int item);
import void AnimateCharacter (CHARID, int loop, int delay, int repeat);
import void AnimateCharacterEx (CHARID, int loop, int delay, int repeat, int direction, int blocking);
import void MoveCharacter(CHARID, int x, int y);
import void MoveCharacterDirect(CHARID, int x, int y);
import void MoveCharacterPath(CHARID, int x, int y);
import void MoveCharacterStraight(CHARID, int x,int y);
import void MoveCharacterToHotspot(CHARID, int hotspot);
import void MoveCharacterToObject(CHARID, int object);
import void MoveCharacterBlocking(CHARID, int x, int y, int direct);
import void MoveToWalkableArea(CHARID);
import void FaceCharacter(CHARID, CHARID toFace);
import void FaceLocation(CHARID, int x, int y);
import void SetCharacterView(CHARID, int view);
import void SetCharacterViewEx(CHARID, int view, int loop, HorizontalAlignment align);
import void SetCharacterViewOffset(CHARID, int view, int x_offset, int y_offset);
import void SetCharacterFrame(CHARID, int view, int loop, int frame);
import void ReleaseCharacterView(CHARID);
import void ChangeCharacterView(CHARID, int view);
import void SetCharacterSpeechView(CHARID, int view);
import void SetCharacterBlinkView(CHARID, int view, int interval);
import void SetCharacterIdle(CHARID, int idleView, int delay);
import void StopMoving(CHARID);
import int  AreCharObjColliding(CHARID, int object);
import int  AreCharactersColliding(CHARID, CHARID);
import void SetCharacterSpeed(CHARID, int speed);
import void SetCharacterSpeedEx(CHARID, int x_speed, int y_speed);
import void SetTalkingColor(CHARID, int colour);
import void SetCharacterTransparency(CHARID, int transparency);
import void SetCharacterClickable(CHARID, int clickable);
import void SetCharacterBaseline(CHARID, int baseline);
import void SetCharacterIgnoreLight (CHARID, int ignoreLight);
import void SetCharacterIgnoreWalkbehinds (CHARID, int ignoreWBs);
import void SetCharacterProperty (CHARID, int property, int newValue);
import int  GetPlayerCharacter();

// obsolete file I/O functions
#define FILE_WRITE "wb"
#define FILE_APPEND "ab"
#define FILE_READ  "rb"
#define WRITE FILE_WRITE
#define READ  FILE_READ
import int  FileOpen(const string filename, const string mode);
import void FileWrite(int fileHandle, const string text);
import void FileWriteRawLine(int fileHandle, const string text);
import void FileRead(int fileHandle, string buffer);
import void FileClose(int fileHandle);
import void FileWriteInt(int fileHandle, int value);
import int  FileReadInt(int fileHandle);
import char FileReadRawChar(int fileHandle);
import void FileWriteRawChar(int fileHandle, int value);
import int  FileReadRawInt(int fileHandle);
import int  FileIsEOF(int fileHandle);
import int  FileIsError(int fileHandle);

// obsolete hotspot/region funcs
import void DisableHotspot(int hotspot);
import void EnableHotspot(int hotspot);
import void GetHotspotName(int hotspot, string buffer);
import int  GetHotspotPointX(int hotspot);
import int  GetHotspotPointY(int hotspot);
import int  GetHotspotProperty(int hotspot, const string property);
import void GetHotspotPropertyText(int hotspot, const string property, string buffer);
import void RunHotspotInteraction (int hotspot, CursorMode);
import void DisableRegion(int region);
import void EnableRegion(int region);
import void RunRegionInteraction (int region, int event);
import void SetAreaLightLevel(int area, int lightLevel);
import void SetRegionTint(int area, int red, int green, int blue, int amount);

// obsolete inv functions
import int  GetInvProperty(int invItem, const string property);
import void GetInvPropertyText(int invItem, const string property, string buffer);
import void GetInvName(int item, string buffer);
import int  GetInvGraphic(int item);
import void SetInvItemPic(int item, int spriteSlot);
import void SetInvItemName(int item, const string name);
import int  IsInventoryInteractionAvailable (int item, CursorMode);
import void RunInventoryInteraction (int item, CursorMode);

import int  GetTime(int whichValue);
import int  GetRawTime();

import int  LoadSaveSlotScreenshot(int saveSlot, int width, int height);
import int  LoadImageFile(const string filename);
import void DeleteSprite(int spriteSlot);

import void SetSpeechFont(FontType);
import void SetNormalFont(FontType);

#define GP_SPRITEWIDTH   1
#define GP_SPRITEHEIGHT  2
#define GP_NUMLOOPS      3
#define GP_NUMFRAMES     4
#define GP_ISRUNNEXTLOOP 5
#define GP_FRAMESPEED    6
#define GP_FRAMEIMAGE    7
#define GP_FRAMESOUND    8
#define GP_NUMGUIS       9
#define GP_NUMOBJECTS    10
#define GP_NUMCHARACTERS 11
#define GP_NUMINVITEMS   12
#define GP_ISFRAMEFLIPPED 13

import int  GetGameParameter(int parameter, int data1=0, int data2=0, int data3=0);
import void SetDialogOption(int topic, int option, DialogOptionState);
import DialogOptionState GetDialogOption(int topic, int option);
import void RunDialog(int topic);

// obsolete raw draw stuff
import void RawClearScreen (int colour);
import void RawDrawCircle (int x, int y, int radius);
import void RawDrawImage (int x, int y, int spriteSlot);
import void RawDrawImageOffset(int x, int y, int spriteSlot);
import void RawDrawImageResized(int x, int y, int spriteSlot, int width, int height);
import void RawDrawImageTransparent(int x, int y, int spriteSlot, int transparency);
import void RawDrawLine (int x1, int y1, int x2, int y2);
import void RawDrawRectangle (int x1, int y1, int x2, int y2);
import void RawDrawTriangle (int x1, int y1, int x2, int y2, int x3, int y3);
import void RawPrint (int x, int y, const string message, ...);
import void RawPrintMessageWrapped (int x, int y, int width, FontType, int messageNumber);
import void RawSetColor(int colour);
import void RawSetColorRGB(int red, int green, int blue);
import void RawDrawFrameTransparent (int frame, int transparency);
import void RawSaveScreen ();
import void RawRestoreScreen ();
// obsolete RawRestoreScreenTinted(int red, int green, int blue, int opacity);

#endif // !STRICT

/// Gets the width of the specified text in the specified font
import int  GetTextWidth(const string text, FontType);
/// Gets the height of the specified text in the specified font when wrapped at the specified width
import int  GetTextHeight(const string text, FontType, int width);
/// Gets the font's height, in pixels
import int  GetFontHeight(FontType);
/// Gets the default step between two lines of text for the specified font
import int  GetFontLineSpacing(FontType);
/// Adds to the player's score and plays the score sound, if set.
import void GiveScore(int points);
/// Refreshes the on-screen inventory display.
import void UpdateInventory();
/// From within dialog_request, tells AGS not to return to the dialog after this function ends.
import void StopDialog();
/// Determines whether two objects or characters are overlapping each other.
import int  AreThingsOverlapping(int thing1, int thing2);

#ifdef SCRIPT_COMPAT_v321
/// Sets whether voice and/or text are used in the game.
import void SetVoiceMode(eVoiceMode);
/// Sets how the player can skip speech lines.
import void SetSkipSpeech(int skipFlag);
/// Changes the style in which speech is displayed.
import void SetSpeechStyle(eSpeechStyle);
#endif // SCRIPT_COMPAT_v321

/// Starts a timer, which will expire after the specified number of game loops.
import void SetTimer(int timerID, int timeout);
/// Returns true the first time this is called after the timer expires.
import bool IsTimerExpired(int timerID);
#ifdef SCRIPT_API_v362
/// Returns the specified timer's time value; returns 0 if timer is not running, and 1 if it's expiring.
import int  GetTimerPos(int timerID);
#endif // SCRIPT_API_v362
/// Sets whether the game can continue to run in the background if the player switches to another application.
import void SetMultitaskingMode (int mode);
/// Converts a floating point value to an integer.
import int  FloatToInt(float value, RoundDirection=eRoundDown);
/// Converts an integer to a floating point number.
import float IntToFloat(int value);

// File I/O
enum FileMode {
  eFileRead = 1,
  eFileWrite = 2,
  eFileAppend = 3
};

#ifdef SCRIPT_API_v340
enum FileSeek {
  eSeekBegin = 0,
  eSeekCurrent = 1,
  eSeekEnd = 2
};
#endif // SCRIPT_API_v340

builtin managed struct DateTime;

builtin managed struct File {
  /// Delets the specified file from the disk.
  import static bool Delete(const string filename);   // $AUTOCOMPLETESTATICONLY$
  /// Checks if the specified file exists on the disk.
  import static bool Exists(const string filename);   // $AUTOCOMPLETESTATICONLY$
  /// Opens the specified file in order to read from or write to it.
  import static File *Open(const string filename, FileMode);   // $AUTOCOMPLETESTATICONLY$
  /// Closes the file.
  import void Close();
  /// Reads an integer value from the file. The integer is expected to be written by WriteInt and be prepended by a tag.
  import int  ReadInt();
  /// Reads the next raw byte from the file.
  import int  ReadRawChar();
  /// Reads the next raw 32-bit int from the file.
  import int  ReadRawInt();
#ifndef STRICT_STRINGS
  import void ReadRawLine(string buffer);
  import void ReadString(string buffer);
#endif // !STRICT_STRINGS
  /// Reads the next raw line of text from the file.
  import String ReadRawLineBack();
  /// Reads the next string from the file. The string is expected to be written by WriteString, or have its length prepended as a 32-bit integer.
  import String ReadStringBack();
  /// Writes an integer to the file, safeguarding it by a tag.
  import void WriteInt(int value);
  /// Writes a raw byte to the file.
  import void WriteRawChar(int value);
  /// Writes a raw line of text to the file.
  import void WriteRawLine(const string text);
  /// Writes a string to the file, prepending it with a string's length.
  import void WriteString(const string text);
  /// Gets whether you have reached the end of the file.
  readonly import attribute bool EOF;
  /// Gets whether any errors occurred reading or writing the file.
  readonly import attribute bool Error;
#ifdef SCRIPT_API_v340
  /// Moves file cursor by specified offset, returns new position.
  import int Seek(int offset, FileSeek origin = eSeekCurrent);
  /// Gets current cursor position inside the file.
  readonly import attribute int Position;
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v36026
  /// Writes a raw 32-bit integer to the file.
  import void WriteRawInt(int value);
#endif // SCRIPT_API_v36026
#ifdef SCRIPT_API_v361
  /// Resolves the script path into the system filepath; for diagnostic purposes only.
  import static String ResolvePath(const string filename);   // $AUTOCOMPLETESTATICONLY$
  /// Gets the path to opened file.
  readonly import attribute String Path;
#endif // SCRIPT_API_v361
#ifdef SCRIPT_API_v362
  /// Creates a copy of an existing file; if there's already a file with the new name then it will be overwritten
  import static bool Copy(const string old_filename, const string new_filename);   // $AUTOCOMPLETESTATICONLY$
  /// Retrieves specified file's last write time; returns null if file does not exist
  import static DateTime* GetFileTime(const string filename); // $AUTOCOMPLETESTATICONLY$
  /// Renames an existing file; if there's already a file with the new name then it will be overwritten
  import static bool Rename(const string old_filename, const string new_filename);   // $AUTOCOMPLETESTATICONLY$
  /// Returns an array of filenames that match the specified file mask
  import static String[] GetFiles(const string fileMask, FileSortStyle fileSortStyle = eFileSort_Name, SortDirection sortDirection = eSortAscending);  // $AUTOCOMPLETESTATICONLY$
  /// Reads a float value from the file. The float is expected to be written by WriteFloat and be prepended by a tag.
  import float  ReadFloat();
  /// Writes a float to the file, safeguarding it by a tag.
  import void   WriteFloat(float value);
  /// Reads the next raw 32-bit float from the file.
  import float  ReadRawFloat();
  /// Writes a raw 32-bit float to the file.
  import void   WriteRawFloat(float value);
  /// Reads up to "count" number of bytes and stores them in a provided dynamic array, starting with certain index. Returns actual number of read bytes.
  import int    ReadRawBytes(char bytes[], int index, int count);
  /// Writes up to "count" number of bytes from the provided dynamic array, starting with certain index. Returns actual number of written bytes.
  import int    WriteRawBytes(char bytes[], int index, int count);
#endif // SCRIPT_API_v362
  int reserved[2];   // $AUTOCOMPLETEIGNORE$
};

builtin managed struct InventoryItem {
  /// Returns the inventory item at the specified location.
  import static InventoryItem* GetAtScreenXY(int x, int y);    // $AUTOCOMPLETESTATICONLY$
#ifdef SCRIPT_API_v361
  import static InventoryItem* GetByName(const string scriptName); // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v361
  /// Gets an integer custom property for this item.
  import int  GetProperty(const string property);
  /// Gets a text custom property for this item.
  import String GetTextProperty(const string property);
  /// Checks whether an event handler has been registered for clicking on this item in the specified cursor mode.
  import int  IsInteractionAvailable(CursorMode);
  /// Runs the registered event handler for this item.
  import void RunInteraction(CursorMode);
  /// Gets/sets the sprite used as the item's mouse cursor.
  import attribute int  CursorGraphic;
  /// Gets/sets the sprite used to display the inventory item.
  import attribute int  Graphic;
  /// Gets the ID number of the inventory item.
  readonly import attribute int ID;
  /// Gets/sets the human-readable name of the inventory item.
  import attribute String Name;
#ifdef SCRIPT_API_v340
  /// Sets an integer custom property for this item.
  import bool SetProperty(const string property, int value);
  /// Sets a text custom property for this item.
  import bool SetTextProperty(const string property, const string value);
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v361
  /// Gets the script name of the inventory item.
  import readonly attribute String ScriptName;
#endif // SCRIPT_API_v361
#ifndef STRICT_STRINGS
  import void GetName(string buffer);
  import void GetPropertyText(const string property, string buffer);
  import void SetName(const string newName);
#endif // !STRICT_STRINGS

  int reserved[2];   // $AUTOCOMPLETEIGNORE$
};

builtin managed struct Overlay {
  /// Creates an overlay that displays a sprite.
  import static Overlay* CreateGraphical(int x, int y, int slot, bool transparent = true
#ifdef SCRIPT_API_v360
	, bool clone = false
#endif
  );  // $AUTOCOMPLETESTATICONLY$
  /// Creates an overlay that displays some text.
  import static Overlay* CreateTextual(int x, int y, int width, FontType, int colour, const string text, ...);  // $AUTOCOMPLETESTATICONLY$
  /// Changes the text on the overlay.
  import void SetText(int width, FontType, int colour, const string text, ...);
  /// Removes the overlay from the screen.
  import void Remove();
  /// Checks whether this overlay is currently valid.
  readonly import attribute bool Valid;
  /// Gets/sets the X position on the screen where this overlay is displayed.
  import attribute int X;
  /// Gets/sets the Y position on the screen where this overlay is displayed.
  import attribute int Y;
#ifdef SCRIPT_API_v360
  /// Creates an overlay that displays a sprite inside the room.
  import static Overlay* CreateRoomGraphical(int x, int y, int slot, bool transparent = true, bool clone = false);  // $AUTOCOMPLETESTATICONLY$
  /// Creates an overlay that displays some text inside the room.
  import static Overlay* CreateRoomTextual(int x, int y, int width, FontType, int colour, const string text, ...);  // $AUTOCOMPLETESTATICONLY$
  /// Gets whether this overlay is located inside the room, as opposed to the screen layer.
  import readonly attribute bool InRoom;
  /// Gets/sets the width of this overlay. Resizing overlay will scale its image.
  import attribute int Width;
  /// Gets/sets the height of this overlay. Resizing overlay will scale its image.
  import attribute int Height;
  /// Gets/sets the sprite's number which is displayed on this overlay. Returns -1 for generated image without actual number.
  import attribute int Graphic;
  /// Gets the original width of this overlay's graphic.
  import readonly attribute int GraphicWidth;
  /// Gets the original height of this overlay's graphic.
  import readonly attribute int GraphicHeight;
  /// Gets/sets the transparency of this overlay.
  import attribute int Transparency;
  /// Gets/sets the overlay's z-order relative to other overlays and on-screen objects.
  import attribute int ZOrder;
#endif // SCRIPT_API_v360
#ifdef SCRIPT_API_v362
  /// Sets this overlay's position, and optionally its size.
  import void SetPosition(int x, int y, int width = 0, int height = 0);
  /// Changes the size of the overlay.
  import void SetSize(int width, int height);
#endif // SCRIPT_API_v362
};

builtin managed struct DynamicSprite {
  /// Creates a blank dynamic sprite of the specified size.
  import static DynamicSprite* Create(int width, int height, bool hasAlphaChannel=false);    // $AUTOCOMPLETESTATICONLY$
  /// Creates a dynamic sprite as a copy of a room background.
  import static DynamicSprite* CreateFromBackground(int frame=SCR_NO_VALUE, int x=SCR_NO_VALUE, int y=SCR_NO_VALUE, int width=SCR_NO_VALUE, int height=SCR_NO_VALUE);    // $AUTOCOMPLETESTATICONLY$
  /// Creates a dynamic sprite as a copy of a drawing surface.
  import static DynamicSprite* CreateFromDrawingSurface(DrawingSurface* surface, int x, int y, int width, int height);    // $AUTOCOMPLETESTATICONLY$
  /// Creates a dynamic sprite as a copy of an existing sprite.
  import static DynamicSprite* CreateFromExistingSprite(int slot, bool preserveAlphaChannel=0);    // $AUTOCOMPLETESTATICONLY$
  /// Creates a dynamic sprite from a BMP or PCX file.
  import static DynamicSprite* CreateFromFile(const string filename);              // $AUTOCOMPLETESTATICONLY$
  /// Creates a dynamic sprite from a save game screenshot.
  import static DynamicSprite* CreateFromSaveGame(int slot, int width, int height);  // $AUTOCOMPLETESTATICONLY$
  /// Creates a dynamic sprite as a copy of the current screen.
#ifdef SCRIPT_API_v362
  import static DynamicSprite* CreateFromScreenShot(int width=0, int height=0, RenderLayer layer=eRenderLayerAll);  // $AUTOCOMPLETESTATICONLY$
#else // !SCRIPT_API_v362
  import static DynamicSprite* CreateFromScreenShot(int width=0, int height=0);  // $AUTOCOMPLETESTATICONLY$
#endif // !SCRIPT_API_v362
  /// Enlarges the size of the sprite, but does not resize the image.
  import void ChangeCanvasSize(int width, int height, int x, int y);
  /// Copies the transparency mask and/or alpha channel from the specified sprite onto this dynamic sprite.
  import void CopyTransparencyMask(int fromSpriteSlot);
  /// Reduces the size of the sprite, but does not resize the image.
  import void Crop(int x, int y, int width, int height);
  /// Deletes the dynamic sprite from memory when you no longer need it.
  import void Delete();
  /// Flips the sprite in the specified direction.
  import void Flip(eFlipDirection);
  /// Gets a drawing surface that can be used to manually draw onto the sprite.
  import DrawingSurface* GetDrawingSurface();
  /// Resizes the sprite.
  import void Resize(int width, int height);
  /// Rotates the sprite by the specified number of degrees.
  import void Rotate(int angle, int width=SCR_NO_VALUE, int height=SCR_NO_VALUE);
  /// Saves the sprite to a BMP or PCX file.
  import int  SaveToFile(const string filename);
  /// Permanently tints the sprite to the specified colour.
  import void Tint(int red, int green, int blue, int saturation, int luminance);
  /// Gets the colour depth of this sprite.
  readonly import attribute int ColorDepth;
  /// Gets the sprite number of this dynamic sprite, which you can use to display it in the game.
  readonly import attribute int Graphic;
  /// Gets the height of this sprite.
  readonly import attribute int Height;
  /// Gets the width of this sprite.
  readonly import attribute int Width;
};

// Palette FX
/// Fades the screen in from black to the normal palette.
import void FadeIn(int speed);
/// Fades the screen out to the current fade colour.
import void FadeOut(int speed);
/// Cycles the palette entries between start and end. (8-bit games only)
import void CyclePalette(int start, int end);
/// Changes the RGB colour of a palette slot. (8-bit games only)
import void SetPalRGB(int slot, int r, int g, int b);
/// Updates the screen with manual changes to the palette. (8-bit games only)
import void UpdatePalette();
/// Tints the whole screen to the specified colour.
import void TintScreen (int red, int green, int blue);
/// Sets an ambient tint that affects all objects and characters in the room.
import void SetAmbientTint(int red, int green, int blue, int saturation, int luminance);
/// Returns a random number between 0 and MAX, inclusive.
import int  Random(int max);
/// Locks the current room to the specified background.
import void SetBackgroundFrame(int frame);
/// Gets the current background frame number.
import int  GetBackgroundFrame();
/// Shakes the screen by the specified amount.
import void ShakeScreen(int amount);
/// Shakes the screen but does not pause the game while it does so.
import void ShakeScreenBackground(int delay, int amount, int length);
/// Changes the room transition style.
import void SetScreenTransition(TransitionStyle);
/// Changes the room transition style for the next room change only.
import void SetNextScreenTransition(TransitionStyle);
/// Changes the colour to which the screen fades out with a FadeOut call.
import void SetFadeColor(int red, int green, int blue);
/// Checks whether an event handler is registered to handle clicking at the specified location on the screen.
import int  IsInteractionAvailable (int x, int y, CursorMode);
/// Removes the specified walkable area from the room.
import void RemoveWalkableArea(int area);
/// Brings back a previously removed walkable area.
import void RestoreWalkableArea(int area);
/// Changes the specified walkable area's scaling level.
import void SetAreaScaling(int area, int min, int max);
/// Disables all region events, and optionally light levels and tints.
import void DisableGroundLevelAreas(int disableTints);
/// Re-enables region events, light levels and tints.
import void EnableGroundLevelAreas();
/// Changes the baseline of the specified walk-behind area.
import void SetWalkBehindBase(int area, int baseline);
/// Performs various commands to start and stop an audio CD.
import int  CDAudio(eCDAudioFunction, int data);
/// Plays a FLI/FLC animation.
import void PlayFlic(int flcNumber, int options);
#ifdef SCRIPT_API_v363
/// Plays a video file of any supported format.
import void PlayVideo(const string filename, VideoSkipStyle, VideoPlayStyle style);
#else // !SCRIPT_API_v363
/// Plays a video file of any supported format.
import void PlayVideo(const string filename, VideoSkipStyle, int flags);
#endif // !SCRIPT_API_v363
#ifdef SCRIPT_API_v340
/// Sets an ambient light level that affects all objects and characters in the room.
import void SetAmbientLightLevel(int light_level);
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v362
/// Gets the baseline of the specified walk-behind area.
import int GetWalkBehindBase(int area);
#endif // SCRIPT_API_v362

#ifndef STRICT_AUDIO
// **** OLD MUSIC/SOUND FUNCTIONS ****
/// Starts the specified music playing.
import void PlayMusic(int musicNumber);
/// Queues up the specified music to play after the current one finishes.
import void PlayMusicQueued(int musicNumber);
/// Plays a MIDI, but mutes the channel. This allows you to use it to time game events.
import void PlaySilentMIDI(int musicNumber);
/// Plays a specified MP3 or OGG file, that is not part of the normal game package.
import void PlayMP3File(const string filename);
/// Starts the specified sound number playing.
import int  PlaySound(int soundNumber);
/// Starts the specified sound number playing on the specified channel.
import void PlaySoundEx(int soundNumber, int channel);
/// Starts an ambient looping sound playing.
import void PlayAmbientSound (int channel, int sound, int volume, int x, int y);
/// Stops an ambient sound from playing.
import void StopAmbientSound (int channel);
/// Returns the currently playing music number.
import int  GetCurrentMusic();
/// Sets whether music tracks should repeat once they reach the end.
import void SetMusicRepeat(int repeat);
/// Changes the current room's music volume modifier.
import void SetMusicVolume(int volume);
/// Changes the sound volume.
import void SetSoundVolume(int volume);
/// Changes the music volume.
import void SetMusicMasterVolume(int volume);
/// Changes the volume of all digital sound and music.
import void SetDigitalMasterVolume(int volume);
/// Seeks to a specified pattern in a MOD/XM file.
import void SeekMODPattern(int pattern);
/// Returns whether sound is playing on the specified sound channel.
import int  IsChannelPlaying(int channel);
/// Returns whether a sound effect is currently playing.
import int  IsSoundPlaying();
/// Returns whether background music is currently playing.
import int  IsMusicPlaying();
/// Returns the current MIDI beat number.
import int  GetMIDIPosition();
/// Seeks the MIDI player to the specified beat number.
import void SeekMIDIPosition(int position);
/// Gets the offset into the currently playing MP3 or OGG music.
import int  GetMP3PosMillis();
/// Seeks into the currently playing MP3 or OGG music.
import void SeekMP3PosMillis(int offset);
/// Changes the volume of the specified sound channel.
import void SetChannelVolume(int channel, int volume);
/// Stops the sound currently playing on the specified sound channel.
import void StopChannel(int channel);
/// Stops the currently playing music.
import void StopMusic();
// **** END OLD MUSIC/SOUND FUNCTIONS ****
#endif // !STRICT_AUDIO

import int  IsVoxAvailable();
/// Changes the voice speech volume.
import void SetSpeechVolume(int volume);
/// Checks whether a MUSIC.VOX file was found.
import int  IsMusicVoxAvailable();
#ifdef SCRIPT_API_v362
/// Saves a screenshot of the current game position to a file.
import int  SaveScreenShot(const string filename, int width=0, int height=0, RenderLayer layer=eRenderLayerAll);
#else // !SCRIPT_API_v362
/// Saves a screenshot of the current game position to a file.
import int  SaveScreenShot(const string filename);
#endif // !SCRIPT_API_v362
/// Pauses the game, which stops all animations and movement.
import void PauseGame();
/// Resumes the game after it was paused earlier.
import void UnPauseGame();
/// Blocks the script for the specified number of game loops.
import void Wait(int waitLoops);
/// Blocks the script for the specified number of game loops, unless a key is pressed.
import int  WaitKey(int waitLoops = -1);
/// Blocks the script for the specified number of game loops, unless a key is pressed or the mouse is clicked.
import int  WaitMouseKey(int waitLoops = -1);
#ifdef SCRIPT_API_v360
/// Blocks the script for the specified number of game loops, unless the mouse is clicked.
import int  WaitMouse(int waitLoops = -1);
/// Cancels current Wait function, regardless of its type, if one was active at the moment.
import void SkipWait();
#endif // SCRIPT_API_v360
#ifdef SCRIPT_API_v36026
/// Blocks the script for the specified number of game loops, unless a input is issued. Input are flags, and can be combined using bitwise operators.
import int  WaitInput(InputType inputs, int waitLoops = -1);
#endif // SCRIPT_API_v36026
/// Checks whether the specified key is currently held down.
import bool IsKeyPressed(eKeyCode);
import void FlipScreen(int way);
/// Fast-forwards the game until the specified character finishes moving.
import void SkipUntilCharacterStops(CHARID);
/// Specifies the start of a skippable cutscene.
import void StartCutscene(CutsceneSkipType);
/// Specifies the end of a skippable cutscene.
import int  EndCutscene();
/// Prevents further event handlers running for this event.
import void ClaimEvent();
// Changes the GUI used to render standard game text windows.
import void SetTextWindowGUI (int gui);
import int  FindGUIID(const string);  // $AUTOCOMPLETEIGNORE$

#ifdef SCRIPT_API_v3507
/// Skip current cutscene (if one is currently in progress)
import void SkipCutscene();
#endif // SCRIPT_API_v3507

#ifndef STRICT
// Obsolete GUI functions
import void SetInvDimensions(int width, int height);
import int  GetGUIAt (int x, int y);
import int  GetGUIObjectAt (int x, int y);
import void InterfaceOn(int gui);   // $AUTOCOMPLETEIGNORE$
import void InterfaceOff(int gui);  // $AUTOCOMPLETEIGNORE$
//import void GUIOn (int gui);   // this being here makes the autocomplete recognise it
//import void GUIOff (int gui);
import void SetGUIPosition(int gui, int x, int y);
import void SetGUISize(int gui, int width, int height);
import void CentreGUI(int gui);
import int  IsGUIOn (int gui);
import void SetGUIBackgroundPic (int gui, int spriteSlot);
import void SetGUITransparency(int gui, int amount);
import void SetGUIClickable(int gui, int clickable);
import void SetGUIZOrder(int gui, int z);
#define GUIOn InterfaceOn
#define GUIOff InterfaceOff

import void SetGUIObjectEnabled(int gui, int object, int enable);
import void SetGUIObjectPosition(int gui, int object, int x, int y);
import void SetGUIObjectSize(int gui, int object, int width, int height);
import void SetLabelColor(int gui, int object, int colour);
import void SetLabelText(int gui, int object, const string text);
import void SetLabelFont(int gui, int object, FontType);
import void SetButtonText(int gui, int object, const string text);
import void SetButtonPic(int gui, int object, int which, int spriteSlot);
import int  GetButtonPic(int gui, int object, int which);
import void AnimateButton(int gui, int object, int view, int loop, int delay, int repeat);
import void SetSliderValue(int gui, int object, int value);
import int  GetSliderValue(int gui, int object);
import void SetTextBoxFont(int gui, int object, FontType);
import void GetTextBoxText(int gui, int object, string buffer);
import void SetTextBoxText(int gui, int object, const string text);
import void ListBoxClear(int gui, int object);
import void ListBoxAdd(int gui, int object, const string text);
import int  ListBoxGetSelected(int gui, int object);
import void ListBoxGetItemText(int gui, int object, int listIndex, string buffer);
import void ListBoxSetSelected(int gui, int object, int listIndex);
import void ListBoxSetTopItem (int gui, int object, int listIndex);
import void ListBoxDirList (int gui, int object, const string fileMask);
import int  ListBoxGetNumItems (int gui, int object);
import int  ListBoxSaveGameList (int gui, int object);
import void ListBoxRemove (int gui, int object, int listIndex);

#define LEAVE_ROOM 1
#define ENTER_ROOM 2
#define EGO_DIES   3
#define GOT_SCORE  4
#define GUI_MDOWN  5
#define GUI_MUP    6
#define ADD_INVENTORY  7
#define LOSE_INVENTORY 8
#define RESTORE_GAME   9

import void SetFrameSound (int view, int loop, int frame, int sound);
#endif // !STRICT

#ifdef SCRIPT_API_v350
enum GUIPopupStyle {
  eGUIPopupNormal = 0,
  eGUIPopupMouseYPos = 1,
  eGUIPopupModal = 2,
  eGUIPopupPersistent = 3
};
#endif // SCRIPT_API_v350

enum BlockingStyle {
  eBlock = 919,
  eNoBlock = 920
};
enum Direction {
  eForwards = 1062,
  eBackwards = 1063
};
enum WalkWhere {
  eAnywhere = 304,
  eWalkableAreas = 305
};

// forward-declare these so that they can be returned by GUIControl class
builtin managed struct GUI;
builtin managed struct Label;
builtin managed struct Button;
builtin managed struct Slider;
builtin managed struct TextBox;
builtin managed struct InvWindow;
builtin managed struct ListBox;
builtin managed struct Character;
#ifdef SCRIPT_API_v350
builtin managed struct TextWindowGUI;
#endif // SCRIPT_API_v350

builtin managed struct GUIControl {
  /// Brings this control to the front of the z-order, in front of all other controls.
  import void BringToFront();
  /// Gets the GUI Control that is visible at the specified location on the screen, or null.
  import static GUIControl* GetAtScreenXY(int x, int y);    // $AUTOCOMPLETESTATICONLY$  $AUTOCOMPLETENOINHERIT$
#ifdef SCRIPT_API_v361
  import static GUIControl* GetByName(const string scriptName); // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v361
  /// Sends this control to the back of the z-order, behind all other controls.
  import void SendToBack();
  /// Moves the control to the specified position within the GUI.
  import void SetPosition(int x, int y);
  /// Changes the control to the specified size.
  import void SetSize(int width, int height);
  /// If this control is a button, returns the Button interface; otherwise null.
  readonly import attribute Button*  AsButton;   // $AUTOCOMPLETENOINHERIT$
  /// If this control is a inventory window, returns the InvWindow interface; otherwise null.
  readonly import attribute InvWindow* AsInvWindow;  // $AUTOCOMPLETENOINHERIT$
  /// If this control is a label, returns the Label interface; otherwise null.
  readonly import attribute Label*   AsLabel;    // $AUTOCOMPLETENOINHERIT$
  /// If this control is a list box, returns the ListBox interface; otherwise null.
  readonly import attribute ListBox* AsListBox;  // $AUTOCOMPLETENOINHERIT$
  /// If this control is a slider, returns the Slider interface; otherwise null.
  readonly import attribute Slider*  AsSlider;   // $AUTOCOMPLETENOINHERIT$
  /// If this control is a text box, returns the TextBox interface; otherwise null.
  readonly import attribute TextBox* AsTextBox;  // $AUTOCOMPLETENOINHERIT$
  /// Gets/sets whether this control can be clicked on or whether clicks pass straight through it.
  import attribute bool Clickable;
  /// Gets/sets whether this control is currently enabled.
  import attribute bool Enabled;
  /// Gets/sets the height of the control.
  import attribute int  Height;
  /// Gets the ID number of the control within its owning GUI.
  readonly import attribute int  ID;
  /// Gets the GUI that this control is placed onto.
  readonly import attribute GUI* OwningGUI;
  /// Gets/sets whether this control is currently visible.
  import attribute bool Visible;
  /// Gets/sets the width of the control.
  import attribute int  Width;
  /// Gets/sets the X position of the control's top-left corner.
  import attribute int  X;
  /// Gets/sets the Y position of the control's top-left corner.
  import attribute int  Y;
#ifdef SCRIPT_API_v340
  /// Gets/sets the control's z-order relative to other controls within the same owning GUI.
  import attribute int  ZOrder;
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v360
  /// Gets/sets the control's transparency.
  import attribute int  Transparency;
#endif // SCRIPT_API_v360
#ifdef SCRIPT_API_v361
  /// Gets the script name of this control.
  import readonly attribute String ScriptName;
#endif // SCRIPT_API_v361
};

builtin managed struct Label extends GUIControl {
#ifndef STRICT_STRINGS
  import void GetText(string buffer);
  import void SetText(const string text);
#endif // !STRICT_STRINGS
  /// Gets/sets the font that is used to draw the label text.
  import attribute FontType Font;
  /// Gets/sets the text that is shown on the label.
  import attribute String Text;
  /// Gets/sets the colour in which the label text is drawn.
  import attribute int  TextColor;
#ifdef SCRIPT_API_v350
  /// Gets/sets label's text alignment.
  import attribute Alignment TextAlignment;
#endif // SCRIPT_API_v350
};

builtin managed struct Button extends GUIControl {
#ifdef SCRIPT_API_v360
  /// Animates the button graphic using the specified view loop.
  import void Animate(int view, int loop, int delay, RepeatStyle=eOnce, BlockingStyle=eNoBlock, Direction=eForwards, int frame=0, int volume=100);
#else // !SCRIPT_API_v360
  /// Animates the button graphic using the specified view loop.
  import void Animate(int view, int loop, int delay, RepeatStyle);
#endif // SCRIPT_API_v360
#ifndef STRICT_STRINGS
  import void GetText(string buffer);
  import void SetText(const string text);
#endif // !STRICT_STRINGS
  /// Gets/sets whether the image is clipped to the size of the control.
  import attribute bool ClipImage;
  /// Gets/sets the font used to display text on the button.
  import attribute FontType Font;
  /// Gets the currently displayed sprite number.
  readonly import attribute int  Graphic;
  /// Gets/sets the image that is shown when the player moves the mouse over the button (-1 if none)
  import attribute int  MouseOverGraphic;
  /// Gets/sets the image that is shown when the button is not pressed or mouse-overed
  import attribute int  NormalGraphic;
  /// Gets/sets the image that is shown when the button is pressed
  import attribute int  PushedGraphic;
  /// Gets/sets the colour in which the button text is drawn.
  import attribute int  TextColor;
  /// Gets/sets the text to be drawn on the button.
  import attribute String Text;
#ifdef SCRIPT_API_v340
  /// Runs the OnClick event handler for this Button.
  import void Click(MouseButton);
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v341
  /// Gets whether the button is currently animating.
  readonly import attribute bool Animating;
  /// Gets the current frame number during an animation.
  readonly import attribute int  Frame;
  /// Gets the current loop number during an animation.
  readonly import attribute int  Loop;
  /// Gets the current view number during an animation.
  readonly import attribute int  View;
#endif // SCRIPT_API_v341
#ifdef SCRIPT_API_v350
  /// Gets/sets text alignment inside the button.
  import attribute Alignment TextAlignment;
#endif // SCRIPT_API_v350
#ifdef SCRIPT_API_v362
  /// Gets/sets whether the button will wrap its text.
  import attribute bool WrapText;
  /// Gets/sets amount of padding, restricting the text from left and right
  import attribute int TextPaddingHorizontal;
  /// Gets/sets amount of padding, restricting the text from top and bottom
  import attribute int TextPaddingVertical;
#endif // SCRIPT_API_v362
};

builtin managed struct Slider extends GUIControl {
  /// Gets/sets the image that is tiled to make up the background of the slider.
  import attribute int  BackgroundGraphic;
  /// Gets/sets the image used for the 'handle' that represents the current slider position.
  import attribute int  HandleGraphic;
  /// Gets/sets the pixel offset at which the handle is drawn.
  import attribute int  HandleOffset;
  /// Gets/sets the maximum value that the slider can have.
  import attribute int  Max;
  /// Gets/sets the minimum value that the slider can have.
  import attribute int  Min;
  /// Gets/sets the current value of the slider.
  import attribute int  Value;
};

builtin managed struct TextBox extends GUIControl {
#ifndef STRICT_STRINGS
  import void GetText(string buffer);
  import void SetText(const string text);
#endif // !STRICT_STRINGS
  /// Gets/sets the font used to draw the text in the text box.
  import attribute FontType Font;
  /// Gets/sets the text that is currently in the text box.
  import attribute String Text;
  /// Gets/sets the color of the text in the text box.
  import attribute int TextColor;
#ifdef SCRIPT_API_v350
  /// Gets/sets whether the border around the text box is shown.
  import attribute bool ShowBorder;
#endif // SCRIPT_API_v350
};

builtin managed struct InvWindow extends GUIControl {
  /// Scrolls the inventory window down one row.
  import void ScrollDown();
  /// Scrolls the inventory window up one row.
  import void ScrollUp();
  /// Gets/sets which character's inventory is displayed in this window.
  import attribute Character* CharacterToUse;
  /// Gets the inventory item at the specified index in the window.
  readonly import attribute InventoryItem* ItemAtIndex[];
  /// Gets the number of inventory items currently shown in the window.
  readonly import attribute int ItemCount;
  /// Gets the height of each row of items.
  import attribute int ItemHeight;
  /// Gets the width of each column of items.
  import attribute int ItemWidth;
  /// Gets the index of the first visible item in the window.
  import attribute int TopItem;
  /// Gets the number of items shown per row in this inventory window.
  readonly import attribute int ItemsPerRow;
  /// Gets the number of visible rows in this inventory window.
  readonly import attribute int RowCount;
};

builtin managed struct ListBox extends GUIControl {
	/// Adds a new item to the bottom of the list with the specified text.
	import bool AddItem(const string text);
	/// Removes all the items from the list.
	import void Clear();
#ifdef SCRIPT_API_v362
	/// Fills the list box with all the filenames that match the specified file mask.
	import void FillDirList(const string fileMask, FileSortStyle fileSortStyle = eFileSort_Name, SortDirection sortDirection = eSortAscending);
	/// Fills the list box with the current user's saved games in the given range of slots. Returns true if all slots in range are occupied.
	import bool FillSaveGameList(int min_slot = 0, int max_slot = 99, SaveGameSortStyle saveSortStyle = eSaveGameSort_Time, SortDirection sortDirection = eSortDescending);
	/// Fills the list box with the current user's saved games using the array of slot indexes.
	import void FillSaveGameSlots(int save_slots[], SaveGameSortStyle saveSortStyle = eSaveGameSort_None, SortDirection sortDirection = eSortNoDirection);
#else // !SCRIPT_API_v362
    /// Fills the list box with all the filenames that match the specified file mask.
	import void FillDirList(const string fileMask);
	/// Fills the list box with the current user's saved games in the given range of slots.
	import int  FillSaveGameList();
#endif // !SCRIPT_API_v362
	/// Gets the item index at the specified screen co-ordinates, if they lie within the list box.
	import int  GetItemAtLocation(int x, int y);
#ifndef STRICT_STRINGS
	import void GetItemText(int listIndex, string buffer);
	import void SetItemText(int listIndex, const string newText);
#endif // !STRICT_STRINGS
	/// Inserts a new item before the specified index.
	import bool InsertItemAt(int listIndex, const string text);
	/// Removes the specified item from the list.
	import void RemoveItem(int listIndex);
	/// Scrolls the list down one row.
	import void ScrollDown();
	/// Scrolls the list up one row.
	import void ScrollUp();
	/// Gets/sets the font used to draw the list items.
	import attribute FontType Font;
#ifdef SCRIPT_COMPAT_v341
	/// Gets/sets whether the border around the list box is hidden.
	import attribute bool HideBorder;
	/// Gets/sets whether the clickable scroll arrows are hidden.
	import attribute bool HideScrollArrows;
#endif // SCRIPT_COMPAT_v341
	/// Gets the number of items currently in the list.
	readonly import attribute int ItemCount;
	/// Accesses the text for the items in the list.
	import attribute String Items[];
	/// Gets the number of visible rows that the listbox can display.
	readonly import attribute int RowCount;
	/// Gets the save game number that each row in the list corresponds to, after using FillSaveGameList.
	readonly import attribute int SaveGameSlots[];
	/// Gets/sets the currently selected item.
	import attribute int  SelectedIndex;
	/// Gets/sets the first visible item in the list.
	import attribute int  TopItem;
#ifdef SCRIPT_API_v350
	/// Gets/sets whether the border around the list box is shown.
	import attribute bool ShowBorder;
	/// Gets/sets whether the clickable scroll arrows are shown.
	import attribute bool ShowScrollArrows;
	/// Gets/sets color of the list item's selection
	import attribute int  SelectedBackColor;
	/// Gets/sets selected list item's text color
	import attribute int  SelectedTextColor;
	/// Gets/sets list item's text alignment.
	import attribute HorizontalAlignment TextAlignment;
	/// Gets/sets regular list item's text color
	import attribute int  TextColor;
#endif // SCRIPT_API_v350
};

builtin managed struct GUI {
  /// Moves the GUI to be centred on the screen.
  import void Centre();
  /// Gets the topmost GUI visible on the screen at the specified co-ordinates.
  import static GUI* GetAtScreenXY(int x, int y);    // $AUTOCOMPLETESTATICONLY$
#ifdef SCRIPT_API_v361
  import static GUI* GetByName(const string scriptName); // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v361
  /// Moves the GUI to have its top-left corner at the specified position.
  import void SetPosition(int x, int y);
  /// Changes the size of the GUI.
  import void SetSize(int width, int height);
  /// Gets/sets the sprite used to draw the GUI's background image.
  import attribute int  BackgroundGraphic;
  /// Gets/sets whether the GUI can be clicked on, or whether clicks pass straight through.
  import attribute bool Clickable;
  /// Accesses the controls that are on this GUI.
  readonly import attribute GUIControl *Controls[];
  /// Gets the number of controls on this GUI.
  readonly import attribute int  ControlCount;
  /// Gets/sets the height of the GUI.
  import attribute int  Height;
  /// Gets the ID number of the GUI.
  readonly import attribute int  ID;
  /// Gets/sets the transparency of the GUI.
  import attribute int  Transparency;
  /// Gets/sets whether the GUI is visible.
  import attribute bool Visible;
  /// Gets/sets the width of the GUI.
  import attribute int  Width;
  /// Gets/sets the X co-ordinate of the GUI's top-left corner.
  import attribute int  X;
  /// Gets/sets the Y co-ordinate of the GUI's top-left corner.
  import attribute int  Y;
  /// Gets/sets the GUI's z-order relative to other GUIs.
  import attribute int  ZOrder;
#ifdef SCRIPT_API_v340
  /// Runs the OnClick event handler for this GUI.
  import void Click(MouseButton);
  /// Performs default processing of a mouse click at the specified co-ordinates.
  import static void ProcessClick(int x, int y, MouseButton);
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v350
  /// Gets/sets the background color.
  import attribute int  BackgroundColor;
  /// Gets/sets the border color. Not applicable to TextWindow GUIs.
  import attribute int  BorderColor;
  /// If this GUI is a TextWindow, returns the TextWindowGUI interface; otherwise null.
  import readonly attribute TextWindowGUI* AsTextWindow; // $AUTOCOMPLETENOINHERIT$
  /// Gets the style of GUI behavior on screen.
  import readonly attribute GUIPopupStyle PopupStyle;
  /// Gets/sets the Y co-ordinate at which the GUI will appear when using MouseYPos popup style.
  import attribute int  PopupYPos;
#endif // SCRIPT_API_v350
#ifdef SCRIPT_API_v351
  /// Gets if this GUI is currently active on screen. In certain cases this is different than reading Visible property.
  import readonly attribute bool Shown;
#endif // SCRIPT_API_v351
#ifdef SCRIPT_API_v361
  /// Gets the script name of this GUI.
  import readonly attribute String ScriptName;
#endif // SCRIPT_API_v361

  int   reserved[2];   // $AUTOCOMPLETEIGNORE$
};

#ifdef SCRIPT_API_v350
builtin managed struct TextWindowGUI extends GUI {
  /// Gets/sets the text color.
  import attribute int  TextColor;
  /// Gets/sets the amount of padding, in pixels, surrounding the text in the TextWindow.
  import attribute int  TextPadding;
};
#endif // SCRIPT_API_v350

builtin managed struct Hotspot {
  /// Gets the hotspot that is at the specified position on the screen.
  import static Hotspot* GetAtScreenXY(int x, int y);    // $AUTOCOMPLETESTATICONLY$
#ifndef STRICT_STRINGS
  import void GetName(string buffer);
  import void GetPropertyText(const string property, string buffer);
#endif // !STRICT_STRINGS
#ifdef SCRIPT_API_v361
  import static Hotspot* GetByName(const string scriptName); // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v361
  /// Gets an integer Custom Property for this hotspot.
  import int  GetProperty(const string property);
  /// Gets a text Custom Property for this hotspot.
  import String GetTextProperty(const string property);
  /// Runs the specified event handler for this hotspot.
  import void RunInteraction(CursorMode);
  /// Gets/sets whether this hotspot is enabled.
  import attribute bool Enabled;
  /// Gets the ID of the hotspot.
  readonly import attribute int ID;
  /// Gets/sets the human-readable name of the hotspot.
  import attribute String Name;
  /// Gets the X co-ordinate of the walk-to point for this hotspot.
  readonly import attribute int WalkToX;
  /// Gets the Y co-ordinate of the walk-to point for this hotspot.
  readonly import attribute int WalkToY;
#ifdef SCRIPT_API_v340
  /// Checks whether an event handler has been registered for clicking on this hotspot in the specified cursor mode.
  import bool IsInteractionAvailable(CursorMode);
  /// Sets an integer custom property for this hotspot.
  import bool SetProperty(const string property, int value);
  /// Sets a text custom property for this hotspot.
  import bool SetTextProperty(const string property, const string value);
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v3507
  /// Returns the hotspot at the specified position within this room.
  import static Hotspot* GetAtRoomXY(int x, int y);      // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v3507
#ifdef SCRIPT_API_v360
  /// Gets the drawing surface for the 8-bit hotspots mask
  import static DrawingSurface* GetDrawingSurface();     // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v360
#ifdef SCRIPT_API_v361
  /// Gets the script name of this hotspot.
  import readonly attribute String ScriptName;
#endif // SCRIPT_API_v361

  int reserved[2];   // $AUTOCOMPLETEIGNORE$
};

builtin managed struct Region {
  /// Returns the region at the specified position within this room.
  import static Region* GetAtRoomXY(int x, int y);    // $AUTOCOMPLETESTATICONLY$
  /// Runs the event handler for the specified event for this region.
  import void RunInteraction(int event);
  /// Sets the region tint which will apply to characters that are standing on the region.
  import void Tint(int red, int green, int blue, int amount, int luminance = 100);
  /// Gets/sets whether this region is enabled.
  import attribute bool Enabled;
  /// Gets the ID number for this region.
  readonly import attribute int ID;
  /// Gets/sets the light level for this region.
  import attribute int  LightLevel;
  /// Gets whether a colour tint is set for this region.
  readonly import attribute bool TintEnabled;
  /// Gets the Blue component of this region's colour tint.
  readonly import attribute int  TintBlue;
  /// Gets the Green component of this region's colour tint.
  readonly import attribute int  TintGreen;
  /// Gets the Red component of this region's colour tint.
  readonly import attribute int  TintRed;
  /// Gets the Saturation of this region's colour tint.
  readonly import attribute int  TintSaturation;
  /// Gets the Luminance of this region's colour tint.
  readonly import attribute int  TintLuminance;
#ifdef SCRIPT_API_v3507
  /// Returns the region at the specified position on the screen.
  import static Region* GetAtScreenXY(int x, int y);    // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v3507
#ifdef SCRIPT_API_v360
  /// Gets the drawing surface for the 8-bit regions mask
  import static DrawingSurface* GetDrawingSurface();  // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v360
  int reserved[2];   // $AUTOCOMPLETEIGNORE$
};

builtin managed struct Dialog {
#ifdef SCRIPT_API_v361
  import static Dialog* GetByName(const string scriptName); // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v361
  /// Displays the options for this dialog and returns which one the player selected.
  import int DisplayOptions(DialogOptionSayStyle = eSayUseOptionSetting);
  /// Gets the enabled state for the specified option in this dialog.
  import DialogOptionState GetOptionState(int option);
  /// Gets the text of the specified option in this dialog.
  import String GetOptionText(int option);
  /// Checks whether the player has chosen this option before.
  import bool HasOptionBeenChosen(int option);
  /// Sets the enabled state of the specified option in this dialog.
  import void SetOptionState(int option, DialogOptionState);
  /// Runs the dialog interactively.
  import void Start();
  /// Gets the dialog's ID number for interoperating with legacy code.
  readonly import attribute int ID;
  /// Gets the number of options that this dialog has.
  readonly import attribute int OptionCount;
  /// Gets whether this dialog allows the player to type in text.
  readonly import attribute bool ShowTextParser;
#ifdef SCRIPT_API_v330
  /// Manually marks whether the option was chosen before or not.
  import void SetHasOptionBeenChosen(int option, bool chosen);
#endif // SCRIPT_API_v330
#ifdef SCRIPT_API_v361
  /// Gets the script name of this dialog.
  import readonly attribute String ScriptName;
#endif // SCRIPT_API_v361
#ifdef SCRIPT_API_v362
  /// Stop the currently running dialog.
  import static void Stop(); // $AUTOCOMPLETESTATICONLY$
  /// Gets the currently running dialog, returns null if no dialog is run
  import static readonly attribute Dialog* CurrentDialog; // $AUTOCOMPLETESTATICONLY$
  /// Gets the currently executed dialog option, or -1 if none is
  import static readonly attribute int ExecutedOption; // $AUTOCOMPLETESTATICONLY$
  /// Gets if the dialog options are currently displayed on screen
  import static readonly attribute bool AreOptionsDisplayed; // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v362
#ifdef SCRIPT_API_v363
  /// Gets/sets the color used to draw the active (selected) dialog option
  import static attribute int OptionsHighlightColor; // $AUTOCOMPLETESTATICONLY$
  /// Gets/sets the sprite to use as a bullet point before each dialog option (0 for none)
  import static attribute int OptionsBulletGraphic; // $AUTOCOMPLETESTATICONLY$
  /// Gets/sets the vertical gap between dialog options (in pixels)
  import static attribute int OptionsGap; // $AUTOCOMPLETESTATICONLY$
  /// Gets/sets the GUI that will be used to display dialog options; set null to use default options look
  import static attribute GUI* OptionsGUI; // $AUTOCOMPLETESTATICONLY$
  /// Gets/sets on-screen X position of dialog options GUI; set to -1 if it should use default placement
  import static attribute int OptionsGUIX; // $AUTOCOMPLETESTATICONLY$
  /// Gets/sets on-screen Y position of dialog options GUI; set to -1 if it should use default placement
  import static attribute int OptionsGUIY; // $AUTOCOMPLETESTATICONLY$
  /// Get/sets the maximal width of the auto-resizing GUI on which dialog options are drawn
  import static attribute int OptionsMaxGUIWidth; // $AUTOCOMPLETESTATICONLY$
  /// Get/sets the minimal width of the auto-resizing GUI on which dialog options are drawn
  import static attribute int OptionsMinGUIWidth; // $AUTOCOMPLETESTATICONLY$
  /// Gets/sets whether dialog options have numbers before them, and the numeric keys can be used to select them
  import static attribute DialogOptionsNumbering OptionsNumbering; // $AUTOCOMPLETESTATICONLY$
  /// Gets/sets the horizontal offset at which options are drawn on a standard GUI
  import static attribute int OptionsPaddingX; // $AUTOCOMPLETESTATICONLY$
  /// Gets/sets the vertical offset at which options are drawn on a standard GUI
  import static attribute int OptionsPaddingY; // $AUTOCOMPLETESTATICONLY$
  /// Gets/sets the color used to draw the dialog options that have already been selected once; set to -1 for no distinct color
  import static attribute int OptionsReadColor; // $AUTOCOMPLETESTATICONLY$
  /// Gets/sets the horizontal alignment of each dialog option's text
  import static attribute HorizontalAlignment OptionsTextAlignment; // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v363

  int reserved[2];   // $AUTOCOMPLETEIGNORE$
};

#define IsSpeechVoxAvailable IsVoxAvailable
//import int IsSpeechVoxAvailable();  // make autocomplete recognise

builtin struct Maths {
  /// Calculates the Arc Cosine of the specified value.
  import static float ArcCos(float value);
  /// Calculates the Arc Sine of the specified value.
  import static float ArcSin(float value);
  /// Calculates the Arc Tan of the specified value.
  import static float ArcTan(float value);
  /// Calculates the Arc Tan of y/x.
  import static float ArcTan2(float y, float x);
  /// Calculates the cosine of the specified angle.
  import static float Cos(float radians);
  /// Calculates the hyperbolic cosine of the specified angle.
  import static float Cosh(float radians);
  /// Converts the angle from degrees to radians.
  import static float DegreesToRadians(float degrees);
  /// Calculates the value of e to the power x.
  import static float Exp(float x);
  /// Calculates the natural logarithm (base e) of x.
  import static float Log(float x);
  /// Calculates the base-10 logarithm of x.
  import static float Log10(float x);
  /// Converts the angle from radians to degrees.
  import static float RadiansToDegrees(float radians);
  /// Calculates the base raised to the power of the exponent.
  import static float RaiseToPower(float base, float exponent);
  /// Calculates the sine of the angle.
  import static float Sin(float radians);
  /// Calculates the hyperbolic sine of the specified angle.
  import static float Sinh(float radians);
  /// Calculates the square root of the value.
  import static float Sqrt(float value);
  /// Calculates the tangent of the angle.
  import static float Tan(float radians);
  /// Calculates the hyperbolic tangent of the specified angle.
  import static float Tanh(float radians);
  /// Gets the value of PI
  readonly import static attribute float Pi;
};

builtin managed struct DateTime {
  /// Gets the current date and time on the player's system.
  readonly import static attribute DateTime* Now;   // $AUTOCOMPLETESTATICONLY$
#ifdef SCRIPT_API_v362
  /// Creates DateTime object from the provided date and time; returns invalid object if year is below 1970 or above 2038, or any other value is invalid
  import static DateTime* CreateFromDate(int year, int month, int day, int hour = 0, int minute = 0, int second = 0);   // $AUTOCOMPLETESTATICONLY$
  /// Creates DateTime object from the provided raw time value (in seconds); returns invalid object if rawTime is negative
  import static DateTime* CreateFromRawTime(int rawTime);   // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v362
  /// Gets the Year component of the date.
  readonly import attribute int Year;
  /// Gets the Month (1-12) component of the date.
  readonly import attribute int Month;
  /// Gets the DayOfMonth (1-31) component of the date.
  readonly import attribute int DayOfMonth;
  /// Gets the Hour (0-23) component of the time.
  readonly import attribute int Hour;
  /// Gets the Minute (0-59) component of the time.
  readonly import attribute int Minute;
  /// Gets the Second (0-59) component of the time.
  readonly import attribute int Second;
  /// Gets the raw time value, useful for calculating elapsed time periods.
  readonly import attribute int RawTime;
};

builtin managed struct DialogOptionsRenderingInfo {
  /// The option that the mouse is currently positioned over
  import attribute int ActiveOptionID;
  /// The dialog that is to have its options rendered
  readonly import attribute Dialog* DialogToRender;
  /// The height of the dialog options
  import attribute int Height;
  /// The width of the text box for typing parser input, if enabled
  import attribute int ParserTextBoxWidth;
  /// The X co-ordinate of the top-left corner of the text box for typing input
  import attribute int ParserTextBoxX;
  /// The Y co-ordinate of the top-left corner of the text box for typing input
  import attribute int ParserTextBoxY;
  /// The surface that the dialog options need to be rendered to
  readonly import attribute DrawingSurface* Surface;
  /// The width of the dialog options
  import attribute int Width;
  /// The X co-ordinate of the top-left corner of the dialog options
  import attribute int X;
  /// The Y co-ordinate of the top-left corner of the dialog options
  import attribute int Y;
#ifdef SCRIPT_API_v330
  /// Should the drawing surface have alpha channel
  import attribute bool HasAlphaChannel;
#endif // SCRIPT_API_v330
#ifdef SCRIPT_API_v340
  /// Runs the active dialog option
  import bool RunActiveOption();
  /// Forces dialog options to redraw itself
  import void Update();
#endif // SCRIPT_API_v340
};

builtin managed struct AudioChannel {
  /// Changes playback to continue from the specified position. The position units depend on the audio type.
  import void Seek(int position);
  /// Sets the audio to have its location at (x,y); it will get quieter the further away the player is.
  import void SetRoomLocation(int x, int y);
  /// Stops the sound currently playing on this channel.
  import void Stop();
  /// The channel ID of this channel (for use with legacy script).
  readonly import attribute int ID;
  /// Whether this channel is currently playing something.
  readonly import attribute bool IsPlaying;
  /// The length of the currently playing audio clip, in milliseconds.
  readonly import attribute int LengthMs;
  /// The stereo panning of the channel, from -100 to 100.
  import attribute int Panning;
  /// The audio clip that is currently being played on this channel, or null if none.
  readonly import attribute AudioClip* PlayingClip;
  /// The current offset into the sound. What this represents depends on the audio type.
  readonly import attribute int Position;
  /// The current offset into the sound, in milliseconds.
  readonly import attribute int PositionMs;
  /// The volume of this sound channel, from 0 to 100.
  import attribute int Volume;
#ifdef SCRIPT_API_v340
  /// The speed of playing, in clip milliseconds per second (1000 is default).
  import attribute int Speed;
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v360
  /// Pauses the playback on this channel.
  import void Pause();
  /// Resumes the paused playback on this channel.
  import void Resume();
  /// Whether this channel is currently paused.
  readonly import attribute bool IsPaused;
#endif // SCRIPT_API_v360
#ifdef SCRIPT_API_v36026
  /// Changes playback to continue from the specified position in milliseconds.
  import void SeekMs(int position);
#endif // SCRIPT_API_v36026
};

builtin managed struct AudioClip {
#ifdef SCRIPT_API_v361
  import static AudioClip* GetByName(const string scriptName); // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v361
  /// Plays this audio clip.
  import AudioChannel* Play(AudioPriority=SCR_NO_VALUE, RepeatStyle=SCR_NO_VALUE);
  /// Plays this audio clip, starting from the specified offset.
  import AudioChannel* PlayFrom(int position, AudioPriority=SCR_NO_VALUE, RepeatStyle=SCR_NO_VALUE);
  /// Plays this audio clip, or queues it if all channels are busy.
  import AudioChannel* PlayQueued(AudioPriority=SCR_NO_VALUE, RepeatStyle=SCR_NO_VALUE);
#ifdef SCRIPT_API_v360
  /// Plays this audio clip, explicitly putting it on the particular channel.
  import AudioChannel* PlayOnChannel(int chan, AudioPriority=SCR_NO_VALUE, RepeatStyle=SCR_NO_VALUE);
#endif // SCRIPT_API_v360
  /// Stops all currently playing instances of this audio clip.
  import void Stop();
  /// Gets the file type of the sound.
  readonly import attribute AudioFileType FileType;
  /// Checks whether this audio file is available on the player's system.
  readonly import attribute bool IsAvailable;
  /// Gets the type of audio that this clip contains.
  readonly import attribute AudioType Type;
#ifdef SCRIPT_API_v350
  /// Gets the clip's ID number.
  readonly import attribute int ID;
#endif // SCRIPT_API_v350
#ifdef SCRIPT_API_v361
  /// Gets the script name of this clip.
  import readonly attribute String ScriptName;
#endif // SCRIPT_API_v361
};


#ifdef SCRIPT_API_v362
// Engine value constant name pattern:
// ENGINE_VALUE_<I,II,S,SI>_NAME, where
//   I - integer, II - indexed integer, S - string, SI - indexed string.
// When adding indexed values - make sure to also add a value that tells their count.

enum EngineValueID {
  ENGINE_VALUE_UNDEFINED = 0,            // formality...
  ENGINE_VALUE_SI_VALUENAME,             // get engine value's own name, by its index
  ENGINE_VALUE_S_ENGINE_NAME,
  ENGINE_VALUE_S_ENGINE_VERSION,         // N.N.N.N (with an optional custom tag)
  ENGINE_VALUE_S_ENGINE_VERSION_FULL,    // full, with bitness, endianess and any tag list
  ENGINE_VALUE_S_DISPLAY_MODE_STR,
  ENGINE_VALUE_S_GFXRENDERER,
  ENGINE_VALUE_S_GFXFILTER,
  ENGINE_VALUE_I_SPRCACHE_MAXNORMAL,
  ENGINE_VALUE_I_SPRCACHE_NORMAL,
  ENGINE_VALUE_I_SPRCACHE_LOCKED,
  ENGINE_VALUE_I_SPRCACHE_EXTERNAL,
  ENGINE_VALUE_I_TEXCACHE_MAXNORMAL,
  ENGINE_VALUE_I_TEXCACHE_NORMAL,
  ENGINE_VALUE_I_FPS_MAX,
  ENGINE_VALUE_I_FPS,
  ENGINE_VALUE_LAST                      // in case user wants to iterate them
};
#endif // SCRIPT_API_v362

builtin struct System {
#ifdef SCRIPT_COMPAT_v350
  readonly int  screen_width,screen_height;
  readonly int  color_depth;
  readonly int  os;
  readonly int  windowed;
  int  vsync;
  readonly int  viewport_width, viewport_height;
#ifndef STRICT_STRINGS
  readonly char version[10];
#endif // !STRICT_STRINGS
#endif // SCRIPT_COMPAT_v350
  /// Gets whether Caps Lock is currently on.
  readonly import static attribute bool CapsLock;
  /// Gets a specific audio channel instance.
  readonly import static attribute AudioChannel *AudioChannels[];   // $AUTOCOMPLETESTATICONLY$
  /// Gets the number of audio channels supported by AGS.
  readonly import static attribute int  AudioChannelCount;   // $AUTOCOMPLETESTATICONLY$
  /// Gets the colour depth that the game is running at.
  readonly import static attribute int  ColorDepth;
  /// Gets/sets the gamma correction level.
  import static attribute int  Gamma;
  /// Gets whether the game is running with 3D Hardware Acceleration.
  readonly import static attribute bool HardwareAcceleration;
  /// Gets whether Num Lock is currently on.
  readonly import static attribute bool NumLock;
  /// Gets which operating system the game is running on.
  readonly import static attribute eOperatingSystem OperatingSystem;
#ifdef SCRIPT_COMPAT_v350
  /// Gets the screen height of the current resolution.
  readonly import static attribute int  ScreenHeight;
  /// Gets the screen width of the current resolution.
  readonly import static attribute int  ScreenWidth;
#endif // SCRIPT_COMPAT_v350
  /// Gets whether Scroll Lock is currently on.
  readonly import static attribute bool ScrollLock;
  /// Gets whether the player's system supports gamma adjustment.
  readonly import static attribute bool SupportsGammaControl;
  /// Gets the AGS engine version number.
  readonly import static attribute String Version;
#ifdef SCRIPT_COMPAT_v350
  /// Gets the height of the visible area in which the game is displayed.
  readonly import static attribute int  ViewportHeight;
  /// Gets the width of the visible area in which the game is displayed.
  readonly import static attribute int  ViewportWidth;
#endif // SCRIPT_COMPAT_v350
  /// Gets/sets the audio output volume, from 0-100.
  import static attribute int  Volume;
  /// Gets/sets whether waiting for the vertical sync is enabled.
  import static attribute bool VSync;
  /// Gets/sets whether the game runs in a window or fullscreen.
  import static attribute bool Windowed;
#ifdef SCRIPT_API_v335
  /// Gets whether the game window has input focus
  readonly import static attribute bool HasInputFocus;
#endif // SCRIPT_API_v335
#ifdef SCRIPT_API_v340
  /// Gets a report about the runtime engine the game is running under.
  readonly import static attribute String RuntimeInfo;
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v341
  /// Gets/sets whether sprites are rendered at screen resolution or native game resolution.
  import static attribute bool RenderAtScreenResolution;
#endif // SCRIPT_API_v341
#ifdef SCRIPT_API_v351
  /// Saves current runtime settings into configuration file
  import static void SaveConfigToFile();
#endif // SCRIPT_API_v351
#ifdef SCRIPT_API_v360
  /// Prints message
  import static void Log(LogLevel level, const string format, ...);    // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v360
#ifdef SCRIPT_API_v362
  /// Gets a runtime engine value represented as integer by the given identifier; is meant for diagnostic purposes only
  import static int GetEngineInteger(EngineValueID value, int index = 0); // $AUTOCOMPLETESTATICONLY$
  /// Gets a runtime engine string by the given identifier; is meant for diagnostic purposes only
  import static String GetEngineString(EngineValueID value, int index = 0); // $AUTOCOMPLETESTATICONLY$
  /// Gets/sets whether the engine displays the FPS counter
  import static attribute bool DisplayFPS; // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v362
};

builtin managed struct Object {
  /// Animates the object using its current view.
  import void Animate(int loop, int delay, RepeatStyle=eOnce, BlockingStyle=eBlock, Direction=eForwards
#ifdef SCRIPT_API_v3507
    , int frame=0
#endif  
#ifdef SCRIPT_API_v360
    , int volume=100
#endif
  );
  /// Gets the object that is on the screen at the specified co-ordinates.
  import static Object* GetAtScreenXY(int x, int y);    // $AUTOCOMPLETESTATICONLY$
#ifndef STRICT_STRINGS
  import void GetName(string buffer);
  import void GetPropertyText(const string property, string buffer);
#endif // !STRICT_STRINGS
#ifdef SCRIPT_API_v361
  import static Object* GetByName(const string scriptName); // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v361
  /// Gets an integer Custom Property for this object.
  import int GetProperty(const string property);
  /// Gets a text Custom Property for this object.
  import String   GetTextProperty(const string property);
  /// Checks whether this object is colliding with another.
  import bool IsCollidingWithObject(Object*);
  /// Merges the object's image into the room background, and disables the object.
  import void MergeIntoBackground();
  /// Starts the object moving towards the specified co-ordinates.
  import void Move(int x, int y, int speed, BlockingStyle=eNoBlock, WalkWhere=eWalkableAreas);
  /// Removes a specific object tint, and returns the object to using the ambient room tint.
  import void RemoveTint();
  /// Runs the event handler for the specified event.
  import void RunInteraction(CursorMode);
  /// Instantly moves the object to have its bottom-left at the new co-ordinates.
  import void SetPosition(int x, int y);
#ifdef SCRIPT_API_v360
  /// Sets the object to use the specified view, ahead of doing an animation.
  import void SetView(int view, int loop=0, int frame=0);
#else // !SCRIPT_API_v360
  /// Sets the object to use the specified view, ahead of doing an animation.
  import void SetView(int view, int loop=-1, int frame=-1);
#endif // !SCRIPT_API_v360
  /// Stops any currently running animation on the object.
  import void StopAnimating();
  /// Stops any currently running move on the object.
  import void StopMoving();
  /// Tints the object to the specified colour.
  import void Tint(int red, int green, int blue, int saturation, int luminance);
  /// Gets whether the object is currently animating.
  readonly import attribute bool Animating;
  /// Gets/sets the object's baseline. This can be 0 to use the object's Y position as its baseline.
  import attribute int  Baseline;
  /// Allows you to manually specify the blocking height of the base of the object.
  import attribute int  BlockingHeight;
  /// Allows you to manually specify the blocking width of the base of the object.
  import attribute int  BlockingWidth;
  /// Gets/sets whether the mouse can be clicked on this object or whether it passes straight through.
  import attribute bool Clickable;
  /// Gets the current frame number during an animation.
  readonly import attribute int  Frame;
  /// Gets/sets the sprite number that is currently displayed on the object.
  import attribute int  Graphic;
  /// Gets the object's ID number.
  readonly import attribute int ID;
#ifdef SCRIPT_COMPAT_v351
  /// Gets/sets whether the object ignores walkable area scaling.
  import attribute bool IgnoreScaling;
#endif // SCRIPT_COMPAT_v351
#ifdef SCRIPT_COMPAT_v340
  /// Gets/sets whether the object ignores walk-behind areas.
  import attribute bool IgnoreWalkbehinds;
#endif // SCRIPT_COMPAT_v340
  /// Gets the current loop number during an animation.
  readonly import attribute int  Loop;
  /// Gets whether the object is currently moving.
  readonly import attribute bool Moving;
  /// Gets/sets the human-readable name of the object.
  import attribute String Name;
  /// Gets/sets whether other objects and characters can move through this object.
  import attribute bool Solid;
  /// Gets/sets the object's transparency.
  import attribute int  Transparency;
  /// Gets the current view number during an animation.
  readonly import attribute int View;
  /// Gets/sets whether the object is currently visible.
  import attribute bool Visible;
  /// Gets/sets the X co-ordinate of the object's bottom-left hand corner.
  import attribute int  X;
  /// Gets/sets the Y co-ordinate of the object's bottom-left hand corner.
  import attribute int  Y;
#ifdef SCRIPT_API_v340
  /// Checks whether an event handler has been registered for clicking on this object in the specified cursor mode.
  import bool     IsInteractionAvailable(CursorMode);
  /// Sets the individual light level for this object.
  import void SetLightLevel(int light_level);
  /// Sets an integer custom property for this object.
  import bool SetProperty(const string property, int value);
  /// Sets a text custom property for this object.
  import bool SetTextProperty(const string property, const string value);
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v341
  /// Gets whether the object has an explicit light level set.
  readonly import attribute bool HasExplicitLight;
  /// Gets whether the object has an explicit tint set.
  readonly import attribute bool HasExplicitTint;
  /// Gets the individual light level for this object.
  readonly import attribute int  LightLevel;
  /// Gets the Blue component of this object's colour tint.
  readonly import attribute int  TintBlue;
  /// Gets the Green component of this object's colour tint.
  readonly import attribute int  TintGreen;
  /// Gets the Red component of this object's colour tint.
  readonly import attribute int  TintRed;
  /// Gets the Saturation of this object's colour tint.
  readonly import attribute int  TintSaturation;
  /// Gets the Luminance of this object's colour tint.
  readonly import attribute int  TintLuminance;
#endif // SCRIPT_API_v341
#ifdef SCRIPT_API_v3507
  /// Returns the object at the specified position within this room.
  import static Object* GetAtRoomXY(int x, int y);      // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v3507
#ifdef SCRIPT_API_v360
  /// Gets/sets whether the object uses manually specified scaling instead of using walkable area scaling.
  import attribute bool ManualScaling;
  /// Gets/sets the object's current scaling level.
  import attribute int  Scaling;
#endif // SCRIPT_API_v360
#ifdef SCRIPT_API_v361
  /// Gets/sets the volume modifier (0-100) of frame-linked sounds for this object.
  import attribute int  AnimationVolume;
  /// Gets the script name of this object.
  import readonly attribute String ScriptName;
#endif // SCRIPT_API_v361
#ifdef SCRIPT_API_v362
  /// Gets the X coordinate of the object's final moving destination; or current position if object is not moving.
  readonly import attribute int DestinationX;
  /// Gets the Y coordinate of the object's final moving destination; or current position if object is not moving.
  readonly import attribute int DestinationY;
#endif

  int reserved[2];  // $AUTOCOMPLETEIGNORE$
};

#ifdef SCRIPT_API_v341
enum StopMovementStyle
{
  eKeepMoving = 0,
  eStopMoving = 1
};
#endif // SCRIPT_API_v341

builtin managed struct Character {
  /// Adds the specified item to the character's inventory.
  import void AddInventory(InventoryItem *item, int addAtIndex=SCR_NO_VALUE);
  /// Manually adds a waypoint to the character's movement path.
  import void AddWaypoint(int x, int y);
  /// Animates the character using its current locked view.
  import void Animate(int loop, int delay, RepeatStyle=eOnce, BlockingStyle=eBlock, Direction=eForwards
#ifdef SCRIPT_API_v3507
    , int frame=0
#endif  
#ifdef SCRIPT_API_v360
    , int volume=100
#endif
  );
#ifdef SCRIPT_API_v340
  /// Moves the character to another room. If this is the player character, the game will also switch to that room.
  import void ChangeRoom(int room, int x=SCR_NO_VALUE, int y=SCR_NO_VALUE, CharacterDirection direction=eDirectionNone);
#else // !SCRIPT_API_v340
  /// Moves the character to another room. If this is the player character, the game will also switch to that room.
  import void ChangeRoom(int room, int x=SCR_NO_VALUE, int y=SCR_NO_VALUE);
#endif // !SCRIPT_API_v340
  /// Moves the character to another room, using the old-style position variable
  import void ChangeRoomAutoPosition(int room, int position=0);
  /// Changes the character's normal walking view.
  import void ChangeView(int view);
  /// Turns this character to face the other character.
  import void FaceCharacter(Character* , BlockingStyle=eBlock);
  /// Turns this character to face the specified location in the room.
  import void FaceLocation(int x, int y, BlockingStyle=eBlock);
  /// Turns this character to face the specified object.
  import void FaceObject(Object* , BlockingStyle=eBlock);
  /// Starts this character following the other character.
  import void FollowCharacter(Character*, int dist=10, int eagerness=97);
  /// Returns the character that is at the specified position on the screen.
  import static Character* GetAtScreenXY(int x, int y);    // $AUTOCOMPLETESTATICONLY$
#ifdef SCRIPT_API_v361
  import static Character* GetByName(const string scriptName); // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v361
  /// Gets a numeric custom property for this character.
  import int GetProperty(const string property);
#ifndef STRICT_STRINGS
  import void     GetPropertyText(const string property, string buffer);
#endif // !STRICT_STRINGS
  /// Gets a text custom property for this character.
  import String   GetTextProperty(const string property);
  /// Checks whether the character currently has the specified inventory item.
  import bool     HasInventory(InventoryItem *item);
  /// Checks whether this character is in collision with the other character.
  import bool IsCollidingWithChar(Character*);
  /// Checks whether this character is in collision with the object.
  import bool IsCollidingWithObject(Object* );
#ifdef SCRIPT_API_v341
  /// Locks the character to this view, ready for doing animations.
  import void LockView(int view, StopMovementStyle=eStopMoving);
  /// Locks the character to this view, and aligns it against one side of the existing sprite.
  import void LockViewAligned(int view, int loop, HorizontalAlignment, StopMovementStyle=eStopMoving);
  /// Locks the character to the specified view frame
  import void LockViewFrame(int view, int loop, int frame, StopMovementStyle=eStopMoving);
  /// Locks the character to is view, with high-resolution position adjustment.
  import void LockViewOffset(int view, int xOffset, int yOffset, StopMovementStyle=eStopMoving);
#else // !SCRIPT_API_v341
  /// Locks the character to this view, ready for doing animations.
  import void LockView(int view);
  /// Locks the character to this view, and aligns it against one side of the existing sprite.
  import void LockViewAligned(int view, int loop, Alignment);
  /// Locks the character to the specified view frame
  import void LockViewFrame(int view, int loop, int frame);
  /// Locks the character to is view, with high-resolution position adjustment.
  import void LockViewOffset(int view, int xOffset, int yOffset);
#endif // !SCRIPT_API_v341
  /// Removes the item from this character's inventory.
  import void LoseInventory(InventoryItem *item);
  /// Moves the character to the destination, without playing his walking animation.
  import void Move(int x, int y, BlockingStyle=eNoBlock, WalkWhere=eWalkableAreas);
  /// Moves the character to the nearest walkable area.
  import void PlaceOnWalkableArea();
  /// Removes an existing colour tint from the character.
  import void RemoveTint();
  /// Runs one of the character's interaction events.
  import void RunInteraction(CursorMode);
  /// Says the specified text using the character's speech settings.
  import void Say(const string message, ...);
#ifdef SCRIPT_API_v361
  /// Says the specified text at the specified position on the screen using the character's speech settings.
  import void SayAt(int x, int y, int width, const string message, ...);
  /// Displays the text as lucasarts-style speech but does not block the game.
  import Overlay* SayBackground(const string message, ...);
#else // !SCRIPT_API_v361
  /// Says the specified text at the specified position on the screen using the character's speech settings.
  import void SayAt(int x, int y, int width, const string message);
  /// Displays the text as lucasarts-style speech but does not block the game.
  import Overlay* SayBackground(const string message);
#endif // !SCRIPT_API_v361
  /// Makes this character the player character.
  import void SetAsPlayer();
  /// Changes the character's idle view.
  import void SetIdleView(int view, int delay);
  /// Changes the character's movement speed.
  import void SetWalkSpeed(int x, int y);
  /// Stops the character from moving.
  import void StopMoving();
  /// The specified text is displayed in a thought-bubble GUI.
  import void Think(const string message, ...);
  /// Tints the character to the specified colour.
  import void     Tint(int red, int green, int blue, int saturation, int luminance);
#ifdef SCRIPT_API_v341
  /// Unlocks the view after an animation has finished.
  import void UnlockView(StopMovementStyle=eStopMoving);
#else // !SCRIPT_API_v341
  /// Unlocks the view after an animation has finished.
  import void UnlockView();
#endif // !SCRIPT_API_v341
  /// Moves the character to the destination, automatically playing his walking animation.
  import void Walk(int x, int y, BlockingStyle=eNoBlock, WalkWhere=eWalkableAreas);
  /// Moves the character in a straight line as far as possible towards the co-ordinates. Useful for keyboard movement.
  import void WalkStraight(int x, int y, BlockingStyle=eNoBlock);
  /// Gets/sets the character's current inventory item. null if no item selected.
  import attribute InventoryItem* ActiveInventory;
  /// Gets whether the character is currently animating.
  readonly import attribute bool Animating;
  /// Gets/sets the character's animation speed.
  import attribute int  AnimationSpeed;
  /// Gets/sets a specific baseline for the character. 0 means character's Y-pos will be used.
  import attribute int  Baseline;
  /// Gets/sets the interval at which the character will blink while talking, in game loops.
  import attribute int  BlinkInterval;
  /// Gets/sets the view used for the character's blinking animation. -1 to disable.
  import attribute int  BlinkView;
  /// Gets/sets whether the character will blink while thinking as well as talking.
  import attribute bool BlinkWhileThinking;
  /// Allows you to manually specify the height of the blocking area at the character's feet.
  import attribute int  BlockingHeight;
  /// Allows you to manually specify the width of the blocking area at the character's feet.
  import attribute int  BlockingWidth;
  /// Gets/sets whether the mouse can be clicked on the character, or whether it passes straight through.
  import attribute bool Clickable;
  /// Gets/sets whether this character has an 8-loop walking view with diagonal loops.
  import attribute bool DiagonalLoops;
  /// Gets/sets the character's current frame number within its current view.
  import attribute int  Frame;
  /// Gets whether the character has an explicit tint set.
  readonly import attribute bool HasExplicitTint;
  /// Gets the character's ID number.
  readonly import attribute int ID;
  /// Gets the character's idle view (-1 if none).
  readonly import attribute int IdleView;
  /// Gets/sets whether the character ignores region tints and lighting.
  import attribute bool IgnoreLighting;
  import attribute bool IgnoreScaling;       // obsolete. $AUTOCOMPLETEIGNORE$
#ifdef SCRIPT_COMPAT_v340
  /// Gets/sets whether the character ignores walk-behind areas and is always placed on top.
  import attribute bool IgnoreWalkbehinds; 
#endif // SCRIPT_COMPAT_v340
  /// Accesses the number of each inventory item that the character currently has.
  import attribute int  InventoryQuantity[];
  /// Gets/sets the character's current loop number within its current view.
  import attribute int  Loop;
  /// Gets/sets whether the character uses manually specified scaling instead of using walkable area scaling.
  import attribute bool ManualScaling;
  /// Gets/sets whether the character only moves when their animation frame changes.
  import attribute bool MovementLinkedToAnimation;
  /// Gets whether the character is currently moving.
  readonly import attribute bool Moving;
  /// Gets/sets the human-readable character's name.
  import attribute String Name;
  /// Gets the character's normal walking view.
  readonly import attribute int NormalView;
  /// Gets the room number that the character was in before this one.
  readonly import attribute int PreviousRoom;
  /// Gets the room number that the character is currently in.
  readonly import attribute int Room;
  /// Gets/sets whether the character's movement speed is adjusted in line with its scaling level.
  import attribute bool ScaleMoveSpeed;
  /// Gets/sets whether the volume of frame-linked sounds for the character are adjusted in line with its scaling level.
  import attribute bool ScaleVolume;
  /// Gets/sets the character's current scaling level.
  import attribute int  Scaling;
  /// Gets/sets whether this character blocks other objects and characters from moving through it.
  import attribute bool Solid;
  /// Gets whether the character is currently in the middle of a Say command.
  readonly import attribute bool Speaking;
  /// Gets the current frame of the character's speaking animation (only valid when Speaking is true)
  readonly import attribute int SpeakingFrame;
  /// Gets/sets the character's speech animation delay (only if not using global setting).
  import attribute int  SpeechAnimationDelay;
  /// Gets/sets the character's speech text colour.
  import attribute int  SpeechColor;
  /// Gets/sets the character's speech view.
  import attribute int  SpeechView;
  /// Gets/sets the character's thinking view.
  import attribute int  ThinkView;
  /// Gets/sets the character's current transparency level.
  import attribute int  Transparency;
  /// Gets/sets whether the character turns on the spot to face the correct direction before walking.
  import attribute bool TurnBeforeWalking;
  /// Gets the character's current view number.
  readonly import attribute int View;
  /// Gets the character's X movement speed.
  readonly import attribute int WalkSpeedX;
  /// Gets the character's Y movement speed.
  readonly import attribute int WalkSpeedY;
#ifdef SCRIPT_API_v334
  /// Gets whether the character is currently in the middle of a Think command.
  readonly import attribute bool Thinking;
  /// Gets the current frame of the character's thinking animation (only valid when Thinking is true)
  readonly import attribute int ThinkingFrame;
#endif // SCRIPT_API_v334
#ifdef SCRIPT_API_v340
  /// Turns this character to face the specified direction.
  import void FaceDirection(CharacterDirection direction, BlockingStyle=eBlock);
  /// Sets an integer custom property for this character.
  import bool SetProperty(const string property, int value);
  /// Sets a text custom property for this character.
  import bool SetTextProperty(const string property, const string value);
  /// Checks whether an event handler has been registered for clicking on this character in the specified cursor mode.
  import bool IsInteractionAvailable(CursorMode);
  /// Sets the individual light level for this character.
  import void SetLightLevel(int light_level);
  /// Gets the X coordinate of the character's final moving destination; or current position if character is not moving.
  readonly import attribute int DestinationX;
  /// Gets the Y coordinate of the character's final moving destination; or current position if character is not moving.
  readonly import attribute int DestinationY;
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v341
  /// Gets whether the character has an explicit light level set.
  readonly import attribute bool HasExplicitLight;
  /// Gets the individual light level for this character.
  readonly import attribute int  LightLevel;
  /// Gets the Blue component of this character's colour tint.
  readonly import attribute int  TintBlue;
  /// Gets the Green component of this character's colour tint.
  readonly import attribute int  TintGreen;
  /// Gets the Red component of this character's colour tint.
  readonly import attribute int  TintRed;
  /// Gets the Saturation of this character's colour tint.
  readonly import attribute int  TintSaturation;
  /// Gets the Luminance of this character's colour tint.
  readonly import attribute int  TintLuminance;
#endif // SCRIPT_API_v341
#ifdef SCRIPT_API_v3507
  /// Returns the character at the specified position within this room.
  import static Character* GetAtRoomXY(int x, int y);      // $AUTOCOMPLETESTATICONLY$
#endif // SCRIPT_API_v3507
#ifdef SCRIPT_API_v360
  /// Gets/sets the volume modifier (0-100) of frame-linked sounds for this character.
  import attribute int  AnimationVolume;
  /// Gets/sets the character's idle animation delay.
  import attribute int  IdleAnimationDelay;
#endif // SCRIPT_API_v360
#ifdef SCRIPT_API_v361
  /// Gets the script name of this character.
  import readonly attribute String ScriptName;
#endif // SCRIPT_API_v361
#ifdef SCRIPT_API_v362
  /// Moves the character in a straight line as far as possible towards the co-ordinates, without walking animation. Useful for keyboard movement.
  import void MoveStraight(int x, int y, BlockingStyle=eNoBlock);
  /// Gets/sets whether the character turns on the spot when ordered to face the new standing direction.
  import attribute bool TurnWhenFacing;
  /// Gets the character this character is following
  readonly import attribute Character* Following;
#endif // SCRIPT_API_v362
#ifdef STRICT
  /// The character's current X-position.
  import attribute int  x;
  /// The character's current Y-position.
  import attribute int  y;
  /// The character's current Z-position.
  import attribute int  z;
  readonly int reserved_a[28];   // $AUTOCOMPLETEIGNORE$
  readonly short reserved_f[MAX_INV];  // $AUTOCOMPLETEIGNORE$
  readonly int   reserved_e;   // $AUTOCOMPLETEIGNORE$
  char  reserved_g[40];   // $AUTOCOMPLETEIGNORE$
  readonly char  scrname[20];
  char  on;  // $AUTOCOMPLETEIGNORE$
#else // !STRICT
  int   defview;
  int   talkview;
  int   view;
  int   room;
  int   prevroom;
  int   x;
  int   y;
  int   wait;
  readonly int flags;
  readonly short following;
  readonly short followinfo;
  readonly int   idleview;
  readonly short idletime;
  readonly short idleleft;
  short transparency;
  short baseline;
  int   activeinv;
  int   talkcolor;
  int   thinkview;
  short blinkview;
  short blinkinterval;
  readonly short blinktimer;
  readonly short blinkframe;
  short walkspeed_y;
  short pic_yoffs;
  int   z;
  readonly int walkwait;
  readonly short speech_anim_speed;
  readonly short idle_anim_speed;
  short blocking_width;
  short blocking_height;
  readonly int index_id;
  short pic_xoffs;
  readonly short walkwaitcounter;
  short loop;
  short frame;
  readonly short walking;
  readonly short animating;
  readonly short walkspeed;
  short animspeed;
  short inv[MAX_INV];
  readonly short actx;
  readonly short acty;
  char  name[40];
  readonly char  scrname[20];
  char  on;
#endif // !STRICT
  };

builtin struct Game {
  /// Changes the active translation.
  import static bool   ChangeTranslation(const string newName);
  /// Returns true the first time this command is called with this token.
  import static bool   DoOnceOnly(const string token);
  /// Gets the AGS Colour Number for the specified RGB colour.
  import static int    GetColorFromRGB(int red, int green, int blue);
  /// Gets the number of frames in the specified view loop.
  import static int    GetFrameCountForLoop(int view, int loop);
  /// Gets the name of whatever is on the screen at (x,y)
  import static String GetLocationName(int x, int y);
  /// Gets the number of loops in the specified view.
  import static int    GetLoopCountForView(int view);
  /// Returns the current pattern/track number if the current music is MOD or XM.
  import static int    GetMODPattern();
  /// Gets whether the "Run next loop after this" setting is checked for the specified loop.
  import static bool   GetRunNextSettingForLoop(int view, int loop);
  /// Gets the description of the specified save game slot.
  import static String GetSaveSlotDescription(int saveSlot);
  /// Gets the ViewFrame instance for the specified view frame.
  import static ViewFrame* GetViewFrame(int view, int loop, int frame);
  /// Prompts the user to type in a string, and returns the text that they type in.
  import static String InputBox(const string prompt);
  /// Gets whether any audio (of this type) is currently playing.
  import static bool   IsAudioPlaying(AudioType audioType=SCR_NO_VALUE);
  /// Changes the volume drop applied to this audio type when speech is played
  import static void   SetAudioTypeSpeechVolumeDrop(AudioType, int volumeDrop);
  /// Changes the default volume of audio clips of the specified type.
  import static void   SetAudioTypeVolume(AudioType, int volume, ChangeVolumeType);
  /// Sets the directory where AGS will save and load saved games.
  import static bool   SetSaveGameDirectory(const string directory);
  /// Stops all currently playing audio (optionally of the specified type).
  import static void   StopAudio(AudioType audioType=SCR_NO_VALUE);
#ifndef STRICT_AUDIO
  /// Stops all currently playing sound effects.
  import static void   StopSound(bool includeAmbientSounds=false);   // $AUTOCOMPLETEIGNORE$
#endif // !STRICT_AUDIO
  /// Gets the number of characters in the game.
  readonly import static attribute int CharacterCount;
  /// Gets the number of dialogs in the game.
  readonly import static attribute int DialogCount;
  /// Gets the name of the game EXE file.
  readonly import static attribute String FileName;
  /// Gets the number of fonts in the game.
  readonly import static attribute int FontCount;
  /// Accesses the legacy Global Messages, from AGS 2.x
  readonly import static attribute String GlobalMessages[];
  /// Accesses the global strings collection. This is obsolete.
  import static attribute String GlobalStrings[];
  /// Gets the number of GUIs in the game.
  readonly import static attribute int GUICount;
  /// Gets/sets the time for which user input is ignored after some text is automatically removed
  import static attribute int IgnoreUserInputAfterTextTimeoutMs;
  /// Checks whether the game is currently in the middle of a skippable cutscene.
  readonly import static attribute bool InSkippableCutscene;
  /// Gets the number of inventory items in the game.
  readonly import static attribute int InventoryItemCount;
  /// Gets/sets the minimum time that a piece of speech text stays on screen (in milliseconds)
  import static attribute int MinimumTextDisplayTimeMs;
  /// Gets the number of mouse cursors in the game.
  readonly import static attribute int MouseCursorCount;
  /// Gets/sets the game name.
  import static attribute String Name;
  /// Gets/sets the normal font used for displaying text.
  import static attribute FontType NormalFont;
  /// Checks whether the game is currently skipping over a cutscene.
  readonly import static attribute bool SkippingCutscene;
  /// Gets/sets the font used for displaying speech text.
  import static attribute FontType SpeechFont;
  /// Gets the height of the specified sprite.
  readonly import static attribute int SpriteHeight[];
  /// Gets the width of the specified sprite.
  readonly import static attribute int SpriteWidth[];
  /// Gets/sets how fast speech text is removed from the screen.
  import static attribute int TextReadingSpeed;
  /// Gets name of the currently active translation.
  readonly import static attribute String TranslationFilename;
  /// Gets whether the game is using native co-ordinates.
  readonly import static attribute bool UseNativeCoordinates;
  /// Gets the number of views in the game.
  readonly import static attribute int ViewCount;
#ifdef SCRIPT_API_v340
  /// Returns true if the given plugin is currently loaded.
  import static bool   IsPluginLoaded(const string name);
  /// Gets the number of audio clips in the game.
  readonly import static attribute int AudioClipCount;
  /// Accesses the audio clips collection.
  readonly import static attribute AudioClip *AudioClips[];
#endif // SCRIPT_API_v340
#ifdef SCRIPT_API_v350
  /// Play speech voice-over in non-blocking mode, optionally apply music and sound volume reduction
  import static AudioChannel* PlayVoiceClip(Character*, int cue, bool as_speech = true);
  /// Simulate a keypress on the keyboard.
  import static void   SimulateKeyPress(eKeyCode key);
#endif // SCRIPT_API_v350
#ifdef SCRIPT_API_v3507
  /// Gets the primary camera
  import static readonly attribute Camera *Camera;
  /// Gets the Camera by index.
  import static readonly attribute Camera *Cameras[];
  /// Gets the number of cameras.
  import static readonly attribute int CameraCount;
#endif // SCRIPT_API_v3507
#ifdef SCRIPT_API_v360
  /// Changes the active voice-over pack.
  import static bool   ChangeSpeechVox(const string newName);
  /// Gets the code which describes how was the last blocking state skipped by a user (or autotimer).
  import static readonly attribute int BlockingWaitSkipped;
  /// Gets name of the currently active voice-over pack.
  import static readonly attribute String SpeechVoxFilename;
#endif // SCRIPT_API_v360
#ifdef SCRIPT_API_v361
  /// Resets all of the "DoOnceOnly" token states
  import static void   ResetDoOnceOnly();
  /// Preloads and caches a single sprite.
  import static void   PrecacheSprite(int sprnum);
  /// Preloads and caches sprites and linked sounds for a view, within a selected range of loops.
  import static void   PrecacheView(int view, int first_loop, int last_loop);
#endif // SCRIPT_API_v361
#ifdef SCRIPT_API_v362
  /// Gets the write time of the specified save game slot.
  import static DateTime* GetSaveSlotTime(int saveSlot);
  /// Returns a dynamic array, containing indexes of found save slots in the range between "min_slot" and "max_slot"
  import static int[]  GetSaveSlots(int min_slot, int max_slot, SaveGameSortStyle saveSortStyle = eSaveGameSort_None, SortDirection sortDirection = eSortNoDirection);
  /// Prescans save slots from "min_slot" to "max_slot" and fills the compatible ones into the provided dynamic array.
  import static void   ScanSaveSlots(int valid_slots[], int min_slot, int max_slot, SaveGameSortStyle saveSortStyle = eSaveGameSort_None, SortDirection sortDirection = eSortNoDirection, int user_param = 0);
  /// Gets whether the game is currently in a blocking state, that is during a blocking action or a Wait() call.
  import static readonly attribute bool InBlockingWait;
#endif // SCRIPT_API_v362
};

#ifdef SCRIPT_API_v363
builtin struct GameInfo {
  /// Gets the game's title
  import static readonly attribute String Title;
  /// Gets the game's description
  import static readonly attribute String Description;
  /// Gets the game's developer's name
  import static readonly attribute String DeveloperName;
  /// Gets the game's developer's URL string
  import static readonly attribute String DeveloperURL;
  /// Gets the game's genre description
  import static readonly attribute String Genre;
  /// Gets the game's release date as a DateTime instance
  import static readonly attribute DateTime* ReleaseDate;
  /// Gets the game's release version, represented as "X.Y.Z.W" string
  import static readonly attribute String Version;
};
#endif

builtin struct GameState {
  int  score;
  int  used_mode;
  int  disabled_user_interface;
  int  gscript_timer;
  int  debug_mode;
  int  globalvars[MAX_LEGACY_GLOBAL_VARS];
  int  messagetime;   // for auto-remove messages
  int  usedinv;
#ifdef STRICT
  readonly int  reserved__[4];   // $AUTOCOMPLETEIGNORE$
#else // !STRICT
  int  top_inv_item;
  readonly int  num_inv_displayed;
  readonly int  num_inv_items;
  readonly int  items_per_line;
#endif // !STRICT
  int  text_speed;
  int  sierra_inv_color;
  int  talkanim_speed;  // $AUTOCOMPLETEIGNORE$
  int  inv_item_wid;
  int  inv_item_hit;
  int  text_shadow_color;
  int  swap_portrait;
  int  speech_text_gui;
  int  following_room_timer;
  int  total_score;
  int  skip_display;
  int  no_multiloop_repeat;
  int  roomscript_finished;
  int  inv_activated;
  int  no_textbg_when_voice;
  int  max_dialogoption_width;
  int  no_hicolor_fadein;
  int  bgspeech_game_speed;
  int  bgspeech_stay_on_display;
  int  unfactor_speech_from_textlength;
  int  mp3_loop_before_end;
  int  speech_music_drop;
  readonly int  in_cutscene;
  readonly int  skipping_cutscene;
  readonly int  room_width;
  readonly int  room_height;
  int  game_speed_modifier;  // $AUTOCOMPLETEIGNORE$
  int  score_sound;
  int  previous_game_data;
#ifdef SCRIPT_COMPAT_v341
  int  replay_hotkey;
#else // !SCRIPT_COMPAT_v341
  readonly readonly int unused__041; // $AUTOCOMPLETEIGNORE$
#endif // !SCRIPT_COMPAT_v341
  int  dialog_options_x;
  int  dialog_options_y;
  int  narrator_speech;
  int  ambient_sounds_persist;
  int  lipsync_speed;
#ifdef SCRIPT_COMPAT_v321
  int  close_mouth_end_speech_time;
#else // !SCRIPT_COMPAT_v321
  readonly int  reserved__4;   // $AUTOCOMPLETEIGNORE$
#endif // !SCRIPT_COMPAT_v321
  int  disable_antialiasing;
  int  text_speed_modifier;
  int  text_align;
  int  speech_bubble_width;
  int  min_dialogoption_width;
  int  disable_dialog_parser;
  int  anim_background_speed;
  int  top_bar_backcolor;
  int  top_bar_textcolor;
  int  top_bar_bordercolor;
  int  top_bar_borderwidth;
  int  top_bar_ypos;
  int  screenshot_width;
  int  screenshot_height;
  int  top_bar_font;
#ifdef SCRIPT_COMPAT_v321
  int  speech_text_align;
#else // !SCRIPT_COMPAT_v321
  readonly int  reserved__2;   // $AUTOCOMPLETEIGNORE$
#endif // !SCRIPT_COMPAT_v321
  int  auto_use_walkto_points;
  int  inventory_greys_out;
#ifdef SCRIPT_COMPAT_v321
  int  skip_speech_specific_key;
#else // !SCRIPT_COMPAT_v321
  readonly int  reserved__3;   // $AUTOCOMPLETEIGNORE$
#endif // !SCRIPT_COMPAT_v321
  int  abort_key;
  readonly int fade_color_red;
  readonly int fade_color_green;
  readonly int fade_color_blue;
  int  show_single_dialog_option;
  int  keep_screen_during_instant_transition;
  int  read_dialog_option_color;
  int  stop_dialog_at_end;   // $AUTOCOMPLETEIGNORE$
#ifdef SCRIPT_API_v340
  readonly int  reserved__5;   // $AUTOCOMPLETEIGNORE$
  readonly int  reserved__6;   // $AUTOCOMPLETEIGNORE$
  readonly int  reserved__7;   // $AUTOCOMPLETEIGNORE$
  readonly int  reserved__8;   // $AUTOCOMPLETEIGNORE$
  int  dialog_options_highlight_color;
#endif // SCRIPT_API_v340
  };
  
#ifdef SCRIPT_API_v330
enum SkipSpeechStyle {
#ifdef SCRIPT_API_v360
  eSkipNone         = -1,
#endif // SCRIPT_API_v360
  eSkipKeyMouseTime = 0,
  eSkipKeyTime      = 1,
  eSkipTime         = 2,
  eSkipKeyMouse     = 3,
  eSkipMouseTime    = 4,
  eSkipKey          = 5,
  eSkipMouse        = 6
};
  
builtin struct Speech {
  /// Stop speech animation this number of game loops before speech ends (text mode only).
  import static attribute int             AnimationStopTimeMargin;
  /// Enables/disables the custom speech portrait placement.
  import static attribute bool            CustomPortraitPlacement;
  /// Gets/sets extra time the speech will always stay on screen after its common time runs out.
  import static attribute int             DisplayPostTimeMs;
  /// Gets/sets global speech animation delay (if using global setting).
  import static attribute int             GlobalSpeechAnimationDelay;
  /// Gets/sets speech portrait x offset relative to screen side.
  import static attribute int             PortraitXOffset;
  /// Gets/sets speech portrait y position.
  import static attribute int             PortraitY;
  /// Gets/sets specific key which can skip speech text.
  import static attribute eKeyCode        SkipKey;
  /// Gets/sets how the player can skip speech lines.
  import static attribute SkipSpeechStyle SkipStyle;
  /// Gets/sets the style in which speech is displayed.
  import static attribute eSpeechStyle    Style;
  /// Gets/sets how text in message boxes and Sierra-style speech is aligned.
  import static attribute HorizontalAlignment TextAlignment;
  /// Gets/sets whether speech animation delay should use global setting (or Character setting).
  import static attribute bool            UseGlobalSpeechAnimationDelay;
  /// Gets/sets whether voice and/or text are used in the game.
  import static attribute eVoiceMode      VoiceMode;
#ifdef SCRIPT_API_v360
  /// Gets the overlay representing displayed blocking text, or null if no such text none is displayed at the moment.
  import static readonly attribute Overlay* TextOverlay;
  /// Gets the overlay representing displayed portrait, or null if it is not displayed at the moment.
  import static readonly attribute Overlay* PortraitOverlay;
#endif // SCRIPT_API_v360
#ifdef SCRIPT_API_v362
  /// Gets the currently speaking Character (only works for blocking speech).
  import static readonly attribute Character* SpeakingCharacter;
#endif; // SCRIPT_API_v362
};
#endif // SCRIPT_API_v330

#ifdef SCRIPT_API_v3507
builtin managed struct Camera {
  /// Gets/sets the X position of this camera in the room.
  import attribute int X;
  /// Gets/sets the Y position of this camera in the room.
  import attribute int Y;
  /// Gets/sets the camera's capture width in room coordinates.
  import attribute int Width;
  /// Gets/sets the camera's capture height in room coordinates.
  import attribute int Height;

  /// Gets/sets whether this camera will follow the player character automatically.
  import attribute bool AutoTracking;

  /// Creates a new Camera.
  import static Camera *Create();
  /// Removes an existing camera; note that primary camera will never be removed
  import void Delete();
  /// Changes camera position in the room and disables automatic tracking of the player character.
  import void SetAt(int x, int y);
  /// Changes camera's capture dimensions in room coordinates.
  import void SetSize(int width, int height);
};

builtin managed struct Viewport {
  /// Gets/sets the X position on the screen where this viewport is located.
  import attribute int X;
  /// Gets/sets the Y position on the screen where this viewport is located.
  import attribute int Y;
  /// Gets/sets the viewport's width in screen coordinates.
  import attribute int Width;
  /// Gets/sets the viewport's height in screen coordinates.
  import attribute int Height;
  /// Gets/sets the room camera displayed in this viewport.
  import attribute Camera *Camera;
  /// Gets/sets whether the viewport is drawn on screen.
  import attribute bool Visible;
  /// Gets/sets the Viewport's z-order relative to other viewports.
  import attribute int ZOrder;

  /// Creates a new Viewport.
  import static Viewport *Create();
  /// Returns the viewport at the specified screen location.
  import static Viewport *GetAtScreenXY(int x, int y);
  /// Removes an existing viewport; note that primary viewport will never be removed
  import void Delete();
  /// Changes viewport's position on the screen
  import void SetPosition(int x, int y, int width, int height);
  /// Returns the point in room corresponding to the given screen coordinates if seen through this viewport.
  import Point *ScreenToRoomPoint(int scrx, int scry, bool clipViewport = true);
  /// Returns the point on screen corresponding to the given room coordinates if seen through this viewport.
  import Point *RoomToScreenPoint(int roomx, int roomy, bool clipViewport = true);
};

builtin struct Screen {
  /// Gets the width of the game resolution.
  import static readonly attribute int Width;
  /// Gets the height of the game resolution.
  import static readonly attribute int Height;
  /// Gets/sets whether the viewport should automatically adjust itself and camera to the new room's background size.
  import static attribute bool AutoSizeViewportOnRoomLoad;
  /// Gets the primary room viewport.
  import static readonly attribute Viewport *Viewport;
  /// Gets a Viewport by index.
  import static readonly attribute Viewport *Viewports[];
  /// Gets the number of viewports.
  import static readonly attribute int ViewportCount;

#ifdef SCRIPT_API_v36026
  /// Returns the point in room which is displayed at the given screen coordinates.
  import static Point *ScreenToRoomPoint(int sx, int sy, bool restrictToViewport = false);
#else // !SCRIPT_API_v36026
  /// Returns the point in room which is displayed at the given screen coordinates.
  import static Point *ScreenToRoomPoint(int sx, int sy);
#endif // !SCRIPT_API_v36026
  /// Returns the point on screen corresponding to the given room coordinates relative to the main viewport.
  import static Point *RoomToScreenPoint(int rx, int ry);
};
#endif // SCRIPT_API_v3507

enum SaveComponentSelection
{
    eSaveCmp_None           = 0,
    eSaveCmp_Audio          = 0x00000002,
    eSaveCmp_Dialogs        = 0x00000008,
    eSaveCmp_GUI            = 0x00000010,
    eSaveCmp_Cursors        = 0x00000040,
    eSaveCmp_Views          = 0x00000080,
    eSaveCmp_DynamicSprites = 0x00000100,
    eSaveCmp_Plugins        = 0x00002000
};

#ifdef SCRIPT_API_v362
builtin managed struct RestoredSaveInfo
{
  /// Gets/sets whether this game's save should be cancelled.
  import attribute bool Cancel;
  /// Gets/sets whether this game's save should be reloaded again without particular components.
  import attribute SaveComponentSelection RetryWithoutComponents;

  /// Gets whether this save was only prescanned, and not loaded into the game.
  import readonly attribute bool IsPrescan;
  /// Gets whether this save has extra data that cannot be directly applied to the current game.
  import readonly attribute bool HasExtraData;
  /// Gets whether this save has less data than in the current game.
  import readonly attribute bool HasMissingData;

  /// Gets the save's slot number.
  import readonly attribute int Slot;
  /// Gets the save's description string, which it was written with.
  import readonly attribute String Description;
  /// Gets the version of the engine which wrote this save.
  import readonly attribute String EngineVersion;
  /// Gets the room number this save was made in.
  import readonly attribute int Room;
  /// Gets the number of Audio Types present in this save.
  import readonly attribute int AudioClipTypeCount;
  /// Gets the number of Characters present in this save.
  import readonly attribute int CharacterCount;
  /// Gets the number of Dialogs present in this save.
  import readonly attribute int DialogCount;
  /// Gets the number of GUIs present in this save.
  import readonly attribute int GUICount;
  /// Gets the number of controls on each GUI present in this save.
  import readonly attribute int GUIControlCount[];
  /// Gets the number of Inventory Items present in this save.
  import readonly attribute int InventoryItemCount;
  /// Gets the number of Cursors present in this save.
  import readonly attribute int CursorCount;
  /// Gets the number of Views present in this save.
  import readonly attribute int ViewCount;
  /// Gets the number of loops in each View present in this save.
  import readonly attribute int ViewLoopCount[];
  /// Gets the number of frames in each View present in this save.
  import readonly attribute int ViewFrameCount[];
  /// Gets the total size of the global script data (variables), in bytes, present in this save.
  import readonly attribute int GlobalScriptDataSize;
  /// Gets the number of script modules (except for the global script) present in this save.
  import readonly attribute int ScriptModuleCount;
  /// Gets the name of each of the script module present in this save.
  import readonly attribute String ScriptModuleNames[];
  /// Gets the total size of each of the script module's data (variables), in bytes, present in this save.
  import readonly attribute int ScriptModuleDataSizes[];
};
#endif



import readonly Character *player;
import Mouse mouse;
#ifdef SCRIPT_COMPAT_v350
import System system;
#endif // SCRIPT_COMPAT_v350
import GameState  game;

import Object object[AGS_MAX_OBJECTS];
import Hotspot hotspot[AGS_MAX_HOTSPOTS];
import Region region[AGS_MAX_REGIONS];

import int   gs_globals[MAX_LEGACY_GLOBAL_VARS];
import short savegameindex[MAX_LEGACY_SAVED_GAMES];
import ColorType palette[PALETTE_SIZE];

// Undef temporary macros which are meant to be used only when generating standard declarations
#undef MAX_LEGACY_GLOBAL_VARS
#undef MAX_LEGACY_SAVED_GAMES

#ifndef SCRIPT_API_v330
#undef PALETTE_SIZE
#endif // !SCRIPT_API_v330
#ifndef SCRIPT_API_v350
#undef HorizontalAlignment
#endif // !SCRIPT_API_v350

#undef CursorMode
#undef FontType
#undef AudioType
