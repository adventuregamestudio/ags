using namespace AGS::Types;
using namespace System;
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "util/string.h"

extern void warningBox(const char *fmt, ...);

#define MAX_ICONS_IN_FILE 15

typedef struct
{
    BYTE        bWidth;          // Width, in pixels, of the image
    BYTE        bHeight;         // Height, in pixels, of the image
    BYTE        bColorCount;     // Number of colors in image (0 if >=8bpp)
    BYTE        bReserved;       // Reserved ( must be 0)
    WORD        wPlanes;         // Color Planes
    WORD        wBitCount;       // Bits per pixel
    DWORD       dwBytesInRes;    // How many bytes in this resource?
    DWORD       dwImageOffset;   // Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
    WORD           idReserved;   // Reserved (must be 0)
    WORD           idType;       // Resource Type (1 for icons)
    WORD           idCount;      // How many images?
    ICONDIRENTRY   idEntries[MAX_ICONS_IN_FILE]; // An entry for each image (idCount of 'em)
} ICONDIR, *LPICONDIR;

#pragma pack( push )
#pragma pack( 2 )
typedef struct
{
   BYTE   bWidth;               // Width, in pixels, of the image
   BYTE   bHeight;              // Height, in pixels, of the image
   BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
   BYTE   bReserved;            // Reserved
   WORD   wPlanes;              // Color Planes
   WORD   wBitCount;            // Bits per pixel
   DWORD   dwBytesInRes;         // how many bytes in this resource?
   WORD   nID;                  // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;
#pragma pack( pop )

#pragma pack( push )
#pragma pack( 2 )
typedef struct 
{
   WORD            idReserved;   // Reserved (must be 0)
   WORD            idType;       // Resource type (1 for icons)
   WORD            idCount;      // How many images?
   GRPICONDIRENTRY   idEntries[MAX_ICONS_IN_FILE]; // The entries for each image
} GRPICONDIR, *LPGRPICONDIR;
#pragma pack( pop )


const int FIRST_ICON_ID = 551; // arbitrary first ID


// Resource enumeration callback; find only first resource of given kind and breaks enumeration
BOOL CALLBACK FindFirstResource(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam)
{
    LPTSTR *res_name = (LPTSTR*)lParam;
    if (IS_INTRESOURCE(lpszName))
        *res_name = lpszName;
    else
        *res_name = strdup(lpszName);
    return FALSE; // do not continue enumeration
}

// Queries engine executable for the icon resource ID
bool FindResID(const char *exeName, LPCSTR lpType, LPTSTR &lpIconResName, String^% err_msg)
{
    HMODULE hExe;
    // Load the .EXE file
    hExe = LoadLibrary(exeName); 
    if (hExe == NULL) 
    {
        err_msg = "Unable to load executable.";
        return false;
    }

    lpIconResName = NULL;
    EnumResourceNames(hExe, lpType, FindFirstResource, (LONG_PTR)&lpIconResName);

    // Clean up. 
    if (!FreeLibrary(hExe)) 
    { 
        err_msg = "Could not free executable."; 
        return false;
    }
    if (lpIconResName == NULL)
        err_msg = "No icons found.";
    return lpIconResName != NULL;
}

void ReplaceIconFromFile(const char *iconName, const char *exeName) {

  int i;
  FILE *icoin = fopen(iconName, "rb");
  if (icoin == NULL)
    return;

  ICONDIR iconHeader;
  fread(&iconHeader, 6, 1, icoin);

  if ((iconHeader.idCount > MAX_ICONS_IN_FILE) || (iconHeader.idCount < 1)) {
    fclose(icoin);
    throw gcnew AGSEditorException("Unable to replace icon: Too many icons within this icon file, or this may not be a valid .ICO file.");
  }
  
  if (iconHeader.idType != 1) {
    fclose(icoin);
    throw gcnew AGSEditorException("Unable to replace icon: this is not a valid icon file");
  }

  fread(&iconHeader.idEntries[0], sizeof(ICONDIRENTRY), iconHeader.idCount, icoin);

  GRPICONDIR resIconHeader;
  resIconHeader.idReserved = 0;
  resIconHeader.idType = iconHeader.idType;
  resIconHeader.idCount = iconHeader.idCount;

  for (i = 0; i < iconHeader.idCount; i++) {
    resIconHeader.idEntries[i].bColorCount = iconHeader.idEntries[i].bColorCount;
    resIconHeader.idEntries[i].bHeight = iconHeader.idEntries[i].bHeight;
    resIconHeader.idEntries[i].bReserved = iconHeader.idEntries[i].bReserved;
    resIconHeader.idEntries[i].bWidth = iconHeader.idEntries[i].bWidth;
    resIconHeader.idEntries[i].dwBytesInRes = iconHeader.idEntries[i].dwBytesInRes;
    resIconHeader.idEntries[i].wBitCount = iconHeader.idEntries[i].wBitCount;
    resIconHeader.idEntries[i].wPlanes = iconHeader.idEntries[i].wPlanes;
    resIconHeader.idEntries[i].nID = i + FIRST_ICON_ID;  
  }

  LPTSTR lpIconResName = NULL;
  String ^err_msg;
  if (!FindResID(exeName, RT_GROUP_ICON, lpIconResName, err_msg))
  {
    fclose(icoin);
    throw gcnew AGSEditorException("Unable to find icon ID in the engine executable:\n" + err_msg);
  }

  HANDLE hUpdate = BeginUpdateResource(exeName, FALSE);
  if (hUpdate == NULL) {
    fclose(icoin);
    if (!IS_INTRESOURCE(lpIconResName))
        free(lpIconResName);
    throw gcnew AGSEditorException("Unable to load the custom icon: BeginUpdateResource failed");
  }

  int retcode = UpdateResource(hUpdate, RT_GROUP_ICON, lpIconResName,
                    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK),
                    &resIconHeader,
                    6 + sizeof(GRPICONDIRENTRY) * iconHeader.idCount );
  const char *errorMsg = NULL;

  if (retcode == 0) {
    errorMsg = "Unable to load the custom icon: UpdateResource RT_GROUP_ICON failed";
  }
  else {

    for (i = 0; i < iconHeader.idCount; i++) {
      fseek(icoin, iconHeader.idEntries[i].dwImageOffset, SEEK_SET);

      int iconSize = iconHeader.idEntries[i].dwBytesInRes;
      char *iconbuffer = (char*)malloc(iconSize);
      fread(iconbuffer, iconSize, 1, icoin);

      if (UpdateResource(hUpdate, RT_ICON, MAKEINTRESOURCE(i + FIRST_ICON_ID),
                    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK),
                    iconbuffer, iconSize) == 0)
        errorMsg = "Warning: Icon update for icon failed.";

      free(iconbuffer);
    }

  }

  fclose(icoin);
  if (!IS_INTRESOURCE(lpIconResName))
      free(lpIconResName);

  if (EndUpdateResource(hUpdate, FALSE) == 0) {
    if (errorMsg == NULL) errorMsg = "Unable to load the custom icon: EndUpdateResource failed";
  }

  if (errorMsg != NULL)
  {
	  throw gcnew AGSEditorException(gcnew String(errorMsg));
  }
  return;
}

