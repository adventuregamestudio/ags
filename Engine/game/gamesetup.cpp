
#include "game/gamesetup.h"
#include "allegro.h"

namespace AGS
{
namespace Engine
{

GameSetup::GameSetup()
{
    InitDefaults();
}

GameSetup::~GameSetup()
{
}

void GameSetup::InitDefaults()
{
    DigitalSoundCard = DIGI_AUTODETECT;
    MidiSoundCard = MIDI_AUTODETECT;
    ModPlayer = true;
    Mp3Player = true;
    TextHeight = 0;
    WantLetterbox = false;
    Windowed = 0;
    BaseWidth = 320;
    BaseHeight = 200;
    RefreshRate = 0;
    NoSpeechPack = false;
    EnableAntiAliasing = false;
    ForceHicolorMode = false;
    DisableExceptionHandling = false;
    EnableSideBorders = true;
    DataFilesDir.Empty();
    MainDataFilename.Empty();
    Translation.Empty();
    GfxFilterID.Empty();
    GfxDriverID.Empty();
}

} // namespace Engine
} // namespace AGS
