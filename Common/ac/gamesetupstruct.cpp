//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/audiocliptype.h"
#include "ac/gamesetupstruct.h"
#include "ac/oldgamesetupstruct.h"
#include "ac/wordsdictionary.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "game/interactions.h"
#include "util/string_utils.h"

using namespace AGS::Common;

GameSetupStruct::GameSetupStruct()
    : filever(0)
    , roomCount(0)
    , scoreClipID(0)
{
    memset(lipSyncFrameLetters, 0, sizeof(lipSyncFrameLetters));
    memset(guid, 0, sizeof(guid));
    memset(saveGameFileExtension, 0, sizeof(saveGameFileExtension));
}

GameSetupStruct::~GameSetupStruct()
{
    Free();
}

void GameSetupStruct::Free()
{
    GameSetupStructBase::Free();

    intrChar.clear();
    charScripts.clear();

    // TODO: find out if it really needs to begin with 1 here?
    for (size_t i = 1; i < (size_t)MAX_INV; i++)
        intrInv[i].reset();
    invScripts.clear();
    numinvitems = 0;

    roomNames.clear();
    roomNumbers.clear();
    roomCount = 0;

    audioClips.clear();
    audioClipTypes.clear();

    charProps.clear();
    viewNames.clear();
}

// Assigns font info parameters using legacy flags value read from the game data
void SetFontInfoFromLegacyFlags(FontInfo &finfo, const uint8_t data)
{
    finfo.Flags = (data >> 6) & 0xFF;
    finfo.Size = data & FFLG_LEGACY_SIZEMASK;
}

void AdjustFontInfoUsingFlags(FontInfo &finfo, const uint32_t flags)
{
    finfo.Flags = flags;
    if ((flags & FFLG_SIZEMULTIPLIER) != 0)
    {
        finfo.SizeMultiplier = finfo.Size;
        finfo.Size = 0;
    }
}

