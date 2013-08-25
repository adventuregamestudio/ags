#include <stdio.h>
#include <string.h>

#include <jni.h>
#include <android/log.h>
#include "../Engine/platform/util/pe.h"

void android_debug_printf(char* format, ...)
{
  char buffer[200];
  va_list ap;
  va_start(ap, format);
  vsprintf(buffer, format, ap);
  va_end(ap);

  __android_log_print(ANDROID_LOG_DEBUG, "Allegro", "%s", buffer);
}

int IsDataFile(version_info_t* version_info)
{
  return (strcmp(version_info->internal_name, "acwin") == 0);
}

int IsCompatibleOldDatafile(version_info_t* version_info)
{
  int major = 0;
  int minor = 0;
  int rev = 0;
  int build = 0;
  
  sscanf(version_info->version, "%d.%d.%d.%d", &major, &minor, &rev, &build);
  
  return ((major == 3) || ((major == 2) && (minor >= 50)));
}


JNIEXPORT jboolean JNICALL 
  Java_com_bigbluecup_android_PEHelper_isAgsDatafile(JNIEnv* env, jobject object, jclass stringclass, jstring filename)
{
  version_info_t info;
  const char* filename_char = (*env)->GetStringUTFChars(env, filename, NULL);

  getVersionInformation(filename_char, &info);

  (*env)->ReleaseStringUTFChars(env, filename, filename_char);
  
  return IsDataFile(&info) && IsCompatibleOldDatafile(&info);
}
