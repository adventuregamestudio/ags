//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#include "util/android_file.h"
#if AGS_PLATFORM_OS_ANDROID
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <SDL.h>

namespace AGS
{
namespace Common
{

static jobject gAAssetManagerRef { nullptr };
static AAssetManager *gAAssetManager { nullptr };

void InitAndroidFile()
{
    assert(gAAssetManager == nullptr);
    if (gAAssetManager)
        return; // already initialized

    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();

    jobject activity = (jobject)SDL_AndroidGetActivity();

    jclass activity_class = env->GetObjectClass(activity);

    jmethodID activity_class_getAssets = env->GetMethodID(activity_class, "getAssets", "()Landroid/content/res/AssetManager;");
    jobject asset_manager = env->CallObjectMethod(activity, activity_class_getAssets); // activity.getAssets();
    gAAssetManagerRef = env->NewGlobalRef(asset_manager);
    if (!gAAssetManagerRef)
        return; // error
    gAAssetManager = AAssetManager_fromJava(env, gAAssetManagerRef);
    if (!gAAssetManager)
    {
        env->DeleteGlobalRef(gAAssetManagerRef);
        gAAssetManagerRef = nullptr;
    }
}

void ShutdownAndroidFile()
{
    if (gAAssetManager)
    {
        JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
        env->DeleteGlobalRef(gAAssetManagerRef);
        gAAssetManagerRef = nullptr;
        gAAssetManager = nullptr;
    }
}

AAssetManager* GetAAssetManager()
{
    if (!gAAssetManager) { InitAndroidFile(); }
    assert(gAAssetManager);
    return gAAssetManager;
}

} // namespace Common
} // namespace AGS
#endif // AGS_PLATFORM_OS_ANDROID
