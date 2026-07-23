//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <memory>
#include <vector>
#include "gtest/gtest.h"
#include "ac/characterinfo.h"
#include "ac/audiocliptype.h"
#include "ac/dialogtopic.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "ac/game_version.h"
#include "ac/gamesetupstructbase.h"
#include "ac/inventoryiteminfo.h"
#include "ac/mousecursor.h"
#include "ac/wordsdictionary.h"
#include "ac/view.h"
#include "data/data_file_writer.h"
#include "game/customproperties.h"
#include "game/interactions.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guimain.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"
#include "util/memorystream.h"
#include "util/string_utils.h"

using namespace AGS;
using namespace AGS::Common;

namespace
{

using GameBlockWriter = void (*)(const DataUtil::GameData&, Stream*);

std::vector<uint8_t> WriteGameBlock(const DataUtil::GameData &game,
    GameBlockWriter writer)
{
    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    writer(game, out.get());
    out.reset();
    return buffer;
}

std::vector<uint8_t> WriteExtensionPayload(const DataUtil::GameData &game,
    DataFileWriter::ExtensionWriter writer)
{
    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    writer(out.get(), game);
    out.reset();
    return buffer;
}

void WriteSmallExtensionPayload(Stream *out, const DataUtil::GameData &)
{
    for (int i = 0; i < 5; ++i)
        out->WriteInt8(i);
}

void WriteLargeExtensionPayload(Stream *out, const DataUtil::GameData &)
{
    out->WriteInt32(10);
    out->WriteInt32(20);
    out->WriteInt32(30);
}

struct LoadedAudioBlock
{
    std::vector<AudioClipType> Types;
    std::vector<ScriptAudioClip> Clips;
    int ScoreClipID = -1;
};

LoadedAudioBlock RoundTripAudioBlock(const DataUtil::GameData &game)
{
    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteAudioBlock(game, out.get());
    out.reset();
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    LoadedAudioBlock loaded;
    loaded.Types.resize(in->ReadInt32());
    for (auto &type : loaded.Types)
        type.ReadFromFile(in.get());
    loaded.Clips.resize(in->ReadInt32());
    for (auto &clip : loaded.Clips)
        clip.ReadFromFile(in.get());
    loaded.ScoreClipID = in->ReadInt32();
    EXPECT_EQ(in->GetLength(), in->GetPosition());
    return loaded;
}

} // namespace

TEST(DataFileWriter, RoundTripGameSetupStructBase)
{
    DataUtil::GameData game;
    game.Settings.GameName = "Round-trip game";
    game.Settings.MaximumScore = 42;
    game.Settings.ColorDepth = DataUtil::kGameColorDepth_TrueColor;
    game.Settings.DialogOptionsBullet = 17;
    game.Settings.UniqueID = 123456;
    game.Settings.CustomResolution = "640 x 360";
    game.Settings.SkipSpeech = DataUtil::kSkipSpeech_MouseOnly;
    game.Settings.InventoryHotspotMarkerStyle = DataUtil::kInventoryHotspot_Sprite;
    game.Settings.InventoryHotspotMarkerDotColor = 10;
    game.Settings.InventoryHotspotMarkerCrosshairColor = 11;
    game.Settings.InventoryHotspotMarkerSprite = 12;
    game.PlayerCharacter = 1;
    game.LipSync = DataUtil::kLipSync_Text;
    game.LipSyncDefaultFrame = 3;
    game.Views.resize(2);
    game.Characters.resize(2);
    game.Inventory.resize(1);
    game.Fonts.resize(1);
    game.GUI.resize(1);
    game.Cursors.resize(2);
    game.GlobalMessages.resize(4);
    game.GlobalMessages[3] = "message";

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    soff_t extension_offset_pos = 0;
    DataFileWriter::WriteGameSetupStructBase(game, out.get(), extension_offset_pos);
    out.reset();

    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));
    GameSetupStructBase loaded;
    GameSetupStructBase::SerializeInfo info;
    loaded.ReadFromFile(in.get(), kGameVersion_Current, info);

    EXPECT_STREQ("Round-trip game", loaded.gamename.GetCStr());
    EXPECT_EQ(42, loaded.totalscore);
    EXPECT_EQ(2, loaded.numviews);
    EXPECT_EQ(2, loaded.numcharacters);
    EXPECT_EQ(1, loaded.playercharacter);
    EXPECT_EQ(2, loaded.numinvitems); // inventory slot 0 is reserved
    EXPECT_EQ(1, loaded.numfonts);
    EXPECT_EQ(1, loaded.numgui);
    EXPECT_EQ(2, loaded.numcursors);
    EXPECT_EQ(4, loaded.color_depth);
    EXPECT_EQ(17, loaded.dialog_bullet);
    EXPECT_EQ(123456, loaded.uniqueid);
    EXPECT_EQ(640, loaded.GetDefaultRes().Width);
    EXPECT_EQ(360, loaded.GetDefaultRes().Height);
    EXPECT_EQ(3, loaded.default_lipsync_frame);
    EXPECT_EQ(10, loaded.inv_hot_color);
    EXPECT_EQ(11, loaded.inv_hot_cross_color);
    EXPECT_EQ(12, loaded.inv_hot_sprite);
    EXPECT_EQ(DataUtil::kSkipSpeech_MouseOnly, loaded.options[OPT_NOSKIPTEXT]);
    EXPECT_EQ(1, loaded.options[OPT_LIPSYNCTEXT]);
    EXPECT_EQ(1, info.HasMessages[3]);
    EXPECT_TRUE(info.HasWordsDict);
    EXPECT_FALSE(info.HasCCScript);
    EXPECT_EQ(0u, info.ExtensionOffset);
}

TEST(DataFileWriter, RoundTripGlobalMessagesBounds)
{
    DataUtil::GameData game;
    game.GlobalMessages.resize(MAXGLOBALMES);
    game.GlobalMessages[0] = "first message";
    game.GlobalMessages[MAXGLOBALMES - 1] = "last valid message";

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    soff_t extension_offset_pos = 0;
    DataFileWriter::WriteGameSetupStructBase(game, out.get(), extension_offset_pos);
    DataFileWriter::WriteGlobalMessagesBlock(game, out.get());
    out.reset();

    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));
    GameSetupStructBase loaded;
    GameSetupStructBase::SerializeInfo info;
    loaded.ReadFromFile(in.get(), kGameVersion_Current, info);

    // GameSetupStruct::read_messages() performs this same flag-driven loop,
    // but linking that aggregate class would pull Engine runtime dependencies
    // into this data-only test target.
    for (int i = 0; i < MAXGLOBALMES; ++i)
    {
        if (info.HasMessages[i])
            loaded.messages[i] = read_string_decrypt(in.get());
    }

    // The flags array enforces the format's 500-message ceiling; the text
    // block itself does not write a count.
    EXPECT_EQ(1, info.HasMessages[0]);
    EXPECT_EQ(1, info.HasMessages[MAXGLOBALMES - 1]);
    EXPECT_STREQ("first message", loaded.messages[0].GetCStr());
    EXPECT_STREQ("last valid message",
        loaded.messages[MAXGLOBALMES - 1].GetCStr());
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripCharacter)
{
    DataUtil::GameData game;
    game.PlayerCharacter = 7;
    DataUtil::InventoryItemData inventory;
    inventory.ID = 2;
    inventory.PlayerStartsWith = true;
    game.Inventory.push_back(inventory);

    DataUtil::CharacterData character;
    character.ID = 7;
    character.ScriptName = "cHero";
    character.RealName = "Hero";
    character.NormalView = 3;
    character.SpeechView = 5;
    character.StartingRoom = 6;
    character.StartX = 100;
    character.StartY = 120;
    character.BlockingX = -4;
    character.BlockingY = 8;
    character.Clickable = true;
    character.DiagonalLoops = true;
    character.Solid = true;
    character.TurnBeforeWalking = true;
    character.TurnWhenFacing = true;
    character.UseRoomAreaLighting = true;
    character.UseRoomAreaScaling = true;
    character.UniformMovementSpeed = true;
    character.MovementSpeed = 4;
    character.AdjustSpeedWithScaling = true;

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteCharacter(out.get(), game, character, 0);
    out.reset();

    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));
    CharacterInfo loaded;
    CharacterInfo2 loaded_ext;
    loaded.ReadFromFile(loaded_ext, in.get(), kGameVersion_Current);

    EXPECT_EQ(7, loaded.index_id);
    EXPECT_EQ(2, loaded.defview);
    EXPECT_EQ(4, loaded.talkview);
    EXPECT_EQ(6, loaded.room);
    EXPECT_EQ(100, loaded.x);
    EXPECT_EQ(120, loaded.y);
    EXPECT_EQ(4, loaded.walkspeed);
    EXPECT_NE(0, loaded.flags & CHF_SCALEMOVESPEED);
    EXPECT_NE(0, loaded.flags & CHF_TURNWHENFACE);
    EXPECT_EQ(1, loaded.inv[2]);
    EXPECT_STREQ("Hero", loaded_ext.name_new.GetCStr());
    EXPECT_STREQ("cHero", loaded_ext.scrname_new.GetCStr());
    EXPECT_EQ(-4, loaded_ext.blocking_x);
    EXPECT_EQ(8, loaded_ext.blocking_y);
}

