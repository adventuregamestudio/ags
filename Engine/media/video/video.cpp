
#include "wgt2allg.h"
#include "video.h"
#include "ali3d.h"
#include "apeg.h"
#include "acmain/ac_maindefines.h"
#include "ac/ac_gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "acmain/ac_draw.h"
#include "acmain/ac_file.h"
#include "acmain/ac_message.h"
#include "acmain/ac_mouse.h"
#include "acmain/ac_record.h"
#include "media/audio/audio.h"
#include "platform/agsplatformdriver.h"


extern GameSetupStruct game;
extern GameState play;

// defined in ac_screen
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern int scrnwid,scrnhit;


#if !defined(IOS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
extern int psp_video_framedrop;
#endif


// FLIC player start
block fli_buffer;
short fliwidth,fliheight;
int canabort=0, stretch_flc = 1;
block hicol_buf=NULL;
IDriverDependantBitmap *fli_ddb;
BITMAP *fli_target;
int fliTargetWidth, fliTargetHeight;
int check_if_user_input_should_cancel_video()
{
    NEXT_ITERATION();
    if (kbhit()) {
        if ((getch()==27) && (canabort==1))
            return 1;
        if (canabort >= 2)
            return 1;  // skip on any key
    }
    if (canabort == 3) {  // skip on mouse click
        if (mgetbutton()!=NONE) return 1;
    }
    return 0;
}

