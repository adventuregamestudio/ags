
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
    ccAddExternalObjectFunction("AudioChannel::Seek^1",(void *)AudioChannel_Seek);
    ccAddExternalObjectFunction("AudioChannel::SetRoomLocation^2",(void *)AudioChannel_SetRoomLocation);
    ccAddExternalObjectFunction("AudioChannel::Stop^0",(void *)AudioChannel_Stop);
    ccAddExternalObjectFunction("AudioChannel::get_ID",(void *)AudioChannel_GetID);
    ccAddExternalObjectFunction("AudioChannel::get_IsPlaying",(void *)AudioChannel_GetIsPlaying);
    ccAddExternalObjectFunction("AudioChannel::get_LengthMs",(void *)AudioChannel_GetLengthMs);
    ccAddExternalObjectFunction("AudioChannel::get_Panning",(void *)AudioChannel_GetPanning);
    ccAddExternalObjectFunction("AudioChannel::set_Panning",(void *)AudioChannel_SetPanning);
    ccAddExternalObjectFunction("AudioChannel::get_PlayingClip",(void *)AudioChannel_GetPlayingClip);
    ccAddExternalObjectFunction("AudioChannel::get_Position",(void *)AudioChannel_GetPosition);
    ccAddExternalObjectFunction("AudioChannel::get_PositionMs",(void *)AudioChannel_GetPositionMs);
    ccAddExternalObjectFunction("AudioChannel::get_Volume",(void *)AudioChannel_GetVolume);
    ccAddExternalObjectFunction("AudioChannel::set_Volume",(void *)AudioChannel_SetVolume);

    ccAddExternalObjectFunction("AudioClip::Play^2",(void *)AudioClip_Play);
    ccAddExternalObjectFunction("AudioClip::PlayFrom^3",(void *)AudioClip_PlayFrom);
    ccAddExternalObjectFunction("AudioClip::PlayQueued^2",(void *)AudioClip_PlayQueued);
    ccAddExternalObjectFunction("AudioClip::Stop^0",(void *)AudioClip_Stop);
    ccAddExternalObjectFunction("AudioClip::get_FileType",(void *)AudioClip_GetFileType);
    ccAddExternalObjectFunction("AudioClip::get_IsAvailable",(void *)AudioClip_GetIsAvailable);
    ccAddExternalObjectFunction("AudioClip::get_Type",(void *)AudioClip_GetType);

    ccAddExternalStaticFunction("Game::IsAudioPlaying^1",(void *)Game_IsAudioPlaying);
    ccAddExternalStaticFunction("Game::SetAudioTypeSpeechVolumeDrop^2", (void*)Game_SetAudioTypeSpeechVolumeDrop);
    ccAddExternalStaticFunction("Game::SetAudioTypeVolume^3", (void*)Game_SetAudioTypeVolume);
    ccAddExternalStaticFunction("Game::StopAudio^1",(void *)Game_StopAudio);

    ccAddExternalStaticFunction("System::get_AudioChannelCount", (void*)System_GetAudioChannelCount);
    ccAddExternalStaticFunction("System::geti_AudioChannels", (void*)System_GetAudioChannels);
}