TEST(DataFileWriter, RoundTripInventoryItem)
{
    DataUtil::InventoryItemData item;
    item.Description = "Key";
    item.Image = 10;
    item.CursorImage = 11;
    item.HotspotX = 3;
    item.HotspotY = 4;
    item.PlayerStartsWith = true;

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteInventoryItem(out.get(), item);
    out.reset();

    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));
    InventoryItemInfo loaded{};
    loaded.ReadFromFile(in.get());

    EXPECT_STREQ("Key", loaded.name.GetCStr());
    EXPECT_EQ(10, loaded.pic);
    EXPECT_EQ(11, loaded.cursorPic);
    EXPECT_EQ(3, loaded.hotx);
    EXPECT_EQ(4, loaded.hoty);
    EXPECT_NE(0, loaded.flags & IFLG_STARTWITH);
}

TEST(DataFileWriter, RoundTripInventoryBlock)
{
    DataUtil::GameData game;
    DataUtil::InventoryItemData key;
    key.Description = "Key";
    key.Image = 10;
    DataUtil::InventoryItemData coin;
    coin.Description = "Coin";
    coin.Image = 20;
    game.Inventory = { key, coin };

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteInventoryBlock(game, out.get());
    out.reset();
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    InventoryItemInfo loaded[3]{};
    for (auto &item : loaded)
        item.ReadFromFile(in.get());
    EXPECT_TRUE(loaded[0].name.IsEmpty());
    EXPECT_EQ(0, loaded[0].pic);
    EXPECT_STREQ("Key", loaded[1].name.GetCStr());
    EXPECT_EQ(10, loaded[1].pic);
    EXPECT_STREQ("Coin", loaded[2].name.GetCStr());
    EXPECT_EQ(20, loaded[2].pic);
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripCursor)
{
    DataUtil::CursorData cursor;
    cursor.Name = "Interact";
    cursor.Image = 20;
    cursor.HotspotX = 5;
    cursor.HotspotY = 6;
    cursor.Animate = true;
    cursor.View = 4;
    cursor.AnimateOnlyOnHotspots = true;
    cursor.AnimateOnlyWhenMoving = true;
    cursor.StandardMode = true;

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteCursor(out.get(), cursor);
    out.reset();

    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));
    MouseCursor loaded;
    loaded.ReadFromFile(in.get());

    EXPECT_STREQ("Interact", loaded.name.GetCStr());
    EXPECT_EQ(20, loaded.pic);
    EXPECT_EQ(5, loaded.hotx);
    EXPECT_EQ(6, loaded.hoty);
    EXPECT_EQ(3, loaded.view);
    EXPECT_NE(0, loaded.flags & MCF_HOTSPOT);
    EXPECT_NE(0, loaded.flags & MCF_ANIMMOVE);
    EXPECT_NE(0, loaded.flags & MCF_STANDARD);
}

TEST(DataFileWriter, RoundTripCursorBlock)
{
    DataUtil::GameData game;
    DataUtil::CursorData walk;
    walk.Name = "Walk";
    walk.Image = 1;
    DataUtil::CursorData look;
    look.Name = "Look";
    look.Image = 2;
    game.Cursors = { walk, look };

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteCursorBlock(game, out.get());
    out.reset();
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    MouseCursor loaded[2];
    for (auto &cursor : loaded)
        cursor.ReadFromFile(in.get());
    EXPECT_STREQ("Walk", loaded[0].name.GetCStr());
    EXPECT_EQ(1, loaded[0].pic);
    EXPECT_STREQ("Look", loaded[1].name.GetCStr());
    EXPECT_EQ(2, loaded[1].pic);
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripTextParserDictionary)
{
    DataUtil::GameData game;
    game.ParserWords.push_back({ "look, examine", 10 });
    game.ParserWords.push_back({ "inventory", 20 });

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteTextParserDictionary(game, out.get());
    out.reset();

    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));
    WordsDictionary loaded;
    loaded.ReadFromFile(in.get());

    EXPECT_EQ(3u, loaded.GetWords().size());
    EXPECT_EQ(10, loaded.FindWord("look"));
    EXPECT_EQ(10, loaded.FindWord("examine"));
    EXPECT_EQ(20, loaded.FindWord("inventory"));
}

TEST(DataFileWriter, RoundTripView)
{
    DataUtil::GameData game;
    DataUtil::AudioClipData audio;
    audio.ID = 99;
    audio.Index = 42;
    game.AudioClips.push_back(audio);

    DataUtil::ViewFrameData frame;
    frame.Image = 12;
    frame.Delay = 3;
    frame.Flipped = true;
    frame.Sound = 42;
    DataUtil::ViewLoopData loop;
    loop.RunNextLoop = true;
    loop.Frames.push_back(frame);
    DataUtil::ViewData view;
    view.Loops.push_back(loop);

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteView(out.get(), game, view);
    out.reset();

    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));
    ViewStruct loaded;
    loaded.ReadFromFile(in.get());

    ASSERT_EQ(1, loaded.numLoops);
    ASSERT_EQ(1, loaded.loops[0].numFrames);
    ASSERT_EQ(1u, loaded.loops[0].frames.size());
    EXPECT_TRUE(loaded.loops[0].RunNextLoop());
    EXPECT_EQ(12, loaded.loops[0].frames[0].pic);
    EXPECT_EQ(3, loaded.loops[0].frames[0].speed);
    EXPECT_NE(0, loaded.loops[0].frames[0].flags & VFLG_FLIPSPRITE);
    EXPECT_EQ(99, loaded.loops[0].frames[0].sound);
}

TEST(DataFileWriter, RoundTripViewsBlock)
{
    DataUtil::GameData game;
    DataUtil::ViewData first;
    DataUtil::ViewLoopData first_loop;
    first_loop.Frames.push_back({ 1, false, 10, 0 });
    first.Loops.push_back(first_loop);
    DataUtil::ViewData second;
    DataUtil::ViewLoopData second_loop;
    second_loop.Frames.push_back({ 2, true, 20, 0 });
    second.Loops.push_back(second_loop);
    game.Views = { first, second };

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteViewsBlock(game, out.get());
    out.reset();
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    ViewStruct loaded[2];
    for (auto &view : loaded)
        view.ReadFromFile(in.get());
    ASSERT_EQ(1u, loaded[0].loops[0].frames.size());
    ASSERT_EQ(1u, loaded[1].loops[0].frames.size());
    EXPECT_EQ(10, loaded[0].loops[0].frames[0].pic);
    EXPECT_EQ(20, loaded[1].loops[0].frames[0].pic);
    EXPECT_NE(0, loaded[1].loops[0].frames[0].flags & VFLG_FLIPSPRITE);
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripCharactersBlock)
{
    DataUtil::GameData game;
    DataUtil::CharacterData first;
    first.RealName = "First";
    first.ScriptName = "cFirst";
    DataUtil::CharacterData second;
    second.RealName = "Second";
    second.ScriptName = "cSecond";
    game.Characters = { first, second };

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteCharactersBlock(game, out.get());
    out.reset();
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    CharacterInfo loaded[2];
    CharacterInfo2 loaded_ext[2];
    for (int i = 0; i < 2; ++i)
        loaded[i].ReadFromFile(loaded_ext[i], in.get(), kGameVersion_Current);
    EXPECT_EQ(0, loaded[0].index_id);
    EXPECT_EQ(1, loaded[1].index_id);
    EXPECT_STREQ("First", loaded_ext[0].name_new.GetCStr());
    EXPECT_STREQ("Second", loaded_ext[1].name_new.GetCStr());
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripAudioType)
{
    DataUtil::AudioTypeData type;
    type.ID = 4;
    type.MaxChannels = 2;
    type.VolumeReductionWhileSpeechPlaying = 30;
    type.Crossfade = DataUtil::kCrossfade_Medium;

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteAudioType(out.get(), &type, 4);
    out.reset();

    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));
    AudioClipType loaded;
    loaded.ReadFromFile(in.get());

    EXPECT_EQ(4, loaded.id);
    EXPECT_EQ(2, loaded.reservedChannels);
    EXPECT_EQ(30, loaded.volume_reduction_while_speech_playing);
    EXPECT_EQ(DataUtil::kCrossfade_Medium, loaded.crossfadeSpeed);
}

