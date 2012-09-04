/* 
** ALI3D -- Allegro Interface for 3D
** Copyright (C) 2007, Chris Jones
** All Rights Reserved.
*/

#ifndef __ALI3D_H
#define __ALI3D_H

#include "gfx/blender.h"
#include "gfx/gfxfilter.h"

// Forward declaration
namespace AGS { namespace Engine { class IGraphicsDriver; } }
using namespace AGS::Engine; // FIXME later

class Ali3DException
{
public:
  Ali3DException(const char *message)
  {
    _message = message;
  }

  const char *_message;
};

class Ali3DFullscreenLostException : public Ali3DException
{
public:
  Ali3DFullscreenLostException() : Ali3DException("User has switched away from application")
  {
  }

  const char *_message;
};

extern IGraphicsDriver* GetOGLGraphicsDriver(GFXFilter *);
extern IGraphicsDriver* GetD3DGraphicsDriver(GFXFilter *);
extern IGraphicsDriver* GetSoftwareGraphicsDriver(GFXFilter *);

#endif
