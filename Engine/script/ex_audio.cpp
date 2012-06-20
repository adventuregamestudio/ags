
//=============================================================================
//
// Exporting Audio script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_audio_script_functions()
{
    ccAddExternalSymbol("AudioChannel::Seek^1",(void *)AudioChannel_Seek);
    ccAddExternalSymbol("AudioChannel::SetRoomLocation^2",(void *)AudioChannel_SetRoomLocation);
    ccAddExternalSymbol("AudioChannel::Stop^0",(void *)AudioChannel_Stop);
    ccAddExternalSymbol("AudioChannel::get_ID",(void *)AudioChannel_GetID);
    ccAddExternalSymbol("AudioChannel::get_IsPlaying",(void *)AudioChannel_GetIsPlaying);
    ccAddExternalSymbol("AudioChannel::get_LengthMs",(void *)AudioChannel_GetLengthMs);
    ccAddExternalSymbol("AudioChannel::get_Panning",(void *)AudioChannel_GetPanning);
    ccAddExternalSymbol("AudioChannel::set_Panning",(void *)AudioChannel_SetPanning);
    ccAddExternalSymbol("AudioChannel::get_PlayingClip",(void *)AudioChannel_GetPlayingClip);
    ccAddExternalSymbol("AudioChannel::get_Position",(void *)AudioChannel_GetPosition);
    ccAddExternalSymbol("AudioChannel::get_PositionMs",(void *)AudioChannel_GetPositionMs);
    ccAddExternalSymbol("AudioChannel::get_Volume",(void *)AudioChannel_GetVolume);
    ccAddExternalSymbol("AudioChannel::set_Volume",(void *)AudioChannel_SetVolume);

    ccAddExternalSymbol("AudioClip::Play^2",(void *)AudioClip_Play);
    ccAddExternalSymbol("AudioClip::PlayFrom^3",(void *)AudioClip_PlayFrom);
    ccAddExternalSymbol("AudioClip::PlayQueued^2",(void *)AudioClip_PlayQueued);
    ccAddExternalSymbol("AudioClip::Stop^0",(void *)AudioClip_Stop);
    ccAddExternalSymbol("AudioClip::get_FileType",(void *)AudioClip_GetFileType);
    ccAddExternalSymbol("AudioClip::get_IsAvailable",(void *)AudioClip_GetIsAvailable);
    ccAddExternalSymbol("AudioClip::get_Type",(void *)AudioClip_GetType);

    ccAddExternalSymbol("Game::IsAudioPlaying^1",(void *)Game_IsAudioPlaying);
    ccAddExternalSymbol("Game::SetAudioTypeSpeechVolumeDrop^2", (void*)Game_SetAudioTypeSpeechVolumeDrop);
    ccAddExternalSymbol("Game::SetAudioTypeVolume^3", (void*)Game_SetAudioTypeVolume);
    ccAddExternalSymbol("Game::StopAudio^1",(void *)Game_StopAudio);

    ccAddExternalSymbol("System::get_AudioChannelCount", (void*)System_GetAudioChannelCount);
    ccAddExternalSymbol("System::geti_AudioChannels", (void*)System_GetAudioChannels);
}