TEST(DataFileWriter, RoundTripAudioClip)
{
    DataUtil::AudioClipData clip;
    clip.ID = 8;
    clip.ScriptName = "aMusic";
    clip.CacheFileName = "music.ogg";
    clip.BundlingType = DataUtil::kAudioBundling_InSeparateVOX;
    clip.Type = 3;
    clip.FileType = DataUtil::kAudioFile_OGG;
    clip.Repeat = true;
    clip.Priority = 75;
    clip.Volume = 80;

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteAudioClip(out.get(), clip, 0);
    out.reset();

    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));
    ScriptAudioClip loaded;
    loaded.ReadFromFile(in.get());

    EXPECT_EQ(8, loaded.id);
    EXPECT_STREQ("aMusic", loaded.scriptName.GetCStr());
    EXPECT_STREQ("music.ogg", loaded.fileName.GetCStr());
    EXPECT_EQ(DataUtil::kAudioBundling_InSeparateVOX, loaded.bundlingType);
    EXPECT_EQ(3, loaded.type);
    EXPECT_EQ(eAudioFileOGG, loaded.fileType);
    EXPECT_EQ(1, loaded.defaultRepeat);
    EXPECT_EQ(75, loaded.defaultPriority);
    EXPECT_EQ(80, loaded.defaultVolume);
}

TEST(DataFileWriter, RoundTripAudioBlock)
{
    DataUtil::GameData game;
    DataUtil::AudioTypeData music;
    music.ID = 10;
    music.MaxChannels = 2;
    DataUtil::AudioTypeData sound;
    sound.ID = 20;
    sound.MaxChannels = 4;
    game.AudioTypes = { music, sound };

    DataUtil::AudioClipData theme;
    theme.ID = 101;
    theme.Index = 10;
    theme.ScriptName = "aTheme";
    theme.CacheFileName = "theme.ogg";
    DataUtil::AudioClipData click;
    click.ID = 202;
    click.Index = 20;
    click.ScriptName = "aClick";
    click.CacheFileName = "click.wav";
    game.AudioClips = { theme, click };
    game.Settings.PlaySoundOnScore = 20;

    LoadedAudioBlock loaded = RoundTripAudioBlock(game);
    ASSERT_EQ(3u, loaded.Types.size());
    EXPECT_EQ(0, loaded.Types[0].id); // synthetic reserved type 0
    EXPECT_EQ(1, loaded.Types[0].reservedChannels);
    // DataFileWriter.cs serializes audio type IDs from collection positions,
    // rather than from AudioClipType.TypeID.
    EXPECT_EQ(1, loaded.Types[1].id);
    EXPECT_EQ(2, loaded.Types[2].id);
    ASSERT_EQ(2u, loaded.Clips.size());
    EXPECT_EQ(101, loaded.Clips[0].id);
    EXPECT_STREQ("aTheme", loaded.Clips[0].scriptName.GetCStr());
    EXPECT_EQ(202, loaded.Clips[1].id);
    EXPECT_STREQ("aClick", loaded.Clips[1].scriptName.GetCStr());
    EXPECT_EQ(202, loaded.ScoreClipID);

    // PlaySoundOnScore stores the AGF clip index, not the script name. A
    // missing index is serialized as the Engine's "none" value.
    game.Settings.PlaySoundOnScore = -1;
    loaded = RoundTripAudioBlock(game);
    EXPECT_EQ(-1, loaded.ScoreClipID);
}

TEST(DataFileWriter, RoundTripExt363Dialogs)
{
    DataUtil::DialogRef dialog;
    dialog.ScriptName = "dGreeting";
    dialog.ShowTextParser = true;
    dialog.Options.push_back({ "Hello", true, true });
    dialog.Options.push_back({ "Goodbye", false, false });
    DataUtil::GameData game;
    game.Dialogs.push_back(dialog);

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteExt363Dialogs(out.get(), game);
    out.reset();

    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));
    ASSERT_EQ(1, in->ReadInt32());
    DialogTopic loaded;
    loaded.ReadFromFile_v363(in.get());

    EXPECT_STREQ("dGreeting", loaded.ScriptName.GetCStr());
    EXPECT_NE(0, loaded.Flags & DTFLG_SHOWPARSER);
    ASSERT_EQ(2u, loaded.Options.size());
    EXPECT_STREQ("Hello", loaded.Options[0].Text.GetCStr());
    EXPECT_NE(0, loaded.Options[0].Flags & DFLG_ON);
    EXPECT_STREQ("Goodbye", loaded.Options[1].Text.GetCStr());
    EXPECT_EQ(0, loaded.Options[1].Flags & DFLG_ON);
    EXPECT_NE(0, loaded.Options[1].Flags & DFLG_NOREPEAT);
}

TEST(DataFileWriter, RoundTripExtensionFraming)
{
    DataUtil::GameData game;
    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteExtension(out.get(), "small", game,
        WriteSmallExtensionPayload);
    DataFileWriter::WriteExtension(out.get(), "large", game,
        WriteLargeExtensionPayload);
    out.reset();

    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));
    char id[17] = {};
    EXPECT_EQ(0, in->ReadInt8());
    in->Read(id, 16);
    EXPECT_STREQ("small", id);
    const int64_t small_length = in->ReadInt64();
    EXPECT_EQ(5, small_length);
    const soff_t small_payload_pos = in->GetPosition();
    for (int i = 0; i < small_length; ++i)
        EXPECT_EQ(i, in->ReadInt8());
    EXPECT_EQ(small_payload_pos + small_length, in->GetPosition());

    for (char &ch : id)
        ch = 0;
    EXPECT_EQ(0, in->ReadInt8());
    in->Read(id, 16);
    EXPECT_STREQ("large", id);
    const int64_t large_length = in->ReadInt64();
    EXPECT_EQ(12, large_length);
    const soff_t large_payload_pos = in->GetPosition();
    EXPECT_EQ(10, in->ReadInt32());
    EXPECT_EQ(20, in->ReadInt32());
    EXPECT_EQ(30, in->ReadInt32());
    EXPECT_EQ(large_payload_pos + large_length, in->GetPosition());
    EXPECT_NE(small_length, large_length);
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripCustomProperties)
{
    DataUtil::CustomPropertySchemaItem schema_item;
    schema_item.Name = "Health";
    schema_item.Type = kPropertyInteger;
    schema_item.Description = "Character health";
    schema_item.DefaultValue = "100";
    std::vector<DataUtil::CustomPropertySchemaItem> schema = { schema_item };

    std::vector<uint8_t> schema_buffer;
    auto schema_out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(schema_buffer, kStream_Write));
    DataFileWriter::WritePropertySchemaBlock(schema_out.get(), schema);
    schema_out.reset();

    auto schema_in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(schema_buffer));
    PropertySchema loaded_schema;
    ASSERT_EQ(kPropertyErr_NoError,
        Properties::ReadSchema(loaded_schema, schema_in.get()));
    ASSERT_EQ(1u, loaded_schema.size());
    const auto loaded_item_it = loaded_schema.find("Health");
    ASSERT_NE(loaded_schema.end(), loaded_item_it);
    const PropertyDesc &loaded_item = loaded_item_it->second;
    EXPECT_EQ(kPropertyInteger, loaded_item.Type);
    EXPECT_STREQ("Character health", loaded_item.Description.GetCStr());
    EXPECT_STREQ("100", loaded_item.DefaultValue.GetCStr());

    std::vector<DataUtil::CustomPropertyValue> values = {
        { "Health", "75" }, { "Mood", "Happy" }
    };
    std::vector<uint8_t> values_buffer;
    auto values_out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(values_buffer, kStream_Write));
    DataFileWriter::WritePropertyValues(values_out.get(), values);
    values_out.reset();

    auto values_in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(values_buffer));
    StringIMap loaded_values;
    ASSERT_EQ(kPropertyErr_NoError,
        Properties::ReadValues(loaded_values, values_in.get()));
    const auto health_it = loaded_values.find("Health");
    const auto mood_it = loaded_values.find("Mood");
    ASSERT_NE(loaded_values.end(), health_it);
    ASSERT_NE(loaded_values.end(), mood_it);
    EXPECT_STREQ("75", health_it->second.GetCStr());
    EXPECT_STREQ("Happy", mood_it->second.GetCStr());
}

