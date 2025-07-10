//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_AC__GAMESTRUCTDEFINES_H
#define __AGS_CN_AC__GAMESTRUCTDEFINES_H

#include "core/types.h"
#include "util/geometry.h"
#include "util/string.h"

// CLNUP check what's actually needed
// Palette sections
#define PAL_GAMEWIDE        0
#define PAL_LOCKED          1
#define PAL_BACKGROUND      2

// Game options

// General game options
#define OPT_DEBUGMODE       0
#define OPT_OBSOLETE_SCORESOUND     1 // [DEPRECATED]
#define OPT_WALKONLOOK      2
#define OPT_DIALOGIFACE     3
#define OPT_ANTIGLIDE       4
#define OPT_TWCUSTOM        5
#define OPT_DIALOGGAP       6
#define OPT_NOSKIPTEXT      7
#define OPT_DISABLEOFF      8
#define OPT_ALWAYSSPCH      9
#define OPT_SPEECHTYPE      10
#define OPT_PIXPERFECT      11
#define OPT_NOWALKMODE      12
#define OPT_OBSOLETE_LETTERBOX      13 // [DEPRECATED] old-style resolution mode
#define OPT_FIXEDINVCURSOR  14
#define OPT_NOLOSEINV       15
#define OPT_OBSOLETE_HIRES_FONTS    16
#define OPT_SPLITRESOURCES  17
#define OPT_CHARTURNWHENWALK 18 // characters turn step-by-step when changing walking direction
#define OPT_FADETYPE        19
#define OPT_HANDLEINVCLICKS 20
#define OPT_OBSOLETE_MOUSEWHEEL     21
#define OPT_DIALOGNUMBERED  22
#define OPT_DIALOGUPWARDS   23
#define OPT_OBSOLETE_CROSSFADEMUSIC 24 // [DEPRECATED] old-style audio crossfade
#define OPT_ANTIALIASFONTS  25
#define OPT_THOUGHTGUI      26
#define OPT_CHARTURNWHENFACE 27  // characters turn step-by-step when facing new standing direction
#define OPT_RIGHTLEFTWRITE  28  // right-to-left text writing
#define OPT_DUPLICATEINV    29  // if they have 2 of the item, draw it twice
#define OPT_SAVESCREENSHOT  30  // write screenshots into savegame (on/off)
#define OPT_PORTRAITSIDE    31
#define OPT_OBSOLETE_STRICTSCRIPTING   32  // don't allow MoveCharacter-style commands
#define OPT_OBSOLETE_LEFTTORIGHTEVAL   33  // [DEPRECATED] left-to-right operator evaluation
#define OPT_COMPRESSSPRITES 34  // sprite compression type (None, RLE, LZW, Deflate)
#define OPT_OBSOLETE_STRICTSTRINGS     35  // don't allow old-style strings, for reference only
#define OPT_OBSOLETE_NEWGUIALPHA       36 fixme
#define OPT_NEWGUIALPHA     36  // alpha blending method when drawing GUI and controls
#define OPT_RUNGAMEDLGOPTS  37
#define OPT_OBSOLETE_NATIVECOORDINATES 38 // [DEPRECATED] defines coordinate relation between game logic and game screen
#define OPT_GLOBALTALKANIMSPD  39
#define OPT_HIGHESTOPTION_321  39
#define OPT_OBSOLETE_SPRITEALPHA   40 // [DEPRECATED] alpha blending method when drawing images on DrawingSurface
#define OPT_OBSOLETE_SAFEFILEPATHS 41 // [DEPRECATED] restricted file path in script (not writing to the game dir, etc)
#define OPT_DIALOGOPTIONSAPI   42 // version of dialog options API (-1 for pre-3.4.0 API)
#define OPT_BASESCRIPTAPI      43 // version of the Script API (ScriptAPIVersion) used to compile game script
#define OPT_SCRIPTCOMPATLEV    44 // level of API compatibility (ScriptAPIVersion) used to compile game script
#define OPT_RENDERATSCREENRES  45 // scale sprites at the (final) screen resolution
#define OPT_OBSOLETE_RELATIVEASSETRES  46 // relative asset resolution mode (where sprites are resized to match game type)
#define OPT_OBSOLETE_WALKSPEEDABSOLUTE 47 // if movement speeds are independent of walkable mask resolution
#define OPT_CLIPGUICONTROLS    48 // clip drawn gui control contents to the control's rectangle
#define OPT_GAMETEXTENCODING   49 // how the text in the game data should be interpreted
#define OPT_KEYHANDLEAPI       50 // key handling mode (old/new)
#define OPT_CUSTOMENGINETAG    51 // custom engine tag (for overriding behavior)
#define OPT_SCALECHAROFFSETS   52 // apply character scaling to the sprite offsets (z, locked offs)
#define OPT_SAVESCREENSHOTLAYER 53 // which render layers to include into savegame screenshot
#define OPT_VOICECLIPNAMERULE  54 // which rule to use for a voice clip name based on character's name (old/new)
#define OPT_SAVECOMPONENTSIGNORE 55 // ignore these savegame components (flag mask)
#define OPT_GAMEFPS            56
#define OPT_GUICONTROLMOUSEBUT 57 // whether common gui controls should react only to LMB (0 - any, 1 - LMB)
#define OPT_HIGHESTOPTION      OPT_GUICONTROLMOUSEBUT
#define OPT_LIPSYNCTEXT        99

