
//=============================================================================
//
// Driver-dependant bitmap interface
//
//=============================================================================
#ifndef __AGS_EE_GFX__DDB_H
#define __AGS_EE_GFX__DDB_H

namespace AGS
{
namespace Engine
{

class IDriverDependantBitmap
{
public:
  virtual void SetTransparency(int transparency) = 0;  // 0-255
  virtual void SetFlippedLeftRight(bool isFlipped) = 0;
  virtual void SetStretch(int width, int height) = 0;
  virtual void SetLightLevel(int light_level) = 0;   // 0-255
  virtual void SetTint(int red, int green, int blue, int tintSaturation) = 0;  // 0-255

  virtual int GetWidth() = 0;
  virtual int GetHeight() = 0;
  virtual int GetColorDepth() = 0;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__DDB_H
