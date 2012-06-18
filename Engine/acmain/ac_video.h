
void play_flc_file(int numb,int playflags);

int theora_playing_callback(BITMAP *theoraBuffer);
void calculate_destination_size_maintain_aspect_ratio(int vidWidth, int vidHeight, int *targetWidth, int *targetHeight);
void play_theora_video(const char *name, int skip, int flags);
void pause_sound_if_necessary_and_play_video(const char *name, int skip, int flags);
void scrPlayVideo(const char* name, int skip, int flags);
