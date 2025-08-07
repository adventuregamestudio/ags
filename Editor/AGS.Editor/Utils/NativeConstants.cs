﻿using System;
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
        public static readonly int FFLG_SIZEMULTIPLIER = (int)Factory.NativeProxy.GetNativeConstant("FFLG_SIZEMULTIPLIER");
        public static readonly char IFLG_STARTWITH = (char)(int)Factory.NativeProxy.GetNativeConstant("IFLG_STARTWITH");
        public static readonly char MCF_ANIMMOVE = (char)(int)Factory.NativeProxy.GetNativeConstant("MCF_ANIMMOVE");
        public static readonly char MCF_STANDARD = (char)(int)Factory.NativeProxy.GetNativeConstant("MCF_STANDARD");
        public static readonly char MCF_HOTSPOT = (char)(int)Factory.NativeProxy.GetNativeConstant("MCF_HOTSPOT");
        public static readonly int CHF_MANUALSCALING = (int)Factory.NativeProxy.GetNativeConstant("CHF_MANUALSCALING");
        public static readonly int CHF_NOINTERACT = (int)Factory.NativeProxy.GetNativeConstant("CHF_NOINTERACT");
        public static readonly int CHF_NODIAGONAL = (int)Factory.NativeProxy.GetNativeConstant("CHF_NODIAGONAL");
        public static readonly int CHF_NOLIGHTING = (int)Factory.NativeProxy.GetNativeConstant("CHF_NOLIGHTING");
        public static readonly int CHF_NOTURNWHENWALK = (int)Factory.NativeProxy.GetNativeConstant("CHF_NOTURNWHENWALK");
        public static readonly int CHF_NOBLOCKING = (int)Factory.NativeProxy.GetNativeConstant("CHF_NOBLOCKING");
        public static readonly int CHF_SCALEMOVESPEED = (int)Factory.NativeProxy.GetNativeConstant("CHF_SCALEMOVESPEED");
        public static readonly int CHF_SCALEVOLUME = (int)Factory.NativeProxy.GetNativeConstant("CHF_SCALEVOLUME");
        public static readonly int CHF_ANTIGLIDE = (int)Factory.NativeProxy.GetNativeConstant("CHF_ANTIGLIDE");
        public static readonly int CHF_TURNWHENFACE = (int)Factory.NativeProxy.GetNativeConstant("CHF_TURNWHENFACE");
        public static readonly int CHF_ENABLED = (int)Factory.NativeProxy.GetNativeConstant("CHF_ENABLED");
        public static readonly int CHF_VISIBLE = (int)Factory.NativeProxy.GetNativeConstant("CHF_VISIBLE");
        public static readonly int DFLG_ON = (int)Factory.NativeProxy.GetNativeConstant("DFLG_ON");
        public static readonly int DFLG_NOREPEAT = (int)Factory.NativeProxy.GetNativeConstant("DFLG_NOREPEAT");
        public static readonly int DTFLG_SHOWPARSER = (int)Factory.NativeProxy.GetNativeConstant("DTFLG_SHOWPARSER");
        public static readonly sbyte FONT_OUTLINE_AUTO = (sbyte)(int)Factory.NativeProxy.GetNativeConstant("FONT_OUTLINE_AUTO");
        public static readonly int MAX_STATIC_SPRITES = (int)Factory.NativeProxy.GetNativeConstant("MAX_STATIC_SPRITES");
        public static readonly int MAX_PARSER_WORD_LENGTH = (int)Factory.NativeProxy.GetNativeConstant("MAX_PARSER_WORD_LENGTH");
        public static readonly int MAX_INV = (int)Factory.NativeProxy.GetNativeConstant("MAX_INV");
        public static readonly int MAXLIPSYNCFRAMES = (int)Factory.NativeProxy.GetNativeConstant("MAXLIPSYNCFRAMES");
        public static readonly int MAXTOPICOPTIONS = (int)Factory.NativeProxy.GetNativeConstant("MAXTOPICOPTIONS");
        public static readonly short UNIFORM_WALK_SPEED = (short)(int)Factory.NativeProxy.GetNativeConstant("UNIFORM_WALK_SPEED");
        public static readonly int GAME_RESOLUTION_CUSTOM = (int)Factory.NativeProxy.GetNativeConstant("GAME_RESOLUTION_CUSTOM");
        public static readonly string SPRSET_NAME = (string)Factory.NativeProxy.GetNativeConstant("SPRSET_NAME");
        public static readonly byte SPF_KEEPDEPTH = (byte)(int)Factory.NativeProxy.GetNativeConstant("SPF_KEEPDEPTH");
        public static readonly byte[] PASSWORD_ENC_STRING = (byte[])Factory.NativeProxy.GetNativeConstant("PASSWORD_ENC_STRING");
        public static readonly int LOOPFLAG_RUNNEXTLOOP = (int)Factory.NativeProxy.GetNativeConstant("LOOPFLAG_RUNNEXTLOOP");
        public static readonly uint GUIMAGIC = (uint)Factory.NativeProxy.GetNativeConstant("GUIMAGIC");
        public static readonly int SAVEBUFFERSIZE = (int)Factory.NativeProxy.GetNativeConstant("SAVEBUFFERSIZE");
        public static readonly int GUIMAIN_CLICKABLE = (int)Factory.NativeProxy.GetNativeConstant("GUIMAIN_CLICKABLE");
        public static readonly int GUIMAIN_VISIBLE = (int)Factory.NativeProxy.GetNativeConstant("GUIMAIN_VISIBLE");
        public static readonly int GUIF_ENABLED = (int)Factory.NativeProxy.GetNativeConstant("GUIF_ENABLED");
        public static readonly int GUIF_CLICKABLE = (int)Factory.NativeProxy.GetNativeConstant("GUIF_CLICKABLE");
        public static readonly int GUIF_VISIBLE = (int)Factory.NativeProxy.GetNativeConstant("GUIF_VISIBLE");
        public static readonly int GUIF_CLIP = (int)Factory.NativeProxy.GetNativeConstant("GUIF_CLIP");
        public static readonly int GUIF_TRANSLATED = (int)Factory.NativeProxy.GetNativeConstant("GUIF_TRANSLATED");
        public static readonly int GUIF_WRAPTEXT = (int)Factory.NativeProxy.GetNativeConstant("GUIF_WRAPTEXT");
        public static readonly int GLF_SHOWBORDER = (int)Factory.NativeProxy.GetNativeConstant("GLF_SHOWBORDER");
        public static readonly int GLF_SHOWARROWS = (int)Factory.NativeProxy.GetNativeConstant("GLF_SHOWARROWS");
        public static readonly int GUI_POPUP_MODAL = (int)Factory.NativeProxy.GetNativeConstant("GUI_POPUP_MODAL");
        public static readonly int GUIMAIN_TEXTWINDOW = (int)Factory.NativeProxy.GetNativeConstant("GUIMAIN_TEXTWINDOW");
        public static readonly int GTF_SHOWBORDER = (int)Factory.NativeProxy.GetNativeConstant("GTF_SHOWBORDER");
        public static readonly int GOBJ_BUTTON = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_BUTTON");
        public static readonly int GOBJ_LABEL = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_LABEL");
        public static readonly int GOBJ_INVENTORY = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_INVENTORY");
        public static readonly int GOBJ_SLIDER = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_SLIDER");
        public static readonly int GOBJ_TEXTBOX = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_TEXTBOX");
        public static readonly int GOBJ_LISTBOX = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_LISTBOX");
        public static readonly int TEXTWINDOW_PADDING_DEFAULT = (int)Factory.NativeProxy.GetNativeConstant("TEXTWINDOW_PADDING_DEFAULT");

        public static readonly int FFLG_REPORTNOMINALHEIGHT = (int)Factory.NativeProxy.GetNativeConstant("FFLG_REPORTNOMINALHEIGHT");

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
            public static readonly int OPT_FIXEDINVCURSOR = (int)Factory.NativeProxy.GetNativeConstant("OPT_FIXEDINVCURSOR");
            public static readonly int OPT_SPLITRESOURCES = (int)Factory.NativeProxy.GetNativeConstant("OPT_SPLITRESOURCES");
            public static readonly int OPT_CHARTURNWHENWALK = (int)Factory.NativeProxy.GetNativeConstant("OPT_CHARTURNWHENWALK");
            public static readonly int OPT_FADETYPE = (int)Factory.NativeProxy.GetNativeConstant("OPT_FADETYPE");
            public static readonly int OPT_HANDLEINVCLICKS = (int)Factory.NativeProxy.GetNativeConstant("OPT_HANDLEINVCLICKS");
            public static readonly int OPT_DIALOGNUMBERED = (int)Factory.NativeProxy.GetNativeConstant("OPT_DIALOGNUMBERED");
            public static readonly int OPT_DIALOGUPWARDS = (int)Factory.NativeProxy.GetNativeConstant("OPT_DIALOGUPWARDS");
            public static readonly int OPT_ANTIALIASFONTS = (int)Factory.NativeProxy.GetNativeConstant("OPT_ANTIALIASFONTS");
            public static readonly int OPT_THOUGHTGUI = (int)Factory.NativeProxy.GetNativeConstant("OPT_THOUGHTGUI");
            public static readonly int OPT_CHARTURNWHENFACE = (int)Factory.NativeProxy.GetNativeConstant("OPT_CHARTURNWHENFACE");
            public static readonly int OPT_RIGHTLEFTWRITE = (int)Factory.NativeProxy.GetNativeConstant("OPT_RIGHTLEFTWRITE");
            public static readonly int OPT_DUPLICATEINV = (int)Factory.NativeProxy.GetNativeConstant("OPT_DUPLICATEINV");
            public static readonly int OPT_SAVESCREENSHOT = (int)Factory.NativeProxy.GetNativeConstant("OPT_SAVESCREENSHOT");
            public static readonly int OPT_SAVESCREENSHOTLAYER = (int)Factory.NativeProxy.GetNativeConstant("OPT_SAVESCREENSHOTLAYER");
            public static readonly int OPT_PORTRAITSIDE = (int)Factory.NativeProxy.GetNativeConstant("OPT_PORTRAITSIDE");
            public static readonly int OPT_COMPRESSSPRITES = (int)Factory.NativeProxy.GetNativeConstant("OPT_COMPRESSSPRITES");
            public static readonly int OPT_RUNGAMEDLGOPTS = (int)Factory.NativeProxy.GetNativeConstant("OPT_RUNGAMEDLGOPTS");
            public static readonly int OPT_GLOBALTALKANIMSPD = (int)Factory.NativeProxy.GetNativeConstant("OPT_GLOBALTALKANIMSPD");
            public static readonly int OPT_DIALOGOPTIONSAPI = (int)Factory.NativeProxy.GetNativeConstant("OPT_DIALOGOPTIONSAPI");
            public static readonly int OPT_BASESCRIPTAPI = (int)Factory.NativeProxy.GetNativeConstant("OPT_BASESCRIPTAPI");
            public static readonly int OPT_SCRIPTCOMPATLEV = (int)Factory.NativeProxy.GetNativeConstant("OPT_SCRIPTCOMPATLEV");
            public static readonly int OPT_RENDERATSCREENRES = (int)Factory.NativeProxy.GetNativeConstant("OPT_RENDERATSCREENRES");
            public static readonly int OPT_CLIPGUICONTROLS = (int)Factory.NativeProxy.GetNativeConstant("OPT_CLIPGUICONTROLS");
            public static readonly int OPT_GAMETEXTENCODING = (int)Factory.NativeProxy.GetNativeConstant("OPT_GAMETEXTENCODING");
            public static readonly int OPT_KEYHANDLEAPI = (int)Factory.NativeProxy.GetNativeConstant("OPT_KEYHANDLEAPI");
            public static readonly int OPT_SCALECHAROFFSETS = (int)Factory.NativeProxy.GetNativeConstant("OPT_SCALECHAROFFSETS");
            public static readonly int OPT_VOICECLIPNAMERULE = (int)Factory.NativeProxy.GetNativeConstant("OPT_VOICECLIPNAMERULE");
            public static readonly int OPT_GAMEFPS = (int)Factory.NativeProxy.GetNativeConstant("OPT_GAMEFPS");
            public static readonly int OPT_GUICONTROLMOUSEBUT = (int)Factory.NativeProxy.GetNativeConstant("OPT_GUICONTROLMOUSEBUT");
            public static readonly int OPT_LIPSYNCTEXT = (int)Factory.NativeProxy.GetNativeConstant("OPT_LIPSYNCTEXT");
        }
    }
}
