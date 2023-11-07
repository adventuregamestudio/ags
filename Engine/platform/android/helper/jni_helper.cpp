//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "core/platform.h"

#if AGS_PLATFORM_OS_ANDROID
#include <jni.h>
#include <allegro.h>
#include "game/main_game_file.h"

using namespace AGS::Common;

extern "C"
JNIEXPORT jstring JNICALL
Java_uk_co_adventuregamestudio_runtime_NativeHelper_findGameDataInDirectory(
    JNIEnv* env, jobject object, jstring path)
{
    auto cpath = env->GetStringUTFChars(path, nullptr);
    // We have to configure parts of allegro lib because
    // - we call al_find* during the search
    // - allegro might not have been initialised yet
    int tmp_errno = 0;
    int *orig_allegro_errno = allegro_errno;
    allegro_errno = &tmp_errno;
    set_filename_encoding(U_UNICODE);

    auto result_str = FindGameData(cpath);

    allegro_errno = orig_allegro_errno;
    env->ReleaseStringUTFChars(path, cpath);

    if (result_str.IsEmpty()) return nullptr;
    return env->NewStringUTF(result_str.GetCStr());
}

#endif // AGS_PLATFORM_OS_ANDROID