TEST(DataFileWriter, RoundTripCustomPropertiesBlock)
{
    DataUtil::GameData game;
    DataUtil::CustomPropertySchemaItem schema_item;
    schema_item.Name = "Value";
    schema_item.Type = kPropertyInteger;
    schema_item.DefaultValue = "0";
    game.PropertySchema.push_back(schema_item);

    DataUtil::CharacterData first_character;
    first_character.Properties.push_back({ "Value", "11" });
    DataUtil::CharacterData second_character;
    second_character.Properties.push_back({ "Value", "22" });
    game.Characters = { first_character, second_character };

    DataUtil::InventoryItemData first_item;
    first_item.Properties.push_back({ "Value", "33" });
    DataUtil::InventoryItemData second_item;
    second_item.Properties.push_back({ "Value", "44" });
    game.Inventory = { first_item, second_item };

    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteCustomPropertiesBlock(game, out.get());
    out.reset();
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    PropertySchema schema;
    ASSERT_EQ(kPropertyErr_NoError, Properties::ReadSchema(schema, in.get()));
    ASSERT_EQ(1u, schema.size());
    StringIMap character_properties[2];
    for (auto &properties : character_properties)
        ASSERT_EQ(kPropertyErr_NoError,
            Properties::ReadValues(properties, in.get()));
    StringIMap unused_inventory_properties;
    ASSERT_EQ(kPropertyErr_NoError,
        Properties::ReadValues(unused_inventory_properties, in.get()));
    StringIMap inventory_properties[2];
    for (auto &properties : inventory_properties)
        ASSERT_EQ(kPropertyErr_NoError,
            Properties::ReadValues(properties, in.get()));

    EXPECT_STREQ("11", character_properties[0]["Value"].GetCStr());
    EXPECT_STREQ("22", character_properties[1]["Value"].GetCStr());
    EXPECT_TRUE(unused_inventory_properties.empty());
    EXPECT_STREQ("33", inventory_properties[0]["Value"].GetCStr());
    EXPECT_STREQ("44", inventory_properties[1]["Value"].GetCStr());
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripFontBlock)
{
    DataUtil::GameData game;
    DataUtil::FontData multiplier_font;
    multiplier_font.PointSize = 0;
    multiplier_font.SizeMultiplier = 3;
    multiplier_font.HeightDefinedBy = DataUtil::kFontHeight_NominalHeight;
    multiplier_font.MetricsFixup = DataUtil::kFontMetrics_SetAscenderToHeight;
    multiplier_font.OutlineStyle = DataUtil::kFontOutline_Automatic;
    multiplier_font.VerticalOffset = 7;
    multiplier_font.LineSpacing = 9;
    DataUtil::FontData sized_font;
    sized_font.PointSize = 12;
    sized_font.SizeMultiplier = 2;
    sized_font.HeightDefinedBy = DataUtil::kFontHeight_CustomValue;
    sized_font.OutlineStyle = DataUtil::kFontOutline_UseOutlineFont;
    sized_font.OutlineFont = 4;
    game.Fonts = { multiplier_font, sized_font };

    std::vector<uint8_t> buffer = WriteGameBlock(game,
        DataFileWriter::WriteFontBlock);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    FontInfo loaded[2];
    for (int i = 0; i < 2; ++i)
    {
        loaded[i].FontID = i;
        const uint32_t flags = in->ReadInt32();
        loaded[i].Size = in->ReadInt32();
        loaded[i].Outline = in->ReadInt32();
        loaded[i].YOffset = in->ReadInt32();
        loaded[i].LineSpacing = in->ReadInt32();
        loaded[i].SetFlags(flags);
    }

    EXPECT_EQ(0, loaded[0].Size);
    EXPECT_EQ(3, loaded[0].SizeMultiplier);
    EXPECT_NE(0u, loaded[0].Flags & FFLG_SIZEMULTIPLIER);
    EXPECT_NE(0u, loaded[0].Flags & FFLG_LOGICALNOMINALHEIGHT);
    EXPECT_NE(0u, loaded[0].Flags & FFLG_ASCENDERFIXUP);
    EXPECT_EQ(FONT_OUTLINE_AUTO, loaded[0].Outline);
    EXPECT_EQ(7, loaded[0].YOffset);
    EXPECT_EQ(9, loaded[0].LineSpacing);
    EXPECT_EQ(24, loaded[1].Size);
    EXPECT_NE(0u, loaded[1].Flags & FFLG_LOGICALCUSTOMHEIGHT);
    EXPECT_EQ(4, loaded[1].Outline);
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripSpriteFlags)
{
    DataUtil::GameData game;
    game.Sprites = {
        { 1, DataUtil::kSpriteImport_Real, true },
        { 3, DataUtil::kSpriteImport_HighRes, false }
    };

    std::vector<uint8_t> buffer = WriteGameBlock(game,
        DataFileWriter::WriteSpriteFlags);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));
    const int count = in->ReadInt32();
    ASSERT_EQ(4, count);
    std::vector<uint8_t> flags(count);
    in->Read(flags.data(), flags.size());

    EXPECT_EQ(0, flags[0]);
    EXPECT_EQ(SPF_ALPHACHANNEL, flags[1] & SPF_ALPHACHANNEL);
    EXPECT_EQ(0, flags[2]);
    EXPECT_EQ(SPF_VAR_RESOLUTION, flags[3] & SPF_VAR_RESOLUTION);
    EXPECT_EQ(SPF_HIRES, flags[3] & SPF_HIRES);
    EXPECT_EQ(0, flags[3] & SPF_ALPHACHANNEL);
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripInteractionScriptsBlock)
{
    DataUtil::GameData game;
    game.Characters.resize(2);
    game.Inventory.resize(2);
    std::vector<uint8_t> buffer = WriteGameBlock(game,
        DataFileWriter::WriteInteractionScriptsBlock);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    // GameSetupStruct reads two characters, then inventory slots 1 and 2;
    // unused inventory slot 0 has no v361 record in this block.
    for (int i = 0; i < 4; ++i)
    {
        HError error;
        auto events = InteractionEvents::CreateFromStream_v361(in.get(), error);
        ASSERT_TRUE(error);
        ASSERT_NE(nullptr, events);
        EXPECT_TRUE(events->Events.empty());
    }
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripLipSyncBlock)
{
    DataUtil::GameData game;
    game.LipSyncFrames = { "A", "BCD" };
    std::vector<uint8_t> buffer = WriteGameBlock(game,
        DataFileWriter::WriteLipSyncBlock);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    char frames[MAXLIPSYNCFRAMES][50] = {{ 0 }};
    in->ReadArray(&frames[0][0], MAXLIPSYNCFRAMES, 50);
    EXPECT_STREQ("A", frames[0]);
    EXPECT_STREQ("BCD", frames[1]);
    EXPECT_STREQ("", frames[2]);
    EXPECT_EQ(0, frames[0][49]);
    EXPECT_EQ(0, frames[MAXLIPSYNCFRAMES - 1][49]);
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripSaveGameInfo)
{
    DataUtil::GameData game;
    game.Settings.GUIDAsString = "01234567-89ab-cdef-0123-456789abcdef";
    game.Settings.SaveGameFileExtension = "sav";
    game.Settings.SaveGameFolderName = "Round Trip Saves";
    std::vector<uint8_t> buffer = WriteGameBlock(game,
        DataFileWriter::WriteSaveGameInfo);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    char guid[MAX_GUID_LENGTH] = {};
    char extension[MAX_SG_EXT_LENGTH] = {};
    StrUtil::ReadCStrCount(guid, in.get(), MAX_GUID_LENGTH);
    StrUtil::ReadCStrCount(extension, in.get(), MAX_SG_EXT_LENGTH);
    String folder;
    folder.ReadCount(in.get(), LEGACY_MAX_SG_FOLDER_LEN);
    EXPECT_STREQ(game.Settings.GUIDAsString.GetCStr(), guid);
    EXPECT_STREQ("sav", extension);
    EXPECT_STREQ("Round Trip Saves", folder.GetCStr());
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripLegacyScriptNamesBlock)
{
    DataUtil::GameData game;
    DataUtil::ViewData first_view;
    first_view.ScriptName = "vFirst";
    DataUtil::ViewData second_view;
    second_view.TypeName = "SecondView";
    game.Views = { first_view, second_view };
    DataUtil::InventoryItemData key;
    key.ScriptName = "iKey";
    DataUtil::InventoryItemData coin;
    coin.TypeName = "CoinItem";
    game.Inventory = { key, coin };
    std::vector<uint8_t> buffer = WriteGameBlock(game,
        DataFileWriter::WriteLegacyScriptNamesBlock);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    EXPECT_STREQ("vFirst", String::FromStream(in.get()).GetCStr());
    EXPECT_STREQ("SecondView", String::FromStream(in.get()).GetCStr());
    // This is the third independent serialization of unused inventory slot 0.
    EXPECT_TRUE(String::FromStream(in.get()).IsEmpty());
    EXPECT_STREQ("iKey", String::FromStream(in.get()).GetCStr());
    EXPECT_STREQ("CoinItem", String::FromStream(in.get()).GetCStr());
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripPluginsBlock)
{
    DataUtil::GameData game;
    game.Plugins = {
        { "plugin-one", { 0, 1, 2, 255 } },
        { "empty-plugin", {} }
    };
    std::vector<uint8_t> buffer = WriteGameBlock(game,
        DataFileWriter::WritePluginsBlock);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    EXPECT_EQ(1, in->ReadInt32());
    ASSERT_EQ(2, in->ReadInt32());
    EXPECT_STREQ("plugin-one", String::FromStream(in.get()).GetCStr());
    const int data_size = in->ReadInt32();
    ASSERT_EQ(4, data_size);
    std::vector<uint8_t> data(data_size);
    in->Read(data.data(), data.size());
    EXPECT_EQ(game.Plugins[0].Data, data);
    EXPECT_STREQ("empty-plugin", String::FromStream(in.get()).GetCStr());
    EXPECT_EQ(0, in->ReadInt32());
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripRoomNamesBlock)
{
    DataUtil::GameData game;
    game.Settings.DebugMode = true;
    game.Rooms = { { 1, "Hall" }, { 42, "Secret room" } };
    std::vector<uint8_t> buffer = WriteGameBlock(game,
        DataFileWriter::WriteRoomNamesBlock);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    ASSERT_EQ(2, in->ReadInt32());
    EXPECT_EQ(1, in->ReadInt32());
    EXPECT_STREQ("Hall", String::FromStream(in.get()).GetCStr());
    EXPECT_EQ(42, in->ReadInt32());
    EXPECT_STREQ("Secret room", String::FromStream(in.get()).GetCStr());
    EXPECT_EQ(in->GetLength(), in->GetPosition());

    game.Settings.DebugMode = false;
    buffer = WriteGameBlock(game, DataFileWriter::WriteRoomNamesBlock);
    EXPECT_TRUE(buffer.empty());
}

