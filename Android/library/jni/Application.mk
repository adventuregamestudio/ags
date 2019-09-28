APP_PROJECT_PATH := $(call my-dir)/..
APP_MODULES      := agsengine pe ags_snowrain agsblend agsflashlight agsspritefont ags_parallax agspalrender
APP_STL          := gnustl_static
APP_OPTIM        := release
APP_GNUSTL_FORCE_CPP_FEATURES := exceptions
#NDK_TOOLCHAIN_VERSION := 4.4.3
APP_ABI          := arm64-v8a armeabi armeabi-v7a x86 x86_64 mips 