// Compatibility engine modes (hacks)
#define CUSTOMENG_NONE      0
#define CUSTOMENG_DRACONIAN 1 // Draconian Edition
#define CUSTOMENG_CLIFFTOP  2 // Clifftop Games [UNSUPPORTED in AGS 4]

// Sierra-style portrait position style
#define PORTRAIT_LEFT       0
#define PORTRAIT_RIGHT      1
#define PORTRAIT_ALTERNATE  2
#define PORTRAIT_XPOSITION  3

// Room transition style
enum ScreenTransitionStyle
{
    kScrTran_Fade = 0,
    kScrTran_Instant = 1,
    kScrTran_Dissolve = 2,
    kScrTran_Boxout = 3,
    kScrTran_Crossfade = 4,
    kNumScrTransitions
};

// Style of the character speech:
// defines how it's displayed and animated
enum SpeechStyle
{
    kSpeechStyle_LucasArts = 0,
    kSpeechStyle_SierraTransparent = 1,
    kSpeechStyle_SierraBackground = 2,
    kSpeechStyle_QFG4 = 4, // fullscreen aka Quest-for-Glory-4 style

    kSpeechStyle_First = kSpeechStyle_LucasArts,
    kSpeechStyle_Last = kSpeechStyle_QFG4,
};

// Contemporary font flags
#define FFLG_SIZEMULTIPLIER        0x01  // size data means multiplier
#define FFLG_DEFLINESPACING        0x02  // linespacing derived from the font height
// Font load flags, primarily for backward compatibility:
// REPORTNOMINALHEIGHT: get_font_height should return nominal font's height,
// eq to "font size" parameter, otherwise returns real pixel height.
#define FFLG_REPORTNOMINALHEIGHT   0x04
//#define FFLG_ASCENDERFIXUP       0x08 // [DEPRECATED]
// Collection of flags defining font's load mode
#define FFLG_LOADMODEMASK         (FFLG_REPORTNOMINALHEIGHT)
// Font outline types
#define FONT_OUTLINE_NONE -1
#define FONT_OUTLINE_AUTO -10

#define DIALOG_OPTIONS_HIGHLIGHT_COLOR_DEFAULT  14 // Yellow

// MAXVIEWNAMELENGTH comes from unknown old engine version
#define LEGACY_MAXVIEWNAMELENGTH 15
#define MAXLIPSYNCFRAMES  20
#define MAX_GUID_LENGTH   40
#define MAX_SG_EXT_LENGTH 20
#define LEGACY_MAX_SG_FOLDER_LEN 50

enum GameResolutionType
{
    kGameResolution_Undefined   = -1,
    // Do not support any other since AGS 4
    kGameResolution_Custom      = 8,
};

// Automatic numbering of dialog options (OPT_DIALOGNUMBERED)
enum DialogOptionNumbering
{
    kDlgOptNoNumbering = -1,
    kDlgOptKeysOnly    =  0, // implicit key shortcuts
    kDlgOptNumbering   =  1  // draw option indices and use key shortcuts
};