TEST(DataFileWriter, RoundTripExt360Fonts)
{
    DataUtil::GameData game;
    DataUtil::FontData first;
    first.AutoOutlineThickness = 2;
    first.AutoOutlineStyle = DataUtil::kFontAutoOutline_Rounded;
    first.CharacterSpacing = 3;
    first.CustomHeightValue = 17;
    DataUtil::FontData second;
    second.AutoOutlineThickness = 4;
    second.CharacterSpacing = 5;
    second.CustomHeightValue = 19;
    game.Fonts = { first, second };
    std::vector<uint8_t> buffer = WriteExtensionPayload(game,
        DataFileWriter::WriteExt360Fonts);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    for (const auto &font : game.Fonts)
    {
        EXPECT_EQ(font.AutoOutlineThickness, in->ReadInt32());
        EXPECT_EQ(font.AutoOutlineStyle, in->ReadInt32());
        EXPECT_EQ(font.CharacterSpacing, in->ReadInt32());
        EXPECT_EQ(font.CustomHeightValue, in->ReadInt32());
        EXPECT_EQ(0, in->ReadInt32());
        EXPECT_EQ(0, in->ReadInt32());
    }
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripExt360Cursors)
{
    DataUtil::GameData game;
    DataUtil::CursorData first;
    first.AnimationDelay = 6;
    DataUtil::CursorData second;
    second.AnimationDelay = 12;
    game.Cursors = { first, second };
    std::vector<uint8_t> buffer = WriteExtensionPayload(game,
        DataFileWriter::WriteExt360Cursors);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    for (const auto &cursor : game.Cursors)
    {
        EXPECT_EQ(cursor.AnimationDelay, in->ReadInt32());
        EXPECT_EQ(0, in->ReadInt32());
        EXPECT_EQ(0, in->ReadInt32());
        EXPECT_EQ(0, in->ReadInt32());
    }
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripExt361ObjNames)
{
    DataUtil::GameData game;
    game.Settings.GameName = "Extended Game";
    game.Settings.SaveGameFolderName = "Extended Saves";
    DataUtil::CharacterData character;
    character.ScriptName = "cHero";
    character.RealName = "The Hero";
    game.Characters.push_back(character);
    DataUtil::InventoryItemData item;
    item.Description = "Brass key";
    game.Inventory.push_back(item);
    DataUtil::CursorData cursor;
    cursor.ScriptName = "Look";
    game.Cursors.push_back(cursor);
    DataUtil::AudioClipData clip;
    clip.ScriptName = "aTheme";
    clip.CacheFileName = "theme.ogg";
    game.AudioClips.push_back(clip);
    std::vector<uint8_t> buffer = WriteExtensionPayload(game,
        DataFileWriter::WriteExt361ObjNames);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    EXPECT_STREQ("Extended Game", StrUtil::ReadString(in.get()).GetCStr());
    EXPECT_STREQ("Extended Saves", StrUtil::ReadString(in.get()).GetCStr());
    ASSERT_EQ(1, in->ReadInt32());
    EXPECT_STREQ("cHero", StrUtil::ReadString(in.get()).GetCStr());
    EXPECT_STREQ("The Hero", StrUtil::ReadString(in.get()).GetCStr());
    ASSERT_EQ(2, in->ReadInt32());
    // Unlike the legacy v361 interaction block, this extension includes slot 0.
    EXPECT_TRUE(StrUtil::ReadString(in.get()).IsEmpty());
    EXPECT_STREQ("Brass key", StrUtil::ReadString(in.get()).GetCStr());
    ASSERT_EQ(1, in->ReadInt32());
    EXPECT_STREQ("Look", StrUtil::ReadString(in.get()).GetCStr());
    ASSERT_EQ(1, in->ReadInt32());
    EXPECT_STREQ("aTheme", StrUtil::ReadString(in.get()).GetCStr());
    EXPECT_STREQ("theme.ogg", StrUtil::ReadString(in.get()).GetCStr());
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripExt362Interactions)
{
    DataUtil::GameData game;
    DataUtil::CharacterData hero;
    hero.ScriptModule = "HeroModule";
    hero.InteractionEvents = { "LookAtHero", "TalkToHero" };
    DataUtil::CharacterData npc;
    npc.ScriptModule = "NpcModule";
    npc.InteractionEvents = { "LookAtNpc" };
    game.Characters = { hero, npc };
    DataUtil::InventoryItemData key;
    key.ScriptModule = "InventoryModule";
    key.InteractionEvents = { "UseKey" };
    game.Inventory.push_back(key);
    DataUtil::GUIData gui;
    gui.ScriptModule = "GuiModule";
    game.GUI.push_back(gui);
    std::vector<uint8_t> buffer = WriteExtensionPayload(game,
        DataFileWriter::WriteExt362Interactions);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    EXPECT_TRUE(StrUtil::ReadString(in.get()).IsEmpty()); // global script asset
    EXPECT_TRUE(StrUtil::ReadString(in.get()).IsEmpty()); // dialog script asset
    EXPECT_EQ(0, in->ReadInt32()); // separately stored script modules
    ASSERT_EQ(2, in->ReadInt32());
    HError error;
    auto loaded_hero = InteractionEvents::CreateFromStream_v362(in.get(), error);
    ASSERT_TRUE(error);
    ASSERT_NE(nullptr, loaded_hero);
    EXPECT_STREQ("HeroModule", loaded_hero->ScriptModule.GetCStr());
    ASSERT_EQ(2u, loaded_hero->Events.size());
    EXPECT_STREQ("TalkToHero", loaded_hero->Events[1].FunctionName.GetCStr());
    auto loaded_npc = InteractionEvents::CreateFromStream_v362(in.get(), error);
    ASSERT_TRUE(error);
    ASSERT_NE(nullptr, loaded_npc);
    EXPECT_STREQ("NpcModule", loaded_npc->ScriptModule.GetCStr());

    ASSERT_EQ(2, in->ReadInt32()); // unused slot 0 plus one real item
    auto unused_item = InteractionEvents::CreateFromStream_v362(in.get(), error);
    ASSERT_TRUE(error);
    ASSERT_NE(nullptr, unused_item);
    EXPECT_TRUE(unused_item->ScriptModule.IsEmpty());
    EXPECT_TRUE(unused_item->Events.empty());
    auto loaded_key = InteractionEvents::CreateFromStream_v362(in.get(), error);
    ASSERT_TRUE(error);
    ASSERT_NE(nullptr, loaded_key);
    EXPECT_STREQ("InventoryModule", loaded_key->ScriptModule.GetCStr());
    ASSERT_EQ(1u, loaded_key->Events.size());
    EXPECT_STREQ("UseKey", loaded_key->Events[0].FunctionName.GetCStr());

    ASSERT_EQ(1, in->ReadInt32());
    EXPECT_STREQ("GuiModule", StrUtil::ReadString(in.get()).GetCStr());
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripExt363GameInfo)
{
    DataUtil::GameData game;
    game.Settings.GameName = "My Game";
    game.Settings.Description = "A test game";
    game.Settings.DeveloperName = "AGS Team";
    game.Settings.DeveloperURL = "https://example.invalid";
    game.Settings.Genre = "Adventure";
    game.Settings.ReleaseDate = "2026-07-21";
    game.Settings.Version = "1.2.3";
    game.Settings.GameTextLanguage = "pt-BR";
    std::vector<uint8_t> buffer = WriteExtensionPayload(game,
        DataFileWriter::WriteExt363GameInfo);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    StringMap info;
    StrUtil::ReadStringMap(info, in.get());
    ASSERT_EQ(8u, info.size());
    EXPECT_STREQ("My Game", info["title"].GetCStr());
    EXPECT_STREQ("A test game", info["description"].GetCStr());
    EXPECT_STREQ("AGS Team", info["dev_name"].GetCStr());
    EXPECT_STREQ("https://example.invalid", info["dev_url"].GetCStr());
    EXPECT_STREQ("Adventure", info["genre"].GetCStr());
    EXPECT_STREQ("21.07.2026", info["release_date"].GetCStr());
    EXPECT_STREQ("1.2.3", info["version"].GetCStr());
    EXPECT_STREQ("pt_BR", info["text_lang"].GetCStr());
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

// GUI serialization readers are implemented together with GUI runtime code.
// The tests below use the shared runtime stubs from Common/test/common_stubs.cpp
// instead of defining a separate set of stubs for the Tools tests.

namespace
{

struct LoadedGuiBlock
{
    std::vector<GUIMain> Guis;
    GUICollection Objects;
    GuiVersion Version = kGuiVersion_Initial;
    soff_t StreamLength = 0;
    soff_t StreamPosition = 0;
};

LoadedGuiBlock ReadGuiBlock(const DataUtil::GameData &game)
{
    std::vector<uint8_t> buffer = WriteGameBlock(game,
        DataFileWriter::WriteGuiBlock);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    LoadedGuiBlock loaded;
    GUIRefCollection refs(loaded.Objects);
    HError error = GUI::ReadGUI(loaded.Guis, kGameVersion_Current,
        loaded.Version, refs, in.get());
    EXPECT_TRUE(error);
    loaded.StreamLength = in->GetLength();
    loaded.StreamPosition = in->GetPosition();
    return loaded;
}

DataUtil::GUIData MakeGui()
{
    DataUtil::GUIData gui;
    gui.ScriptName = "gMain";
    gui.OnClick = "gMain_OnClick";
    gui.Left = 11;
    gui.Top = 12;
    gui.Width = 320;
    gui.Height = 180;
    gui.PopupStyle = DataUtil::kGUIPopupStyle_MouseYPos;
    gui.PopupYPos = 23;
    gui.BackgroundColor = 24;
    gui.BackgroundImage = 25;
    gui.BorderColor = 26;
    gui.Clickable = true;
    gui.Visible = true;
    gui.Transparency = 35;
    gui.ZOrder = 4;
    return gui;
}

template <typename T>
std::shared_ptr<T> MakeControl(const char *name)
{
    auto control = std::make_shared<T>();
    control->ScriptName = name;
    control->Left = 31;
    control->Top = 32;
    control->Width = 130;
    control->Height = 40;
    control->ZOrder = 3;
    control->Clickable = true;
    control->Enabled = true;
    control->Visible = true;
    control->Translated = true;
    control->ShowBorder = true;
    control->SolidBackground = true;
    return control;
}

void ExpectControlBase(const DataUtil::GUIControlData &expected,
    const GUIObject &actual)
{
    EXPECT_STREQ(expected.ScriptName.GetCStr(), actual.GetName().GetCStr());
    EXPECT_EQ(expected.Left, actual.GetX());
    EXPECT_EQ(expected.Top, actual.GetY());
    EXPECT_EQ(expected.Width, actual.GetWidth());
    EXPECT_EQ(expected.Height, actual.GetHeight());
    EXPECT_EQ(expected.ZOrder, actual.GetZOrder());
    EXPECT_EQ(expected.Clickable, actual.IsClickable());
    EXPECT_EQ(expected.Enabled, actual.IsEnabled());
    EXPECT_EQ(expected.Visible, actual.IsVisible());
    EXPECT_EQ(expected.Translated, actual.IsTranslated());
    EXPECT_EQ(expected.ShowBorder, actual.IsShowBorder());
    EXPECT_EQ(expected.SolidBackground, actual.IsSolidBackground());
}

void SetControlLooks(DataUtil::GUIControlData &control, int base)
{
    control.BackgroundColor = base + 1;
    control.BorderColor = base + 2;
    control.BorderWidth = base + 3;
    control.PaddingX = base + 4;
    control.PaddingY = base + 5;
}

void ExpectControlLooks(const DataUtil::GUIControlData &expected,
    const GUIObject &actual)
{
    EXPECT_EQ(expected.BackgroundColor, actual.GetBackColor());
    EXPECT_EQ(expected.BorderColor, actual.GetBorderColor());
    EXPECT_EQ(expected.BorderWidth, actual.GetBorderWidth());
    EXPECT_EQ(expected.PaddingX, actual.GetPaddingX());
    EXPECT_EQ(expected.PaddingY, actual.GetPaddingY());
}

} // namespace

TEST(DataFileWriter, RoundTripGuiMain)
{
    DataUtil::GameData game;
    DataUtil::GUIData gui = MakeGui();
    gui.Controls = {
        MakeControl<DataUtil::GUIButtonData>("btn"),
        MakeControl<DataUtil::GUILabelData>("lbl"),
        MakeControl<DataUtil::GUIInventoryData>("inv"),
        MakeControl<DataUtil::GUISliderData>("sld"),
        MakeControl<DataUtil::GUITextBoxData>("txt"),
        MakeControl<DataUtil::GUIListBoxData>("lst")
    };
    game.GUI.push_back(gui);

    LoadedGuiBlock loaded = ReadGuiBlock(game);
    ASSERT_EQ(1u, loaded.Guis.size());
    const GUIMain &actual = loaded.Guis[0];
    EXPECT_EQ(kGuiVersion_Current, loaded.Version);
    EXPECT_STREQ("gMain", actual.GetName().GetCStr());
    EXPECT_STREQ("gMain_OnClick", actual.GetOnClickHandler().GetCStr());
    EXPECT_EQ(gui.Left, actual.GetX());
    EXPECT_EQ(gui.Top, actual.GetY());
    EXPECT_EQ(gui.Width, actual.GetWidth());
    EXPECT_EQ(gui.Height, actual.GetHeight());
    EXPECT_EQ(kGUIPopupMouseY, actual.GetPopupStyle());
    EXPECT_EQ(gui.PopupYPos, actual.GetPopupAtY());
    EXPECT_EQ(gui.BackgroundColor, actual.GetBgColor());
    EXPECT_EQ(gui.BackgroundImage, actual.GetBgImage());
    EXPECT_EQ(gui.BorderColor, actual.GetFgColor());
    EXPECT_TRUE(actual.IsClickable());
    EXPECT_TRUE(actual.IsVisible());
    EXPECT_EQ(GfxDef::Trans100ToLegacyTrans255(gui.Transparency),
        actual.GetTransparency());
    EXPECT_EQ(gui.ZOrder, actual.GetZOrder());
    // ReadGUI restores serialized references; runtime control pointers are
    // connected later by GUI::RebuildGUI() during game initialization.
    ASSERT_EQ(6u, actual.GetControlRefs().size());
    EXPECT_EQ(kGUIButton, actual.GetControlType(0));
    EXPECT_EQ(kGUILabel, actual.GetControlType(1));
    EXPECT_EQ(kGUIInvWindow, actual.GetControlType(2));
    EXPECT_EQ(kGUISlider, actual.GetControlType(3));
    EXPECT_EQ(kGUITextBox, actual.GetControlType(4));
    EXPECT_EQ(kGUIListBox, actual.GetControlType(5));
    for (size_t i = 0; i < actual.GetControlRefs().size(); ++i)
        EXPECT_EQ(0, actual.GetControlID(i));
    EXPECT_EQ(loaded.StreamLength, loaded.StreamPosition);
}

TEST(DataFileWriter, RoundTripGuiButton)
{
    DataUtil::GameData game;
    DataUtil::GUIData gui = MakeGui();
    auto button = MakeControl<DataUtil::GUIButtonData>("btnPlay");
    button->ClickAction = "RunScript";
    button->ClipImage = true;
    button->WrapText = true;
    button->Font = 2;
    button->Image = 41;
    button->MouseoverImage = 42;
    button->PushedImage = 43;
    button->NewModeNumber = 7;
    button->OnClick = "btnPlay_OnClick";
    button->Text = "Play";
    button->TextAlignment = kAlignBottomRight;
    button->TextColor = 15;
    gui.Controls.push_back(button);
    game.GUI.push_back(gui);

    LoadedGuiBlock loaded = ReadGuiBlock(game);
    ASSERT_EQ(1u, loaded.Objects.Buttons.size());
    const GUIButton &actual = loaded.Objects.Buttons[0];
    ExpectControlBase(*button, actual);
    EXPECT_TRUE(actual.IsClippingImage());
    EXPECT_TRUE(actual.IsWrapText());
    EXPECT_EQ(button->Image, actual.GetNormalImage());
    EXPECT_EQ(button->MouseoverImage, actual.GetMouseOverImage());
    EXPECT_EQ(button->PushedImage, actual.GetPushedImage());
    EXPECT_EQ(button->Font, actual.GetFont());
    EXPECT_EQ(button->TextColor, actual.GetTextColor());
    EXPECT_EQ(kGUIAction_RunScript, actual.GetClickAction(kGUIClickLeft));
    EXPECT_EQ(button->NewModeNumber, actual.GetClickData(kGUIClickLeft));
    EXPECT_STREQ(button->Text.GetCStr(), actual.GetText().GetCStr());
    EXPECT_EQ(button->TextAlignment, actual.GetTextAlignment());
    EXPECT_STREQ(button->OnClick.GetCStr(), actual.GetEventHandler(0).GetCStr());
    EXPECT_EQ(loaded.StreamLength, loaded.StreamPosition);
}

TEST(DataFileWriter, RoundTripGuiLabel)
{
    DataUtil::GameData game;
    DataUtil::GUIData gui = MakeGui();
    auto label = MakeControl<DataUtil::GUILabelData>("lblScore");
    label->Text = "Score: @score@";
    label->Font = 3;
    label->TextColor = 16;
    label->TextAlignment = kAlignMiddleCenter;
    gui.Controls.push_back(label);
    game.GUI.push_back(gui);

    LoadedGuiBlock loaded = ReadGuiBlock(game);
    ASSERT_EQ(1u, loaded.Objects.Labels.size());
    const GUILabel &actual = loaded.Objects.Labels[0];
    ExpectControlBase(*label, actual);
    EXPECT_STREQ(label->Text.GetCStr(), actual.GetText().GetCStr());
    EXPECT_EQ(label->Font, actual.GetFont());
    EXPECT_EQ(label->TextColor, actual.GetTextColor());
    EXPECT_EQ(label->TextAlignment, actual.GetTextAlignment());
    EXPECT_EQ(loaded.StreamLength, loaded.StreamPosition);
}

TEST(DataFileWriter, RoundTripGuiInvWindow)
{
    DataUtil::GameData game;
    DataUtil::GUIData gui = MakeGui();
    auto inv = MakeControl<DataUtil::GUIInventoryData>("invMain");
    inv->CharacterID = 5;
    inv->ItemWidth = 33;
    inv->ItemHeight = 22;
    gui.Controls.push_back(inv);
    game.GUI.push_back(gui);

    LoadedGuiBlock loaded = ReadGuiBlock(game);
    ASSERT_EQ(1u, loaded.Objects.InvWindows.size());
    const GUIInvWindow &actual = loaded.Objects.InvWindows[0];
    ExpectControlBase(*inv, actual);
    EXPECT_EQ(inv->ItemWidth, actual.GetItemWidth());
    EXPECT_EQ(inv->ItemHeight, actual.GetItemHeight());
    EXPECT_EQ(loaded.StreamLength, loaded.StreamPosition);
}

TEST(DataFileWriter, RoundTripGuiSlider)
{
    DataUtil::GameData game;
    DataUtil::GUIData gui = MakeGui();
    auto slider = MakeControl<DataUtil::GUISliderData>("sldVolume");
    slider->MinValue = 10;
    slider->MaxValue = 90;
    slider->Value = 55;
    slider->HandleImage = 51;
    slider->HandleOffset = 4;
    slider->BackgroundImage = 52;
    slider->OnChange = "sldVolume_OnChange";
    gui.Controls.push_back(slider);
    game.GUI.push_back(gui);

    LoadedGuiBlock loaded = ReadGuiBlock(game);
    ASSERT_EQ(1u, loaded.Objects.Sliders.size());
    const GUISlider &actual = loaded.Objects.Sliders[0];
    ExpectControlBase(*slider, actual);
    EXPECT_EQ(slider->MinValue, actual.GetMinValue());
    EXPECT_EQ(slider->MaxValue, actual.GetMaxValue());
    EXPECT_EQ(slider->Value, actual.GetValue());
    EXPECT_EQ(slider->HandleImage, actual.GetHandleImage());
    EXPECT_EQ(slider->HandleOffset, actual.GetHandleOffset());
    EXPECT_EQ(slider->BackgroundImage, actual.GetBgImage());
    EXPECT_STREQ(slider->OnChange.GetCStr(), actual.GetEventHandler(0).GetCStr());
    EXPECT_EQ(loaded.StreamLength, loaded.StreamPosition);
}

TEST(DataFileWriter, RoundTripGuiTextBox)
{
    DataUtil::GameData game;
    DataUtil::GUIData gui = MakeGui();
    auto text_box = MakeControl<DataUtil::GUITextBoxData>("txtName");
    text_box->Text = "Arthur";
    text_box->Font = 4;
    text_box->TextColor = 17;
    text_box->ShowBorder = true;
    text_box->OnActivate = "txtName_OnActivate";
    gui.Controls.push_back(text_box);
    game.GUI.push_back(gui);

    LoadedGuiBlock loaded = ReadGuiBlock(game);
    ASSERT_EQ(1u, loaded.Objects.TextBoxes.size());
    const GUITextBox &actual = loaded.Objects.TextBoxes[0];
    ExpectControlBase(*text_box, actual);
    EXPECT_STREQ(text_box->Text.GetCStr(), actual.GetText().GetCStr());
    EXPECT_EQ(text_box->Font, actual.GetFont());
    EXPECT_EQ(text_box->TextColor, actual.GetTextColor());
    EXPECT_NE(0, actual.GetTextBoxFlags() & kTextBox_ShowBorder);
    EXPECT_STREQ(text_box->OnActivate.GetCStr(), actual.GetEventHandler(0).GetCStr());
    EXPECT_EQ(loaded.StreamLength, loaded.StreamPosition);
}

TEST(DataFileWriter, RoundTripGuiListBox)
{
    DataUtil::GameData game;
    DataUtil::GUIData gui = MakeGui();
    auto list_box = MakeControl<DataUtil::GUIListBoxData>("lstSaves");
    list_box->Font = 5;
    list_box->TextColor = 18;
    list_box->SelectedTextColor = 19;
    list_box->SelectedBackgroundColor = 20;
    list_box->ShowBorder = true;
    list_box->ShowScrollArrows = true;
    list_box->TextAlignment = kHAlignCenter;
    list_box->OnSelectionChanged = "lstSaves_OnSelectionChanged";
    gui.Controls.push_back(list_box);
    game.GUI.push_back(gui);

    LoadedGuiBlock loaded = ReadGuiBlock(game);
    ASSERT_EQ(1u, loaded.Objects.ListBoxes.size());
    const GUIListBox &actual = loaded.Objects.ListBoxes[0];
    ExpectControlBase(*list_box, actual);
    EXPECT_EQ(0u, actual.GetItemCount());
    EXPECT_EQ(list_box->Font, actual.GetFont());
    EXPECT_EQ(list_box->TextColor, actual.GetTextColor());
    EXPECT_EQ(list_box->SelectedTextColor, actual.GetSelectedTextColor());
    EXPECT_EQ(list_box->SelectedBackgroundColor, actual.GetSelectedBgColor());
    EXPECT_NE(0, actual.GetListBoxFlags() & kListBox_ShowBorder);
    EXPECT_NE(0, actual.GetListBoxFlags() & kListBox_ShowArrows);
    EXPECT_EQ(static_cast<FrameAlignment>(list_box->TextAlignment),
        actual.GetTextAlignment());
    EXPECT_STREQ(list_box->OnSelectionChanged.GetCStr(),
        actual.GetEventHandler(0).GetCStr());
    EXPECT_EQ(loaded.StreamLength, loaded.StreamPosition);
}

TEST(DataFileWriter, RoundTripGuiControlLooks363)
{
    DataUtil::GUIControlData control;
    SetControlLooks(control, 60);
    std::vector<uint8_t> buffer;
    auto out = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer, kStream_Write));
    DataFileWriter::WriteGuiControlLooks363(out.get(), control);
    out.reset();
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    GUIObject loaded;
    loaded.ReadFromFile_Ext363(in.get(), kGuiVersion_Current);
    ExpectControlLooks(control, loaded);
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}