void ReplaceResourceInEXE(const char *exeName, const char *resourceName, const unsigned char *data, int dataLength, const char *resourceType)
{
  HANDLE hUpdate = BeginUpdateResource(exeName, FALSE);
  if (hUpdate == NULL) 
  {
    throw gcnew AGSEditorException("Unable to replace resource: BeginUpdateResource failed");
  }

  int retcode = UpdateResource(hUpdate, resourceType, resourceName,
                    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK),
                    (void*)data, dataLength);
  AGS::Common::String errorMsg;

  if (retcode == 0) 
  {
    DWORD errorCode = GetLastError();
    if (IS_INTRESOURCE(resourceName))
        errorMsg.Format("Unable to replace resource: UpdateResource RT_RCDATA for %08X failed: %08X", (WORD)(resourceName), errorCode);
    else
        errorMsg.Format("Unable to replace resource: UpdateResource RT_RCDATA for %s failed: %08X", resourceName, errorCode);
  }

  if (EndUpdateResource(hUpdate, FALSE) == 0) 
  {
    DWORD errorCode = GetLastError();
    if (errorMsg == NULL) 
    {
      if (IS_INTRESOURCE(resourceName))
          errorMsg.Format("Unable to replace resource: EndUpdateResource for %08X failed: %08X", (WORD)(resourceName), errorCode);
      else
          errorMsg.Format("Unable to replace resource: EndUpdateResource for %s failed: %08X", resourceName, errorCode);
    }
  }

  if (!errorMsg.IsEmpty())
  {
	  throw gcnew AGSEditorException(gcnew String(errorMsg));
  }
}