// Version of the script api (OPT_BASESCRIPTAPI and OPT_SCRIPTCOMPATLEV).
// If the existing script function meaning had changed, that may be
// possible to find out which implementation to use by checking one of those
// two options.
// NOTE: please remember that those values are valid only for games made with
// 3.4.0 final and above.
enum ScriptAPIVersion
{
    kScriptAPI_Undefined = INT32_MIN,
    kScriptAPI_v321 = 0,
    kScriptAPI_v330 = 1,
    kScriptAPI_v334 = 2,
    kScriptAPI_v335 = 3,
    kScriptAPI_v340 = 4,
    kScriptAPI_v341 = 5,
    kScriptAPI_v350 = 6,
    kScriptAPI_v3507= 7,
    kScriptAPI_v351 = 8,
    kScriptAPI_v360 = 3060000,
    kScriptAPI_v36026 = 3060026,
    kScriptAPI_v361 = 3060100,
    kScriptAPI_v362 = 3060200,
    kScriptAPI_v363 = 3060300,
    kScriptAPI_v399 = 3990000,
    kScriptAPI_v400 = 4000003,
    kScriptAPI_v400_07 = 4000007,
    kScriptAPI_v400_14 = 4000014,
    kScriptAPI_v400_16 = 4000016,
    kScriptAPI_v400_18 = 4000018,
    kScriptAPI_Current = kScriptAPI_v400_18
};

const char *GetScriptAPIName(ScriptAPIVersion v);

// Determines whether the graphics renderer should scale sprites at the final
// screen resolution, as opposed to native resolution
enum RenderAtScreenRes
{
    kRenderAtScreenRes_UserDefined  = 0,
    kRenderAtScreenRes_Enabled      = 1,
    kRenderAtScreenRes_Disabled     = 2,
};


// Sprite flags
// SERIALIZATION NOTE: serialized as 8-bit in game data and legacy saves
//                     serialized as 32-bit in new saves (for dynamic sprites only).
// WARNING: 0x3B bits were taken by now deprecated flags (see SPF_OBSOLETEMASK);
// should not reuse these bits unless explicitly set a new game data format version!
#define SPF_OBSOLETEMASK    0x3B
#define SPF_DYNAMICALLOC    0x04  // created by runtime script
#define SPF_KEEPDEPTH       0x40  // sprite must explicitly retain its original color depth
// Runtime sprite flags follow
#define SPF_OBJECTOWNED     0x0100 // owned by a game object (not created in user script)

// General information about sprite (properties, size)
struct SpriteInfo
{
    int      Width = 0;
    int      Height = 0;
    int      ColorDepth = 0;
    uint32_t Flags = 0u; // SPF_* flags

    SpriteInfo() = default;
    SpriteInfo(int w, int h, int color_depth, uint32_t flags)
        : Width(w), Height(h), ColorDepth(color_depth), Flags(flags) {}

    inline bool IsValid() const { return Width > 0 && Height > 0; }
    inline Size GetResolution() const { return Size(Width, Height); }
    // Gets if sprite is created at runtime (by engine, or a script command)
    inline bool IsDynamicSprite() const { return (Flags & SPF_DYNAMICALLOC) != 0; }
};

// Various font parameters, defining and extending font rendering behavior.
// While FontRenderer object's main goal is to render single line of text at
// the strictly determined position on canvas, FontInfo may additionally
// provide instructions on adjusting drawing position, as well as arranging
// multiple lines, and similar cases.
struct FontInfo
{
    enum AutoOutlineStyle : int
    {
        kSquared = 0,
        kRounded = 1,
    };

    // Font's source filename
    AGS::Common::String Filename;
    // General font's loading and rendering flags
    uint32_t      Flags;
    // Nominal font import size (in pixels)
    int           Size;
    // Factor to multiply base font size by
    int           SizeMultiplier;
    // Outlining font index, or auto-outline flag
    int           Outline;
    // Custom vertical render offset, used mainly for fixing broken fonts
    int           YOffset;
    // Custom line spacing between two lines of text (0 = use font height)
    int           LineSpacing;
    // When automatic outlining, style of the outline
    AutoOutlineStyle AutoOutlineStyle;
    // When automatic outlining, thickness of the outline (0 = no auto outline)
    int           AutoOutlineThickness;

    FontInfo();
};

#endif // __AGS_CN_AC__GAMESTRUCTDEFINES_H
