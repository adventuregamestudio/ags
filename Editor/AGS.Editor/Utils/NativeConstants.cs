using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public class NativeConstants
    {
        // NOTE: GetNativeConstant returns only int or string, so some additional casting may be required
        public static readonly string GAME_FILE_SIG = (string)Factory.NativeProxy.GetNativeConstant("GAME_FILE_SIG");
        public static readonly int GAME_DATA_VERSION_CURRENT = (int)Factory.NativeProxy.GetNativeConstant("GAME_DATA_VERSION_CURRENT");
        public static readonly int MAX_GUID_LENGTH = (int)Factory.NativeProxy.GetNativeConstant("MAX_GUID_LENGTH");
        public static readonly int MAX_SG_EXT_LENGTH = (int)Factory.NativeProxy.GetNativeConstant("MAX_SG_EXT_LENGTH");
        public static readonly int MAX_SG_FOLDER_LEN = (int)Factory.NativeProxy.GetNativeConstant("MAX_SG_FOLDER_LEN");
        public static readonly int MAX_SCRIPT_NAME_LEN = (int)Factory.NativeProxy.GetNativeConstant("MAX_SCRIPT_NAME_LEN");
        public static readonly int FFLG_SIZEMASK = (int)Factory.NativeProxy.GetNativeConstant("FFLG_SIZEMASK");
        public static readonly char IFLG_STARTWITH = (char)(int)Factory.NativeProxy.GetNativeConstant("IFLG_STARTWITH");
        public static readonly char MCF_ANIMMOVE = (char)(int)Factory.NativeProxy.GetNativeConstant("MCF_ANIMMOVE");
        public static readonly char MCF_STANDARD = (char)(int)Factory.NativeProxy.GetNativeConstant("MCF_STANDARD");
        public static readonly char MCF_HOTSPOT = (char)(int)Factory.NativeProxy.GetNativeConstant("MCF_HOTSPOT");
        public static readonly int CHF_MANUALSCALING = (int)Factory.NativeProxy.GetNativeConstant("CHF_MANUALSCALING");
        public static readonly int CHF_NOINTERACT = (int)Factory.NativeProxy.GetNativeConstant("CHF_NOINTERACT");
        public static readonly int CHF_NODIAGONAL = (int)Factory.NativeProxy.GetNativeConstant("CHF_NODIAGONAL");
        public static readonly int CHF_NOLIGHTING = (int)Factory.NativeProxy.GetNativeConstant("CHF_NOLIGHTING");
        public static readonly int CHF_NOTURNING = (int)Factory.NativeProxy.GetNativeConstant("CHF_NOTURNING");
        public static readonly int CHF_NOBLOCKING = (int)Factory.NativeProxy.GetNativeConstant("CHF_NOBLOCKING");
        public static readonly int CHF_SCALEMOVESPEED = (int)Factory.NativeProxy.GetNativeConstant("CHF_SCALEMOVESPEED");
        public static readonly int CHF_SCALEVOLUME = (int)Factory.NativeProxy.GetNativeConstant("CHF_SCALEVOLUME");
        public static readonly int CHF_ANTIGLIDE = (int)Factory.NativeProxy.GetNativeConstant("CHF_ANTIGLIDE");
        public static readonly int DFLG_ON = (int)Factory.NativeProxy.GetNativeConstant("DFLG_ON");
        public static readonly int DFLG_NOREPEAT = (int)Factory.NativeProxy.GetNativeConstant("DFLG_NOREPEAT");
        public static readonly int DTFLG_SHOWPARSER = (int)Factory.NativeProxy.GetNativeConstant("DTFLG_SHOWPARSER");
        public static readonly sbyte FONT_OUTLINE_AUTO = (sbyte)(int)Factory.NativeProxy.GetNativeConstant("FONT_OUTLINE_AUTO");
        public static readonly int MAX_SPRITES = (int)Factory.NativeProxy.GetNativeConstant("MAX_SPRITES");
        public static readonly int MAX_CURSOR = (int)Factory.NativeProxy.GetNativeConstant("MAX_CURSOR");
        public static readonly int MAX_PARSER_WORD_LENGTH = (int)Factory.NativeProxy.GetNativeConstant("MAX_PARSER_WORD_LENGTH");
        public static readonly int MAX_INV = (int)Factory.NativeProxy.GetNativeConstant("MAX_INV");
        public static readonly int MAXLIPSYNCFRAMES = (int)Factory.NativeProxy.GetNativeConstant("MAXLIPSYNCFRAMES");
        public static readonly int MAXGLOBALMES = (int)Factory.NativeProxy.GetNativeConstant("MAXGLOBALMES");
        public static readonly int MAXTOPICOPTIONS = (int)Factory.NativeProxy.GetNativeConstant("MAXTOPICOPTIONS");
        public static readonly short UNIFORM_WALK_SPEED = (short)(int)Factory.NativeProxy.GetNativeConstant("UNIFORM_WALK_SPEED");
        public static readonly int GAME_RESOLUTION_CUSTOM = (int)Factory.NativeProxy.GetNativeConstant("GAME_RESOLUTION_CUSTOM");
        public static readonly int CHUNKSIZE = (int)Factory.NativeProxy.GetNativeConstant("CHUNKSIZE");
        public static readonly string SPRSET_NAME = (string)Factory.NativeProxy.GetNativeConstant("SPRSET_NAME");
        public static readonly byte SPF_640x400 = (byte)(int)Factory.NativeProxy.GetNativeConstant("SPF_640x400");
        public static readonly byte SPF_ALPHACHANNEL = (byte)(int)Factory.NativeProxy.GetNativeConstant("SPF_ALPHACHANNEL");
        public static readonly byte[] PASSWORD_ENC_STRING = (byte[])Factory.NativeProxy.GetNativeConstant("PASSWORD_ENC_STRING");
        public static readonly int LOOPFLAG_RUNNEXTLOOP = (int)Factory.NativeProxy.GetNativeConstant("LOOPFLAG_RUNNEXTLOOP");
        public static readonly int VFLG_FLIPSPRITE = (int)Factory.NativeProxy.GetNativeConstant("VFLG_FLIPSPRITE");
        public static readonly uint GUIMAGIC = (uint)Factory.NativeProxy.GetNativeConstant("GUIMAGIC");
        public static readonly int SAVEBUFFERSIZE = (int)Factory.NativeProxy.GetNativeConstant("SAVEBUFFERSIZE");
        public static readonly int GUIMAIN_NOCLICK = (int)Factory.NativeProxy.GetNativeConstant("GUIMAIN_NOCLICK");
        public static readonly int GUIF_CLIP = (int)Factory.NativeProxy.GetNativeConstant("GUIF_CLIP");
        public static readonly int GUIF_TRANSLATED = (int)Factory.NativeProxy.GetNativeConstant("GUIF_TRANSLATED");
        public static readonly int GLF_NOBORDER = (int)Factory.NativeProxy.GetNativeConstant("GLF_NOBORDER");
        public static readonly int GLF_NOARROWS = (int)Factory.NativeProxy.GetNativeConstant("GLF_NOARROWS");
        public static readonly int GUI_POPUP_MODAL = (int)Factory.NativeProxy.GetNativeConstant("GUI_POPUP_MODAL");
        public static readonly int GUIMAIN_TEXTWINDOW = (int)Factory.NativeProxy.GetNativeConstant("GUIMAIN_TEXTWINDOW");
        public static readonly byte GUIMAIN_LEGACYTEXTWINDOW = (byte)(int)Factory.NativeProxy.GetNativeConstant("GUIMAIN_LEGACYTEXTWINDOW");
        public static readonly int GTF_NOBORDER = (int)Factory.NativeProxy.GetNativeConstant("GTF_NOBORDER");
        public static readonly int GOBJ_BUTTON = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_BUTTON");
        public static readonly int GOBJ_LABEL = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_LABEL");
        public static readonly int GOBJ_INVENTORY = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_INVENTORY");
        public static readonly int GOBJ_SLIDER = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_SLIDER");
        public static readonly int GOBJ_TEXTBOX = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_TEXTBOX");
        public static readonly int GOBJ_LISTBOX = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_LISTBOX");
        public static readonly int TEXTWINDOW_PADDING_DEFAULT = (int)Factory.NativeProxy.GetNativeConstant("TEXTWINDOW_PADDING_DEFAULT");
        public static readonly int MAX_PLUGINS = (int)Factory.NativeProxy.GetNativeConstant("MAX_PLUGINS");

        public class GUIVersion
        {
            public static readonly int Current = (int)Factory.NativeProxy.GetNativeConstant("GUI_VERSION_CURRENT");
        }

        public class CustomPropertyVersion
        {
            public static readonly int Current = (int)Factory.NativeProxy.GetNativeConstant("CUSTOM_PROPERTY_SCHEMA_VERSION");
        }

        public class GameOptions
        {
            public static readonly int OPT_DEBUGMODE = (int)Factory.NativeProxy.GetNativeConstant("OPT_DEBUGMODE");
            public static readonly int OPT_WALKONLOOK = (int)Factory.NativeProxy.GetNativeConstant("OPT_WALKONLOOK");
            public static readonly int OPT_DIALOGIFACE = (int)Factory.NativeProxy.GetNativeConstant("OPT_DIALOGIFACE");
            public static readonly int OPT_ANTIGLIDE = (int)Factory.NativeProxy.GetNativeConstant("OPT_ANTIGLIDE");
            public static readonly int OPT_TWCUSTOM = (int)Factory.NativeProxy.GetNativeConstant("OPT_TWCUSTOM");
            public static readonly int OPT_DIALOGGAP = (int)Factory.NativeProxy.GetNativeConstant("OPT_DIALOGGAP");
            public static readonly int OPT_NOSKIPTEXT = (int)Factory.NativeProxy.GetNativeConstant("OPT_NOSKIPTEXT");
            public static readonly int OPT_DISABLEOFF = (int)Factory.NativeProxy.GetNativeConstant("OPT_DISABLEOFF");
            public static readonly int OPT_ALWAYSSPCH = (int)Factory.NativeProxy.GetNativeConstant("OPT_ALWAYSSPCH");
            public static readonly int OPT_SPEECHTYPE = (int)Factory.NativeProxy.GetNativeConstant("OPT_SPEECHTYPE");
            public static readonly int OPT_PIXPERFECT = (int)Factory.NativeProxy.GetNativeConstant("OPT_PIXPERFECT");
            public static readonly int OPT_NOWALKMODE = (int)Factory.NativeProxy.GetNativeConstant("OPT_NOWALKMODE");
            public static readonly int OPT_LETTERBOX = (int)Factory.NativeProxy.GetNativeConstant("OPT_LETTERBOX");
            public static readonly int OPT_FIXEDINVCURSOR = (int)Factory.NativeProxy.GetNativeConstant("OPT_FIXEDINVCURSOR");
            public static readonly int OPT_NOSCALEFNT = (int)Factory.NativeProxy.GetNativeConstant("OPT_NOSCALEFNT");
            public static readonly int OPT_SPLITRESOURCES = (int)Factory.NativeProxy.GetNativeConstant("OPT_SPLITRESOURCES");
            public static readonly int OPT_ROTATECHARS = (int)Factory.NativeProxy.GetNativeConstant("OPT_ROTATECHARS");
            public static readonly int OPT_FADETYPE = (int)Factory.NativeProxy.GetNativeConstant("OPT_FADETYPE");
            public static readonly int OPT_HANDLEINVCLICKS = (int)Factory.NativeProxy.GetNativeConstant("OPT_HANDLEINVCLICKS");
            public static readonly int OPT_MOUSEWHEEL = (int)Factory.NativeProxy.GetNativeConstant("OPT_MOUSEWHEEL");
            public static readonly int OPT_DIALOGNUMBERED = (int)Factory.NativeProxy.GetNativeConstant("OPT_DIALOGNUMBERED");
            public static readonly int OPT_DIALOGUPWARDS = (int)Factory.NativeProxy.GetNativeConstant("OPT_DIALOGUPWARDS");
            public static readonly int OPT_ANTIALIASFONTS = (int)Factory.NativeProxy.GetNativeConstant("OPT_ANTIALIASFONTS");
            public static readonly int OPT_THOUGHTGUI = (int)Factory.NativeProxy.GetNativeConstant("OPT_THOUGHTGUI");
            public static readonly int OPT_TURNTOFACELOC = (int)Factory.NativeProxy.GetNativeConstant("OPT_TURNTOFACELOC");
            public static readonly int OPT_RIGHTLEFTWRITE = (int)Factory.NativeProxy.GetNativeConstant("OPT_RIGHTLEFTWRITE");
            public static readonly int OPT_DUPLICATEINV = (int)Factory.NativeProxy.GetNativeConstant("OPT_DUPLICATEINV");
            public static readonly int OPT_SAVESCREENSHOT = (int)Factory.NativeProxy.GetNativeConstant("OPT_SAVESCREENSHOT");
            public static readonly int OPT_PORTRAITSIDE = (int)Factory.NativeProxy.GetNativeConstant("OPT_PORTRAITSIDE");
            public static readonly int OPT_STRICTSCRIPTING = (int)Factory.NativeProxy.GetNativeConstant("OPT_STRICTSCRIPTING");
            public static readonly int OPT_LEFTTORIGHTEVAL = (int)Factory.NativeProxy.GetNativeConstant("OPT_LEFTTORIGHTEVAL");
            public static readonly int OPT_COMPRESSSPRITES = (int)Factory.NativeProxy.GetNativeConstant("OPT_COMPRESSSPRITES");
            public static readonly int OPT_STRICTSTRINGS = (int)Factory.NativeProxy.GetNativeConstant("OPT_STRICTSTRINGS");
            public static readonly int OPT_NEWGUIALPHA = (int)Factory.NativeProxy.GetNativeConstant("OPT_NEWGUIALPHA");
            public static readonly int OPT_RUNGAMEDLGOPTS = (int)Factory.NativeProxy.GetNativeConstant("OPT_RUNGAMEDLGOPTS");
            public static readonly int OPT_NATIVECOORDINATES = (int)Factory.NativeProxy.GetNativeConstant("OPT_NATIVECOORDINATES");
            public static readonly int OPT_GLOBALTALKANIMSPD = (int)Factory.NativeProxy.GetNativeConstant("OPT_GLOBALTALKANIMSPD");
            public static readonly int OPT_SPRITEALPHA = (int)Factory.NativeProxy.GetNativeConstant("OPT_SPRITEALPHA");
            public static readonly int OPT_SAFEFILEPATHS = (int)Factory.NativeProxy.GetNativeConstant("OPT_SAFEFILEPATHS");
            public static readonly int OPT_DIALOGOPTIONSAPI = (int)Factory.NativeProxy.GetNativeConstant("OPT_DIALOGOPTIONSAPI");
            public static readonly int OPT_BASESCRIPTAPI = (int)Factory.NativeProxy.GetNativeConstant("OPT_BASESCRIPTAPI");
            public static readonly int OPT_SCRIPTCOMPATLEV = (int)Factory.NativeProxy.GetNativeConstant("OPT_SCRIPTCOMPATLEV");
            public static readonly int OPT_RENDERATSCREENRES = (int)Factory.NativeProxy.GetNativeConstant("OPT_RENDERATSCREENRES");
            public static readonly int OPT_LIPSYNCTEXT = (int)Factory.NativeProxy.GetNativeConstant("OPT_LIPSYNCTEXT");
        }
    }
}