#if defined(WINDOWS_VERSION)
int __cdecl fli_callback() {
#elif defined(LINUX_VERSION) || defined(MAC_VERSION)
extern "C" int fli_callback() {
#else
int fli_callback(...) {
#endif
    block usebuf = fli_buffer;

    update_polled_stuff_and_crossfade ();

    if (game.color_depth > 1) {
        blit(fli_buffer,hicol_buf,0,0,0,0,fliwidth,fliheight);
        usebuf=hicol_buf;
    }
    if (stretch_flc == 0)
        blit(usebuf, fli_target, 0,0,scrnwid/2-fliwidth/2,scrnhit/2-fliheight/2,scrnwid,scrnhit);
    else 
        stretch_blit(usebuf, fli_target, 0,0,fliwidth,fliheight,0,0,scrnwid,scrnhit);

    gfxDriver->UpdateDDBFromBitmap(fli_ddb, fli_target, false);
    gfxDriver->DrawSprite(0, 0, fli_ddb);
    render_to_screen(fli_target, 0, 0);

    return check_if_user_input_should_cancel_video();
}


// FLIC player end

int theora_playing_callback(BITMAP *theoraBuffer)
{
    if (theoraBuffer == NULL)
    {
        // No video, only sound
        return check_if_user_input_should_cancel_video();
    }

    int drawAtX = 0, drawAtY = 0;
    if (fli_ddb == NULL)
    {
        fli_ddb = gfxDriver->CreateDDBFromBitmap(theoraBuffer, false, true);
    }
    if (stretch_flc) 
    {
        drawAtX = scrnwid / 2 - fliTargetWidth / 2;
        drawAtY = scrnhit / 2 - fliTargetHeight / 2;
        if (!gfxDriver->HasAcceleratedStretchAndFlip())
        {
            stretch_blit(theoraBuffer, fli_target, 0, 0, theoraBuffer->w, theoraBuffer->h, 
                drawAtX, drawAtY, fliTargetWidth, fliTargetHeight);
            gfxDriver->UpdateDDBFromBitmap(fli_ddb, fli_target, false);
            drawAtX = 0;
            drawAtY = 0;
        }
        else
        {
            gfxDriver->UpdateDDBFromBitmap(fli_ddb, theoraBuffer, false);
            fli_ddb->SetStretch(fliTargetWidth, fliTargetHeight);
        }
    }
    else
    {
        gfxDriver->UpdateDDBFromBitmap(fli_ddb, theoraBuffer, false);
        drawAtX = scrnwid / 2 - theoraBuffer->w / 2;
        drawAtY = scrnhit / 2 - theoraBuffer->h / 2;
    }

    gfxDriver->DrawSprite(drawAtX, drawAtY, fli_ddb);
    render_to_screen(virtual_screen, 0, 0);
    update_polled_stuff_and_crossfade ();

    return check_if_user_input_should_cancel_video();
}


APEG_STREAM* get_theora_size(const char *fileName, int *width, int *height)
{
    APEG_STREAM* oggVid = apeg_open_stream(fileName);
    if (oggVid != NULL)
    {
        apeg_get_video_size(oggVid, width, height);
    }
    else
    {
        *width = 0;
        *height = 0;
    }
    return oggVid;
}

void calculate_destination_size_maintain_aspect_ratio(int vidWidth, int vidHeight, int *targetWidth, int *targetHeight)
{
    float aspectRatioVideo = (float)vidWidth / (float)vidHeight;
    float aspectRatioScreen = (float)scrnwid / (float)scrnhit;

    if (aspectRatioVideo == aspectRatioScreen)
    {
        *targetWidth = scrnwid;
        *targetHeight = scrnhit;
    }
    else if (aspectRatioVideo > aspectRatioScreen)
    {
        *targetWidth = scrnwid;
        *targetHeight = (int)(((float)scrnwid / aspectRatioVideo) + 0.5f);
    }
    else
    {
        *targetHeight = scrnhit;
        *targetWidth = (float)scrnhit * aspectRatioVideo;
    }

}

void play_theora_video(const char *name, int skip, int flags)
{
    apeg_set_display_depth(bitmap_color_depth(screen));
    // we must disable length detection, otherwise it takes ages to start
    // playing if the file is large because it seeks through the whole thing
    apeg_disable_length_detection(TRUE);
    // Disable framedrop because it can lead to the PSP not playing the video at all.
    apeg_enable_framedrop(psp_video_framedrop);
    update_polled_stuff_if_runtime();

    stretch_flc = (flags % 10);
    canabort = skip;
    apeg_ignore_audio((flags >= 10) ? 1 : 0);

    int videoWidth, videoHeight;
    APEG_STREAM *oggVid = get_theora_size(name, &videoWidth, &videoHeight);

    if (videoWidth == 0)
    {
        Display("Unable to load theora video '%s'", name);
        return;
    }

    if (flags < 10)
    {
        stop_all_sound_and_music();
    }

    fli_target = NULL;
    //fli_buffer = create_bitmap_ex(final_col_dep, videoWidth, videoHeight);
    calculate_destination_size_maintain_aspect_ratio(videoWidth, videoHeight, &fliTargetWidth, &fliTargetHeight);

    if ((fliTargetWidth == videoWidth) && (fliTargetHeight == videoHeight) && (stretch_flc))
    {
        // don't need to stretch after all
        stretch_flc = 0;
    }

    if ((stretch_flc) && (!gfxDriver->HasAcceleratedStretchAndFlip()))
    {
        fli_target = create_bitmap_ex(final_col_dep, scrnwid, scrnhit);
        clear(fli_target);
        fli_ddb = gfxDriver->CreateDDBFromBitmap(fli_target, false, true);
    }
    else
    {
        fli_ddb = NULL;
    }

    update_polled_stuff_if_runtime();

    clear(virtual_screen);

    if (apeg_play_apeg_stream(oggVid, NULL, 0, theora_playing_callback) == APEG_ERROR)
    {
        Display("Error playing theora video '%s'", name);
    }
    apeg_close_stream(oggVid);

    //destroy_bitmap(fli_buffer);
    if (fli_target != NULL)
        destroy_bitmap(fli_target);
    gfxDriver->DestroyDDB(fli_ddb);
    fli_ddb = NULL;
    invalidate_screen();
}

void pause_sound_if_necessary_and_play_video(const char *name, int skip, int flags)
{
    int musplaying = play.cur_music_number, i;
    int ambientWas[MAX_SOUND_CHANNELS];
    for (i = 1; i < MAX_SOUND_CHANNELS; i++)
        ambientWas[i] = ambient[i].channel;

    if ((strlen(name) > 3) && (stricmp(&name[strlen(name) - 3], "ogv") == 0))
    {
        play_theora_video(name, skip, flags);
    }
    else
    {
        char videoFilePath[MAX_PATH];
        get_current_dir_path(videoFilePath, name);

        platform->PlayVideo(videoFilePath, skip, flags);
    }

    if (flags < 10) 
    {
        update_music_volume();
        // restart the music
        if (musplaying >= 0)
            newmusic (musplaying);
        for (i = 1; i < MAX_SOUND_CHANNELS; i++) {
            if (ambientWas[i] > 0)
                PlayAmbientSound(ambientWas[i], ambient[i].num, ambient[i].vol, ambient[i].x, ambient[i].y);
        }
    }
}
