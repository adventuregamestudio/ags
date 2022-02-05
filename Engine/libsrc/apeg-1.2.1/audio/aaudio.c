#include "mpeg1dec.h"


int _apeg_audio_flush(APEG_LAYER *layer)
{
	layer->audio.pos = -1;
	layer->audio.pcm.point = 0;
	layer->stream.audio.flushed = TRUE;

	return 0;
}

int _apeg_audio_close(APEG_LAYER *layer)
{
	layer->audio.voice = -1;

	return 0;
}
