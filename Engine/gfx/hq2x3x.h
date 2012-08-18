#ifndef __AC_HQ2X3X_H
#define __AC_HQ2X3X_H

#if defined(ANDROID_VERSION) || defined(PSP_VERSION) || defined(IOS_VERSION)
void InitLUTs(){}
void hq2x_32( unsigned char * pIn, unsigned char * pOut, int Xres, int Yres, int BpL ){}
void hq3x_32( unsigned char * pIn, unsigned char * pOut, int Xres, int Yres, int BpL ){}
#else
void InitLUTs();
void hq2x_32( unsigned char * pIn, unsigned char * pOut, int Xres, int Yres, int BpL );
void hq3x_32( unsigned char * pIn, unsigned char * pOut, int Xres, int Yres, int BpL );
#endif

#endif // __AC_HQ2X3X_H