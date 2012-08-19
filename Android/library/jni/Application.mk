APP_PROJECT_PATH := $(call my-dir)/..
APP_MODULES      := agsengine pe ags_snowrain agsblend agsflashlight agslua
APP_STL          := gnustl_static
APP_OPTIM        := release
APP_GNUSTL_FORCE_CPP_FEATURES := exceptions
#NDK_TOOLCHAIN_VERSION := 4.4.3
APP_ABI          := armeabi armeabi-v7a x86 mips