TEST(DataFileWriter, RoundTripExt363GuiControls)
{
    // The real extension loader runs after ReadGUI has established this
    // context. GUIListBox uses it while updating metrics after extension data.
    GUI::GameGuiVersion = kGuiVersion_Current;

    DataUtil::GameData game;
    DataUtil::GUIData gui = MakeGui();
    auto button = MakeControl<DataUtil::GUIButtonData>("button");
    auto label = MakeControl<DataUtil::GUILabelData>("label");
    auto inv = MakeControl<DataUtil::GUIInventoryData>("inv");
    auto slider = MakeControl<DataUtil::GUISliderData>("slider");
    auto text_box = MakeControl<DataUtil::GUITextBoxData>("textbox");
    auto list_box = MakeControl<DataUtil::GUIListBoxData>("listbox");
    SetControlLooks(*button, 100);
    SetControlLooks(*label, 110);
    SetControlLooks(*inv, 120);
    SetControlLooks(*slider, 130);
    SetControlLooks(*text_box, 140);
    SetControlLooks(*list_box, 150);
    button->ColorStyle = DataUtil::kButtonColor_DynamicFlat;
    button->BorderShadeColor = 201;
    button->MouseOverBackgroundColor = 202;
    button->PushedBackgroundColor = 203;
    button->MouseOverBorderColor = 204;
    button->PushedBorderColor = 205;
    button->MouseOverTextColor = 206;
    button->PushedTextColor = 207;
    button->TextOutlineColor = 208;
    label->TextOutlineColor = 211;
    slider->HandleColor = 221;
    slider->BorderShadeColor = 222;
    text_box->TextAlignment = kAlignBottomCenter;
    text_box->TextOutlineColor = 231;
    list_box->TextOutlineColor = 241;
    gui.Controls = { button, label, inv, slider, text_box, list_box };
    game.GUI.push_back(gui);
    std::vector<uint8_t> buffer = WriteExtensionPayload(game,
        DataFileWriter::WriteExt363GuiControls);
    auto in = std::make_unique<Stream>(
        std::make_unique<VectorStream>(buffer));

    ASSERT_EQ(1, in->ReadInt32());
    GUIButton loaded_button;
    loaded_button.ReadFromFile_Ext363(in.get(), kGuiVersion_Current);
    ExpectControlLooks(*button, loaded_button);
    EXPECT_TRUE(loaded_button.IsDynamicColors());
    EXPECT_TRUE(loaded_button.IsFlatStyle());
    EXPECT_EQ(button->BorderShadeColor, loaded_button.GetBorderShadeColor());
    EXPECT_EQ(button->MouseOverBackgroundColor, loaded_button.GetMouseOverBackColor());
    EXPECT_EQ(button->PushedBackgroundColor, loaded_button.GetPushedBackColor());
    EXPECT_EQ(button->MouseOverBorderColor, loaded_button.GetMouseOverBorderColor());
    EXPECT_EQ(button->PushedBorderColor, loaded_button.GetPushedBorderColor());
    EXPECT_EQ(button->MouseOverTextColor, loaded_button.GetMouseOverTextColor());
    EXPECT_EQ(button->PushedTextColor, loaded_button.GetPushedTextColor());

    ASSERT_EQ(1, in->ReadInt32());
    GUILabel loaded_label;
    loaded_label.ReadFromFile_Ext363(in.get(), kGuiVersion_Current);
    ExpectControlLooks(*label, loaded_label);
    ASSERT_EQ(1, in->ReadInt32());
    GUIInvWindow loaded_inv;
    loaded_inv.ReadFromFile_Ext363(in.get(), kGuiVersion_Current);
    ExpectControlLooks(*inv, loaded_inv);
    ASSERT_EQ(1, in->ReadInt32());
    GUISlider loaded_slider;
    loaded_slider.ReadFromFile_Ext363(in.get(), kGuiVersion_Current);
    ExpectControlLooks(*slider, loaded_slider);
    EXPECT_EQ(slider->HandleColor, loaded_slider.GetHandleColor());
    EXPECT_EQ(slider->BorderShadeColor, loaded_slider.GetBorderShadeColor());
    ASSERT_EQ(1, in->ReadInt32());
    GUITextBox loaded_text_box;
    loaded_text_box.ReadFromFile_Ext363(in.get(), kGuiVersion_Current);
    ExpectControlLooks(*text_box, loaded_text_box);
    EXPECT_EQ(text_box->TextAlignment, loaded_text_box.GetTextAlignment());
    ASSERT_EQ(1, in->ReadInt32());
    GUIListBox loaded_list_box;
    loaded_list_box.ReadFromFile_Ext363(in.get(), kGuiVersion_Current);
    ExpectControlLooks(*list_box, loaded_list_box);
    EXPECT_EQ(in->GetLength(), in->GetPosition());
}