ScriptAudioClip* GetAudioClipForOldStyleNumber(GameSetupStruct &game, bool is_music, int num)
{
    String clip_name;
    if (is_music)
        clip_name.Format("aMusic%d", num);
    else
        clip_name.Format("aSound%d", num);

    for (size_t i = 0; i < game.audioClips.size(); ++i)
    {
        if (clip_name.CompareNoCase(game.audioClips[i].scriptName) == 0)
            return &game.audioClips[i];
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
// Reading Part 1

void GameSetupStruct::read_savegame_info(Common::Stream *in, GameDataVersion data_ver)
{
    if (data_ver > kGameVersion_272) // only 3.x
    {
        StrUtil::ReadCStrCount(guid, in, MAX_GUID_LENGTH);
        StrUtil::ReadCStrCount(saveGameFileExtension, in, MAX_SG_EXT_LENGTH);
        saveGameFolderName.ReadCount(in, LEGACY_MAX_SG_FOLDER_LEN);
    }
}

void GameSetupStruct::read_font_infos(Common::Stream *in, GameDataVersion data_ver)
{
    fonts.resize(numfonts);
    if (data_ver < kGameVersion_350)
    {
        for (int i = 0; i < numfonts; ++i)
            SetFontInfoFromLegacyFlags(fonts[i], in->ReadInt8());
        for (int i = 0; i < numfonts; ++i)
            fonts[i].Outline = in->ReadInt8(); // size of char
        if (data_ver < kGameVersion_341)
            return;
        for (int i = 0; i < numfonts; ++i)
        {
            fonts[i].YOffset = in->ReadInt32();
            if (data_ver >= kGameVersion_341_2)
                fonts[i].LineSpacing = std::max(0, in->ReadInt32());
        }
    }
    else
    {
        for (int i = 0; i < numfonts; ++i)
        {
            uint32_t flags = in->ReadInt32();
            fonts[i].Size = in->ReadInt32();
            fonts[i].Outline = in->ReadInt32();
            fonts[i].YOffset = in->ReadInt32();
            fonts[i].LineSpacing = std::max(0, in->ReadInt32());
            AdjustFontInfoUsingFlags(fonts[i], flags);
        }
    }
}

void GameSetupStruct::ReadInvInfo(Stream *in)
{
    for (int i = 0; i < numinvitems; ++i)
    {
        invinfo[i].ReadFromFile(in);
    }
}

void GameSetupStruct::WriteInvInfo(Stream *out)
{
    for (int i = 0; i < numinvitems; ++i)
    {
        invinfo[i].WriteToFile(out);
    }
}

HGameFileError GameSetupStruct::read_cursors(Common::Stream *in)
{
    mcurs.resize(numcursors);
    ReadMouseCursors(in);
    return HGameFileError::None();
}

void GameSetupStruct::read_interaction_scripts(Common::Stream *in, GameDataVersion data_ver)
{
    numGlobalVars = 0;

    if (data_ver > kGameVersion_272) // 3.x
    {
        charScripts.resize(numcharacters);
        invScripts.resize(numinvitems);
        for (size_t i = 0; i < (size_t)numcharacters; ++i)
            charScripts[i].reset(InteractionScripts::CreateFromStream(in));
        // NOTE: new inventory items' events are loaded starting from 1 for some reason
        for (size_t i = 1; i < (size_t)numinvitems; ++i)
            invScripts[i].reset(InteractionScripts::CreateFromStream(in));
    }
    else // 2.x
    {
        intrChar.resize(numcharacters);
        for (size_t i = 0; i < (size_t)numcharacters; ++i)
            intrChar[i].reset(Interaction::CreateFromStream(in));
        for (size_t i = 0; i < (size_t)numinvitems; ++i)
            intrInv[i].reset(Interaction::CreateFromStream(in));

        numGlobalVars = in->ReadInt32();
        for (size_t i = 0; i < (size_t)numGlobalVars; ++i)
            globalvars[i].Read(in);
    }
}

void GameSetupStruct::read_words_dictionary(Common::Stream *in)
{
    dict.reset(new WordsDictionary());
    read_dictionary(dict.get(), in);
}

void GameSetupStruct::ReadMouseCursors(Stream *in)
{
    for (int i = 0; i < numcursors; ++i)
    {
        mcurs[i].ReadFromFile(in);
    }
}

void GameSetupStruct::WriteMouseCursors(Stream *out)
{
    for (int i = 0; i < numcursors; ++i)
    {
        mcurs[i].WriteToFile(out);
    }
}

//-----------------------------------------------------------------------------
// Reading Part 2

void GameSetupStruct::read_characters(Common::Stream *in)
{
    chars.resize(numcharacters);
    chars2.resize(numcharacters);
    ReadCharacters(in);
}

void GameSetupStruct::read_lipsync(Common::Stream *in, GameDataVersion data_ver)
{
    if (data_ver >= kGameVersion_254) // lip syncing was introduced in 2.54
        in->ReadArray(&lipSyncFrameLetters[0][0], MAXLIPSYNCFRAMES, 50);
}

void GameSetupStruct::read_messages(Common::Stream *in,
    const std::array<int, MAXGLOBALMES> &load_messages, GameDataVersion data_ver)
{
    char mbuf[GLOBALMESLENGTH];
    for (int i = 0; i < MAXGLOBALMES; ++i)
    {
        if (!load_messages[i])
            continue;
        if (data_ver < kGameVersion_261) // Global messages are not encrypted on < 2.61
        {
            char* nextchar = mbuf;
            // TODO: probably this is same as fgetstring
            while (1)
            {
                *nextchar = in->ReadInt8();
                if (*nextchar == 0)
                    break;
                nextchar++;
            }
        }
        else
        {
            read_string_decrypt(in, mbuf, GLOBALMESLENGTH);
        }
        messages[i] = mbuf;
    }
}

void GameSetupStruct::ReadCharacters(Stream *in)
{
    for (int i = 0; i < numcharacters; ++i)
    {
        chars[i].ReadFromFile(in, chars2[i], loaded_game_file_version);
    }
}

void GameSetupStruct::WriteCharacters(Stream *out)
{
    for (int i = 0; i < numcharacters; ++i)
    {
        chars[i].WriteToFile(out);
    }
}

//-----------------------------------------------------------------------------
// Reading Part 3

HGameFileError GameSetupStruct::read_customprops(Common::Stream *in, GameDataVersion data_ver)
{
    dialogScriptNames.resize(numdialog);
    viewNames.resize(numviews);
    if (data_ver >= kGameVersion_260) // >= 2.60
    {
        if (Properties::ReadSchema(propSchema, in) != kPropertyErr_NoError)
            return new MainGameFileError(kMGFErr_InvalidPropertySchema);

        int errors = 0;

        charProps.resize(numcharacters);
        for (int i = 0; i < numcharacters; ++i)
        {
            errors += Properties::ReadValues(charProps[i], in);
        }
        for (int i = 0; i < numinvitems; ++i)
        {
            errors += Properties::ReadValues(invProps[i], in);
        }

        if (errors > 0)
            return new MainGameFileError(kMGFErr_InvalidPropertyValues);

        for (int i = 0; i < numviews; ++i)
            viewNames[i] = String::FromStream(in);

        if (data_ver >= kGameVersion_270)
        {
            for (int i = 0; i < numinvitems; ++i)
                invScriptNames[i] = String::FromStream(in);

            if (data_ver >= kGameVersion_272)
            {
                for (int i = 0; i < numdialog; ++i)
                    dialogScriptNames[i] = String::FromStream(in);
            }
        }
    }
    return HGameFileError::None();
}

HGameFileError GameSetupStruct::read_audio(Common::Stream *in, GameDataVersion data_ver)
{
    if (data_ver >= kGameVersion_320)
    {
        size_t audiotype_count = in->ReadInt32();
        audioClipTypes.resize(audiotype_count);
        for (size_t i = 0; i < audiotype_count; ++i)
        {
            audioClipTypes[i].ReadFromFile(in);
        }

        size_t audioclip_count = in->ReadInt32();
        audioClips.resize(audioclip_count);
        ReadAudioClips(in, audioclip_count);

        scoreClipID = in->ReadInt32();
    }
    return HGameFileError::None();
}

void GameSetupStruct::read_room_names(Stream *in, GameDataVersion data_ver)
{
    if ((data_ver >= kGameVersion_301) && (options[OPT_DEBUGMODE] != 0))
    {
        roomCount = in->ReadInt32();
        roomNumbers.resize(roomCount);
        roomNames.resize(roomCount);
        for (int i = 0; i < roomCount; ++i)
        {
            roomNumbers[i] = in->ReadInt32();
            roomNames[i].Read(in);
        }
    }
    else
    {
        roomCount = 0;
    }
}

void GameSetupStruct::ReadAudioClips(Common::Stream *in, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        audioClips[i].ReadFromFile(in);
    }
}

//=============================================================================
#if defined (OBSOLETE)
#include <stdio.h>

void ConvertOldGameStruct (OldGameSetupStruct *ogss, GameSetupStruct *gss) {
    snprintf(gss->gamename, sizeof(GameSetupStruct::gamename), "%s", ogss->gamename);
    for (int i = 0; i < 20; i++)
        gss->options[i] = ogss->options[i];
    memcpy (&gss->paluses[0], &ogss->paluses[0], 256);
    memcpy (&gss->defpal[0],  &ogss->defpal[0],  256 * sizeof(RGB));
    gss->numviews = ogss->numviews;
    gss->numcharacters = ogss->numcharacters;
    gss->playercharacter = ogss->playercharacter;
    gss->totalscore = ogss->totalscore;
    gss->numinvitems = ogss->numinvitems;
    gss->numdialog = ogss->numdialog;
    gss->numdlgmessage = ogss->numdlgmessage;
    gss->numfonts = ogss->numfonts;
    gss->color_depth = ogss->color_depth;
    gss->target_win = ogss->target_win;
    gss->dialog_bullet = ogss->dialog_bullet;
    gss->hotdot = ogss->hotdot;
    gss->hotdotouter = ogss->hotdotouter;
    gss->uniqueid = ogss->uniqueid;
    gss->numgui = ogss->numgui;
    for (int i = 0; i < 10; ++i)
    {
        SetFontInfoFromLegacyFlags(gss->fonts[i], ogss->fontflags[i]);
        gss->fonts[i].Outline = ogss->fontoutline[i];
    }

    for (int i = 0; i < LEGACY_MAX_SPRITES_V25; ++i)
    {
        gss->SpriteInfos[i].Flags = ogss->spriteflags[i];
    }

    memcpy (&gss->invinfo[0], &ogss->invinfo[0], 100 * sizeof(InventoryItemInfo));
    for (int i = 0; i < 10; ++i)
        gss->mcurs[i] = ogss->mcurs[i];
    for (int i = 0; i < MAXGLOBALMES; i++)
        gss->messages[i] = ogss->messages[i];
    gss->dict = ogss->dict;
    gss->globalscript = ogss->globalscript;
    gss->chars = nullptr; //ogss->chars;
    gss->compiled_script = ogss->compiled_script;
    gss->numcursors = 10;
}
#endif // OBSOLETE

void GameSetupStruct::ReadFromSavegame(Stream *in)
{
    // of GameSetupStruct
    in->ReadArrayOfInt32(options, OPT_HIGHESTOPTION_321 + 1);
    options[OPT_LIPSYNCTEXT] = in->ReadInt32();
    // of GameSetupStructBase
    playercharacter = in->ReadInt32();
    dialog_bullet = in->ReadInt32();
    hotdot = in->ReadInt16();
    hotdotouter = in->ReadInt16();
    invhotdotsprite = in->ReadInt32();
    default_lipsync_frame = in->ReadInt32();
}

void GameSetupStruct::WriteForSavegame(Stream *out)
{
    // of GameSetupStruct
    out->WriteArrayOfInt32(options, OPT_HIGHESTOPTION_321 + 1);
    out->WriteInt32(options[OPT_LIPSYNCTEXT]);
    // of GameSetupStructBase
    out->WriteInt32(playercharacter);
    out->WriteInt32(dialog_bullet);
    out->WriteInt16(hotdot);
    out->WriteInt16(hotdotouter);
    out->WriteInt32(invhotdotsprite);
    out->WriteInt32(default_lipsync_frame);
}
