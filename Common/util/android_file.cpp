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
#include "util/android_file.h"
#if AGS_PLATFORM_OS_ANDROID
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <SDL.h>
#include "util/path.h"

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
    if(SDL_WasInit(SDL_INIT_EVERYTHING) == 0)
        return; // SDL has not been initialized yet

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
    return gAAssetManager;
}

bool GetAAssetExists(const String &filename)
{
    AAssetManager* mgr = GetAAssetManager();
    if(mgr == nullptr) return false;
    String a_filename = Path::GetPathInForeignAsset(filename);
    // TODO: find out if it's acceptable to open/close asset to only test its existance;
    // the alternative is to use AAssetManager_openDir and read asset list using AAssetDir_getNextFileName
    AAsset *asset = AAssetManager_open(mgr, a_filename.GetCStr(), AASSET_MODE_UNKNOWN);
    if (!asset) return false;
    AAsset_close(asset);
    return true;
}

soff_t GetAAssetSize(const String &filename)
{
    AAssetManager* mgr = GetAAssetManager();
    if(mgr == nullptr) return -1;
    String a_filename = Path::GetPathInForeignAsset(filename);
    AAsset *asset = AAssetManager_open(mgr, a_filename.GetCStr(), AASSET_MODE_UNKNOWN);
    if (!asset) return -1;
    soff_t len = AAsset_getLength64(asset);
    AAsset_close(asset);
    return len;
}


AndroidADir::AndroidADir(AndroidADir &&aadir)
{
    _dir = aadir._dir;
    aadir._dir = nullptr;
}

AndroidADir::AndroidADir(const String &dirname)
{
    AAssetManager* mgr = GetAAssetManager();
    if(mgr == nullptr) {_dir = nullptr; return; }
    String assetDirName = Path::GetPathInForeignAsset(dirname);
    _dir = AAssetManager_openDir(mgr, assetDirName.GetCStr());
}

AndroidADir::~AndroidADir()
{
    if (_dir)
        AAssetDir_close(_dir);
}

String AndroidADir::Next()
{
    if (_dir)
        return AAssetDir_getNextFileName(_dir);
    return "";
}

String AndroidADir::Next(const std::regex &pattern)
{
    if (!_dir) return "";
    const char *filename = nullptr;
    std::cmatch mr;
    while ((filename = AAssetDir_getNextFileName(_dir)) != nullptr)
    {
        if (std::regex_match(filename, mr, pattern))
            return filename;
    }
    return "";
}

} // namespace Common
} // namespace AGS
#endif // AGS_PLATFORM_OS_ANDROID
